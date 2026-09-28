// This file is part of Noteahead.
// Copyright (C) 2026 Jussi Lind <jussi.lind@iki.fi>
//
// Noteahead is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// Noteahead is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Noteahead. If not, see <http://www.gnu.org/licenses/>.

#include "drum_synth_v2_device.hpp"

#include "../../common/constants.hpp"
#include "../../common/parameter_mapper.hpp"
#include "../../common/utils.hpp"
#include "../../common/xml/project_reader.hpp"
#include "../../common/xml/project_writer.hpp"
#include "../../infra/midi/midi_cc_mapping.hpp"

#include "../dsp/drum/clap_engine.hpp"

#include <cmath>
#include <numbers>

namespace noteahead {

using namespace DrumSynthV2;

void DrumSynthV2Device::Voice::updateEffects()
{
    volumeEffect->setVolume(level);
    panningEffect->setPan(pan);
    lpf->setCutoff(lpfCutoff);
    hpf->setCutoff(hpfCutoff);
    // Parked at the end of its range a filter passes the signal through, so this is what turns the
    // second stage off rather than taking it out of the chain.
    lpfStage2->setCutoff(steepLpf ? lpfCutoff : 1.0f);
    hpfStage2->setCutoff(steepHpf ? hpfCutoff : 0.0f);
}

DrumSynthV2Device::DrumSynthV2Device(std::string name)
  : m_name { std::move(name) }
{
    initializeVoices();

    // 12 dB/oct, which is what the drum synth's voice filters have always been. One pair for the
    // kit rather than one per drum: a kit is mixed as a whole, and eleven of these would be eleven
    // more controls to explain for a choice nobody makes per instrument.
    //
    // One slope was shared by both filters when this was added. That name is kept as a legacy name
    // on the low pass, which is the filter it was really being used for.
    addParameter(Parameter { Constants::NahdXml::xmlKeyLpfSlope().toStdString(), 0.0f, 0, 1, 0, 1, Parameter::Type::Discrete, { Constants::NahdXml::xmlKeyFilterSlope().toStdString() } });
    addParameter(Parameter { Constants::NahdXml::xmlKeyHpfSlope().toStdString(), 0.0f, 0, 1, 0, 1, Parameter::Type::Discrete });

    for (int i { 0 }; i < NumVoices; i++) {
        addVoiceParameters(i);
    }

    DrumSynthV2Device::syncParameters();
}

std::string DrumSynthV2Device::name() const
{
    return m_name;
}

std::string DrumSynthV2Device::category() const
{
    return Constants::NahdXml::xmlValueDrums().toStdString();
}

std::string DrumSynthV2Device::typeName() const
{
    return Constants::drumSynthV2DeviceName().toStdString();
}

std::string DrumSynthV2Device::typeIdString()
{
    return "cf30a9f9-3709-45e3-bf96-80676a5f524b";
}

std::string DrumSynthV2Device::typeId() const
{
    return typeIdString();
}

std::vector<MidiCcController> DrumSynthV2Device::deviceMidiCcControllers() const
{
    using namespace MidiCcMapping;
    std::vector<MidiCcController> list;

    list.push_back(faderMidiCcController());
    list.push_back({ static_cast<uint8_t>(Controller::PanMSB), "Pan" });

    using namespace DrumSynthV2;

    // Range 1: Voices 0-5
    for (int voice { 0 }; voice < NumVoicesRange1; voice++) {
        const uint8_t baseCc = CcStartRange1 + (voice * 3);
        const std::string voiceName = DrumSynthV2::voiceName(voice).toStdString();
        list.push_back({ baseCc, voiceName + " Pan" });
        list.push_back({ static_cast<uint8_t>(baseCc + 1), voiceName + " LPF" });
        list.push_back({ static_cast<uint8_t>(baseCc + 2), voiceName + " HPF" });
    }

    // Range 2: Voices 6-10
    for (int voice { 0 }; voice < NumVoicesRange2; voice++) {
        const uint8_t baseCc = CcStartRange2 + (voice * 3);
        const std::string voiceName = DrumSynthV2::voiceName(static_cast<int>(NumVoicesRange1) + voice).toStdString();
        list.push_back({ baseCc, voiceName + " Pan" });
        list.push_back({ static_cast<uint8_t>(baseCc + 1), voiceName + " LPF" });
        list.push_back({ static_cast<uint8_t>(baseCc + 2), voiceName + " HPF" });
    }

    return list;
}

void DrumSynthV2Device::processMidiNoteOn(uint8_t note, uint8_t velocity)
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };

    using enum DrumSynthV2::MidiNote;

    // Hi-hat choking logic: Closed Hat or Pedal Hat chokes Open Hat
    if (note == static_cast<uint8_t>(ClosedHiHat) || note == static_cast<uint8_t>(PedalHiHat)) {
        m_voices[static_cast<int>(VoiceIndex::OpenHiHat)].engine->stop();
    }

    for (auto && voice : m_voices) {
        if (voice.midiNote == note) {
            const float vel { static_cast<float>(velocity) / 127.0f };
            voice.engine->trigger(vel);
            voice.ampEnvelope.trigger();
            voice.renderedFrames = 0;
            break;
        }
    }
}

void DrumSynthV2Device::processMidiNoteOff(uint8_t note)
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };

    using enum DrumSynthV2::MidiNote;

    // Closed Hat choke logic: smoothly stop Open Hat on CHH/Pedal release
    if (note == static_cast<uint8_t>(ClosedHiHat) || note == static_cast<uint8_t>(PedalHiHat)) {
        m_voices[static_cast<int>(VoiceIndex::OpenHiHat)].engine->stop();
    }

    // A drum is struck rather than held, so this does nothing at all unless the voice has been
    // given a sustain to be let go of: the release falls away from wherever the level stands, and
    // on an envelope with no sustain there is nothing left standing by the time the note ends.
    for (auto && voice : m_voices) {
        if (voice.midiNote == note) {
            voice.ampEnvelope.release();
            break;
        }
    }
}

void DrumSynthV2Device::processDeviceMidiCc(uint8_t controller, uint8_t value, uint8_t /*channel*/)
{
    using namespace MidiCcMapping;

    // Nothing here may emit while the lock is held: dataChanged() receivers read back from the
    // audio engine, whose callback takes the engine mutex before this one. parametersChanged() is
    // emitted below, once the lock is gone, and is what the dialog follows anyway.
    bool changed { false };
    {
        const std::lock_guard<std::recursive_mutex> lock { mutex() };

        if (controller == static_cast<uint8_t>(Controller::ResetAllControllers)) {
            changed |= clearAutomationInternal();
        } else {
            const float val { static_cast<float>(value) / 127.0f };

            if (controller == static_cast<uint8_t>(Controller::ChannelVolumeMSB)) {
                changed |= updateVolumeParameter(faderPositionFromMidiCc(value), false);
            } else if (controller == static_cast<uint8_t>(Controller::PanMSB)) {
                changed |= updatePanParameter(val, false);
            } else if (controller >= CcStartRange1 && controller < CcStartRange1 + (NumVoicesRange1 * 3)) {
                const int voiceIndex { (controller - CcStartRange1) / 3 };
                const int paramType { (controller - CcStartRange1) % 3 };
                if (paramType == 0)
                    changed |= automateVoiceParameter(voiceIndex, Constants::NahdXml::xmlKeyPan().toStdString(), val);
                else if (paramType == 1)
                    changed |= automateVoiceParameter(voiceIndex, Constants::NahdXml::xmlKeyCutoff().toStdString(), val);
                else if (paramType == 2)
                    changed |= automateVoiceParameter(voiceIndex, Constants::NahdXml::xmlKeyHpfCutoff().toStdString(), val);
            } else if (controller >= CcStartRange2 && controller < CcStartRange2 + (NumVoicesRange2 * 3)) {
                const int voiceIndex { NumVoicesRange1 + (controller - CcStartRange2) / 3 };
                const int paramType { (controller - CcStartRange2) % 3 };
                if (paramType == 0)
                    changed |= automateVoiceParameter(voiceIndex, Constants::NahdXml::xmlKeyPan().toStdString(), val);
                else if (paramType == 1)
                    changed |= automateVoiceParameter(voiceIndex, Constants::NahdXml::xmlKeyCutoff().toStdString(), val);
                else if (paramType == 2)
                    changed |= automateVoiceParameter(voiceIndex, Constants::NahdXml::xmlKeyHpfCutoff().toStdString(), val);
            }
        }
    }

    if (changed) {
        emit parametersChanged();
    }
}

void DrumSynthV2Device::processMidiAllNotesOff()
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    for (auto && voice : m_voices) {
        voice.engine->stop();
    }
}

void DrumSynthV2Device::processAudio(AudioContext & context)
{
    setSampleRate(context.sampleRate);
    const uint8_t oversampleFactor = clampOversampleFactor(context.oversampleFactor);
    const uint32_t oversampledRate = context.sampleRate * oversampleFactor;
    const std::lock_guard<std::recursive_mutex> lock { mutex() };

    for (auto && voice : m_voices) {
        voice.engine->setSampleRate(oversampledRate);
        voice.engine->setOversampleFactor(oversampleFactor);
        voice.lpf->setSampleRate(oversampledRate);
        voice.hpf->setSampleRate(oversampledRate);
        voice.lpfStage2->setSampleRate(oversampledRate);
        voice.hpfStage2->setSampleRate(oversampledRate);
        voice.ampEnvelope.setSampleRate(oversampledRate);
    }

    // Snapshot each voice's insert-rack effects for per-sample processing at the oversampled rate. The
    // rack is inserted after the fixed per-voice DSP chain and runs sample-by-sample like the other voice
    // effects, so time-based effects stay correct (their times derive from the oversampled sample rate).
    //
    // No sync() here: every path that changes a parameter already syncs (the controller on a knob
    // move, deserialization on load, EffectRack::setEffect when the effect is added), and syncing
    // per block would resolve each parameter key on the audio thread, which allocates.
    for (int v = 0; v < NumVoices; v++) {
        m_voiceRackEffects.at(v).clear();
        if (m_voices.at(v).effectRack.hasEffects()) {
            m_voices.at(v).effectRack.effects(m_rackEffectScratch);
            for (auto & effect : m_rackEffectScratch) {
                if (effect->enabled()) {
                    effect->setSampleRate(oversampledRate);
                    // The per-voice racks are not reached by Device::setBpm, which only walks the
                    // device's own insert rack, so this is the only place they learn the tempo.
                    effect->setBpm(static_cast<float>(context.bpm));
                    m_voiceRackEffects.at(v).push_back(effect);
                }
            }
        }
    }

    const size_t oversampledSize = static_cast<size_t>(context.frameCount) * oversampleFactor * 2;
    if (m_oversampledBuffer.size() < oversampledSize) {
        m_oversampledBuffer.resize(oversampledSize);
    }
    std::fill(m_oversampledBuffer.begin(), m_oversampledBuffer.begin() + oversampledSize, 0.0f);

    // One scratch buffer per bus, but only while something is routed there: a kit that sends
    // nowhere neither allocates nor clears anything.
    const size_t sendBusCount = hasSendSources() ? context.sendBuses.size() : 0;
    if (m_sendOversampledBuffers.size() != sendBusCount) {
        m_sendOversampledBuffers.resize(sendBusCount);
        m_sendDecimators.resize(sendBusCount);
    }
    for (size_t bus = 0; bus < sendBusCount; bus++) {
        auto & sendBuffer = m_sendOversampledBuffers.at(bus);
        if (sendBuffer.size() < oversampledSize) {
            sendBuffer.resize(oversampledSize);
        }
        std::fill(sendBuffer.begin(), sendBuffer.begin() + oversampledSize, 0.0f);
    }
    auto & oversampledBuffer = m_oversampledBuffer;
    const float globalGain = linearGainInternal();

    for (uint32_t i = 0; i < context.frameCount; i++) {
        for (uint8_t os = 0; os < oversampleFactor; os++) {
            float mixL = 0.0f;
            float mixR = 0.0f;

            for (int v = 0; v < NumVoices; v++) {
                auto & voice = m_voices.at(v);
                // Counted on the first pass of the oversampling only, so it stays a count of output
                // frames and the playhead runs at the same speed whatever the quality is set to.
                if (!os && voice.engine->isActive() && voice.ampEnvelope.isActive()) {
                    voice.renderedFrames++;
                }
                // Once the envelope has closed there is nothing left to hear however much tail the
                // engine still has, so the voice stops costing anything -- which is what makes a
                // short Hold cheaper than a long one rather than merely quieter.
                if (voice.engine->isActive() && voice.ampEnvelope.isActive()) {
                    float sample = voice.engine->nextSample();
                    double l = sample;
                    double r = sample;

                    voice.lpf->process(l, r);
                    voice.lpfStage2->process(l, r);
                    voice.hpf->process(l, r);
                    voice.hpfStage2->process(l, r);

                    const double amp = voice.ampEnvelope.nextSample();
                    l *= amp;
                    r *= amp;

                    voice.volumeEffect->process(l, r);
                    voice.panningEffect->process(l, r);

                    for (auto & effect : m_voiceRackEffects.at(v)) {
                        effect->process(l, r);
                    }

                    // The voice's own tap into the global buses, carrying its filters, envelope,
                    // level, pan and inserts but not the device's soft clip or master pan.
                    for (size_t bus = 0; bus < m_sendOversampledBuffers.size() && bus < voice.sends.size(); bus++) {
                        const double level = static_cast<double>(voice.sends.at(bus));
                        if (level <= 0.0) {
                            continue;
                        }
                        auto & sendBuffer = m_sendOversampledBuffers.at(bus);
                        sendBuffer[(i * oversampleFactor + os) * 2] += static_cast<float>(l * level);
                        sendBuffer[(i * oversampleFactor + os) * 2 + 1] += static_cast<float>(r * level);
                    }

                    mixL += static_cast<float>(l);
                    mixR += static_cast<float>(r);
                }
            }

            oversampledBuffer[(i * oversampleFactor + os) * 2] += mixL * globalGain;
            oversampledBuffer[(i * oversampleFactor + os) * 2 + 1] += mixR * globalGain;
        }
    }

    const double panAngle = static_cast<double>(panInternal()) * std::numbers::pi * 0.5;
    const float panL = static_cast<float>(std::cos(panAngle));
    const float panR = static_cast<float>(std::sin(panAngle));

    std::array<float, 4> highL {};
    std::array<float, 4> highR {};
    for (uint32_t i = 0; i < context.frameCount; i++) {
        // Soft-clip at high rate and then downsample
        for (uint8_t os = 0; os < oversampleFactor; os++) {
            highL[os] = std::tanh(oversampledBuffer[(i * oversampleFactor + os) * 2]);
            highR[os] = std::tanh(oversampledBuffer[(i * oversampleFactor + os) * 2 + 1]);
        }

        const float l = m_downsamplerL.process(highL.data(), oversampleFactor);
        const float r = m_downsamplerR.process(highR.data(), oversampleFactor);

        context.buffer[i * 2] += l * panL;
        context.buffer[i * 2 + 1] += r * panR;
    }

    addSendContributions(context, oversampleFactor, panL, panR);
}

void DrumSynthV2Device::addSendContributions(AudioContext & context, uint8_t oversampleFactor, double panL, double panR)
{
    if (m_sendOversampledBuffers.empty()) {
        return;
    }

    // Post-fader sends follow the fader the engine is about to apply, as a device's own send does.
    const double tapGain = sendTap() == SendTap::PostFader ? faderGain() : 1.0;

    std::array<float, 4> highL {};
    std::array<float, 4> highR {};
    for (size_t bus = 0; bus < m_sendOversampledBuffers.size() && bus < context.sendBuses.size(); bus++) {
        const auto & sendBuffer = m_sendOversampledBuffers.at(bus);
        auto & [decimatorL, decimatorR] = m_sendDecimators.at(bus);
        const auto target = context.sendBuses[bus];
        for (uint32_t i = 0; i < context.frameCount; i++) {
            for (uint8_t os = 0; os < oversampleFactor; os++) {
                highL[os] = sendBuffer[(i * oversampleFactor + os) * 2];
                highR[os] = sendBuffer[(i * oversampleFactor + os) * 2 + 1];
            }
            const double l = decimatorL.process(highL.data(), oversampleFactor);
            const double r = decimatorR.process(highR.data(), oversampleFactor);
            const size_t index = context.sendBusOffset + i * 2;
            if (index + 1 >= target.size()) {
                break;
            }
            target[index] += l * panL * tapGain;
            target[index + 1] += r * panR * tapGain;
        }
    }
}

bool DrumSynthV2Device::hasActiveAudio() const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    for (const auto & voice : m_voices) {
        if (voice.engine->isActive() && voice.ampEnvelope.isActive()) {
            return true;
        }
    }
    return false;
}

void DrumSynthV2Device::reset()
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    Device::reset();
    resetAudio();
}

void DrumSynthV2Device::resetAudio()
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    for (auto && voice : m_voices) {
        voice.engine->reset();
        voice.ampEnvelope.reset();
        voice.effectRack.reset();
    }
    m_downsamplerL.reset();
    m_downsamplerR.reset();
}

void DrumSynthV2Device::serializeToXml(ProjectWriter & writer) const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    writer.writeStartElement(Constants::NahdXml::xmlKeyDevice());
    serializeAttributesToXml(writer);

    writer.writeStartElement(Constants::NahdXml::xmlKeyInsertEffects());
    insertEffectRack().serializeEffectsToXml(writer);
    writer.writeEndElement();

    writer.writeStartElement(Constants::NahdXml::xmlKeyParameters());
    serializeParametersToXml(writer);
    writer.writeEndElement();

    writer.writeStartElement(Constants::NahdXml::xmlKeyVoices());
    for (int v = 0; v < NumVoices; v++) {
        if (m_voices.at(v).effectRack.hasEffects()) {
            writer.writeStartElement(Constants::NahdXml::xmlKeyVoice());
            writer.writeAttribute(Constants::NahdXml::xmlKeyIndex(), QString::number(v));
            writer.writeStartElement(Constants::NahdXml::xmlKeyInsertEffects());
            m_voices.at(v).effectRack.serializeEffectsToXml(writer);
            writer.writeEndElement(); // InsertEffects
            writer.writeEndElement(); // Voice
        }
    }
    writer.writeEndElement(); // Voices

    writer.writeEndElement();
}

//! Puts the amp envelope back the way it shipped, before a project is read.
//!
//! A kit saved before the sustain stage existed carries no sustain attribute, and an absent
//! parameter keeps whatever the container holds -- which is now an envelope that does nothing. Such
//! a kit was voiced against an envelope parked just past each voice's tail, so reading it without
//! putting those values back first would quietly lengthen every drum in it.
//!
//! The same shape as SpeechDevice's legacy voice engine, and for the same reason.
void DrumSynthV2Device::restoreLegacyAmpEnvelope()
{
    struct AmpEnvelopeDefaults
    {
        float hold {};
        float decay {};
    };

    static constexpr std::array<AmpEnvelopeDefaults, NumVoices> legacy { {
      { 0.4661f, 0.6443f }, // Kick: hold 810 ms, decay 580 ms
      { 0.3832f, 0.5759f }, // Snare: hold 450 ms, decay 350 ms
      { 0.1957f, 0.2819f }, // ClosedHiHat: hold 60 ms, decay 40 ms
      { 0.3714f, 0.5129f }, // Clap: hold 410 ms, decay 220 ms
      { 0.3969f, 0.5719f }, // OpenHiHat: hold 500 ms, decay 340 ms
      { 0.5499f, 0.7068f }, // LowTom: hold 1330 ms, decay 920 ms
      { 0.5499f, 0.7068f }, // MidTom: hold 1330 ms, decay 920 ms
      { 0.5499f, 0.7068f }, // HighTom: hold 1330 ms, decay 920 ms
      { 0.6937f, 0.8724f }, // Crash: hold 2670 ms, decay 3120 ms
      { 0.7570f, 0.8345f }, // Ride: hold 3470 ms, decay 2360 ms
      { 0.6300f, 0.5940f }, // ReverseCrash: hold 2000 ms, decay 400 ms
    } };

    // writeVoiceParameter() rather than updateVoiceParameter(), which emits dataChanged(). This
    // runs from deserializeFromXml() with the device mutex held, and that signal reaches
    // MidiService, whose handler goes out to the worker thread: emitting it from under the lock
    // deadlocks the load. The same trap the Speech device's legacy engine fell into.
    for (int index = 0; index < NumVoices; index++) {
        const auto & voiceLegacy = legacy.at(static_cast<size_t>(index));
        writeVoiceParameter(index, Constants::NahdXml::xmlKeyAmpHold().toStdString(), voiceLegacy.hold, true);
        writeVoiceParameter(index, Constants::NahdXml::xmlKeyAmpDecay().toStdString(), voiceLegacy.decay, true);
        writeVoiceParameter(index, Constants::NahdXml::xmlKeyAmpSustain().toStdString(), 0.0f, true);
        writeVoiceParameter(index, Constants::NahdXml::xmlKeyAmpRelease().toStdString(), 0.0f, true);
    }
}

void DrumSynthV2Device::deserializeFromXml(ProjectReader & reader)
{
    {
        const std::lock_guard<std::recursive_mutex> lock { mutex() };
        restoreLegacyAmpEnvelope();
        deserializeAttributesFromXml(reader);

        while (!reader.atEnd() && !reader.hasError()) {
            const auto token = reader.readNext();
            if (token == ProjectReader::TokenType::EndElement && reader.name() == Constants::NahdXml::xmlKeyDevice()) {
                break;
            }

            if (token == ProjectReader::TokenType::StartElement) {
                if (reader.name() == Constants::NahdXml::xmlKeyParameters()) {
                    deserializeParametersFromXml(reader);
                } else if (reader.name() == Constants::NahdXml::xmlKeyInsertEffects()) {
                    insertEffectRack().deserializeEffectsFromXml(reader);
                } else if (reader.name() == Constants::NahdXml::xmlKeyParameter()) {
                    deserializeParameter(reader);
                } else if (reader.name() == Constants::NahdXml::xmlKeyVoices()) {
                    while (reader.readNextStartElement()) {
                        if (reader.name() == Constants::NahdXml::xmlKeyVoice()) {
                            const auto index = Utils::Xml::readIntAttribute(reader, Constants::NahdXml::xmlKeyIndex(), false);
                            while (reader.readNextStartElement()) {
                                if (reader.name() == Constants::NahdXml::xmlKeyInsertEffects() && index.has_value() && index.value() >= 0 && index.value() < NumVoices) {
                                    m_voices.at(index.value()).effectRack.deserializeEffectsFromXml(reader);
                                } else {
                                    reader.skipCurrentElement();
                                }
                            }
                        } else {
                            reader.skipCurrentElement();
                        }
                    }
                } else {
                    reader.skipCurrentElement();
                }
            }
        }

        syncParameters();
    }
    emit dataChanged();
}

int DrumSynthV2Device::selectedVoice() const
{
    return m_selectedVoice;
}

void DrumSynthV2Device::setSelectedVoice(int index)
{
    m_selectedVoice = std::clamp(index, 0, NumVoices - 1);
}

uint8_t DrumSynthV2Device::voiceNote(int index) const
{
    if (index >= 0 && index < NumVoices) {
        return m_voices.at(index).midiNote;
    }
    return 0;
}

size_t DrumSynthV2Device::sendSourceCount() const
{
    return DrumSynthV2::NumVoices;
}

float DrumSynthV2Device::sendSourceLevel(size_t sourceIndex, size_t busIndex) const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    if (sourceIndex < m_voices.size() && busIndex < m_voices.at(sourceIndex).sends.size()) {
        return m_voices.at(sourceIndex).sends.at(busIndex);
    }
    return 0.0f;
}

void DrumSynthV2Device::setSendSourceLevel(size_t sourceIndex, size_t busIndex, float level)
{
    {
        const std::lock_guard<std::recursive_mutex> lock { mutex() };
        if (sourceIndex >= m_voices.size()) {
            return;
        }
        auto & sends = m_voices.at(sourceIndex).sends;
        if (busIndex >= sends.size()) {
            return;
        }
        if (qFuzzyCompare(sends.at(busIndex), level)) {
            return;
        }
        sends.at(busIndex) = level;
    }
    emit dataChanged();
}

EffectRack & DrumSynthV2Device::voiceEffectRack(int index)
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    return m_voices.at(std::clamp(index, 0, NumVoices - 1)).effectRack;
}

void DrumSynthV2Device::initializeVoices()
{
    using enum DrumSynthV2::MidiNote;

    // Define GM mapping
    const std::array<uint8_t, NumVoices> notes {
        static_cast<uint8_t>(Kick),
        static_cast<uint8_t>(Snare),
        static_cast<uint8_t>(ClosedHiHat),
        static_cast<uint8_t>(Clap),
        static_cast<uint8_t>(OpenHiHat),
        static_cast<uint8_t>(LowTom),
        static_cast<uint8_t>(MidTom),
        static_cast<uint8_t>(HiTom),
        static_cast<uint8_t>(Crash),
        static_cast<uint8_t>(Ride),
        static_cast<uint8_t>(ReverseCrash)
    };

    for (int i { 0 }; i < NumVoices; i++) {
        m_voices.at(i).midiNote = notes.at(i);
        m_voices.at(i).sends.assign(Constants::effectRackSize(), 0.0f);
        m_voices.at(i).lpf = std::make_shared<LowPassFilter>();
        m_voices.at(i).hpf = std::make_shared<HighPassFilter>();
        m_voices.at(i).lpfStage2 = std::make_shared<LowPassFilter>();
        m_voices.at(i).hpfStage2 = std::make_shared<HighPassFilter>();
        m_voices.at(i).volumeEffect = std::make_shared<Volume>();
        m_voices.at(i).panningEffect = std::make_shared<Panning>();

        const auto voiceIdx { static_cast<VoiceIndex>(i) };
        if (voiceIdx == VoiceIndex::Kick)
            m_voices.at(i).engine = std::make_unique<KickEngine>();
        else if (voiceIdx == VoiceIndex::Snare)
            m_voices.at(i).engine = std::make_unique<SnareEngine>();
        else if (voiceIdx == VoiceIndex::Clap)
            m_voices.at(i).engine = std::make_unique<ClapEngine>();
        else if (voiceIdx == VoiceIndex::ClosedHiHat || voiceIdx == VoiceIndex::OpenHiHat)
            m_voices.at(i).engine = std::make_unique<HiHatEngine>();
        else if (voiceIdx >= VoiceIndex::LowTom && voiceIdx <= VoiceIndex::HighTom)
            m_voices.at(i).engine = std::make_unique<TomEngine>();
        else if (voiceIdx == VoiceIndex::Crash) {
            // The cymbals are the two voices V2 does not share with V1. Both were fitted to a
            // recording: the crash had four tenths of steady noise against four of metal, which
            // measured as a wash rather than as struck metal, and neither of them had any body
            // below 600 Hz at all.
            auto crash = std::make_unique<CrashEngine>();
            crash->setVoicing(CrashEngine::Voicing::Rd9);
            m_voices.at(i).engine = std::move(crash);
        } else if (voiceIdx == VoiceIndex::Ride) {
            auto ride = std::make_unique<RideEngine>();
            ride->setVoicing(RideEngine::Voicing::Rd9);
            m_voices.at(i).engine = std::move(ride);
        } else if (voiceIdx == VoiceIndex::ReverseCrash) {
            auto crash = std::make_unique<CrashEngine>();
            crash->setMode(CrashEngine::Mode::Reverse);
            crash->setVoicing(CrashEngine::Voicing::Rd9);
            m_voices.at(i).engine = std::move(crash);
        }
    }
}

void DrumSynthV2Device::addVoiceParameters(int index)
{
    const std::string prefix { voiceId(index) + "_" };

    float defTune = 0.5f;
    float defDecay = 0.5f;

    const auto voiceIdx { static_cast<VoiceIndex>(index) };
    if (voiceIdx == VoiceIndex::ClosedHiHat) {
        defTune = 0.7f;
        defDecay = 0.1f;
    } else if (voiceIdx == VoiceIndex::OpenHiHat) {
        defTune = 0.6f;
        defDecay = 0.8f;
    } else if (voiceIdx == VoiceIndex::LowTom) {
        defTune = 0.2f;
    } else if (voiceIdx == VoiceIndex::MidTom) {
        defTune = 0.5f;
    } else if (voiceIdx == VoiceIndex::HighTom) {
        defTune = 0.8f;
    }

    // Standard voice parameters
    addParameter(Parameter { prefix + Constants::NahdXml::xmlKeyLevel().toStdString(), 0.8f, 0, 10000, 8000, 100 });
    addParameter(Parameter { prefix + Constants::NahdXml::xmlKeyPan().toStdString(), 0.5f, 0, 10000, 5000, 100 });
    addParameter(Parameter { prefix + Constants::NahdXml::xmlKeyCutoff().toStdString(), 1.0f, 0, 10000, 10000, 100 });
    addParameter(Parameter { prefix + Constants::NahdXml::xmlKeyHpfCutoff().toStdString(), 0.0f, 0, 10000, 0, 100 });

    addAmpEnvelopeParameters(index, prefix);

    // Common engine parameters
    addParameter(Parameter { prefix + Constants::NahdXml::xmlKeyTune().toStdString(), defTune, 0, 10000, static_cast<int>(defTune * 10000), 100 });
    addParameter(Parameter { prefix + Constants::NahdXml::xmlKeyDecay().toStdString(), defDecay, 0, 10000, static_cast<int>(defDecay * 10000), 100 });

    if (voiceIdx == VoiceIndex::Kick)
        addKickParameters(prefix);
    else if (voiceIdx == VoiceIndex::Snare)
        addSnareParameters(prefix);
    else if (voiceIdx == VoiceIndex::Clap) {
    } else if (voiceIdx == VoiceIndex::ClosedHiHat || voiceIdx == VoiceIndex::OpenHiHat)
        addHiHatParameters(prefix);
    else if (voiceIdx >= VoiceIndex::LowTom && voiceIdx <= VoiceIndex::HighTom)
        addTomParameters(prefix);
    else if (voiceIdx >= VoiceIndex::Crash && voiceIdx <= VoiceIndex::ReverseCrash)
        addCymbalParameters(prefix);
}

//! Where each voice's amp envelope starts: held open until the voice has fallen to about -40 dBFS,
//! then faded out over the span it used to take to reach -60 dBFS.
//!
//! Measured per voice rather than guessed, so a fresh V2 sounds like the kit people know: the
//! envelope only ever takes over below the point where the engine's own tail stops being audible.
//! It is a starting point, not a description -- pulling Hold down is how a drum gets tightened, and
//! that is the whole reason the stage is here.
void DrumSynthV2Device::addAmpEnvelopeParameters(int index, const std::string & prefix)
{
    struct AmpEnvelopeDefaults
    {
        float hold {};
        float decay {};
    };

    static constexpr std::array<AmpEnvelopeDefaults, NumVoices> defaults { {
      { 0.4661f, 0.6443f }, // Kick: hold 810 ms, decay 580 ms
      { 0.3832f, 0.5759f }, // Snare: hold 450 ms, decay 350 ms
      { 0.1957f, 0.2819f }, // ClosedHiHat: hold 60 ms, decay 40 ms
      { 0.3714f, 0.5129f }, // Clap: hold 410 ms, decay 220 ms
      { 0.3969f, 0.5719f }, // OpenHiHat: hold 500 ms, decay 340 ms
      { 0.5499f, 0.7068f }, // LowTom: hold 1330 ms, decay 920 ms
      { 0.5499f, 0.7068f }, // MidTom: hold 1330 ms, decay 920 ms
      { 0.5499f, 0.7068f }, // HighTom: hold 1330 ms, decay 920 ms
      { 0.6937f, 0.8724f }, // Crash: hold 2670 ms, decay 3120 ms
      { 0.7570f, 0.8345f }, // Ride: hold 3470 ms, decay 2360 ms
      { 0.6300f, 0.5940f }, // ReverseCrash: hold 2000 ms, decay 400 ms
    } };

    // Unused now that the envelope starts out doing nothing, and kept because it is what
    // legacyAmpEnvelopeDefaults() puts back for a kit saved before the sustain existed.
    (void)defaults;

    // A device added now starts with the envelope switched off: everything at its minimum and the
    // sustain at full, which makes the envelope a constant one and the voice exactly V1's. It is a
    // stage to reach for rather than one already shaping the drum.
    addParameter(Parameter { prefix + Constants::NahdXml::xmlKeyAmpAttack().toStdString(), 0.0f, 0, 10000, 0, 100 });
    addParameter(Parameter { prefix + Constants::NahdXml::xmlKeyAmpHold().toStdString(), 0.0f, 0, 10000, 0, 100 });
    addParameter(Parameter { prefix + Constants::NahdXml::xmlKeyAmpDecay().toStdString(), 0.0f, 0, 10000, 0, 100 });
    addParameter(Parameter { prefix + Constants::NahdXml::xmlKeyAmpSustain().toStdString(), 1.0f, 0, 10000, 10000, 100 });
    addParameter(Parameter { prefix + Constants::NahdXml::xmlKeyAmpRelease().toStdString(), 0.0f, 0, 10000, 0, 100 });
    addParameter(Parameter { prefix + Constants::NahdXml::xmlKeyAmpCurve().toStdString(), 0.0f, 0, 10000, 0, 100 });
}

void DrumSynthV2Device::addKickParameters(const std::string & prefix)
{
    addParameter(Parameter { prefix + Constants::NahdXml::xmlKeyAttack().toStdString(), 0.5f, 0, 10000, 5000, 100 });
    addParameter(Parameter { prefix + Constants::NahdXml::xmlKeyClickTune().toStdString(), 0.5f, 0, 10000, 5000, 100 });
    addParameter(Parameter { prefix + Constants::NahdXml::xmlKeyPitchDepth().toStdString(), 0.5f, 0, 10000, 5000, 100 });
    addParameter(Parameter { prefix + Constants::NahdXml::xmlKeyPitchDecay().toStdString(), 0.5f, 0, 10000, 5000, 100 });
}

void DrumSynthV2Device::addSnareParameters(const std::string & prefix)
{
    addParameter(Parameter { prefix + Constants::NahdXml::xmlKeySnappy().toStdString(), 0.5f, 0, 10000, 5000, 100 });
    addParameter(Parameter { prefix + Constants::NahdXml::xmlKeyTone().toStdString(), 0.5f, 0, 10000, 5000, 100 });
}

void DrumSynthV2Device::addTomParameters(const std::string & prefix)
{
    addParameter(Parameter { prefix + Constants::NahdXml::xmlKeyPitchDepth().toStdString(), 0.5f, 0, 10000, 5000, 100 });
    addParameter(Parameter { prefix + Constants::NahdXml::xmlKeyPitchDecay().toStdString(), 0.5f, 0, 10000, 5000, 100 });
}

void DrumSynthV2Device::addHiHatParameters(const std::string & prefix)
{
    addParameter(Parameter { prefix + Constants::NahdXml::xmlKeyResonance().toStdString(), 0.3f, 0, 10000, 3000, 100 });
}

void DrumSynthV2Device::addCymbalParameters(const std::string & prefix)
{
    addParameter(Parameter { prefix + Constants::NahdXml::xmlKeyResonance().toStdString(), 0.3f, 0, 10000, 3000, 100 });
    addParameter(Parameter { prefix + Constants::NahdXml::xmlKeyAttack().toStdString(), 0.0f, 0, 10000, 0, 100 });
}

int DrumSynthV2Device::lpfSlope() const
{
    return static_cast<int>(m_lpfSlope);
}

void DrumSynthV2Device::setLpfSlope(int slope)
{
    setDiscreteParameterValue(Constants::NahdXml::xmlKeyLpfSlope().toStdString(), slope);
}

int DrumSynthV2Device::hpfSlope() const
{
    return static_cast<int>(m_hpfSlope);
}

void DrumSynthV2Device::setHpfSlope(int slope)
{
    setDiscreteParameterValue(Constants::NahdXml::xmlKeyHpfSlope().toStdString(), slope);
}

void DrumSynthV2Device::syncParameters()
{
    Device::syncParameters();

    if (auto p = parameter(Constants::NahdXml::xmlKeyLpfSlope().toStdString()); p) {
        m_lpfSlope = p->get().value();
    }
    if (auto p = parameter(Constants::NahdXml::xmlKeyHpfSlope().toStdString()); p) {
        m_hpfSlope = p->get().value();
    }
    for (auto && voice : m_voices) {
        voice.steepLpf = static_cast<int>(m_lpfSlope) == 1;
        voice.steepHpf = static_cast<int>(m_hpfSlope) == 1;
    }

    for (int i { 0 }; i < NumVoices; i++) {
        syncVoiceParameters(i);
    }
}

void DrumSynthV2Device::syncVoiceParameters(int index)
{
    const std::string prefix { voiceId(index) + "_" };

    if (auto p = parameter(prefix + Constants::NahdXml::xmlKeyLevel().toStdString()); p)
        m_voices.at(index).level = p->get().value();
    if (auto p = parameter(prefix + Constants::NahdXml::xmlKeyPan().toStdString()); p)
        m_voices.at(index).pan = p->get().value();
    if (auto p = parameter(prefix + Constants::NahdXml::xmlKeyCutoff().toStdString()); p)
        m_voices.at(index).lpfCutoff = p->get().value();
    if (auto p = parameter(prefix + Constants::NahdXml::xmlKeyHpfCutoff().toStdString()); p)
        m_voices.at(index).hpfCutoff = p->get().value();

    m_voices.at(index).updateEffects();

    syncAmpEnvelopeParameters(index, prefix);
    syncCommonEngineParameters(index, prefix);

    const auto voiceIdx { static_cast<VoiceIndex>(index) };
    if (voiceIdx == VoiceIndex::Kick)
        syncKickParameters(prefix);
    else if (voiceIdx == VoiceIndex::Snare)
        syncSnareParameters(prefix);
    else if (voiceIdx == VoiceIndex::Clap)
        syncClapParameters(prefix);
    else if (voiceIdx == VoiceIndex::ClosedHiHat || voiceIdx == VoiceIndex::OpenHiHat)
        syncHiHatParameters(index, prefix);
    else if (voiceIdx >= VoiceIndex::LowTom && voiceIdx <= VoiceIndex::HighTom)
        syncTomParameters(index, prefix);
    else if (voiceIdx >= VoiceIndex::Crash && voiceIdx <= VoiceIndex::ReverseCrash)
        syncCymbalParameters(index, prefix);
}

void DrumSynthV2Device::syncAmpEnvelopeParameters(int index, const std::string & prefix)
{
    auto & envelope = m_voices.at(index).ampEnvelope;

    if (const auto p = parameter(prefix + Constants::NahdXml::xmlKeyAmpAttack().toStdString()); p) {
        envelope.setAttackTime(ParameterMapper::mapCubic(p->get().value(), 0.0, AmpEnvelopeMaxAttackSeconds));
    }
    if (const auto p = parameter(prefix + Constants::NahdXml::xmlKeyAmpHold().toStdString()); p) {
        envelope.setHoldTime(ParameterMapper::mapCubic(p->get().value(), 0.0, AmpEnvelopeMaxHoldSeconds));
    }
    if (const auto p = parameter(prefix + Constants::NahdXml::xmlKeyAmpDecay().toStdString()); p) {
        envelope.setDecayTime(ParameterMapper::mapExponential(p->get().value(), AmpEnvelopeMinDecaySeconds, AmpEnvelopeMaxDecaySeconds));
    }
    if (const auto p = parameter(prefix + Constants::NahdXml::xmlKeyAmpSustain().toStdString()); p) {
        envelope.setSustainLevel(p->get().value());
    }
    if (const auto p = parameter(prefix + Constants::NahdXml::xmlKeyAmpRelease().toStdString()); p) {
        envelope.setReleaseTime(ParameterMapper::mapExponential(p->get().value(), AmpEnvelopeMinDecaySeconds, AmpEnvelopeMaxDecaySeconds));
    }
    if (const auto p = parameter(prefix + Constants::NahdXml::xmlKeyAmpCurve().toStdString()); p) {
        envelope.setCurve(p->get().value());
    }
}

void DrumSynthV2Device::syncCommonEngineParameters(int index, const std::string & prefix)
{
    DrumEngine & engine { *m_voices.at(index).engine };

    const auto voiceIdx { static_cast<VoiceIndex>(index) };
    if (auto p = parameter(prefix + Constants::NahdXml::xmlKeyTune().toStdString()); p) {
        const float val { p->get().value() };
        if (voiceIdx == VoiceIndex::Kick)
            static_cast<KickEngine &>(engine).setTune(val);
        else if (voiceIdx == VoiceIndex::Snare)
            static_cast<SnareEngine &>(engine).setTune(val);
        else if (voiceIdx == VoiceIndex::Clap)
            static_cast<ClapEngine &>(engine).setTune(val);
        else if (voiceIdx == VoiceIndex::ClosedHiHat || voiceIdx == VoiceIndex::OpenHiHat)
            static_cast<HiHatEngine &>(engine).setTune(val);
        else if (voiceIdx >= VoiceIndex::LowTom && voiceIdx <= VoiceIndex::HighTom)
            static_cast<TomEngine &>(engine).setTune(val);
        else if (voiceIdx >= VoiceIndex::Crash && voiceIdx <= VoiceIndex::ReverseCrash) {
            if (voiceIdx == VoiceIndex::Ride)
                static_cast<RideEngine &>(engine).setTune(val);
            else
                static_cast<CrashEngine &>(engine).setTune(val);
        }
    }

    if (auto p = parameter(prefix + Constants::NahdXml::xmlKeyDecay().toStdString()); p) {
        const float val { p->get().value() };
        if (voiceIdx == VoiceIndex::Kick)
            static_cast<KickEngine &>(engine).setDecay(val);
        else if (voiceIdx == VoiceIndex::Snare)
            static_cast<SnareEngine &>(engine).setDecay(val);
        else if (voiceIdx == VoiceIndex::Clap)
            static_cast<ClapEngine &>(engine).setDecay(val);
        else if (voiceIdx == VoiceIndex::ClosedHiHat || voiceIdx == VoiceIndex::OpenHiHat)
            static_cast<HiHatEngine &>(engine).setDecay(val);
        else if (voiceIdx >= VoiceIndex::LowTom && voiceIdx <= VoiceIndex::HighTom)
            static_cast<TomEngine &>(engine).setDecay(val);
        else if (voiceIdx >= VoiceIndex::Crash && voiceIdx <= VoiceIndex::ReverseCrash) {
            if (voiceIdx == VoiceIndex::Ride)
                static_cast<RideEngine &>(engine).setDecay(val);
            else
                static_cast<CrashEngine &>(engine).setDecay(val);
        }
    }
}

void DrumSynthV2Device::syncKickParameters(const std::string & prefix)
{
    auto & engine { static_cast<KickEngine &>(*m_voices.at(static_cast<int>(VoiceIndex::Kick)).engine) };
    if (auto p = parameter(prefix + Constants::NahdXml::xmlKeyAttack().toStdString()); p)
        engine.setAttack(p->get().value());
    if (auto p = parameter(prefix + Constants::NahdXml::xmlKeyClickTune().toStdString()); p)
        engine.setClickTune(p->get().value());
    if (auto p = parameter(prefix + Constants::NahdXml::xmlKeyPitchDepth().toStdString()); p)
        engine.setPitchDepth(p->get().value());
    if (auto p = parameter(prefix + Constants::NahdXml::xmlKeyPitchDecay().toStdString()); p)
        engine.setPitchDecay(p->get().value());
}

void DrumSynthV2Device::syncSnareParameters(const std::string & prefix)
{
    auto & engine { static_cast<SnareEngine &>(*m_voices.at(static_cast<int>(VoiceIndex::Snare)).engine) };
    if (auto p = parameter(prefix + Constants::NahdXml::xmlKeySnappy().toStdString()); p)
        engine.setSnappy(p->get().value());
    if (auto p = parameter(prefix + Constants::NahdXml::xmlKeyTone().toStdString()); p)
        engine.setTone(p->get().value());
}

void DrumSynthV2Device::syncClapParameters(const std::string & /*prefix*/)
{
    // Clap currently has no specific parameters beyond Tune and Decay
}

void DrumSynthV2Device::syncTomParameters(int index, const std::string & prefix)
{
    auto & engine { static_cast<TomEngine &>(*m_voices.at(index).engine) };
    if (auto p = parameter(prefix + Constants::NahdXml::xmlKeyPitchDepth().toStdString()); p)
        engine.setPitchDepth(p->get().value());
    if (auto p = parameter(prefix + Constants::NahdXml::xmlKeyPitchDecay().toStdString()); p)
        engine.setPitchDecay(p->get().value());
}

void DrumSynthV2Device::syncHiHatParameters(int index, const std::string & prefix)
{
    auto & engine { static_cast<HiHatEngine &>(*m_voices.at(index).engine) };
    if (auto p = parameter(prefix + Constants::NahdXml::xmlKeyResonance().toStdString()); p)
        engine.setResonance(p->get().value());
}

void DrumSynthV2Device::syncCymbalParameters(int index, const std::string & prefix)
{
    const auto voiceIdx { static_cast<VoiceIndex>(index) };
    if (voiceIdx == VoiceIndex::Crash || voiceIdx == VoiceIndex::ReverseCrash) {
        auto & engine { static_cast<CrashEngine &>(*m_voices.at(index).engine) };
        if (auto p = parameter(prefix + Constants::NahdXml::xmlKeyResonance().toStdString()); p)
            engine.setResonance(p->get().value());
        if (auto p = parameter(prefix + Constants::NahdXml::xmlKeyAttack().toStdString()); p)
            engine.setAttack(p->get().value());
    } else if (voiceIdx == VoiceIndex::Ride) {
        auto & engine { static_cast<RideEngine &>(*m_voices.at(index).engine) };
        if (auto p = parameter(prefix + Constants::NahdXml::xmlKeyResonance().toStdString()); p)
            engine.setResonance(p->get().value());
    }
}

bool DrumSynthV2Device::writeVoiceParameter(int voiceIndex, const std::string & paramName, float value, bool authored)
{
    const std::string prefix { voiceId(voiceIndex) + "_" };
    if (auto p = parameter(prefix + paramName); p) {
        if (authored) {
            p->get().setValue(value);
        } else {
            p->get().setAutomationValue(value);
        }
        syncVoiceParameters(voiceIndex);
        return true;
    }
    return false;
}

bool DrumSynthV2Device::automateVoiceParameter(int voiceIndex, const std::string & paramName, float value)
{
    return writeVoiceParameter(voiceIndex, paramName, value, false);
}

std::optional<double> DrumSynthV2Device::voiceElapsedSeconds(int voiceIndex) const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    if (voiceIndex < 0 || voiceIndex >= NumVoices) {
        return std::nullopt;
    }
    const auto & voice = m_voices.at(static_cast<size_t>(voiceIndex));
    if (!voice.engine->isActive() || !voice.ampEnvelope.isActive()) {
        return std::nullopt;
    }
    const auto rate = sampleRate();
    if (!rate) {
        return std::nullopt;
    }
    return static_cast<double>(voice.renderedFrames) / static_cast<double>(rate);
}

std::vector<double> DrumSynthV2Device::renderVoiceAlone(int voiceIndex, double sampleRate, double maxSeconds)
{
    if (voiceIndex < 0 || voiceIndex >= NumVoices || sampleRate <= 0.0) {
        return {};
    }

    // 512 rather than a whole buffer: the stop condition is only checked between blocks, so a
    // short drum would otherwise be rendered a good deal past the point it went quiet.
    constexpr uint32_t blockFrames { 512 };

    resetAudio();
    setSampleRate(sampleRate);
    processMidiNoteOn(voiceNote(voiceIndex), 127);

    std::vector<double> rendered;
    std::vector<double> buffer(blockFrames * 2, 0.0);
    const auto ceiling = static_cast<size_t>(maxSeconds * sampleRate) * 2;
    rendered.reserve(ceiling);

    while (rendered.size() < ceiling) {
        std::fill(buffer.begin(), buffer.end(), 0.0);
        AudioContext context { std::span(buffer.data(), buffer.size()), blockFrames, static_cast<uint32_t>(sampleRate) };
        processAudio(context);
        rendered.insert(rendered.end(), buffer.begin(), buffer.end());
        if (!hasActiveAudio()) {
            break;
        }
    }
    return rendered;
}

float DrumSynthV2Device::voiceParameterValue(int voiceIndex, const std::string & paramName) const
{
    const std::string prefix { voiceId(voiceIndex) + "_" };
    const auto p = parameter(prefix + paramName);
    return p ? p->get().value() : 0.0f;
}

bool DrumSynthV2Device::updateVoiceParameter(int voiceIndex, const std::string & paramName, float value)
{
    if (writeVoiceParameter(voiceIndex, paramName, value, true)) {
        emit dataChanged();
        return true;
    }
    return false;
}

} // namespace noteahead
