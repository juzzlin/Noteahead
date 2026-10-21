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

#include "ju_synth_08_device.hpp"
#include "../../common/constants.hpp"
#include "../../common/xml/project_reader.hpp"
#include "../../common/xml/project_writer.hpp"
#include "../../infra/midi/midi_cc_mapping.hpp"
#include "ju_synth_08_presets.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace noteahead {

namespace {

namespace C = Constants::NahdXml;

constexpr double TwoPi { 2.0 * std::numbers::pi };
constexpr double MaxPitchLfoSemitones { 2.0 };

// PolyBLEP residual function for bandlimited discontinuities
double polyBlep(double t, double dt)
{
    if (t < dt) {
        t /= dt;
        return t + t - t * t - 1.0;
    }
    if (t > 1.0 - dt) {
        t = (t - 1.0) / dt;
        return t * t + t + t + 1.0;
    }
    return 0.0;
}

Lfo::Waveform lfoWaveformFromIndex(int index)
{
    switch (index) {
    case 0:
        return Lfo::Waveform::Sine;
    case 1:
        return Lfo::Waveform::Triangle;
    case 2:
        return Lfo::Waveform::Saw;
    case 3:
        return Lfo::Waveform::Square;
    case 4:
    case 5:
        return Lfo::Waveform::Random;
    default:
        return Lfo::Waveform::Sine;
    }
}

} // namespace

JuSynth08Device::JuSynth08Device(const std::string & name)
  : Device {}
  , m_name { name.empty() ? Constants::juSynth08DeviceName().toStdString() : name }
{
    setSampleRate(Constants::defaultSampleRate());
    m_lfo.setFreeRunning(true);
    setupParameters();
    syncParameters();
}

JuSynth08Device::~JuSynth08Device() = default;

std::string JuSynth08Device::name() const
{
    return m_name;
}

std::string JuSynth08Device::category() const
{
    return "Synth";
}

std::string JuSynth08Device::typeName() const
{
    return Constants::juSynth08DeviceName().toStdString();
}

std::string JuSynth08Device::typeIdString()
{
    return "4b790d98-dc58-45e0-9e5c-cb69e5d7778b";
}

std::string JuSynth08Device::typeId() const
{
    return typeIdString();
}

void JuSynth08Device::setupParameters()
{
    addParameter(Parameter { C::xmlKeyLfoRate().toStdString(), 0.2f, 0, 10000, 2000, 100 });
    addParameter(Parameter { C::xmlKeyLfoDelay().toStdString(), 0.0f, 0, 10000, 0, 100 });
    addParameter(Parameter { C::xmlKeyWaveform().toStdString(), 0.0f, 0, 5, 0, 1, Parameter::Type::Discrete });

    addParameter(Parameter { C::xmlKeyPitchLfoDepth().toStdString(), 0.0f, 0, 10000, 0, 100 });
    addParameter(Parameter { C::xmlKeyPitchEnvDepth().toStdString(), 0.0f, -10000, 10000, 0, 100 });
    addParameter(Parameter { C::xmlKeyPitchEnvTarget().toStdString(), 2.0f, 0, 2, 2, 1, Parameter::Type::Discrete });
    addParameter(Parameter { C::xmlKeyPulseWidth().toStdString(), 0.5f, 0, 10000, 5000, 100 });
    addParameter(Parameter { C::xmlKeyPwmSource().toStdString(), 0.0f, 0, 2, 0, 1, Parameter::Type::Discrete });
    addParameter(Parameter { C::xmlKeyCrossMod().toStdString(), 0.0f, 0, 10000, 0, 100 });

    addParameter(Parameter { C::xmlKeyVco1Range().toStdString(), 1.0f, 0, 3, 1, 1, Parameter::Type::Discrete });
    addParameter(Parameter { C::xmlKeyVco1Waveform().toStdString(), 1.0f, 0, 3, 1, 1, Parameter::Type::Discrete });
    addParameter(Parameter { C::xmlKeyVco1Level().toStdString(), 0.8f, 0, 10000, 8000, 100 });

    addParameter(Parameter { C::xmlKeyVco2Range().toStdString(), 1.0f, 0, 4, 1, 1, Parameter::Type::Discrete });
    addParameter(Parameter { C::xmlKeyVco2Waveform().toStdString(), 0.0f, 0, 3, 0, 1, Parameter::Type::Discrete });
    addParameter(Parameter { C::xmlKeyVco2FineTune().toStdString(), 0.0f, -10000, 10000, 0, 100 });
    addParameter(Parameter { C::xmlKeyVco2Sync().toStdString(), 0.0f, 0, 1, 0, 1, Parameter::Type::Boolean });
    addParameter(Parameter { C::xmlKeyVco2Level().toStdString(), 0.8f, 0, 10000, 8000, 100 });

    addParameter(Parameter { C::xmlKeyNoiseLevel().toStdString(), 0.0f, 0, 10000, 0, 100 });
    addParameter(Parameter { C::xmlKeyHpfCutoff().toStdString(), 0.0f, 0, 10000, 0, 100 });

    addParameter(Parameter { C::xmlKeyVcfSlope().toStdString(), 1.0f, 0, 1, 1, 1, Parameter::Type::Discrete });
    addParameter(Parameter { C::xmlKeyLpfCutoff().toStdString(), 1.0f, 0, 10000, 10000, 100 });
    addParameter(Parameter { C::xmlKeyLpfResonance().toStdString(), 0.0f, 0, 10000, 0, 100 });
    addParameter(Parameter { C::xmlKeyEnvMod().toStdString(), 0.0f, 0, 10000, 0, 100 });
    addParameter(Parameter { C::xmlKeyEnvPolarity().toStdString(), 0.0f, 0, 1, 0, 1, Parameter::Type::Boolean });
    addParameter(Parameter { C::xmlKeyFilterLfoDepth().toStdString(), 0.0f, 0, 10000, 0, 100 });
    addParameter(Parameter { C::xmlKeyKeyTrack().toStdString(), 1.0f, 0, 10000, 10000, 100 });

    addParameter(Parameter { C::xmlKeyVcaMode().toStdString(), 0.0f, 0, 1, 0, 1, Parameter::Type::Discrete });
    addParameter(Parameter { C::xmlKeyVelocitySensitivity().toStdString(), 0.0f, 0, 10000, 0, 100 });
    addParameter(Parameter { C::xmlKeyVcaLfoDepth().toStdString(), 0.0f, 0, 10000, 0, 100 });

    addParameter(Parameter { C::xmlKeyEnv1Attack().toStdString(), 0.01f, 0, 10000, 100, 100 });
    addParameter(Parameter { C::xmlKeyEnv1Decay().toStdString(), 0.3f, 0, 10000, 3000, 100 });
    addParameter(Parameter { C::xmlKeyEnv1Sustain().toStdString(), 0.5f, 0, 10000, 5000, 100 });
    addParameter(Parameter { C::xmlKeyEnv1Release().toStdString(), 0.2f, 0, 10000, 2000, 100 });
    addParameter(Parameter { C::xmlKeyEnv1KeyFollow().toStdString(), 0.0f, 0, 1, 0, 1, Parameter::Type::Boolean });

    addParameter(Parameter { C::xmlKeyEnv2Attack().toStdString(), 0.01f, 0, 10000, 100, 100 });
    addParameter(Parameter { C::xmlKeyEnv2Decay().toStdString(), 0.3f, 0, 10000, 3000, 100 });
    addParameter(Parameter { C::xmlKeyEnv2Sustain().toStdString(), 1.0f, 0, 10000, 10000, 100 });
    addParameter(Parameter { C::xmlKeyEnv2Release().toStdString(), 0.2f, 0, 10000, 2000, 100 });
    addParameter(Parameter { C::xmlKeyEnv2KeyFollow().toStdString(), 0.0f, 0, 1, 0, 1, Parameter::Type::Boolean });

    addParameter(Parameter { C::xmlKeyDelayTime().toStdString(), 0.3f, 0, 10000, 3000, 100 });
    addParameter(Parameter { C::xmlKeyDelayFeedback().toStdString(), 0.3f, 0, 10000, 3000, 100 });
    addParameter(Parameter { C::xmlKeyDelayLevel().toStdString(), 0.0f, 0, 10000, 0, 100 });

    addParameter(Parameter { C::xmlKeyVoiceMode().toStdString(), 0.0f, 0, 2, 0, 1, Parameter::Type::Discrete });
    addParameter(Parameter { C::xmlKeyUnisonDetune().toStdString(), 0.2f, 0, 10000, 2000, 100 });
    addParameter(Parameter { C::xmlKeyPortamento().toStdString(), 0.0f, 0, 10000, 0, 100 });
    addParameter(Parameter { C::xmlKeyPitchBendRange().toStdString(), 2.0f, 0, 24, 2, 1, Parameter::Type::Discrete });
    addParameter(Parameter { C::xmlKeyPanSpread().toStdString(), 0.5f, 0, 10000, 5000, 100 });

    using Display = ParameterDisplay;
    setParameterDisplay(C::xmlKeyLfoRate().toStdString(), { .mapping = Display::Mapping::Exponential, .unit = "Hz", .min = 0.1, .max = 30.0, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "LFO Rate") });
    setParameterDisplay(C::xmlKeyLfoDelay().toStdString(), { .mapping = Display::Mapping::Linear, .unit = "ms", .min = 0.0, .max = 3000.0, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "LFO Delay") });
    setParameterDisplay(C::xmlKeyPitchLfoDepth().toStdString(), { .mapping = Display::Mapping::Linear, .unit = "%", .min = 0.0, .max = 100.0, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Pitch LFO") });
    setParameterDisplay(C::xmlKeyPitchEnvDepth().toStdString(), { .mapping = Display::Mapping::Linear, .unit = "semitones", .min = -24.0, .max = 24.0, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Pitch Env Depth") });
    setParameterDisplay(C::xmlKeyCrossMod().toStdString(), { .mapping = Display::Mapping::Linear, .unit = "%", .min = 0.0, .max = 100.0, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Cross Mod") });
    setParameterDisplay(C::xmlKeyPulseWidth().toStdString(), { .mapping = Display::Mapping::Linear, .unit = "%", .min = 0.0, .max = 100.0, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Pulse Width") });
    setParameterDisplay(C::xmlKeyVco1Level().toStdString(), { .mapping = Display::Mapping::Linear, .unit = "%", .min = 0.0, .max = 100.0, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "VCO 1 Level") });
    setParameterDisplay(C::xmlKeyVco2FineTune().toStdString(), { .mapping = Display::Mapping::Linear, .unit = "cents", .min = -50.0, .max = 50.0, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "VCO 2 Fine Tune") });
    setParameterDisplay(C::xmlKeyVco2Level().toStdString(), { .mapping = Display::Mapping::Linear, .unit = "%", .min = 0.0, .max = 100.0, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "VCO 2 Level") });
    setParameterDisplay(C::xmlKeyNoiseLevel().toStdString(), { .mapping = Display::Mapping::Linear, .unit = "%", .min = 0.0, .max = 100.0, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Noise Level") });
    setParameterDisplay(C::xmlKeyHpfCutoff().toStdString(), { .mapping = Display::Mapping::Linear, .unit = "%", .min = 0.0, .max = 100.0, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "HPF Cutoff") });
    setParameterDisplay(C::xmlKeyLpfCutoff().toStdString(), { .mapping = Display::Mapping::Exponential, .unit = "Hz", .min = 20.0, .max = 20000.0, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "VCF Cutoff") });
    setParameterDisplay(C::xmlKeyLpfResonance().toStdString(), { .mapping = Display::Mapping::Linear, .unit = "%", .min = 0.0, .max = 100.0, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Resonance") });
    setParameterDisplay(C::xmlKeyEnvMod().toStdString(), { .mapping = Display::Mapping::Linear, .unit = "%", .min = 0.0, .max = 100.0, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Env Mod") });
    setParameterDisplay(C::xmlKeyFilterLfoDepth().toStdString(), { .mapping = Display::Mapping::Linear, .unit = "%", .min = 0.0, .max = 100.0, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Filter LFO") });
    setParameterDisplay(C::xmlKeyKeyTrack().toStdString(), { .mapping = Display::Mapping::Linear, .unit = "%", .min = 0.0, .max = 100.0, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Key Track") });
    setParameterDisplay(C::xmlKeyVcaLfoDepth().toStdString(), { .mapping = Display::Mapping::Linear, .unit = "%", .min = 0.0, .max = 100.0, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Tremolo") });
    setParameterDisplay(C::xmlKeyVelocitySensitivity().toStdString(), { .mapping = Display::Mapping::Linear, .unit = "%", .min = 0.0, .max = 100.0, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Velocity Sensitivity") });
    setParameterDisplay(C::xmlKeyEnv1Attack().toStdString(), { .mapping = Display::Mapping::Exponential, .unit = "ms", .min = 1.0, .max = 3000.0, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "ENV 1 Attack") });
    setParameterDisplay(C::xmlKeyEnv1Decay().toStdString(), { .mapping = Display::Mapping::Exponential, .unit = "ms", .min = 2.0, .max = 12000.0, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "ENV 1 Decay") });
    setParameterDisplay(C::xmlKeyEnv1Sustain().toStdString(), { .mapping = Display::Mapping::Linear, .unit = "%", .min = 0.0, .max = 100.0, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "ENV 1 Sustain") });
    setParameterDisplay(C::xmlKeyEnv1Release().toStdString(), { .mapping = Display::Mapping::Exponential, .unit = "ms", .min = 2.0, .max = 12000.0, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "ENV 1 Release") });
    setParameterDisplay(C::xmlKeyEnv2Attack().toStdString(), { .mapping = Display::Mapping::Exponential, .unit = "ms", .min = 1.0, .max = 3000.0, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "ENV 2 Attack") });
    setParameterDisplay(C::xmlKeyEnv2Decay().toStdString(), { .mapping = Display::Mapping::Exponential, .unit = "ms", .min = 2.0, .max = 12000.0, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "ENV 2 Decay") });
    setParameterDisplay(C::xmlKeyEnv2Sustain().toStdString(), { .mapping = Display::Mapping::Linear, .unit = "%", .min = 0.0, .max = 100.0, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "ENV 2 Sustain") });
    setParameterDisplay(C::xmlKeyEnv2Release().toStdString(), { .mapping = Display::Mapping::Exponential, .unit = "ms", .min = 2.0, .max = 12000.0, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "ENV 2 Release") });
    setParameterDisplay(C::xmlKeyDelayTime().toStdString(), { .mapping = Display::Mapping::Linear, .unit = "ms", .min = 10.0, .max = 1000.0, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Delay Time") });
    setParameterDisplay(C::xmlKeyDelayFeedback().toStdString(), { .mapping = Display::Mapping::Linear, .unit = "%", .min = 0.0, .max = 100.0, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Delay Feedback") });
    setParameterDisplay(C::xmlKeyDelayLevel().toStdString(), { .mapping = Display::Mapping::Linear, .unit = "%", .min = 0.0, .max = 100.0, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Delay Level") });
    setParameterDisplay(C::xmlKeyUnisonDetune().toStdString(), { .mapping = Display::Mapping::Linear, .unit = "cents", .min = 0.0, .max = 50.0, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Unison Detune") });
    setParameterDisplay(C::xmlKeyPortamento().toStdString(), { .mapping = Display::Mapping::Linear, .unit = "ms", .min = 0.0, .max = 2000.0, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Portamento") });
    setParameterDisplay(C::xmlKeyPanSpread().toStdString(), { .mapping = Display::Mapping::Linear, .unit = "%", .min = 0.0, .max = 100.0, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Pan Spread") });
}

std::vector<MidiCcController> JuSynth08Device::deviceMidiCcControllers() const
{
    using namespace MidiCcMapping;
    return {
        faderMidiCcController(),
        { static_cast<uint8_t>(Controller::PanMSB), "Pan" },
        { static_cast<uint8_t>(Controller::SoundController5), "Filter Cutoff" },
        { static_cast<uint8_t>(Controller::GeneralPurpose6), "HPF Cutoff" },
    };
}

void JuSynth08Device::processDeviceMidiNoteOn(uint8_t note, uint8_t velocity)
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    handleNoteOn(note, velocity);
}

void JuSynth08Device::processDeviceMidiNoteOff(uint8_t note)
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    handleNoteOff(note);
}

void JuSynth08Device::handleNoteOn(uint8_t note, uint8_t velocity)
{
    if (velocity == 0) {
        handleNoteOff(note);
        return;
    }

    const double targetFrequency = 440.0 * std::exp2((static_cast<double>(note) - 69.0 + octave() * 12.0) / 12.0);
    const double vel = static_cast<double>(velocity) / 127.0;

    if (m_voiceMode == VoiceMode::Solo) {
        auto & voice = m_voices[0];
        const bool legato = voice.active && voice.gated;
        voice.note = note;
        voice.velocity = vel;
        if (!legato) {
            voice.frequency = targetFrequency;
            voice.glideFrequency = targetFrequency;
            voice.pan = 0.5;
            voice.detuneCents = 0.0;
            voice.active = true;
            voice.startedAt = ++m_noteCounter;
            voice.env1.trigger();
            voice.env2.trigger();
            voice.gate.trigger();
        } else {
            voice.glideFrequency = targetFrequency;
        }
        voice.gated = true;
        return;
    }

    if (m_voiceMode == VoiceMode::Unison) {
        const double maxDetuneCents = static_cast<double>(m_unisonDetune) * 50.0;
        constexpr std::array<double, MaxVoices> UnisonSpreads {
            -1.0, 1.0, -0.65, 0.65, -0.35, 0.35, -0.15, 0.15
        };

        for (size_t i = 0; i < MaxVoices; i++) {
            auto & voice = m_voices[i];
            voice.reset();
            voice.detuneCents = UnisonSpreads[i] * maxDetuneCents;
            voice.pan = voiceSpreadPan(i);
            voice.frequency = targetFrequency;
            voice.glideFrequency = targetFrequency;
            voice.note = note;
            voice.velocity = vel;
            voice.active = true;
            voice.gated = true;
            voice.startedAt = ++m_noteCounter;
            voice.env1.trigger();
            voice.env2.trigger();
            voice.gate.trigger();
        }
        return;
    }

    // Poly mode: voice allocation
    const size_t slot = allocateVoice(note);
    auto & voice = m_voices[slot];
    voice.reset();
    voice.note = note;
    voice.velocity = vel;
    voice.frequency = targetFrequency;
    voice.glideFrequency = targetFrequency;
    voice.pan = voiceSpreadPan(slot);
    voice.detuneCents = 0.0;
    voice.active = true;
    voice.gated = true;
    voice.startedAt = ++m_noteCounter;
    voice.env1.trigger();
    voice.env2.trigger();
    voice.gate.trigger();
}

void JuSynth08Device::handleNoteOff(uint8_t note)
{
    for (auto && voice : m_voices) {
        if (voice.gated && voice.note == note) {
            voice.env1.release();
            voice.env2.release();
            voice.gate.release();
            voice.gated = false;
        }
    }
}

size_t JuSynth08Device::allocateVoice(uint8_t note)
{
    for (size_t i = 0; i < MaxVoices; i++) {
        if (m_voices[i].active && m_voices[i].gated && m_voices[i].note == note) {
            return i;
        }
    }
    for (size_t i = 0; i < MaxVoices; i++) {
        if (!m_voices[i].active) {
            return i;
        }
    }
    for (size_t i = 0; i < MaxVoices; i++) {
        if (!m_voices[i].gated) {
            return i;
        }
    }
    size_t oldest = 0;
    uint64_t oldestTime = m_voices[0].startedAt;
    for (size_t i = 1; i < MaxVoices; i++) {
        if (m_voices[i].startedAt < oldestTime) {
            oldestTime = m_voices[i].startedAt;
            oldest = i;
        }
    }
    return oldest;
}

double JuSynth08Device::voiceSpreadPan(size_t voiceIndex) const
{
    constexpr std::array<double, MaxVoices> PolyPans {
        0.5, 0.4, 0.6, 0.3, 0.7, 0.2, 0.8, 0.5
    };
    const double baseOffset = PolyPans[voiceIndex % MaxVoices] - 0.5;
    return std::clamp(0.5 + baseOffset * m_panSpread, 0.0, 1.0);
}

double JuSynth08Device::rangeMultiplier(int range) const
{
    switch (range) {
    case 0:
        return 0.5; // 16'
    case 1:
        return 1.0; // 8'
    case 2:
        return 2.0; // 4'
    case 3:
        return 4.0; // 2'
    case 4:
        return 0.01; // Low Freq
    default:
        return 1.0;
    }
}

void JuSynth08Device::processDeviceMidiCc(uint8_t controller, uint8_t value, uint8_t)
{
    using namespace MidiCcMapping;

    bool changed = false;
    {
        const std::lock_guard<std::recursive_mutex> lock { mutex() };

        if (controller == static_cast<uint8_t>(Controller::ResetAllControllers)) {
            changed |= clearAutomationInternal();
        } else {
            const float val = static_cast<float>(value) / 127.0f;
            const auto automate = [&](const QString & key) {
                if (auto p = parameter(key.toStdString()); p) {
                    p->get().setAutomationValue(val);
                    syncParameters();
                    changed = true;
                }
            };

            if (controller == static_cast<uint8_t>(Controller::ChannelVolumeMSB)) {
                changed |= updateVolumeParameter(faderPositionFromMidiCc(value), false);
            } else if (controller == static_cast<uint8_t>(Controller::PanMSB)) {
                changed |= updatePanParameter(val, false);
            } else if (controller == static_cast<uint8_t>(Controller::SoundController5)) {
                automate(C::xmlKeyLpfCutoff());
            } else if (controller == static_cast<uint8_t>(Controller::GeneralPurpose6)) {
                automate(C::xmlKeyHpfCutoff());
            }
        }
    }

    if (changed) {
        emit parametersChanged();
    }
}

void JuSynth08Device::processMidiPitchBend(uint16_t value, uint8_t)
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    m_pitchBend = value;
}

void JuSynth08Device::processMidiProgramChange(uint8_t program, uint8_t)
{
    applyPreset(program, false);
}

void JuSynth08Device::loadPreset(int index)
{
    applyPreset(index, true);
}

void JuSynth08Device::applyPreset(int index, bool authored)
{
    {
        const std::lock_guard<std::recursive_mutex> lock { mutex() };
        const auto & presets = JuSynth08Presets::presets();
        if (index < 0 || index >= static_cast<int>(presets.size())) {
            return;
        }

        if (authored) {
            reset();
        } else {
            for (auto && [name, p] : parameters()) {
                p.setAutomationValue(p.defaultValue());
            }
        }

        for (const auto & [key, value] : presets[static_cast<size_t>(index)].parameters) {
            if (auto p = parameter(key); p) {
                if (authored) {
                    p->get().setValue(value);
                } else {
                    p->get().setAutomationValue(value);
                }
            }
        }
        syncParameters();
    }

    if (authored) {
        emit dataChanged();
    } else {
        emit parametersChanged();
    }
}

void JuSynth08Device::processMidiAllNotesOff()
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    for (auto && voice : m_voices) {
        if (voice.active) {
            voice.env1.release();
            voice.env2.release();
            voice.gate.release();
        }
        voice.gated = false;
    }
}

void JuSynth08Device::processAudio(AudioContext & context)
{
    setSampleRate(context.sampleRate);
    const std::lock_guard<std::recursive_mutex> lock { mutex() };

    updateSampleRateDependencies();

    const size_t frameCount = context.frameCount;
    if (context.buffer.size() < frameCount * 2) {
        return;
    }

    const double dt = 1.0 / context.sampleRate;
    const double oversampledRate = context.sampleRate * Oversampling;
    const double dtOversampled = 1.0 / oversampledRate;

    // Pitch bend ratio
    const double pitchBendSemitones = (static_cast<double>(m_pitchBend) - 8192.0) / 8192.0 * m_pitchBendRange;
    const double pitchBendRatio = std::exp2(pitchBendSemitones / 12.0);

    const double portamentoRate = m_portamento > 0.001f ? 1.0 / (m_portamento * 2.0) : 1000.0;

    // Setup delay buffer size if needed
    const size_t maxDelayFrames = static_cast<size_t>(context.sampleRate * 1.2);
    if (m_delayBufferL.size() < maxDelayFrames) {
        m_delayBufferL.resize(maxDelayFrames, 0.0f);
        m_delayBufferR.resize(maxDelayFrames, 0.0f);
        m_delayWritePos = 0;
    }

    const double delayFrames = std::clamp(static_cast<double>(m_delayTime) * context.sampleRate, 1.0, static_cast<double>(maxDelayFrames - 1));
    const float delayFb = std::clamp(m_delayFeedback, 0.0f, 0.95f);
    const float delayMix = std::clamp(m_delayLevel, 0.0f, 1.0f);

    const double gain = (1.0 / MaxVoices) * static_cast<double>(linearGainInternal());
    const double masterPanAngle = static_cast<double>(panInternal()) * std::numbers::pi * 0.5;
    const double masterPanL = std::cos(masterPanAngle) * std::numbers::sqrt2;
    const double masterPanR = std::sin(masterPanAngle) * std::numbers::sqrt2;

    for (size_t frame = 0; frame < frameCount; frame++) {
        // LFO update
        m_lfoDelayElapsed += dt;
        double lfo = 0.0;
        if (m_lfoDelayElapsed >= m_lfoDelay) {
            lfo = m_lfo.nextSample();
        } else {
            m_lfo.nextSample();
        }

        double dryLeft = 0.0;
        double dryRight = 0.0;

        for (auto && voice : m_voices) {
            if (!voice.active) {
                continue;
            }

            // Portamento frequency smoothing
            if (m_portamento > 0.001f) {
                const double diff = voice.glideFrequency - voice.frequency;
                voice.frequency += diff * std::min(1.0, portamentoRate * dt);
            } else {
                voice.frequency = voice.glideFrequency;
            }

            // Envelope advances
            const double env1 = voice.env1.nextSample();
            const double env2 = voice.env2.nextSample();
            const double gateVal = voice.gate.nextSample();

            if (!voice.env2.isActive() && !voice.gate.isActive()) {
                voice.active = false;
                continue;
            }

            // Pitch modulation
            const double pitchLfoSemitones = static_cast<double>(m_pitchLfoDepth * m_pitchLfoDepth) * MaxPitchLfoSemitones;
            const double vibrato = pitchLfoSemitones > 0.0 ? std::exp2(lfo * pitchLfoSemitones / 12.0) : 1.0;

            const double unisonDetuneRatio = voice.detuneCents != 0.0 ? std::exp2(voice.detuneCents / 1200.0) : 1.0;
            const double baseFreq = voice.frequency * pitchBendRatio * unisonDetuneRatio;

            double pitchModVco1 = vibrato;
            double pitchModVco2 = vibrato;

            if (m_pitchEnvDepth != 0.0f) {
                const double pitchEnvSemitones = static_cast<double>(m_pitchEnvDepth) * 24.0 * env1;
                const double pitchEnvRatio = std::exp2(pitchEnvSemitones / 12.0);
                if (m_pitchEnvTarget == PitchEnvTarget::Vco1 || m_pitchEnvTarget == PitchEnvTarget::Both) {
                    pitchModVco1 *= pitchEnvRatio;
                }
                if (m_pitchEnvTarget == PitchEnvTarget::Vco2 || m_pitchEnvTarget == PitchEnvTarget::Both) {
                    pitchModVco2 *= pitchEnvRatio;
                }
            }

            const double freqVco1 = std::clamp(baseFreq * rangeMultiplier(static_cast<int>(m_vco1Range)) * pitchModVco1, 1.0, oversampledRate * 0.45);
            const double vco2FineRatio = std::exp2((static_cast<double>(m_vco2FineTune) * 50.0) / 1200.0);
            const double freqVco2 = std::clamp(baseFreq * rangeMultiplier(static_cast<int>(m_vco2Range)) * vco2FineRatio * pitchModVco2, 1.0, oversampledRate * 0.45);

            // PWM modulation calculation
            double currentPw = m_pulseWidth;
            if (m_pwmSource == PwmSource::Lfo) {
                currentPw += lfo * 0.45;
            } else if (m_pwmSource == PwmSource::Env1) {
                currentPw += (env1 - 0.5) * 0.9;
            }
            currentPw = std::clamp(currentPw, 0.05, 0.95);

            // 2x Oversampled synthesis of VCOs and VCF
            double voiceSampleAccum = 0.0;
            for (size_t os = 0; os < Oversampling; os++) {
                // VCO-1 phase advance
                const double phaseIncVco1 = freqVco1 * dtOversampled;
                voice.phaseVco1 += phaseIncVco1;
                const bool vco1Wrapped = voice.phaseVco1 >= 1.0;
                if (vco1Wrapped) {
                    voice.phaseVco1 -= 1.0;
                }

                // Hard Sync: VCO 2 reset on VCO 1 zero-crossing
                if (m_sync && vco1Wrapped) {
                    voice.phaseVco2 = voice.phaseVco1; // Sync phase alignment
                }

                // Cross-Mod FM: VCO-1 frequency modulates VCO-2
                double vco1Instant = 0.0;
                switch (m_vco1Waveform) {
                case Vco1Waveform::Saw:
                    vco1Instant = (2.0 * voice.phaseVco1 - 1.0) - polyBlep(voice.phaseVco1, phaseIncVco1);
                    break;
                case Vco1Waveform::Square:
                    vco1Instant = (voice.phaseVco1 < currentPw ? 1.0 : -1.0)
                      + polyBlep(voice.phaseVco1, phaseIncVco1)
                      - polyBlep(std::fmod(voice.phaseVco1 + 1.0 - currentPw, 1.0), phaseIncVco1);
                    break;
                case Vco1Waveform::Triangle:
                    vco1Instant = 2.0 * std::abs(2.0 * voice.phaseVco1 - 1.0) - 1.0;
                    break;
                case Vco1Waveform::Sine:
                    vco1Instant = std::sin(voice.phaseVco1 * TwoPi);
                    break;
                }

                // Cross modulation applied to VCO-2 phase advance
                const double crossModOffset = m_crossMod > 0.0f ? vco1Instant * m_crossMod * 2.0 * phaseIncVco1 : 0.0;
                const double phaseIncVco2 = std::max(0.0, freqVco2 * dtOversampled + crossModOffset);
                voice.phaseVco2 += phaseIncVco2;
                if (voice.phaseVco2 >= 1.0) {
                    voice.phaseVco2 -= 1.0;
                }

                // VCO-2 synthesis
                double vco2Instant = 0.0;
                switch (m_vco2Waveform) {
                case Vco2Waveform::Saw:
                    vco2Instant = (2.0 * voice.phaseVco2 - 1.0) - polyBlep(voice.phaseVco2, phaseIncVco2);
                    break;
                case Vco2Waveform::Square:
                    vco2Instant = (voice.phaseVco2 < currentPw ? 1.0 : -1.0)
                      + polyBlep(voice.phaseVco2, phaseIncVco2)
                      - polyBlep(std::fmod(voice.phaseVco2 + 1.0 - currentPw, 1.0), phaseIncVco2);
                    break;
                case Vco2Waveform::Triangle:
                    vco2Instant = 2.0 * std::abs(2.0 * voice.phaseVco2 - 1.0) - 1.0;
                    break;
                case Vco2Waveform::Noise: {
                    voice.noiseState = voice.noiseState * 1664525u + 1013904223u;
                    vco2Instant = static_cast<double>(static_cast<int32_t>(voice.noiseState)) / 2147483648.0;
                    break;
                }
                }

                // Dedicated White Noise generator
                voice.noiseState = voice.noiseState * 1664525u + 1013904223u;
                const double noise = static_cast<double>(static_cast<int32_t>(voice.noiseState)) / 2147483648.0;

                // Oscillator Mixer
                const double oscMix = (vco1Instant * m_vco1Level + vco2Instant * m_vco2Level + noise * m_noiseLevel);

                // High-Pass Filter (One-pole HPF)
                const double hpfCutoffFreq = 10.0 + std::pow(static_cast<double>(m_hpfCutoff), 2.0) * 1500.0;
                voice.hpf.calculate(hpfCutoffFreq, oversampledRate);
                voice.hpf.process(oscMix);
                const double afterHpf = voice.hpf.highPass();

                // OTA Ladder VCF with 12 dB / 24 dB slope switch
                double vcfMod = 0.0;
                if (m_envMod > 0.0f) {
                    const double envVal = m_envPolarity ? (1.0 - env1) : env1;
                    vcfMod += (envVal * 2.0 - 1.0) * m_envMod;
                }
                if (m_filterLfoDepth > 0.0f) {
                    vcfMod += lfo * m_filterLfoDepth;
                }
                if (m_keyTrack > 0.0f) {
                    const double noteFactor = (static_cast<double>(voice.note) - 60.0) / 48.0;
                    vcfMod += noteFactor * m_keyTrack;
                }

                const double baseCutoff = 20.0 * std::pow(20000.0 / 20.0, static_cast<double>(m_lpfCutoff));
                const double modulatedCutoff = std::clamp(baseCutoff * std::exp2(vcfMod * 4.0), 20.0, oversampledRate * 0.45);

                voice.vcf.setSampleRate(oversampledRate);
                voice.vcf.setCutoff(modulatedCutoff);
                voice.vcf.setResonance(m_lpfResonance);
                voice.vcf.setSlope(m_vcfSlope == VcfSlope::Slope12dB ? OtaLadderFilter::Slope::Slope12dB : OtaLadderFilter::Slope::Slope24dB);

                const double afterVcf = voice.vcf.process(afterHpf);
                voiceSampleAccum += afterVcf;
            }

            // Downsample average
            const double voiceOut = voiceSampleAccum / Oversampling;

            // VCA Stage
            double vcaGain = (m_vcaMode == VcaMode::Gate) ? gateVal : env2;
            if (m_velocitySensitivity > 0.0f) {
                const double velScale = 1.0 - m_velocitySensitivity * (1.0 - voice.velocity);
                vcaGain *= velScale;
            }
            if (m_vcaLfoDepth > 0.0f) {
                const double tremolo = 1.0 + lfo * m_vcaLfoDepth * 0.5;
                vcaGain *= std::max(0.0, tremolo);
            }

            const double voiceAmp = voiceOut * vcaGain;

            // Stereo Panning
            const double panL = std::cos(voice.pan * (std::numbers::pi / 2.0));
            const double panR = std::sin(voice.pan * (std::numbers::pi / 2.0));

            dryLeft += voiceAmp * panL;
            dryRight += voiceAmp * panR;
        }

        // Stereo Delay Line
        const size_t readPosL = (m_delayWritePos + maxDelayFrames - static_cast<size_t>(delayFrames)) % maxDelayFrames;
        const size_t readPosR = (m_delayWritePos + maxDelayFrames - static_cast<size_t>(delayFrames * 1.15)) % maxDelayFrames; // subtle ping-pong offset

        const float delayedSampleL = m_delayBufferL[readPosL];
        const float delayedSampleR = m_delayBufferR[readPosR];

        m_delayBufferL[m_delayWritePos] = static_cast<float>(dryLeft) + delayedSampleR * delayFb;
        m_delayBufferR[m_delayWritePos] = static_cast<float>(dryRight) + delayedSampleL * delayFb;
        m_delayWritePos = (m_delayWritePos + 1) % maxDelayFrames;

        const double mixedLeft = dryLeft * (1.0 - delayMix * 0.5) + delayedSampleL * delayMix;
        const double mixedRight = dryRight * (1.0 - delayMix * 0.5) + delayedSampleR * delayMix;

        // DC blocking and output
        context.buffer[frame * 2] += static_cast<double>(m_dcBlockers[0].process(mixedLeft)) * gain * masterPanL;
        context.buffer[frame * 2 + 1] += static_cast<double>(m_dcBlockers[1].process(mixedRight)) * gain * masterPanR;
    }

    processInsertEffects(context);

    publishLfoLed(0, m_lfo.shapeValue());

    // Active voice tracking for idle management
    bool anyActive = false;
    for (const auto & v : m_voices) {
        if (v.active) {
            anyActive = true;
            break;
        }
    }
    if (anyActive || delayMix > 0.01f) {
        m_tailFrames = static_cast<size_t>(context.sampleRate * 2.0);
    } else if (m_tailFrames > 0) {
        m_tailFrames = (m_tailFrames > frameCount) ? (m_tailFrames - frameCount) : 0;
    }
}

bool JuSynth08Device::hasActiveAudio() const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    return m_tailFrames > 0 || std::ranges::any_of(m_voices, [](const auto & voice) { return voice.active; });
}

void JuSynth08Device::advanceIdle(const AudioContext & context)
{
    setSampleRate(context.sampleRate);
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    updateSampleRateDependencies();
    m_lfoDelayElapsed += static_cast<double>(context.frameCount) / context.sampleRate;
    m_lfo.advance(context.frameCount);
    publishLfoLed(0, m_lfo.shapeValue());
}

void JuSynth08Device::reset()
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    Device::reset();
    resetAudio();
    syncParameters();
}

void JuSynth08Device::resetAudio()
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    for (size_t i = 0; i < m_voices.size(); i++) {
        m_voices[i].reset();
        m_voices[i].noiseState = static_cast<uint32_t>(0x9E3779B9u * (i + 1));
    }
    m_noteCounter = 0;
    m_nextVoice = 0;
    m_lfo.reset();
    m_lfoDelayElapsed = 0.0;
    for (auto && dcBlocker : m_dcBlockers) {
        dcBlocker.reset();
    }
    std::fill(m_delayBufferL.begin(), m_delayBufferL.end(), 0.0f);
    std::fill(m_delayBufferR.begin(), m_delayBufferR.end(), 0.0f);
    m_delayWritePos = 0;
    m_tailFrames = 0;
}

void JuSynth08Device::syncParameters()
{
    const auto read = [this](const QString & key) {
        if (const auto p = parameter(key.toStdString()); p) {
            return p->get().value();
        }
        return 0.0f;
    };

    m_lfoRate = read(C::xmlKeyLfoRate());
    m_lfoDelay = read(C::xmlKeyLfoDelay());
    m_lfoWaveform = static_cast<int>(read(C::xmlKeyWaveform()));

    m_vco1Range = static_cast<Vco1Range>(std::clamp(static_cast<int>(read(C::xmlKeyVco1Range())), 0, 4));
    m_vco1Waveform = static_cast<Vco1Waveform>(std::clamp(static_cast<int>(read(C::xmlKeyVco1Waveform())), 0, 3));
    m_crossMod = read(C::xmlKeyCrossMod());
    m_vco1Level = read(C::xmlKeyVco1Level());

    m_vco2Range = static_cast<Vco2Range>(std::clamp(static_cast<int>(read(C::xmlKeyVco2Range())), 0, 4));
    m_vco2Waveform = static_cast<Vco2Waveform>(std::clamp(static_cast<int>(read(C::xmlKeyVco2Waveform())), 0, 3));
    m_vco2FineTune = read(C::xmlKeyVco2FineTune());
    m_sync = read(C::xmlKeyVco2Sync()) >= 0.5f;
    m_vco2Level = read(C::xmlKeyVco2Level());

    m_pitchLfoDepth = read(C::xmlKeyPitchLfoDepth());
    m_pitchEnvDepth = read(C::xmlKeyPitchEnvDepth());
    m_pitchEnvTarget = static_cast<PitchEnvTarget>(std::clamp(static_cast<int>(read(C::xmlKeyPitchEnvTarget())), 0, 2));
    m_pulseWidth = read(C::xmlKeyPulseWidth());
    m_pwmSource = static_cast<PwmSource>(std::clamp(static_cast<int>(read(C::xmlKeyPwmSource())), 0, 2));

    m_noiseLevel = read(C::xmlKeyNoiseLevel());
    m_hpfCutoff = read(C::xmlKeyHpfCutoff());

    m_vcfSlope = static_cast<VcfSlope>(std::clamp(static_cast<int>(read(C::xmlKeyVcfSlope())), 0, 1));
    m_lpfCutoff = read(C::xmlKeyLpfCutoff());
    m_lpfResonance = read(C::xmlKeyLpfResonance());
    m_envMod = read(C::xmlKeyEnvMod());
    m_envPolarity = read(C::xmlKeyEnvPolarity()) >= 0.5f;
    m_filterLfoDepth = read(C::xmlKeyFilterLfoDepth());
    m_keyTrack = read(C::xmlKeyKeyTrack());

    m_vcaMode = static_cast<VcaMode>(std::clamp(static_cast<int>(read(C::xmlKeyVcaMode())), 0, 1));
    m_velocitySensitivity = read(C::xmlKeyVelocitySensitivity());
    m_vcaLfoDepth = read(C::xmlKeyVcaLfoDepth());

    m_env1Attack = read(C::xmlKeyEnv1Attack());
    m_env1Decay = read(C::xmlKeyEnv1Decay());
    m_env1Sustain = read(C::xmlKeyEnv1Sustain());
    m_env1Release = read(C::xmlKeyEnv1Release());
    m_env1KeyFollow = read(C::xmlKeyEnv1KeyFollow()) >= 0.5f;

    m_env2Attack = read(C::xmlKeyEnv2Attack());
    m_env2Decay = read(C::xmlKeyEnv2Decay());
    m_env2Sustain = read(C::xmlKeyEnv2Sustain());
    m_env2Release = read(C::xmlKeyEnv2Release());
    m_env2KeyFollow = read(C::xmlKeyEnv2KeyFollow()) >= 0.5f;

    m_delayTime = read(C::xmlKeyDelayTime());
    m_delayFeedback = read(C::xmlKeyDelayFeedback());
    m_delayLevel = read(C::xmlKeyDelayLevel());

    m_voiceMode = static_cast<VoiceMode>(std::clamp(static_cast<int>(read(C::xmlKeyVoiceMode())), 0, 2));
    m_unisonDetune = read(C::xmlKeyUnisonDetune());
    m_portamento = read(C::xmlKeyPortamento());
    m_pitchBendRange = static_cast<int>(read(C::xmlKeyPitchBendRange()));
    m_panSpread = read(C::xmlKeyPanSpread());
    updateSampleRateDependencies();
}

void JuSynth08Device::serializeToXml(ProjectWriter & writer) const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    writer.writeStartElement(C::xmlKeyDevice());
    serializeAttributesToXml(writer);

    writer.writeStartElement(C::xmlKeyInsertEffects());
    insertEffectRack().serializeEffectsToXml(writer);
    writer.writeEndElement();

    writer.writeStartElement(C::xmlKeyParameters());
    serializeParametersToXml(writer);
    writer.writeEndElement();

    writer.writeEndElement();
}

void JuSynth08Device::deserializeFromXml(ProjectReader & reader)
{
    {
        const std::lock_guard<std::recursive_mutex> lock { mutex() };
        deserializeAttributesFromXml(reader);

        while (!reader.atEnd() && !reader.hasError()) {
            const auto token = reader.readNext();
            if (token == ProjectReader::TokenType::EndElement && reader.name() == C::xmlKeyDevice()) {
                break;
            }
            if (token == ProjectReader::TokenType::StartElement) {
                if (reader.name() == C::xmlKeyParameters()) {
                    deserializeParametersFromXml(reader);
                } else if (reader.name() == C::xmlKeyInsertEffects()) {
                    insertEffectRack().deserializeEffectsFromXml(reader);
                } else {
                    reader.skipCurrentElement();
                }
            }
        }
        syncParameters();
    }
    emit dataChanged();
}

void JuSynth08Device::updateSampleRateDependencies()
{
    if (sampleRate() <= 0.0) {
        return;
    }
    m_lfo.setSampleRate(sampleRate());
    const double lfoFreq { 0.1 * std::pow(300.0, static_cast<double>(m_lfoRate)) };
    m_lfo.setFrequency(lfoFreq);
    m_lfo.setWaveform(lfoWaveformFromIndex(m_lfoWaveform));

    for (auto && voice : m_voices) {
        // ENV-1
        const double a1 = 0.001 * std::pow(3000.0, static_cast<double>(m_env1Attack));
        const double d1 = 0.002 * std::pow(6000.0, static_cast<double>(m_env1Decay));
        const double r1 = 0.002 * std::pow(6000.0, static_cast<double>(m_env1Release));
        voice.env1.setAttackTime(a1);
        voice.env1.setDecayTime(d1);
        voice.env1.setSustainLevel(m_env1Sustain);
        voice.env1.setReleaseTime(r1);
        voice.env1.setSampleRate(sampleRate());

        // ENV-2
        const double a2 = 0.001 * std::pow(3000.0, static_cast<double>(m_env2Attack));
        const double d2 = 0.002 * std::pow(6000.0, static_cast<double>(m_env2Decay));
        const double r2 = 0.002 * std::pow(6000.0, static_cast<double>(m_env2Release));
        voice.env2.setAttackTime(a2);
        voice.env2.setDecayTime(d2);
        voice.env2.setSustainLevel(m_env2Sustain);
        voice.env2.setReleaseTime(r2);
        voice.env2.setSampleRate(sampleRate());

        voice.gate.setSampleRate(sampleRate());
    }
}

// Getters & Setters
float JuSynth08Device::lfoRate() const
{
    return shownParameterValue(C::xmlKeyLfoRate(), m_lfoRate);
}

void JuSynth08Device::setLfoRate(float rate)
{
    setContinuousParameterValue(C::xmlKeyLfoRate().toStdString(), rate);
}

float JuSynth08Device::lfoDelay() const
{
    return shownParameterValue(C::xmlKeyLfoDelay(), m_lfoDelay);
}

void JuSynth08Device::setLfoDelay(float delay)
{
    setContinuousParameterValue(C::xmlKeyLfoDelay().toStdString(), delay);
}

int JuSynth08Device::lfoWaveform() const
{
    return static_cast<int>(shownParameterValue(C::xmlKeyWaveform(), static_cast<float>(m_lfoWaveform)));
}

void JuSynth08Device::setLfoWaveform(int waveform)
{
    setDiscreteParameterValue(C::xmlKeyWaveform().toStdString(), waveform);
}

float JuSynth08Device::pitchLfoDepth() const
{
    return shownParameterValue(C::xmlKeyPitchLfoDepth(), m_pitchLfoDepth);
}

void JuSynth08Device::setPitchLfoDepth(float depth)
{
    setContinuousParameterValue(C::xmlKeyPitchLfoDepth().toStdString(), depth);
}

float JuSynth08Device::pitchEnvDepth() const
{
    return shownParameterValue(C::xmlKeyPitchEnvDepth(), m_pitchEnvDepth);
}

void JuSynth08Device::setPitchEnvDepth(float depth)
{
    setContinuousParameterValue(C::xmlKeyPitchEnvDepth().toStdString(), depth);
}

int JuSynth08Device::pitchEnvTarget() const
{
    return static_cast<int>(shownParameterValue(C::xmlKeyPitchEnvTarget(), static_cast<float>(m_pitchEnvTarget)));
}

void JuSynth08Device::setPitchEnvTarget(int target)
{
    setDiscreteParameterValue(C::xmlKeyPitchEnvTarget().toStdString(), target);
}

float JuSynth08Device::pulseWidth() const
{
    return shownParameterValue(C::xmlKeyPulseWidth(), m_pulseWidth);
}

void JuSynth08Device::setPulseWidth(float width)
{
    setContinuousParameterValue(C::xmlKeyPulseWidth().toStdString(), width);
}

int JuSynth08Device::pwmSource() const
{
    return static_cast<int>(shownParameterValue(C::xmlKeyPwmSource(), static_cast<float>(m_pwmSource)));
}

void JuSynth08Device::setPwmSource(int source)
{
    setDiscreteParameterValue(C::xmlKeyPwmSource().toStdString(), source);
}

float JuSynth08Device::crossMod() const
{
    return shownParameterValue(C::xmlKeyCrossMod(), m_crossMod);
}

void JuSynth08Device::setCrossMod(float amount)
{
    setContinuousParameterValue(C::xmlKeyCrossMod().toStdString(), amount);
}

int JuSynth08Device::vco1Waveform() const
{
    return static_cast<int>(shownParameterValue(C::xmlKeyVco1Waveform(), static_cast<float>(m_vco1Waveform)));
}

void JuSynth08Device::setVco1Waveform(int waveform)
{
    setDiscreteParameterValue(C::xmlKeyVco1Waveform().toStdString(), waveform);
}

int JuSynth08Device::vco1Range() const
{
    return static_cast<int>(shownParameterValue(C::xmlKeyVco1Range(), static_cast<float>(m_vco1Range)));
}

void JuSynth08Device::setVco1Range(int range)
{
    setDiscreteParameterValue(C::xmlKeyVco1Range().toStdString(), range);
}

float JuSynth08Device::vco1Level() const
{
    return shownParameterValue(C::xmlKeyVco1Level(), m_vco1Level);
}

void JuSynth08Device::setVco1Level(float level)
{
    setContinuousParameterValue(C::xmlKeyVco1Level().toStdString(), level);
}

int JuSynth08Device::vco2Waveform() const
{
    return static_cast<int>(shownParameterValue(C::xmlKeyVco2Waveform(), static_cast<float>(m_vco2Waveform)));
}

void JuSynth08Device::setVco2Waveform(int waveform)
{
    setDiscreteParameterValue(C::xmlKeyVco2Waveform().toStdString(), waveform);
}

int JuSynth08Device::vco2Range() const
{
    return static_cast<int>(shownParameterValue(C::xmlKeyVco2Range(), static_cast<float>(m_vco2Range)));
}

void JuSynth08Device::setVco2Range(int range)
{
    setDiscreteParameterValue(C::xmlKeyVco2Range().toStdString(), range);
}

float JuSynth08Device::vco2FineTune() const
{
    return shownParameterValue(C::xmlKeyVco2FineTune(), m_vco2FineTune);
}

void JuSynth08Device::setVco2FineTune(float tune)
{
    setContinuousParameterValue(C::xmlKeyVco2FineTune().toStdString(), tune);
}

bool JuSynth08Device::sync() const
{
    return shownParameterValue(C::xmlKeyVco2Sync(), m_sync ? 1.0f : 0.0f) >= 0.5f;
}

void JuSynth08Device::setSync(bool enable)
{
    setContinuousParameterValue(C::xmlKeyVco2Sync().toStdString(), enable ? 1.0f : 0.0f);
}

float JuSynth08Device::vco2Level() const
{
    return shownParameterValue(C::xmlKeyVco2Level(), m_vco2Level);
}

void JuSynth08Device::setVco2Level(float level)
{
    setContinuousParameterValue(C::xmlKeyVco2Level().toStdString(), level);
}

float JuSynth08Device::noiseLevel() const
{
    return shownParameterValue(C::xmlKeyNoiseLevel(), m_noiseLevel);
}

void JuSynth08Device::setNoiseLevel(float level)
{
    setContinuousParameterValue(C::xmlKeyNoiseLevel().toStdString(), level);
}

float JuSynth08Device::hpfCutoff() const
{
    return shownParameterValue(C::xmlKeyHpfCutoff(), m_hpfCutoff);
}

void JuSynth08Device::setHpfCutoff(float cutoff)
{
    setContinuousParameterValue(C::xmlKeyHpfCutoff().toStdString(), cutoff);
}

int JuSynth08Device::vcfSlope() const
{
    return static_cast<int>(shownParameterValue(C::xmlKeyVcfSlope(), static_cast<float>(m_vcfSlope)));
}

void JuSynth08Device::setVcfSlope(int slope)
{
    setDiscreteParameterValue(C::xmlKeyVcfSlope().toStdString(), slope);
}

float JuSynth08Device::lpfCutoff() const
{
    return shownParameterValue(C::xmlKeyLpfCutoff(), m_lpfCutoff);
}

void JuSynth08Device::setLpfCutoff(float cutoff)
{
    setContinuousParameterValue(C::xmlKeyLpfCutoff().toStdString(), cutoff);
}

float JuSynth08Device::lpfResonance() const
{
    return shownParameterValue(C::xmlKeyLpfResonance(), m_lpfResonance);
}

void JuSynth08Device::setLpfResonance(float resonance)
{
    setContinuousParameterValue(C::xmlKeyLpfResonance().toStdString(), resonance);
}

float JuSynth08Device::envMod() const
{
    return shownParameterValue(C::xmlKeyEnvMod(), m_envMod);
}

void JuSynth08Device::setEnvMod(float mod)
{
    setContinuousParameterValue(C::xmlKeyEnvMod().toStdString(), mod);
}

bool JuSynth08Device::envPolarity() const
{
    return shownParameterValue(C::xmlKeyEnvPolarity(), m_envPolarity ? 1.0f : 0.0f) >= 0.5f;
}

void JuSynth08Device::setEnvPolarity(bool positive)
{
    setContinuousParameterValue(C::xmlKeyEnvPolarity().toStdString(), positive ? 1.0f : 0.0f);
}

float JuSynth08Device::filterLfoDepth() const
{
    return shownParameterValue(C::xmlKeyFilterLfoDepth(), m_filterLfoDepth);
}

void JuSynth08Device::setFilterLfoDepth(float depth)
{
    setContinuousParameterValue(C::xmlKeyFilterLfoDepth().toStdString(), depth);
}

float JuSynth08Device::keyTrack() const
{
    return shownParameterValue(C::xmlKeyKeyTrack(), m_keyTrack);
}

void JuSynth08Device::setKeyTrack(float track)
{
    setContinuousParameterValue(C::xmlKeyKeyTrack().toStdString(), track);
}

int JuSynth08Device::vcaMode() const
{
    return static_cast<int>(shownParameterValue(C::xmlKeyVcaMode(), static_cast<float>(m_vcaMode)));
}

void JuSynth08Device::setVcaMode(int mode)
{
    setDiscreteParameterValue(C::xmlKeyVcaMode().toStdString(), mode);
}

float JuSynth08Device::velocitySensitivity() const
{
    return shownParameterValue(C::xmlKeyVelocitySensitivity(), m_velocitySensitivity);
}

void JuSynth08Device::setVelocitySensitivity(float sens)
{
    setContinuousParameterValue(C::xmlKeyVelocitySensitivity().toStdString(), sens);
}

float JuSynth08Device::vcaLfoDepth() const
{
    return shownParameterValue(C::xmlKeyVcaLfoDepth(), m_vcaLfoDepth);
}

void JuSynth08Device::setVcaLfoDepth(float depth)
{
    setContinuousParameterValue(C::xmlKeyVcaLfoDepth().toStdString(), depth);
}

float JuSynth08Device::env1Attack() const
{
    return shownParameterValue(C::xmlKeyEnv1Attack(), m_env1Attack);
}

void JuSynth08Device::setEnv1Attack(float val)
{
    setContinuousParameterValue(C::xmlKeyEnv1Attack().toStdString(), val);
}

float JuSynth08Device::env1Decay() const
{
    return shownParameterValue(C::xmlKeyEnv1Decay(), m_env1Decay);
}

void JuSynth08Device::setEnv1Decay(float val)
{
    setContinuousParameterValue(C::xmlKeyEnv1Decay().toStdString(), val);
}

float JuSynth08Device::env1Sustain() const
{
    return shownParameterValue(C::xmlKeyEnv1Sustain(), m_env1Sustain);
}

void JuSynth08Device::setEnv1Sustain(float val)
{
    setContinuousParameterValue(C::xmlKeyEnv1Sustain().toStdString(), val);
}

float JuSynth08Device::env1Release() const
{
    return shownParameterValue(C::xmlKeyEnv1Release(), m_env1Release);
}

void JuSynth08Device::setEnv1Release(float val)
{
    setContinuousParameterValue(C::xmlKeyEnv1Release().toStdString(), val);
}

bool JuSynth08Device::env1KeyFollow() const
{
    return shownParameterValue(C::xmlKeyEnv1KeyFollow(), m_env1KeyFollow ? 1.0f : 0.0f) >= 0.5f;
}

void JuSynth08Device::setEnv1KeyFollow(bool val)
{
    setContinuousParameterValue(C::xmlKeyEnv1KeyFollow().toStdString(), val ? 1.0f : 0.0f);
}

float JuSynth08Device::env2Attack() const
{
    return shownParameterValue(C::xmlKeyEnv2Attack(), m_env2Attack);
}

void JuSynth08Device::setEnv2Attack(float val)
{
    setContinuousParameterValue(C::xmlKeyEnv2Attack().toStdString(), val);
}

float JuSynth08Device::env2Decay() const
{
    return shownParameterValue(C::xmlKeyEnv2Decay(), m_env2Decay);
}

void JuSynth08Device::setEnv2Decay(float val)
{
    setContinuousParameterValue(C::xmlKeyEnv2Decay().toStdString(), val);
}

float JuSynth08Device::env2Sustain() const
{
    return shownParameterValue(C::xmlKeyEnv2Sustain(), m_env2Sustain);
}

void JuSynth08Device::setEnv2Sustain(float val)
{
    setContinuousParameterValue(C::xmlKeyEnv2Sustain().toStdString(), val);
}

float JuSynth08Device::env2Release() const
{
    return shownParameterValue(C::xmlKeyEnv2Release(), m_env2Release);
}

void JuSynth08Device::setEnv2Release(float val)
{
    setContinuousParameterValue(C::xmlKeyEnv2Release().toStdString(), val);
}

bool JuSynth08Device::env2KeyFollow() const
{
    return shownParameterValue(C::xmlKeyEnv2KeyFollow(), m_env2KeyFollow ? 1.0f : 0.0f) >= 0.5f;
}

void JuSynth08Device::setEnv2KeyFollow(bool val)
{
    setContinuousParameterValue(C::xmlKeyEnv2KeyFollow().toStdString(), val ? 1.0f : 0.0f);
}

float JuSynth08Device::delayTime() const
{
    return shownParameterValue(C::xmlKeyDelayTime(), m_delayTime);
}

void JuSynth08Device::setDelayTime(float val)
{
    setContinuousParameterValue(C::xmlKeyDelayTime().toStdString(), val);
}

float JuSynth08Device::delayFeedback() const
{
    return shownParameterValue(C::xmlKeyDelayFeedback(), m_delayFeedback);
}

void JuSynth08Device::setDelayFeedback(float val)
{
    setContinuousParameterValue(C::xmlKeyDelayFeedback().toStdString(), val);
}

float JuSynth08Device::delayLevel() const
{
    return shownParameterValue(C::xmlKeyDelayLevel(), m_delayLevel);
}

void JuSynth08Device::setDelayLevel(float val)
{
    setContinuousParameterValue(C::xmlKeyDelayLevel().toStdString(), val);
}

int JuSynth08Device::voiceMode() const
{
    return static_cast<int>(shownParameterValue(C::xmlKeyVoiceMode(), static_cast<float>(m_voiceMode)));
}

void JuSynth08Device::setVoiceMode(int mode)
{
    setDiscreteParameterValue(C::xmlKeyVoiceMode().toStdString(), mode);
}

float JuSynth08Device::unisonDetune() const
{
    return shownParameterValue(C::xmlKeyUnisonDetune(), m_unisonDetune);
}

void JuSynth08Device::setUnisonDetune(float detune)
{
    setContinuousParameterValue(C::xmlKeyUnisonDetune().toStdString(), detune);
}

float JuSynth08Device::portamento() const
{
    return shownParameterValue(C::xmlKeyPortamento(), m_portamento);
}

void JuSynth08Device::setPortamento(float port)
{
    setContinuousParameterValue(C::xmlKeyPortamento().toStdString(), port);
}

int JuSynth08Device::pitchBendRange() const
{
    return static_cast<int>(shownParameterValue(C::xmlKeyPitchBendRange(), static_cast<float>(m_pitchBendRange)));
}

void JuSynth08Device::setPitchBendRange(int range)
{
    setDiscreteParameterValue(C::xmlKeyPitchBendRange().toStdString(), range);
}

float JuSynth08Device::panSpread() const
{
    return shownParameterValue(C::xmlKeyPanSpread(), m_panSpread);
}

void JuSynth08Device::setPanSpread(float spread)
{
    setContinuousParameterValue(C::xmlKeyPanSpread().toStdString(), spread);
}

} // namespace noteahead
