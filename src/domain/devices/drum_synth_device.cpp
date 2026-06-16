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

#include "drum_synth_device.hpp"

#include "../../common/constants.hpp"
#include "../../common/xml/project_reader.hpp"
#include "../../common/xml/project_writer.hpp"
#include "../../infra/midi/midi_cc_mapping.hpp"

#include "../dsp/drum/clap_engine.hpp"

namespace noteahead {

using namespace DrumSynth;

void DrumSynthDevice::Voice::updateEffects()
{
    volumeEffect->setVolume(level);
    panningEffect->setPan(pan);
    lpf->setCutoff(lpfCutoff);
    hpf->setCutoff(hpfCutoff);
}

DrumSynthDevice::DrumSynthDevice(std::string name)
  : m_name { std::move(name) }
{
    initializeVoices();

    for (int i { 0 }; i < NumVoices; i++) {
        addVoiceParameters(i);
    }

    setManualPan(panInternal());
    setManualVolume(volumeInternal());
    setManualGain(gainInternal());

    DrumSynthDevice::syncParameters();
}

std::string DrumSynthDevice::name() const
{
    return m_name;
}

std::string DrumSynthDevice::category() const
{
    return "Drums";
}

std::string DrumSynthDevice::typeName() const
{
    return Constants::drumSynthDeviceName().toStdString();
}

std::string DrumSynthDevice::typeIdString()
{
    return "a7f5a47e-4786-11f1-92b0-0b3f3bef9f74";
}

std::string DrumSynthDevice::typeId() const
{
    return typeIdString();
}

std::vector<MidiCcController> DrumSynthDevice::availableMidiCcControllers() const
{
    using namespace MidiCcMapping;
    std::vector<MidiCcController> list;

    list.push_back({ static_cast<uint8_t>(Controller::ChannelVolumeMSB), "Volume" });
    list.push_back({ static_cast<uint8_t>(Controller::PanMSB), "Pan" });

    using namespace DrumSynth;

    // Range 1: Voices 0-5
    for (int voice { 0 }; voice < NumVoicesRange1; voice++) {
        const uint8_t baseCc = CcStartRange1 + (voice * 3);
        const std::string voiceName = DrumSynth::voiceName(voice).toStdString();
        list.push_back({ baseCc, voiceName + " Pan" });
        list.push_back({ static_cast<uint8_t>(baseCc + 1), voiceName + " LPF" });
        list.push_back({ static_cast<uint8_t>(baseCc + 2), voiceName + " HPF" });
    }

    // Range 2: Voices 6-10
    for (int voice { 0 }; voice < NumVoicesRange2; voice++) {
        const uint8_t baseCc = CcStartRange2 + (voice * 3);
        const std::string voiceName = DrumSynth::voiceName(static_cast<int>(NumVoicesRange1) + voice).toStdString();
        list.push_back({ baseCc, voiceName + " Pan" });
        list.push_back({ static_cast<uint8_t>(baseCc + 1), voiceName + " LPF" });
        list.push_back({ static_cast<uint8_t>(baseCc + 2), voiceName + " HPF" });
    }

    return list;
}

void DrumSynthDevice::processMidiNoteOn(uint8_t note, uint8_t velocity)
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };

    using enum DrumSynth::MidiNote;

    // Hi-hat choking logic: Closed Hat or Pedal Hat chokes Open Hat
    if (note == static_cast<uint8_t>(ClosedHiHat) || note == static_cast<uint8_t>(PedalHiHat)) {
        m_voices[static_cast<int>(VoiceIndex::OpenHiHat)].engine->stop();
    }

    for (auto && voice : m_voices) {
        if (voice.midiNote == note) {
            const float vel { static_cast<float>(velocity) / 127.0f };
            voice.engine->trigger(vel);
            break;
        }
    }
}

void DrumSynthDevice::processMidiNoteOff(uint8_t note)
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };

    using enum DrumSynth::MidiNote;

    // Closed Hat choke logic: smoothly stop Open Hat on CHH/Pedal release
    if (note == static_cast<uint8_t>(ClosedHiHat) || note == static_cast<uint8_t>(PedalHiHat)) {
        m_voices[static_cast<int>(VoiceIndex::OpenHiHat)].engine->stop();
    }
}

void DrumSynthDevice::processMidiCc(uint8_t controller, uint8_t value, uint8_t /*channel*/)
{
    using namespace MidiCcMapping;

    bool changed { false };
    {
        const std::lock_guard<std::recursive_mutex> lock { mutex() };

        if (controller == static_cast<uint8_t>(Controller::ResetAllControllers)) {
            updatePanParameter(manualPanInternal(), false);
            updateVolumeParameter(manualVolumeInternal(), false);
            updateGainParameter(manualGainInternal(), false);
            changed = true;
        } else {
            const float val { static_cast<float>(value) / 127.0f };

            if (controller == static_cast<uint8_t>(Controller::ChannelVolumeMSB)) {
                changed |= updateVolumeParameter(val, false);
            } else if (controller == static_cast<uint8_t>(Controller::PanMSB)) {
                changed |= updatePanParameter(val, false);
            } else if (controller >= CcStartRange1 && controller < CcStartRange1 + (NumVoicesRange1 * 3)) {
                const int voiceIndex { (controller - CcStartRange1) / 3 };
                const int paramType { (controller - CcStartRange1) % 3 };
                if (paramType == 0)
                    changed |= updateVoiceParameter(voiceIndex, Constants::NahdXml::xmlKeyPan().toStdString(), val);
                else if (paramType == 1)
                    changed |= updateVoiceParameter(voiceIndex, Constants::NahdXml::xmlKeyCutoff().toStdString(), val);
                else if (paramType == 2)
                    changed |= updateVoiceParameter(voiceIndex, Constants::NahdXml::xmlKeyHpfCutoff().toStdString(), val);
            } else if (controller >= CcStartRange2 && controller < CcStartRange2 + (NumVoicesRange2 * 3)) {
                const int voiceIndex { NumVoicesRange1 + (controller - CcStartRange2) / 3 };
                const int paramType { (controller - CcStartRange2) % 3 };
                if (paramType == 0)
                    changed |= updateVoiceParameter(voiceIndex, Constants::NahdXml::xmlKeyPan().toStdString(), val);
                else if (paramType == 1)
                    changed |= updateVoiceParameter(voiceIndex, Constants::NahdXml::xmlKeyCutoff().toStdString(), val);
                else if (paramType == 2)
                    changed |= updateVoiceParameter(voiceIndex, Constants::NahdXml::xmlKeyHpfCutoff().toStdString(), val);
            }
        }
    }

    if (changed) {
        emit dataChanged();
    }
}

void DrumSynthDevice::processMidiAllNotesOff()
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    for (auto && voice : m_voices) {
        voice.engine->reset();
    }
}

void DrumSynthDevice::processAudio(AudioContext & context)
{
    setSampleRate(context.sampleRate);
    const uint32_t oversampledRate = context.sampleRate * 2;
    const std::lock_guard<std::recursive_mutex> lock { mutex() };

    for (auto && voice : m_voices) {
        voice.engine->setSampleRate(oversampledRate);
        voice.lpf->setSampleRate(oversampledRate);
        voice.hpf->setSampleRate(oversampledRate);
    }

    std::vector<float> oversampledBuffer(context.frameCount * 4, 0.0f);
    const float globalGain = linearGainInternal();

    for (uint32_t i = 0; i < context.frameCount; i++) {
        for (int os = 0; os < 2; os++) {
            float mixL = 0.0f;
            float mixR = 0.0f;

            for (auto && voice : m_voices) {
                if (voice.engine->isActive()) {
                    float sample = voice.engine->nextSample();
                    double l = sample;
                    double r = sample;

                    voice.lpf->process(l, r);
                    voice.hpf->process(l, r);
                    voice.volumeEffect->process(l, r);
                    voice.panningEffect->process(l, r);

                    mixL += static_cast<float>(l);
                    mixR += static_cast<float>(r);
                }
            }

            oversampledBuffer[(i * 2 + os) * 2] += mixL * globalGain * (1.0f - panInternal()) * 2.0f;
            oversampledBuffer[(i * 2 + os) * 2 + 1] += mixR * globalGain * panInternal() * 2.0f;
        }
    }

    for (uint32_t i = 0; i < context.frameCount; i++) {
        const float l0 = oversampledBuffer[i * 4];
        const float r0 = oversampledBuffer[i * 4 + 1];
        const float l1 = oversampledBuffer[i * 4 + 2];
        const float r1 = oversampledBuffer[i * 4 + 3];

        // Soft-clip at high rate and then downsample
        float l = m_oversamplerL.process(std::tanh(l0), std::tanh(l1));
        float r = m_oversamplerR.process(std::tanh(r0), std::tanh(r1));

        context.buffer[i * 2] += l * volumeInternal();
        context.buffer[i * 2 + 1] += r * volumeInternal();
    }
}

bool DrumSynthDevice::hasActiveAudio() const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    for (const auto & voice : m_voices) {
        if (voice.engine->isActive()) {
            return true;
        }
    }
    return false;
}

void DrumSynthDevice::reset()
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    Device::reset();
    resetAudio();
}

void DrumSynthDevice::resetAudio()
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    for (auto && voice : m_voices) {
        voice.engine->reset();
    }
    m_oversamplerL.reset();
    m_oversamplerR.reset();
}

void DrumSynthDevice::serializeToXml(ProjectWriter & writer) const
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

    writer.writeEndElement();
}

void DrumSynthDevice::deserializeFromXml(ProjectReader & reader)
{
    {
        const std::lock_guard<std::recursive_mutex> lock { mutex() };
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
                } else {
                    reader.skipCurrentElement();
                }
            }
        }

        syncParameters();
    }
    emit dataChanged();
}

int DrumSynthDevice::selectedVoice() const
{
    return m_selectedVoice;
}

void DrumSynthDevice::setSelectedVoice(int index)
{
    m_selectedVoice = std::clamp(index, 0, NumVoices - 1);
}

uint8_t DrumSynthDevice::voiceNote(int index) const
{
    if (index >= 0 && index < NumVoices) {
        return m_voices.at(index).midiNote;
    }
    return 0;
}

void DrumSynthDevice::initializeVoices()
{
    using enum DrumSynth::MidiNote;

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
        m_voices.at(i).lpf = std::make_shared<LowPassFilterEffect>();
        m_voices.at(i).hpf = std::make_shared<HighPassFilterEffect>();
        m_voices.at(i).volumeEffect = std::make_shared<VolumeEffect>();
        m_voices.at(i).panningEffect = std::make_shared<PanningEffect>();

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
            m_voices.at(i).engine = std::make_unique<CrashEngine>();
        } else if (voiceIdx == VoiceIndex::Ride) {
            m_voices.at(i).engine = std::make_unique<RideEngine>();
        } else if (voiceIdx == VoiceIndex::ReverseCrash) {
            auto crash = std::make_unique<CrashEngine>();
            crash->setMode(CrashEngine::Mode::Reverse);
            m_voices.at(i).engine = std::move(crash);
        }
    }
}

void DrumSynthDevice::addVoiceParameters(int index)
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

void DrumSynthDevice::addKickParameters(const std::string & prefix)
{
    addParameter(Parameter { prefix + Constants::NahdXml::xmlKeyAttack().toStdString(), 0.5f, 0, 10000, 5000, 100 });
    addParameter(Parameter { prefix + Constants::NahdXml::xmlKeyClickTune().toStdString(), 0.5f, 0, 10000, 5000, 100 });
    addParameter(Parameter { prefix + Constants::NahdXml::xmlKeyPitchDepth().toStdString(), 0.5f, 0, 10000, 5000, 100 });
    addParameter(Parameter { prefix + Constants::NahdXml::xmlKeyPitchDecay().toStdString(), 0.5f, 0, 10000, 5000, 100 });
}

void DrumSynthDevice::addSnareParameters(const std::string & prefix)
{
    addParameter(Parameter { prefix + Constants::NahdXml::xmlKeySnappy().toStdString(), 0.5f, 0, 10000, 5000, 100 });
    addParameter(Parameter { prefix + Constants::NahdXml::xmlKeyTone().toStdString(), 0.5f, 0, 10000, 5000, 100 });
}

void DrumSynthDevice::addTomParameters(const std::string & prefix)
{
    addParameter(Parameter { prefix + Constants::NahdXml::xmlKeyPitchDepth().toStdString(), 0.5f, 0, 10000, 5000, 100 });
    addParameter(Parameter { prefix + Constants::NahdXml::xmlKeyPitchDecay().toStdString(), 0.5f, 0, 10000, 5000, 100 });
}

void DrumSynthDevice::addHiHatParameters(const std::string & prefix)
{
    addParameter(Parameter { prefix + Constants::NahdXml::xmlKeyResonance().toStdString(), 0.3f, 0, 10000, 3000, 100 });
}

void DrumSynthDevice::addCymbalParameters(const std::string & prefix)
{
    addParameter(Parameter { prefix + Constants::NahdXml::xmlKeyResonance().toStdString(), 0.3f, 0, 10000, 3000, 100 });
    addParameter(Parameter { prefix + Constants::NahdXml::xmlKeyAttack().toStdString(), 0.0f, 0, 10000, 0, 100 });
}

void DrumSynthDevice::syncParameters()
{
    Device::syncParameters();

    for (int i { 0 }; i < NumVoices; i++) {
        syncVoiceParameters(i);
    }
}

void DrumSynthDevice::syncVoiceParameters(int index)
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

void DrumSynthDevice::syncCommonEngineParameters(int index, const std::string & prefix)
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

void DrumSynthDevice::syncKickParameters(const std::string & prefix)
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

void DrumSynthDevice::syncSnareParameters(const std::string & prefix)
{
    auto & engine { static_cast<SnareEngine &>(*m_voices.at(static_cast<int>(VoiceIndex::Snare)).engine) };
    if (auto p = parameter(prefix + Constants::NahdXml::xmlKeySnappy().toStdString()); p)
        engine.setSnappy(p->get().value());
    if (auto p = parameter(prefix + Constants::NahdXml::xmlKeyTone().toStdString()); p)
        engine.setTone(p->get().value());
}

void DrumSynthDevice::syncClapParameters(const std::string & /*prefix*/)
{
    // Clap currently has no specific parameters beyond Tune and Decay
}

void DrumSynthDevice::syncTomParameters(int index, const std::string & prefix)
{
    auto & engine { static_cast<TomEngine &>(*m_voices.at(index).engine) };
    if (auto p = parameter(prefix + Constants::NahdXml::xmlKeyPitchDepth().toStdString()); p)
        engine.setPitchDepth(p->get().value());
    if (auto p = parameter(prefix + Constants::NahdXml::xmlKeyPitchDecay().toStdString()); p)
        engine.setPitchDecay(p->get().value());
}

void DrumSynthDevice::syncHiHatParameters(int index, const std::string & prefix)
{
    auto & engine { static_cast<HiHatEngine &>(*m_voices.at(index).engine) };
    if (auto p = parameter(prefix + Constants::NahdXml::xmlKeyResonance().toStdString()); p)
        engine.setResonance(p->get().value());
}

void DrumSynthDevice::syncCymbalParameters(int index, const std::string & prefix)
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

bool DrumSynthDevice::updateVoiceParameter(int voiceIndex, const std::string & paramName, float value)
{
    const std::string prefix { voiceId(voiceIndex) + "_" };
    if (auto p = parameter(prefix + paramName); p) {
        p->get().setValue(value);
        syncVoiceParameters(voiceIndex);
        emit dataChanged();
        return true;
    }
    return false;
}

} // namespace noteahead
