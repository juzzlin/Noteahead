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

#include "string_voice_device.hpp"

#include "../../common/constants.hpp"
#include "../../common/parameter_mapper.hpp"
#include "../../contrib/SimpleLogger/src/simple_logger.hpp"
#include "../../infra/data_service.hpp"
#include "../../infra/midi/midi_cc_mapping.hpp"
#include "../../infra/xml/nahd_xml_reader.hpp"
#include "../../infra/xml/nahd_xml_writer.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>

namespace noteahead {

namespace {

double midiNoteToFreq(uint8_t note)
{
    return 440.0 * std::pow(2.0, (static_cast<double>(note) - 69.0) / 12.0);
}

} // namespace

void StringVoiceDevice::Voice::reset()
{
    stringOsc8.sync(0.0);
    stringOsc4.sync(0.0);
    voiceOsc8.sync(0.0);
    voiceOsc4.sync(0.0);

    stringEg.reset();
    voiceEg.reset();

    note = 0;
    velocity = 1.0f;
    active = false;
    triggerFrame = 0;
    pan = 0.5;
}

StringVoiceDevice::StringVoiceDevice(std::string name)
  : m_name { std::move(name) }
{
    addParameter(Parameter { Constants::NahdXml::xmlKeyStringsLevel8().toStdString(), 0.8f, 0, 100, 80, 100 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyStringsLevel4().toStdString(), 0.0f, 0, 100, 0, 100 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyStringsAttack().toStdString(), 0.2f, 0, 100, 20, 100 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyStringsRelease().toStdString(), 0.4f, 0, 100, 40, 100 });

    addParameter(Parameter { Constants::NahdXml::xmlKeyVoiceMale8().toStdString(), 0.8f, 0, 100, 80, 100 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyVoiceMale4().toStdString(), 0.0f, 0, 100, 0, 100 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyVoiceFemale8().toStdString(), 0.0f, 0, 100, 0, 100 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyVoiceFemale4().toStdString(), 0.0f, 0, 100, 0, 100 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyVoiceAttack().toStdString(), 0.25f, 0, 100, 25, 100 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyVoiceRelease().toStdString(), 0.45f, 0, 100, 45, 100 });

    addParameter(Parameter { Constants::NahdXml::xmlKeyVibratoRate().toStdString(), 0.596f, 0, 100, 60, 100 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyVibratoDepth().toStdString(), 0.0f, 0, 100, 0, 100 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyVibratoDelay().toStdString(), 0.0f, 0, 100, 0, 100 });

    addParameter(Parameter { Constants::NahdXml::xmlKeyEnsembleEnabled().toStdString(), 1.0f, 0, 1, 1, 1, Parameter::Type::Boolean });
    addParameter(Parameter { Constants::NahdXml::xmlKeyEnsembleMode().toStdString(), 0.0f, 0, 2, 0, 1, Parameter::Type::Discrete });

    addParameter(Parameter { Constants::NahdXml::xmlKeyVocoderEnabled().toStdString(), 0.0f, 0, 1, 0, 1, Parameter::Type::Boolean });
    addParameter(Parameter { Constants::NahdXml::xmlKeyVocoderSidechain().toStdString(), -1.0f, -1, static_cast<int>(Constants::deviceRackSize()) - 1, -1, 1, Parameter::Type::Discrete });

    addParameter(Parameter { Constants::NahdXml::xmlKeyLpfCutoff().toStdString(), 1.0f, 0, 10000, 10000, 100 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyHpfCutoff().toStdString(), 0.0f, 0, 10000, 0, 100 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyPanSpread().toStdString(), 0.0f, 0, 10000, 0, 100 });

    m_lpfL.setMode(CascadedSvf::Mode::LowPass);
    m_lpfR.setMode(CascadedSvf::Mode::LowPass);
    m_hpfL.setMode(CascadedSvf::Mode::HighPass);
    m_hpfR.setMode(CascadedSvf::Mode::HighPass);

    for (int i { 0 }; i < MaxVoices; ++i) {
        auto & v { m_voices[static_cast<size_t>(i)] };
        v.stringOsc8.setWaveform(PolyBlepOscillator::Waveform::Saw);
        v.stringOsc4.setWaveform(PolyBlepOscillator::Waveform::Saw);
        v.voiceOsc8.setWaveform(PolyBlepOscillator::Waveform::Saw);
        v.voiceOsc4.setWaveform(PolyBlepOscillator::Waveform::Saw);

        v.stringEg.setSustainLevel(1.0);
        v.voiceEg.setSustainLevel(1.0);
        v.stringEg.setDecayTime(0.0);
        v.voiceEg.setDecayTime(0.0);

        // Fixed per-voice pitch drift (+/- 0.8 cents), mimicking the natural tuning
        // imprecision of independent analog divide-down oscillator channels. This
        // gives even a single sustained note some "analog" character before the
        // BBD ensemble chorus engages.
        const double spread { (static_cast<double>(i) - (MaxVoices - 1) / 2.0) / (MaxVoices - 1) };
        const double detuneCents { spread * 1.6 };
        v.detuneRatio = std::pow(2.0, detuneCents / 1200.0);

        // Per-voice PWM phase offset so simultaneous notes don't pulse in lockstep.
        v.pwmPhase = static_cast<double>(i) / static_cast<double>(MaxVoices);

        // A choir is not one singer multiplied. Each voice gets its own vibrato phase and a rate a
        // couple of percent off the others, so they never settle into the lockstep a single shared
        // LFO gives them, plus a slow drift of its own that keeps the tuning from sitting perfectly
        // still the way only a synthesiser's does.
        v.vibratoPhase = static_cast<double>(i) / static_cast<double>(MaxVoices);
        v.vibratoRateRatio = 1.0 + spread * 0.08;
        v.driftPhase = static_cast<double>((i * 7) % MaxVoices) / static_cast<double>(MaxVoices);
        v.driftRateRatio = 1.0 + spread * 0.55;
    }

    syncParameters();
}

StringVoiceDevice::~StringVoiceDevice() = default;

std::string StringVoiceDevice::name() const
{
    return m_name;
}

std::string StringVoiceDevice::category() const
{
    return Constants::NahdXml::xmlValueSynths().toStdString();
}

std::string StringVoiceDevice::typeName() const
{
    return Constants::stringVoiceDeviceName().toStdString();
}

std::string StringVoiceDevice::typeIdString()
{
    return "d3c4b5a6-7e8f-9a0b-1c2d-3e4f5a6b7c8d";
}

std::string StringVoiceDevice::typeId() const
{
    return typeIdString();
}

std::vector<MidiCcController> StringVoiceDevice::availableMidiCcControllers() const
{
    using namespace MidiCcMapping;
    return {
        faderMidiCcController(),
        { static_cast<uint8_t>(Controller::PanMSB), "Pan" },
        { static_cast<uint8_t>(Controller::SustainPedal), "Sustain" }
    };
}

int StringVoiceDevice::findVoiceForNote(uint8_t note) const
{
    for (int i { 0 }; i < MaxVoices; ++i) {
        if (m_voices[i].active && m_voices[i].note == note) {
            return i;
        }
    }
    return -1;
}

int StringVoiceDevice::allocateVoice()
{
    // 1. Prefer a fully idle voice.
    for (int i { 0 }; i < MaxVoices; ++i) {
        if (!m_voices[static_cast<size_t>(i)].active) {
            return i;
        }
    }

    // 2. All voices are busy: prefer stealing one that's already releasing,
    //    picking the quietest one so the cut is as inaudible as possible.
    int bestReleasing { -1 };
    double lowestLevel { std::numeric_limits<double>::max() };
    for (int i { 0 }; i < MaxVoices; ++i) {
        const auto & v { m_voices[static_cast<size_t>(i)] };
        const bool releasing { v.stringEg.state() == AdsrEnvelope::State::Release && v.voiceEg.state() == AdsrEnvelope::State::Release };
        if (releasing) {
            const double level { v.stringEg.value() + v.voiceEg.value() };
            if (level < lowestLevel) {
                lowestLevel = level;
                bestReleasing = i;
            }
        }
    }
    if (bestReleasing >= 0) {
        return bestReleasing;
    }

    // 3. No voice is releasing either: steal the oldest one still sounding.
    int oldest { 0 };
    uint64_t oldestTriggerId { m_voices[0].triggerId };
    for (int i { 1 }; i < MaxVoices; ++i) {
        if (m_voices[static_cast<size_t>(i)].triggerId < oldestTriggerId) {
            oldestTriggerId = m_voices[static_cast<size_t>(i)].triggerId;
            oldest = i;
        }
    }
    return oldest;
}

void StringVoiceDevice::processMidiNoteOn(uint8_t note, uint8_t velocity)
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };

    juzzlin::L("StringVoiceDevice").info() << "Note On: " << int(note) << " velocity: " << int(velocity);

    int idx { findVoiceForNote(note) };
    if (idx < 0) {
        idx = allocateVoice();
    }

    auto & v { m_voices[static_cast<size_t>(idx)] };
    v.reset();
    v.triggerId = m_nextTriggerId++;

    const double sampleRateVal { static_cast<double>(sampleRate()) };
    v.stringOsc8.setSampleRate(sampleRateVal);
    v.stringOsc4.setSampleRate(sampleRateVal);
    v.voiceOsc8.setSampleRate(sampleRateVal);
    v.voiceOsc4.setSampleRate(sampleRateVal);

    v.stringEg.setSampleRate(sampleRateVal);
    v.voiceEg.setSampleRate(sampleRateVal);

    const double strAttackSec { ParameterMapper::mapExponential(m_stringsAttack, 0.001, 5.0) };
    const double strReleaseSec { ParameterMapper::mapExponential(m_stringsRelease, 0.005, 10.0) };
    const double vocAttackSec { ParameterMapper::mapExponential(m_voiceAttack, 0.001, 5.0) };
    const double vocReleaseSec { ParameterMapper::mapExponential(m_voiceRelease, 0.005, 10.0) };

    v.stringEg.setAttackTime(strAttackSec);
    v.stringEg.setReleaseTime(strReleaseSec);
    v.voiceEg.setAttackTime(vocAttackSec);
    v.voiceEg.setReleaseTime(vocReleaseSec);

    v.stringEg.trigger();
    v.voiceEg.trigger();

    v.note = note;
    v.velocity = static_cast<float>(velocity) / 127.0f;
    v.active = true;
    v.triggerFrame = 0;
    const float side { (idx % 2 == 0) ? -1.0f : 1.0f };
    v.pan = 0.5 + (side * m_panSpread * 0.5);
}

void StringVoiceDevice::processMidiNoteOff(uint8_t note)
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };

    juzzlin::L("StringVoiceDevice").info() << "Note Off: " << int(note);

    const double strReleaseSec { ParameterMapper::mapExponential(m_stringsRelease, 0.005, 10.0) };
    const double vocReleaseSec { ParameterMapper::mapExponential(m_voiceRelease, 0.005, 10.0) };

    for (auto & v : m_voices) {
        if (v.active && v.note == note) {
            v.stringEg.setReleaseTime(strReleaseSec);
            v.voiceEg.setReleaseTime(vocReleaseSec);
            v.stringEg.release();
            v.voiceEg.release();
        }
    }
}

void StringVoiceDevice::processMidiCc(uint8_t controller, uint8_t value, uint8_t)
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
                changed |= updateVolumeParameter(faderPositionFromMidiCc(value), false);
            } else if (controller == static_cast<uint8_t>(Controller::PanMSB)) {
                changed |= updatePanParameter(val, false);
            }
        }
    }

    if (changed) {
        emit parametersChanged();
    }
}

void StringVoiceDevice::processMidiAllNotesOff()
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    for (auto & v : m_voices) {
        if (v.active) {
            v.stringEg.release();
            v.voiceEg.release();
        }
    }
}

void StringVoiceDevice::processAudio(AudioContext & context)
{
    setSampleRate(context.sampleRate);
    const std::lock_guard<std::recursive_mutex> lock { mutex() };

    const double sRate { static_cast<double>(context.sampleRate) };

    const double vibRateHz { m_vibratoRate * 9.9 + 0.1 };

    m_lpfL.setSampleRate(sRate);
    m_lpfR.setSampleRate(sRate);
    m_hpfL.setSampleRate(sRate);
    m_hpfR.setSampleRate(sRate);
    m_lpfL.setCutoff(static_cast<double>(m_lpfCutoff));
    m_lpfR.setCutoff(static_cast<double>(m_lpfCutoff));
    m_hpfL.setCutoff(static_cast<double>(m_hpfCutoff));
    m_hpfR.setCutoff(static_cast<double>(m_hpfCutoff));

    m_formantFiltersL.setSampleRate(sRate);
    m_formantFiltersR.setSampleRate(sRate);
    m_ensemble.setSampleRate(sRate);
    m_panner.setPan(static_cast<double>(panInternal()));

    // Check Vocoder sidechain
    const auto scIdx { vocoderSidechainIndex() };
    const bool hasSidechain { m_vocoderEnabled && scIdx && *scIdx < context.deviceOutputBuffers.size() };
    const auto sidechainBuffer { hasSidechain ? context.deviceOutputBuffers[*scIdx] : std::span<const double> {} };

    m_vocoder.setSampleRate(sRate);

    // Equal-power headroom compensation: without this, stacking more than a
    // couple of sustained notes clips the summed output (each voice was
    // previously mixed at a fixed level regardless of how many others were
    // also sounding).
    const int activeVoiceCount { static_cast<int>(std::ranges::count_if(m_voices, [](const auto & voice) { return voice.active; })) };
    const double polyphonyGain { 1.0 / std::sqrt(static_cast<double>(std::max(1, activeVoiceCount))) };

    constexpr double pwmRateHz { 0.13 }; // slow, subtle pulse-width drift for a less static choir tone

    // Depth and speed of the per-voice tuning drift. Small enough to read as a singer rather than
    // as a detuned oscillator: a held sung note wanders by a few cents over a second or two.
    constexpr double DriftCents { 3.5 };
    constexpr double DriftRateHz { 0.45 };

    for (uint32_t i { 0 }; i < context.frameCount; ++i) {

        double stringsSumL { 0.0 };
        double stringsSumR { 0.0 };
        double maleSumL { 0.0 };
        double maleSumR { 0.0 };
        double femaleSumL { 0.0 };
        double femaleSumR { 0.0 };

        for (auto & v : m_voices) {
            if (!v.active) {
                continue;
            }

            v.stringEg.setSampleRate(sRate);
            v.voiceEg.setSampleRate(sRate);

            const double strEnv { v.stringEg.nextSample() };
            const double vocEnv { v.voiceEg.nextSample() };

            if (v.stringEg.isSilent() && v.voiceEg.isSilent()) {
                v.active = false;
                continue;
            }

            // Calculate vibrato offset
            double currentVibratoDepth { m_vibratoDepth * 0.06 }; // scale depth to max ~1 semi-tone
            const double delaySamples { m_vibratoDelay * 3.0 * sRate };
            if (delaySamples > 0.0) {
                const double age { static_cast<double>(v.triggerFrame) };
                if (age < delaySamples) {
                    currentVibratoDepth *= (age / delaySamples);
                }
            }

            v.triggerFrame++;

            v.vibratoPhase += vibRateHz * v.vibratoRateRatio / sRate;
            if (v.vibratoPhase >= 1.0) {
                v.vibratoPhase -= 1.0;
            }
            const double vibratoLfoVal { std::sin(2.0 * std::numbers::pi * v.vibratoPhase) };

            v.driftPhase += DriftRateHz * v.driftRateRatio / sRate;
            if (v.driftPhase >= 1.0) {
                v.driftPhase -= 1.0;
            }
            const double driftOctaves { std::sin(2.0 * std::numbers::pi * v.driftPhase) * DriftCents / 1200.0 };

            v.pwmPhase += pwmRateHz / sRate;
            if (v.pwmPhase >= 1.0) {
                v.pwmPhase -= 1.0;
            }
            const double shapeVal { 0.05 + 0.05 * std::sin(2.0 * std::numbers::pi * v.pwmPhase) };
            v.voiceOsc8.setShape(shapeVal);
            v.voiceOsc4.setShape(shapeVal);

            const double pitchModRatio { std::exp2(vibratoLfoVal * currentVibratoDepth) };

            const double baseFreq { midiNoteToFreq(v.note) * pitchModRatio * v.detuneRatio };
            // The drift is the voice section's alone: the strings' fixed detune is meant to read as
            // analog imprecision, not as a singer holding a note.
            const double voiceFreq { baseFreq * std::exp2(driftOctaves) };

            // Setup oscillators
            v.stringOsc8.setSampleRate(sRate);
            v.stringOsc8.setFrequency(baseFreq);
            v.stringOsc4.setSampleRate(sRate);
            v.stringOsc4.setFrequency(baseFreq * 2.0);

            v.voiceOsc8.setSampleRate(sRate);
            v.voiceOsc8.setFrequency(voiceFreq);
            v.voiceOsc4.setSampleRate(sRate);
            v.voiceOsc4.setFrequency(voiceFreq * 2.0);

            // Generate oscillator samples
            const double strSample8 { v.stringOsc8.nextSample() };
            const double strSample4 { v.stringOsc4.nextSample() };

            const double vocSample8 { v.voiceOsc8.nextSample() };
            const double vocSample4 { v.voiceOsc4.nextSample() };

            const double voiceGain { static_cast<double>(v.velocity) * linearGainInternal() * 0.25 * polyphonyGain };

            const double strSample { (strSample8 * m_stringsLevel8 + strSample4 * m_stringsLevel4) * strEnv * voiceGain };
            const double maleSample { (vocSample8 * m_voiceMale8 + vocSample4 * m_voiceMale4) * vocEnv * voiceGain };
            const double femaleSample { (vocSample8 * m_voiceFemale8 + vocSample4 * m_voiceFemale4) * vocEnv * voiceGain };

            stringsSumL += strSample * (1.0 - v.pan);
            stringsSumR += strSample * v.pan;

            maleSumL += maleSample * (1.0 - v.pan);
            maleSumR += maleSample * v.pan;

            femaleSumL += femaleSample * (1.0 - v.pan);
            femaleSumR += femaleSample * v.pan;
        }

        // Apply Formant filter bank to Voice section (male and female registers
        // are filtered independently so they don't bleed into each other's
        // formant coloration)
        double maleOutL { 0.0 };
        double maleOutR { 0.0 };
        double femaleOutL { 0.0 };
        double femaleOutR { 0.0 };
        m_formantFiltersL.process(maleSumL, femaleSumL, maleOutL, femaleOutL);
        m_formantFiltersR.process(maleSumR, femaleSumR, maleOutR, femaleOutR);
        const double voiceFilteredSumL { maleOutL + femaleOutL };
        const double voiceFilteredSumR { maleOutR + femaleOutR };

        // Combine Strings and Choir paths
        double sampleL { stringsSumL + voiceFilteredSumL };
        double sampleR { stringsSumR + voiceFilteredSumR };

        // Apply Vocoder if active
        if (m_vocoderEnabled && !sidechainBuffer.empty()) {
            const double modL { sidechainBuffer[i * 2] };
            const double modR { sidechainBuffer[i * 2 + 1] };
            double vocL { 0.0 };
            double vocR { 0.0 };
            m_vocoder.process(sampleL, sampleR, modL, modR, vocL, vocR);
            sampleL = vocL;
            sampleR = vocR;
        }

        // Apply Ensemble Chorus
        m_ensemble.process(sampleL, sampleR);

        // Built-in tone shaping
        sampleL = m_hpfL.process(m_lpfL.process(sampleL));
        sampleR = m_hpfR.process(m_lpfR.process(sampleR));

        // Safety soft-clip: transparent well below unity, only engages to
        // prevent hard clipping in extreme parameter combinations.
        sampleL = std::tanh(sampleL);
        sampleR = std::tanh(sampleR);

        // Apply global panner. The fader is applied by the engine, either side of the insert
        // rack depending on the device's fader position.
        m_panner.process(sampleL, sampleR);

        // Write to accumulation buffers
        context.buffer[i * 2] += sampleL;
        context.buffer[i * 2 + 1] += sampleR;
    }
}

bool StringVoiceDevice::hasActiveAudio() const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    return std::ranges::any_of(m_voices, [](const auto & voice) { return voice.active; });
}

std::vector<size_t> StringVoiceDevice::sidechainDependencies() const
{
    auto deps { Device::sidechainDependencies() };
    if (m_vocoderEnabled) {
        if (const auto idx { vocoderSidechainIndex() }) {
            deps.push_back(*idx);
        }
    }
    return deps;
}

void StringVoiceDevice::reset()
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    Device::reset();
    resetAudio();
    syncParameters();
}

void StringVoiceDevice::resetAudio()
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    for (auto & v : m_voices) {
        v.reset();
    }
    m_formantFiltersL.reset();
    m_formantFiltersR.reset();
    m_ensemble.reset();
    m_lpfL.reset();
    m_lpfR.reset();
    m_hpfL.reset();
    m_hpfR.reset();
}

void StringVoiceDevice::serializeToXml(ProjectWriter & writer) const
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

void StringVoiceDevice::deserializeFromXml(ProjectReader & reader)
{
    {
        const std::lock_guard<std::recursive_mutex> lock { mutex() };
        deserializeAttributesFromXml(reader);

        while (!reader.atEnd() && !reader.hasError()) {
            const auto token { reader.readNext() };
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
        setManualPan(panInternal());
        setManualVolume(volumeInternal());
        setManualGain(gainInternal());
    }
    emit dataChanged();
}

// Getters and Setters implementation
float StringVoiceDevice::stringsLevel8() const
{
    return m_stringsLevel8;
}

void StringVoiceDevice::setStringsLevel8(float val)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyStringsLevel8().toStdString(), val);
}

float StringVoiceDevice::stringsLevel4() const
{
    return m_stringsLevel4;
}

void StringVoiceDevice::setStringsLevel4(float val)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyStringsLevel4().toStdString(), val);
}

float StringVoiceDevice::stringsAttack() const
{
    return m_stringsAttack;
}

void StringVoiceDevice::setStringsAttack(float val)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyStringsAttack().toStdString(), val);
}

float StringVoiceDevice::stringsRelease() const
{
    return m_stringsRelease;
}

void StringVoiceDevice::setStringsRelease(float val)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyStringsRelease().toStdString(), val);
}

float StringVoiceDevice::voiceMale8() const
{
    return m_voiceMale8;
}

void StringVoiceDevice::setVoiceMale8(float val)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyVoiceMale8().toStdString(), val);
}

float StringVoiceDevice::voiceMale4() const
{
    return m_voiceMale4;
}

void StringVoiceDevice::setVoiceMale4(float val)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyVoiceMale4().toStdString(), val);
}

float StringVoiceDevice::voiceFemale8() const
{
    return m_voiceFemale8;
}

void StringVoiceDevice::setVoiceFemale8(float val)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyVoiceFemale8().toStdString(), val);
}

float StringVoiceDevice::voiceFemale4() const
{
    return m_voiceFemale4;
}

void StringVoiceDevice::setVoiceFemale4(float val)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyVoiceFemale4().toStdString(), val);
}

float StringVoiceDevice::voiceAttack() const
{
    return m_voiceAttack;
}

void StringVoiceDevice::setVoiceAttack(float val)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyVoiceAttack().toStdString(), val);
}

float StringVoiceDevice::voiceRelease() const
{
    return m_voiceRelease;
}

void StringVoiceDevice::setVoiceRelease(float val)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyVoiceRelease().toStdString(), val);
}

float StringVoiceDevice::vibratoRate() const
{
    return m_vibratoRate;
}

void StringVoiceDevice::setVibratoRate(float val)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyVibratoRate().toStdString(), val);
}

float StringVoiceDevice::vibratoDepth() const
{
    return m_vibratoDepth;
}

void StringVoiceDevice::setVibratoDepth(float val)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyVibratoDepth().toStdString(), val);
}

float StringVoiceDevice::vibratoDelay() const
{
    return m_vibratoDelay;
}

void StringVoiceDevice::setVibratoDelay(float val)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyVibratoDelay().toStdString(), val);
}

bool StringVoiceDevice::ensembleEnabled() const
{
    return m_ensembleEnabled;
}

void StringVoiceDevice::setEnsembleEnabled(bool val)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyEnsembleEnabled().toStdString(), val ? 1.0f : 0.0f);
}

int StringVoiceDevice::ensembleMode() const
{
    return m_ensembleMode;
}

void StringVoiceDevice::setEnsembleMode(int val)
{
    setDiscreteParameterValue(Constants::NahdXml::xmlKeyEnsembleMode().toStdString(), val);
}

bool StringVoiceDevice::vocoderEnabled() const
{
    return m_vocoderEnabled;
}

void StringVoiceDevice::setVocoderEnabled(bool val)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyVocoderEnabled().toStdString(), val ? 1.0f : 0.0f);
}

int StringVoiceDevice::vocoderSidechain() const
{
    return m_vocoderSidechain;
}

void StringVoiceDevice::setVocoderSidechain(int val)
{
    setDiscreteParameterValue(Constants::NahdXml::xmlKeyVocoderSidechain().toStdString(), val);
}

std::optional<size_t> StringVoiceDevice::vocoderSidechainIndex() const
{
    if (const auto p = parameter(Constants::NahdXml::xmlKeyVocoderSidechain().toStdString()); p) {
        const int val { p->get().xmlValue() };
        if (val >= 0) {
            return static_cast<size_t>(val);
        }
    }
    return std::nullopt;
}

float StringVoiceDevice::lpfCutoff() const
{
    return m_lpfCutoff;
}

void StringVoiceDevice::setLpfCutoff(float val)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyLpfCutoff().toStdString(), val);
}

float StringVoiceDevice::hpfCutoff() const
{
    return m_hpfCutoff;
}

void StringVoiceDevice::setHpfCutoff(float val)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyHpfCutoff().toStdString(), val);
}

float StringVoiceDevice::panSpread() const
{
    return m_panSpread;
}

void StringVoiceDevice::setPanSpread(float val)
{
    setContinuousParameterValue(Constants::NahdXml::xmlKeyPanSpread().toStdString(), val);
}

void StringVoiceDevice::syncParameters()
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };

    Device::syncParameters();

    if (const auto p = parameter(Constants::NahdXml::xmlKeyStringsLevel8().toStdString()); p) {
        m_stringsLevel8 = p->get().value();
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyStringsLevel4().toStdString()); p) {
        m_stringsLevel4 = p->get().value();
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyStringsAttack().toStdString()); p) {
        m_stringsAttack = p->get().value();
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyStringsRelease().toStdString()); p) {
        m_stringsRelease = p->get().value();
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyVoiceMale8().toStdString()); p) {
        m_voiceMale8 = p->get().value();
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyVoiceMale4().toStdString()); p) {
        m_voiceMale4 = p->get().value();
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyVoiceFemale8().toStdString()); p) {
        m_voiceFemale8 = p->get().value();
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyVoiceFemale4().toStdString()); p) {
        m_voiceFemale4 = p->get().value();
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyVoiceAttack().toStdString()); p) {
        m_voiceAttack = p->get().value();
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyVoiceRelease().toStdString()); p) {
        m_voiceRelease = p->get().value();
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyVibratoRate().toStdString()); p) {
        m_vibratoRate = p->get().value();
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyVibratoDepth().toStdString()); p) {
        m_vibratoDepth = p->get().value();
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyVibratoDelay().toStdString()); p) {
        m_vibratoDelay = p->get().value();
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyEnsembleEnabled().toStdString()); p) {
        m_ensembleEnabled = p->get().xmlValue() != 0;
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyEnsembleMode().toStdString()); p) {
        m_ensembleMode = p->get().xmlValue();
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyVocoderEnabled().toStdString()); p) {
        m_vocoderEnabled = p->get().xmlValue() != 0;
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyVocoderSidechain().toStdString()); p) {
        m_vocoderSidechain = p->get().xmlValue();
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyLpfCutoff().toStdString()); p) {
        m_lpfCutoff = p->get().value();
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyHpfCutoff().toStdString()); p) {
        m_hpfCutoff = p->get().value();
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyPanSpread().toStdString()); p) {
        m_panSpread = p->get().value();
    }

    m_ensemble.setEnabled(m_ensembleEnabled);
    m_ensemble.setMode(m_ensembleMode);
}

} // namespace noteahead
