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

#include "wavetable_synth_device.hpp"

#include "../../common/constants.hpp"
#include "../../common/parameter_mapper.hpp"
#include "../../common/utils.hpp"
#include "../../common/xml/project_reader.hpp"
#include "../../common/xml/project_writer.hpp"
#include "../../infra/midi/midi_cc_mapping.hpp"

#include <algorithm>
#include <cmath>

namespace noteahead {

void WavetableSynthDevice::Voice::reset()
{
    active = false;
    osc1.sync(0.0);
    osc2.sync(0.0);
    lpf.reset();
    hpf.reset();
    ampEg.reset();
    modEg.reset();
    lfo.reset();
    lfo2.reset();
    frequency = 0.0;
    glideFrequency = 0.0;
    pan = 0.5f;
}

void WavetableSynthDevice::Voice::trigger(uint8_t n, double freq, float p, float vel, uint64_t tid)
{
    note = n;
    triggerId = tid;
    velocity = vel;
    frequency = freq;
    if (glideFrequency == 0.0) {
        glideFrequency = freq;
    }
    pan = p;

    // Only sync oscillator phase on a fresh (idle) voice to avoid a pop from a
    // hard phase jump while the voice is still producing audio (retrigger/steal).
    if (!active) {
        osc1.sync(0.0);
        osc2.sync(0.0);
    }

    active = true;
    ampEg.trigger();
    modEg.trigger();
    lfo.reset();
    lfo2.reset();
}

void WavetableSynthDevice::Voice::release()
{
    ampEg.release();
    modEg.release();
}

WavetableSynthDevice::WavetableSynthDevice(std::string name)
  : m_name { std::move(name) }
{
    static const auto defaultWavetables = []() {
        std::vector<Wavetable::WavetableS> tables;
        tables.push_back(Wavetable::createClassicSet());
        tables.push_back(Wavetable::createSpectralSet());
        return tables;
    }();

    m_wavetables = defaultWavetables;

    m_voices.resize(MaxVoices);

    // Initialize Parameters
    addParameter(Parameter { Constants::NahdXml::xmlKeyOsc1Pos().toStdString(), 0.0f, 0, 10000, 0, 100, Parameter::Type::Continuous, { "wavetableSynthOsc1Pos" } });
    addParameter(Parameter { Constants::NahdXml::xmlKeyOsc1Octave().toStdString(), 0.0f, -2, 2, 0, 1, Parameter::Type::Discrete, { "wavetableSynthOsc1Octave" } });
    addParameter(Parameter { Constants::NahdXml::xmlKeyOsc1Pitch().toStdString(), 0.5f, 0, 10000, 5000, 100, Parameter::Type::Continuous, { "wavetableSynthOsc1Pitch" } });
    addParameter(Parameter { Constants::NahdXml::xmlKeyOsc1Level().toStdString(), 1.0f, 0, 10000, 10000, 100, Parameter::Type::Continuous, { "wavetableSynthOsc1Level" } });

    addParameter(Parameter { Constants::NahdXml::xmlKeyOsc2Pos().toStdString(), 0.5f, 0, 10000, 5000, 100, Parameter::Type::Continuous, { "wavetableSynthOsc2Pos" } });
    addParameter(Parameter { Constants::NahdXml::xmlKeyOsc2Octave().toStdString(), 0.0f, -2, 2, 0, 1, Parameter::Type::Discrete, { "wavetableSynthOsc2Octave" } });
    addParameter(Parameter { Constants::NahdXml::xmlKeyOsc2Pitch().toStdString(), 0.5f, 0, 10000, 5000, 100, Parameter::Type::Continuous, { "wavetableSynthOsc2Pitch" } });
    addParameter(Parameter { Constants::NahdXml::xmlKeyOsc2Level().toStdString(), 0.0f, 0, 10000, 0, 100, Parameter::Type::Continuous, { "wavetableSynthOsc2Level" } });

    addParameter(Parameter { Constants::NahdXml::xmlKeyNoiseLevel().toStdString(), 0.0f, 0, 10000, 0, 100, Parameter::Type::Continuous, { "wavetableSynthNoiseLevel" } });

    addParameter(Parameter { Constants::NahdXml::xmlKeyLpfCutoff().toStdString(), 1.0f, 0, 10000, 10000, 100, Parameter::Type::Continuous, { "wavetableSynthLpfCutoff" } });
    addParameter(Parameter { Constants::NahdXml::xmlKeyLpfResonance().toStdString(), 0.0f, 0, 10000, 0, 100, Parameter::Type::Continuous, { "wavetableSynthLpfResonance" } });
    addParameter(Parameter { Constants::NahdXml::xmlKeyHpfCutoff().toStdString(), 0.0f, 0, 10000, 0, 100, Parameter::Type::Continuous, { "wavetableSynthHpfCutoff" } });

    addParameter(Parameter { Constants::NahdXml::xmlKeyAmpAttack().toStdString(), 0.1f, 0, 10000, 1000, 100, Parameter::Type::Continuous, { "wavetableSynthAmpAttack" } });
    addParameter(Parameter { Constants::NahdXml::xmlKeyAmpDecay().toStdString(), 0.2f, 0, 10000, 2000, 100, Parameter::Type::Continuous, { "wavetableSynthAmpDecay" } });
    addParameter(Parameter { Constants::NahdXml::xmlKeyAmpSustain().toStdString(), 1.0f, 0, 10000, 10000, 100, Parameter::Type::Continuous, { "wavetableSynthAmpSustain" } });
    addParameter(Parameter { Constants::NahdXml::xmlKeyAmpRelease().toStdString(), 0.2f, 0, 10000, 2000, 100, Parameter::Type::Continuous, { "wavetableSynthAmpRelease" } });

    addParameter(Parameter { Constants::NahdXml::xmlKeyModAttack().toStdString(), 0.1f, 0, 10000, 1000, 100, Parameter::Type::Continuous, { "wavetableSynthModAttack" } });
    addParameter(Parameter { Constants::NahdXml::xmlKeyModDecay().toStdString(), 0.2f, 0, 10000, 2000, 100, Parameter::Type::Continuous, { "wavetableSynthModDecay" } });
    addParameter(Parameter { Constants::NahdXml::xmlKeyModIntensity().toStdString(), 0.5f, 0, 10000, 5000, 100, Parameter::Type::Continuous, { "wavetableSynthModIntensity" } });
    addParameter(Parameter { Constants::NahdXml::xmlKeyModTarget().toStdString(), 0.0f, 0, 4, 0, 1, Parameter::Type::Discrete, { "wavetableSynthModTarget" } });

    addParameter(Parameter { Constants::NahdXml::xmlKeyLfoWaveform().toStdString(), 1.0f, 0, 4, 1, 1, Parameter::Type::Discrete, { "wavetableSynthLfoWaveform" } });
    addParameter(Parameter { Constants::NahdXml::xmlKeyLfoMode().toStdString(), 0.0f, 0, 2, 0, 1, Parameter::Type::Discrete, { "wavetableSynthLfoMode" } });
    addParameter(Parameter { Constants::NahdXml::xmlKeyLfoRate().toStdString(), 0.5f, 0, 10000, 5000, 100, Parameter::Type::Continuous, { "wavetableSynthLfoRate" } });
    addParameter(Parameter { Constants::NahdXml::xmlKeyLfoIntensity().toStdString(), 0.5f, 0, 10000, 5000, 100, Parameter::Type::Continuous, { "wavetableSynthLfoIntensity" } });
    addParameter(Parameter { Constants::NahdXml::xmlKeyLfoTarget().toStdString(), 0.0f, 0, 3, 0, 1, Parameter::Type::Discrete, { "wavetableSynthLfoTarget" } });

    addParameter(Parameter { Constants::NahdXml::xmlKeyLfo2Waveform().toStdString(), 1.0f, 0, 4, 1, 1, Parameter::Type::Discrete, { "wavetableSynthLfo2Waveform" } });
    addParameter(Parameter { Constants::NahdXml::xmlKeyLfo2Mode().toStdString(), 0.0f, 0, 2, 0, 1, Parameter::Type::Discrete, { "wavetableSynthLfo2Mode" } });
    addParameter(Parameter { Constants::NahdXml::xmlKeyLfo2Rate().toStdString(), 0.5f, 0, 10000, 5000, 100, Parameter::Type::Continuous, { "wavetableSynthLfo2Rate" } });
    addParameter(Parameter { Constants::NahdXml::xmlKeyLfo2Intensity().toStdString(), 0.5f, 0, 10000, 5000, 100, Parameter::Type::Continuous, { "wavetableSynthLfo2Intensity" } });
    addParameter(Parameter { Constants::NahdXml::xmlKeyLfo2Target().toStdString(), 0.0f, 0, 3, 0, 1, Parameter::Type::Discrete, { "wavetableSynthLfo2Target" } });

    addParameter(Parameter { Constants::NahdXml::xmlKeyVoiceMode().toStdString(), 0.0f, 0, 1, 0, 1, Parameter::Type::Discrete, { "wavetableSynthVoiceMode" } });
    addParameter(Parameter { Constants::NahdXml::xmlKeyVoiceDepth().toStdString(), 0.1f, 0, 10000, 1000, 100, Parameter::Type::Continuous, { "wavetableSynthVoiceDepth" } });
    addParameter(Parameter { Constants::NahdXml::xmlKeyPanSpread().toStdString(), 0.5f, 0, 10000, 5000, 100, Parameter::Type::Continuous, { "wavetableSynthPanSpread" } });
    addParameter(Parameter { Constants::NahdXml::xmlKeyPortamento().toStdString(), 0.0f, 0, 10000, 0, 100, Parameter::Type::Continuous, { "wavetableSynthPortamento" } });
    addParameter(Parameter { Constants::NahdXml::xmlKeyWavetableIndex().toStdString(), 0.0f, 0, static_cast<int>(m_wavetables.size()) - 1, 0, 1, Parameter::Type::Discrete, { "wavetableSynthWavetableIndex" } });

    for (auto && voice : m_voices) {
        voice.lpf.setMode(CascadedSvf::Mode::LowPass);
        voice.hpf.setMode(CascadedSvf::Mode::HighPass);
    }

    WavetableSynthDevice::syncParameters();
}

WavetableSynthDevice::~WavetableSynthDevice() = default;

std::string WavetableSynthDevice::name() const
{
    return m_name;
}

std::string WavetableSynthDevice::category() const
{
    return Constants::NahdXml::xmlValueSynths().toStdString();
}

std::string WavetableSynthDevice::typeName() const
{
    return Constants::wavetableSynthDeviceName().toStdString();
}

std::string WavetableSynthDevice::typeIdString()
{
    return "9d4f6a1b-3c2e-4d5f-8a9b-0c1d2e3f4a5b";
}

std::string WavetableSynthDevice::typeId() const
{
    return typeIdString();
}

std::vector<MidiCcController> WavetableSynthDevice::availableMidiCcControllers() const
{
    return {
        MidiCcController { 7, "Volume" },
        MidiCcController { 10, "Pan" },
        MidiCcController { 74, "Cutoff" },
        MidiCcController { 71, "Resonance" },
        MidiCcController { 81, "HPF Cutoff" }
    };
}

void WavetableSynthDevice::processAudio(AudioContext & context)
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };

    prepareForProcessing(context);

    const uint32_t oversampledRate = context.sampleRate * 2;

    const double portamentoTime = ParameterMapper::mapExponential(m_portamento, 0.01, 2.0);
    const double portamentoCoeff = m_portamento > 0 ? 1.0 - std::pow(0.001, 1.0 / (portamentoTime * oversampledRate)) : 1.0;
    const double pbOffset = (static_cast<double>(m_pitchBend) - 8192.0) / 8192.0 * m_pitchBendRange;
    const double pbRatio = std::exp2(pbOffset / 12.0);

    for (auto && voice : m_voices) {
        voice.lfo.setWaveform(m_lfoWaveform);
        voice.lfo.setMode(m_lfoMode);
        if (m_lfoMode == Lfo::Mode::BPM) {
            voice.lfo.setFrequency(m_bpm, m_lfoRate);
        } else {
            voice.lfo.setFrequency(ParameterMapper::mapLfoFrequency(m_lfoRate, 0.05, 20.0));
        }

        voice.lfo2.setWaveform(m_lfo2Waveform);
        voice.lfo2.setMode(m_lfo2Mode);
        if (m_lfo2Mode == Lfo::Mode::BPM) {
            voice.lfo2.setFrequency(m_bpm, m_lfo2Rate);
        } else {
            voice.lfo2.setFrequency(ParameterMapper::mapLfoFrequency(m_lfo2Rate, 0.05, 20.0));
        }

        if (voice.active) {
            renderVoice(voice, context, oversampledRate, portamentoCoeff, pbRatio);
        }
    }

    const float vol = volumeInternal();
    for (uint32_t i = 0; i < context.frameCount; i++) {
        const float l0 = m_oversampledBuffer[i * 4];
        const float r0 = m_oversampledBuffer[i * 4 + 1];
        const float l1 = m_oversampledBuffer[i * 4 + 2];
        const float r1 = m_oversampledBuffer[i * 4 + 3];

        context.buffer[i * 2] += static_cast<double>(m_oversamplerL.process(l0, l1) * vol);
        context.buffer[i * 2 + 1] += static_cast<double>(m_oversamplerR.process(r0, r1) * vol);
    }
}

void WavetableSynthDevice::prepareForProcessing(AudioContext & context)
{
    setSampleRate(context.sampleRate);
    const size_t requiredSize = static_cast<size_t>(context.frameCount) * 4;
    if (m_oversampledBuffer.size() < requiredSize) {
        m_oversampledBuffer.resize(requiredSize);
    }
    std::fill(m_oversampledBuffer.begin(), m_oversampledBuffer.begin() + requiredSize, 0.0f);
}

void WavetableSynthDevice::renderVoice(Voice & voice, AudioContext & context, uint32_t oversampledRate, double portamentoCoeff, double pbRatio)
{
    updateVoiceParameters(voice, oversampledRate);

    const float gain = (1.0f / static_cast<float>(MaxVoices)) * linearGainInternal() * voice.velocity;
    for (uint32_t i = 0; i < context.frameCount; i++) {
        const float voicePan = std::clamp(panInternal() + voice.pan - 0.5f, 0.0f, 1.0f);
        const float panL = (1.0f - voicePan) * 2.0f;
        const float panR = voicePan * 2.0f;

        for (int subSample = 0; subSample < 2; subSample++) {
            voice.glideFrequency += (voice.frequency - voice.glideFrequency) * portamentoCoeff;
            const ModulationValues mods = calculateModulation(voice);
            const float sample = generateVoiceSample(voice, mods, oversampledRate, pbRatio) * gain;

            m_oversampledBuffer[(i * 2 + subSample) * 2] += sample * panL;
            m_oversampledBuffer[(i * 2 + subSample) * 2 + 1] += sample * panR;
        }

        if (voice.ampEg.state() == AdsrEnvelope::State::Idle) {
            voice.active = false;
            break;
        }
    }
}

void WavetableSynthDevice::updateVoiceParameters(Voice & voice, uint32_t oversampledRate)
{
    voice.osc1.setSampleRate(oversampledRate);
    voice.osc2.setSampleRate(oversampledRate);
    voice.lpf.setSampleRate(oversampledRate);
    voice.hpf.setSampleRate(oversampledRate);
    voice.ampEg.setSampleRate(oversampledRate);
    voice.modEg.setSampleRate(oversampledRate);
    voice.lfo.setSampleRate(oversampledRate);
    voice.lfo2.setSampleRate(oversampledRate);

    voice.lpf.setResonance(m_lpfResonance);
    voice.hpf.setResonance(0.0f);
}

bool WavetableSynthDevice::hasActiveAudio() const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    return std::ranges::any_of(m_voices, [](const auto & voice) { return voice.active; });
}

void WavetableSynthDevice::processMidiNoteOn(uint8_t note, uint8_t velocity)
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    handleNoteOn(note, velocity);
}

void WavetableSynthDevice::processMidiNoteOff(uint8_t note)
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    handleNoteOff(note);
}

void WavetableSynthDevice::processMidiCc(uint8_t controller, uint8_t value, uint8_t)
{
    const float val = static_cast<float>(value) / 127.0f;
    bool changed = false;

    {
        const std::lock_guard<std::recursive_mutex> lock { mutex() };

        if (controller == 7) {
            changed = updateVolumeParameter(val, false);
        } else if (controller == 10) {
            changed = updatePanParameter(val, false);
        } else if (controller == 74) {
            m_lpfCutoff = val;
            if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyLpfCutoff().toStdString()); synthParameter) {
                synthParameter->get().setValue(val);
                syncParameters();
                changed = true;
            }
        } else if (controller == 71) {
            m_lpfResonance = val;
            if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyLpfResonance().toStdString()); synthParameter) {
                synthParameter->get().setValue(val);
                syncParameters();
                changed = true;
            }
        } else if (controller == 81) {
            m_hpfCutoff = val;
            if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyHpfCutoff().toStdString()); synthParameter) {
                synthParameter->get().setValue(val);
                syncParameters();
                changed = true;
            }
        }
    }

    if (changed) {
        emit dataChanged();
    }
}

void WavetableSynthDevice::processMidiPitchBend(uint16_t value, uint8_t)
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    m_pitchBend = value;
}

void WavetableSynthDevice::processMidiAllNotesOff()
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    for (auto && voice : m_voices) {
        if (voice.active) {
            voice.release();
        }
    }
}

void WavetableSynthDevice::setBpm(float bpm)
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    m_bpm = bpm;
}

void WavetableSynthDevice::reset()
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    Device::reset();
    resetAudio();
    syncParameters();
}

void WavetableSynthDevice::resetAudio()
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    for (auto && voice : m_voices) {
        voice.reset();
    }
    m_polyNextVoice = 0;
    m_nextTriggerId = 1;
}

void WavetableSynthDevice::handleNoteOn(uint8_t note, uint8_t velocity)
{
    const double freq = midiNoteToFreq(note);
    const float finalVel = static_cast<float>(velocity) / 127.0f;
    const uint64_t tid = m_nextTriggerId++;

    if (m_voiceMode == VoiceMode::Unison) {
        for (size_t i = 0; i < MaxVoices; i++) {
            // Non-linear detune spread for better texture
            const double detuneAmount = (static_cast<double>(i) - (MaxVoices - 1) / 2.0) * std::pow(m_voiceDepth, 1.2) * (0.15 + 0.05 * (i % 2));
            const double voiceFreq = freq * std::pow(2.0, detuneAmount / 12.0);

            if (m_portamento <= 0.001f) {
                m_voices.at(i).glideFrequency = voiceFreq;
            }

            const float side = (i % 2 == 0) ? -1.0f : 1.0f;
            const float depth = 1.0f - static_cast<float>(i / 2) * (2.0f / static_cast<float>(MaxVoices));
            const float pan = 0.5f + (side * depth * m_panSpread * 0.5f);

            m_voices.at(i).trigger(note, voiceFreq, pan, finalVel, tid);
        }
    } else {
        // Polyphonic mode
        size_t voiceIndex = m_polyNextVoice;
        bool found = false;

        for (size_t i = 0; i < MaxVoices; i++) {
            const size_t idx = (m_polyNextVoice + i) % MaxVoices;
            if (!m_voices.at(idx).active) {
                voiceIndex = idx;
                found = true;
                break;
            }
        }

        if (!found) {
            uint64_t oldestId = std::numeric_limits<uint64_t>::max();
            for (size_t i = 0; i < MaxVoices; i++) {
                if (m_voices.at(i).triggerId < oldestId) {
                    oldestId = m_voices.at(i).triggerId;
                    voiceIndex = i;
                }
            }
        }

        if (m_portamento <= 0.001f) {
            m_voices.at(voiceIndex).glideFrequency = freq;
        }

        m_voices.at(voiceIndex).trigger(note, freq, 0.5f, finalVel, tid);
        m_polyNextVoice = (voiceIndex + 1) % MaxVoices;
    }
}

void WavetableSynthDevice::handleNoteOff(uint8_t note)
{
    for (auto && voice : m_voices) {
        if (voice.active && voice.note == note) {
            voice.release();
        }
    }
}

double WavetableSynthDevice::midiNoteToFreq(uint8_t note) const
{
    return 440.0 * std::pow(2.0, (note - 69) / 12.0);
}

WavetableSynthDevice::ModulationValues WavetableSynthDevice::calculateModulation(Voice & voice) const
{
    ModulationValues mods = ModulationValues {};
    mods.ampEnvelope = voice.ampEg.nextSample();
    mods.modEnvelope = voice.modEg.nextSample() * (m_modInt * 2.0 - 1.0);
    mods.lfoValue = voice.lfo.nextSample() * (m_lfoInt * 2.0 - 1.0);
    mods.lfo2Value = voice.lfo2.nextSample() * (m_lfo2Int * 2.0 - 1.0);

    if (m_modTarget == ModTarget::Cutoff) {
        mods.cutoffMod = mods.modEnvelope;
    } else if (m_modTarget == ModTarget::Osc1Pos) {
        mods.osc1PosMod = mods.modEnvelope;
    } else if (m_modTarget == ModTarget::Osc2Pos) {
        mods.osc2PosMod = mods.modEnvelope;
    }

    if (m_lfoTarget == LfoTarget::Cutoff) {
        mods.cutoffMod += mods.lfoValue;
    } else if (m_lfoTarget == LfoTarget::Osc1Pos) {
        mods.osc1PosMod += mods.lfoValue;
    } else if (m_lfoTarget == LfoTarget::Osc2Pos) {
        mods.osc2PosMod += mods.lfoValue;
    }

    if (m_lfo2Target == LfoTarget::Cutoff) {
        mods.cutoffMod += mods.lfo2Value;
    } else if (m_lfo2Target == LfoTarget::Osc1Pos) {
        mods.osc1PosMod += mods.lfo2Value;
    } else if (m_lfo2Target == LfoTarget::Osc2Pos) {
        mods.osc2PosMod += mods.lfo2Value;
    }

    return mods;
}

float WavetableSynthDevice::generateVoiceSample(Voice & voice, const ModulationValues & mods, double /*oversampledRate*/, double pbRatio)
{
    const double freq = voice.glideFrequency * pbRatio;

    double osc1Freq = freq * m_osc1BasePitchRatio;
    double osc2Freq = freq * m_osc2BasePitchRatio;

    double osc1PitchMod = 0.0;
    double osc2PitchMod = 0.0;

    if (m_modTarget == ModTarget::Pitch1) {
        osc1PitchMod += mods.modEnvelope;
    } else if (m_modTarget == ModTarget::Pitch2) {
        osc2PitchMod += mods.modEnvelope;
    }

    if (m_lfoTarget == LfoTarget::Pitch) {
        osc1PitchMod += mods.lfoValue;
        osc2PitchMod += mods.lfoValue;
    }

    if (m_lfo2Target == LfoTarget::Pitch) {
        osc1PitchMod += mods.lfo2Value;
        osc2PitchMod += mods.lfo2Value;
    }

    osc1Freq *= std::exp2(osc1PitchMod);
    osc2Freq *= std::exp2(osc2PitchMod);

    voice.osc1.setFrequency(osc1Freq);
    voice.osc1.setPosition(std::clamp(m_osc1Pos + mods.osc1PosMod, 0.0, 1.0));
    const float osc1Val = static_cast<float>(voice.osc1.nextSample()) * m_osc1Level;

    voice.osc2.setFrequency(osc2Freq);
    voice.osc2.setPosition(std::clamp(m_osc2Pos + mods.osc2PosMod, 0.0, 1.0));
    const float osc2Val = static_cast<float>(voice.osc2.nextSample()) * m_osc2Level;

    const float noise = m_noiseDist(m_rng) * m_noiseLevel;

    const float mix = osc1Val + osc2Val + noise;

    voice.lpf.setCutoff(std::clamp(m_lpfCutoff + static_cast<float>(mods.cutoffMod), 0.0f, 1.0f));
    voice.hpf.setCutoff(m_hpfCutoff);

    return voice.hpf.process(voice.lpf.process(mix)) * static_cast<float>(mods.ampEnvelope);
}

void WavetableSynthDevice::syncParameters()
{
    Device::syncParameters();

    auto updateParam = [this](const QString & key, float & var) {
        if (const auto synthParameter = parameter(key.toStdString()); synthParameter) {
            var = synthParameter->get().value();
        }
    };

    auto updateDiscreteParam = [this](const QString & key, auto & var) {
        if (const auto synthParameter = parameter(key.toStdString()); synthParameter) {
            var = static_cast<std::decay_t<decltype(var)>>(synthParameter->get().xmlValue());
        }
    };

    updateParam(Constants::NahdXml::xmlKeyOsc1Pos(), m_osc1Pos);
    updateDiscreteParam(Constants::NahdXml::xmlKeyOsc1Octave(), m_osc1Octave);
    updateParam(Constants::NahdXml::xmlKeyOsc1Pitch(), m_osc1Pitch);
    updateParam(Constants::NahdXml::xmlKeyOsc1Level(), m_osc1Level);

    updateParam(Constants::NahdXml::xmlKeyOsc2Pos(), m_osc2Pos);
    updateDiscreteParam(Constants::NahdXml::xmlKeyOsc2Octave(), m_osc2Octave);
    updateParam(Constants::NahdXml::xmlKeyOsc2Pitch(), m_osc2Pitch);
    updateParam(Constants::NahdXml::xmlKeyOsc2Level(), m_osc2Level);

    updateParam(Constants::NahdXml::xmlKeyNoiseLevel(), m_noiseLevel);

    updateParam(Constants::NahdXml::xmlKeyLpfCutoff(), m_lpfCutoff);
    updateParam(Constants::NahdXml::xmlKeyLpfResonance(), m_lpfResonance);
    updateParam(Constants::NahdXml::xmlKeyHpfCutoff(), m_hpfCutoff);

    updateParam(Constants::NahdXml::xmlKeyAmpAttack(), m_ampAttack);
    updateParam(Constants::NahdXml::xmlKeyAmpDecay(), m_ampDecay);
    updateParam(Constants::NahdXml::xmlKeyAmpSustain(), m_ampSustain);
    updateParam(Constants::NahdXml::xmlKeyAmpRelease(), m_ampRelease);

    updateParam(Constants::NahdXml::xmlKeyModAttack(), m_modAttack);
    updateParam(Constants::NahdXml::xmlKeyModDecay(), m_modDecay);
    updateParam(Constants::NahdXml::xmlKeyModIntensity(), m_modInt);
    updateDiscreteParam(Constants::NahdXml::xmlKeyModTarget(), m_modTarget);

    updateDiscreteParam(Constants::NahdXml::xmlKeyLfoWaveform(), m_lfoWaveform);
    updateDiscreteParam(Constants::NahdXml::xmlKeyLfoMode(), m_lfoMode);
    updateParam(Constants::NahdXml::xmlKeyLfoRate(), m_lfoRate);
    updateParam(Constants::NahdXml::xmlKeyLfoIntensity(), m_lfoInt);
    updateDiscreteParam(Constants::NahdXml::xmlKeyLfoTarget(), m_lfoTarget);

    updateDiscreteParam(Constants::NahdXml::xmlKeyLfo2Waveform(), m_lfo2Waveform);
    updateDiscreteParam(Constants::NahdXml::xmlKeyLfo2Mode(), m_lfo2Mode);
    updateParam(Constants::NahdXml::xmlKeyLfo2Rate(), m_lfo2Rate);
    updateParam(Constants::NahdXml::xmlKeyLfo2Intensity(), m_lfo2Int);
    updateDiscreteParam(Constants::NahdXml::xmlKeyLfo2Target(), m_lfo2Target);

    updateDiscreteParam(Constants::NahdXml::xmlKeyVoiceMode(), m_voiceMode);
    updateParam(Constants::NahdXml::xmlKeyVoiceDepth(), m_voiceDepth);
    updateParam(Constants::NahdXml::xmlKeyPanSpread(), m_panSpread);
    updateParam(Constants::NahdXml::xmlKeyPortamento(), m_portamento);
    updateDiscreteParam(Constants::NahdXml::xmlKeyWavetableIndex(), m_wavetableIndex);

    m_wavetableIndex = std::clamp(m_wavetableIndex, 0, static_cast<int>(m_wavetables.size()) - 1);
    const auto currentWavetable = m_wavetables.at(static_cast<size_t>(m_wavetableIndex));

    const double osc1PitchOffset = ParameterMapper::mapCubicCentered(m_osc1Pitch * 2.0 - 1.0, -1200, 1200);
    m_osc1BasePitchRatio = std::pow(2.0, (m_osc1Octave * 12.0 + osc1PitchOffset / 100.0) / 12.0);

    const double osc2PitchOffset = ParameterMapper::mapCubicCentered(m_osc2Pitch * 2.0 - 1.0, -1200, 1200);
    m_osc2BasePitchRatio = std::pow(2.0, (m_osc2Octave * 12.0 + osc2PitchOffset / 100.0) / 12.0);

    for (auto && voice : m_voices) {
        voice.osc1.setWavetable(currentWavetable);
        voice.osc2.setWavetable(currentWavetable);

        voice.lfo.setWaveform(m_lfoWaveform);
        voice.lfo.setMode(m_lfoMode);
        if (m_lfoMode == Lfo::Mode::BPM) {
            voice.lfo.setFrequency(m_bpm, m_lfoRate);
        } else {
            voice.lfo.setFrequency(ParameterMapper::mapLfoFrequency(m_lfoRate, 0.05, 20.0));
        }

        voice.lfo2.setWaveform(m_lfo2Waveform);
        voice.lfo2.setMode(m_lfo2Mode);
        if (m_lfo2Mode == Lfo::Mode::BPM) {
            voice.lfo2.setFrequency(m_bpm, m_lfo2Rate);
        } else {
            voice.lfo2.setFrequency(ParameterMapper::mapLfoFrequency(m_lfo2Rate, 0.05, 20.0));
        }

        voice.ampEg.setAttackTime(ParameterMapper::mapExponential(m_ampAttack, 0.001, 10.0));
        voice.ampEg.setDecayTime(ParameterMapper::mapExponential(m_ampDecay, 0.01, 10.0));
        voice.ampEg.setSustainLevel(m_ampSustain);
        voice.ampEg.setReleaseTime(ParameterMapper::mapExponential(m_ampRelease, 0.01, 10.0));

        voice.modEg.setAttackTime(ParameterMapper::mapExponential(m_modAttack, 0.001, 10.0));
        voice.modEg.setDecayTime(ParameterMapper::mapExponential(m_modDecay, 0.01, 10.0));
        voice.modEg.setSustainLevel(0.0);
        voice.modEg.setReleaseTime(ParameterMapper::mapExponential(m_modDecay, 0.01, 10.0));
    }
}

void WavetableSynthDevice::serializeToXml(ProjectWriter & writer) const
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

void WavetableSynthDevice::deserializeFromXml(ProjectReader & reader)
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
                } else {
                    reader.skipCurrentElement();
                }
            }
        }

        syncParameters();
    }
    emit dataChanged();
}

// Accessors (Osc 1)
float WavetableSynthDevice::osc1Pos() const
{
    return m_osc1Pos;
}

void WavetableSynthDevice::setOsc1Pos(float pos)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyOsc1Pos().toStdString()); synthParameter) {
        synthParameter->get().setValue(pos);
        syncParameters();
        emit dataChanged();
    }
}

int WavetableSynthDevice::osc1Octave() const
{
    return m_osc1Octave;
}

void WavetableSynthDevice::setOsc1Octave(int octave)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyOsc1Octave().toStdString()); synthParameter) {
        synthParameter->get().setFromXml(octave);
        syncParameters();
        emit dataChanged();
    }
}

float WavetableSynthDevice::osc1Pitch() const
{
    return m_osc1Pitch;
}

void WavetableSynthDevice::setOsc1Pitch(float pitch)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyOsc1Pitch().toStdString()); synthParameter) {
        synthParameter->get().setValue(pitch);
        syncParameters();
        emit dataChanged();
    }
}

float WavetableSynthDevice::osc1Level() const
{
    return m_osc1Level;
}

void WavetableSynthDevice::setOsc1Level(float level)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyOsc1Level().toStdString()); synthParameter) {
        synthParameter->get().setValue(level);
        syncParameters();
        emit dataChanged();
    }
}

// Accessors (Osc 2)
float WavetableSynthDevice::osc2Pos() const
{
    return m_osc2Pos;
}

void WavetableSynthDevice::setOsc2Pos(float pos)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyOsc2Pos().toStdString()); synthParameter) {
        synthParameter->get().setValue(pos);
        syncParameters();
        emit dataChanged();
    }
}

int WavetableSynthDevice::osc2Octave() const
{
    return m_osc2Octave;
}

void WavetableSynthDevice::setOsc2Octave(int octave)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyOsc2Octave().toStdString()); synthParameter) {
        synthParameter->get().setFromXml(octave);
        syncParameters();
        emit dataChanged();
    }
}

float WavetableSynthDevice::osc2Pitch() const
{
    return m_osc2Pitch;
}

void WavetableSynthDevice::setOsc2Pitch(float pitch)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyOsc2Pitch().toStdString()); synthParameter) {
        synthParameter->get().setValue(pitch);
        syncParameters();
        emit dataChanged();
    }
}

float WavetableSynthDevice::osc2Level() const
{
    return m_osc2Level;
}

void WavetableSynthDevice::setOsc2Level(float level)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyOsc2Level().toStdString()); synthParameter) {
        synthParameter->get().setValue(level);
        syncParameters();
        emit dataChanged();
    }
}

// Noise
float WavetableSynthDevice::noiseLevel() const
{
    return m_noiseLevel;
}

void WavetableSynthDevice::setNoiseLevel(float level)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyNoiseLevel().toStdString()); synthParameter) {
        synthParameter->get().setValue(level);
        syncParameters();
        emit dataChanged();
    }
}

// Filter
float WavetableSynthDevice::lpfCutoff() const
{
    return m_lpfCutoff;
}

void WavetableSynthDevice::setLpfCutoff(float cutoff)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyLpfCutoff().toStdString()); synthParameter) {
        synthParameter->get().setValue(cutoff);
        syncParameters();
        emit dataChanged();
    }
}

float WavetableSynthDevice::lpfResonance() const
{
    return m_lpfResonance;
}

void WavetableSynthDevice::setLpfResonance(float resonance)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyLpfResonance().toStdString()); synthParameter) {
        synthParameter->get().setValue(resonance);
        syncParameters();
        emit dataChanged();
    }
}

float WavetableSynthDevice::hpfCutoff() const
{
    return m_hpfCutoff;
}

void WavetableSynthDevice::setHpfCutoff(float cutoff)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyHpfCutoff().toStdString()); synthParameter) {
        synthParameter->get().setValue(cutoff);
        syncParameters();
        emit dataChanged();
    }
}

// Amp EG
float WavetableSynthDevice::ampAttack() const
{
    return m_ampAttack;
}

void WavetableSynthDevice::setAmpAttack(float a)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyAmpAttack().toStdString()); synthParameter) {
        synthParameter->get().setValue(a);
        syncParameters();
        emit dataChanged();
    }
}

float WavetableSynthDevice::ampDecay() const
{
    return m_ampDecay;
}

void WavetableSynthDevice::setAmpDecay(float d)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyAmpDecay().toStdString()); synthParameter) {
        synthParameter->get().setValue(d);
        syncParameters();
        emit dataChanged();
    }
}

float WavetableSynthDevice::ampSustain() const
{
    return m_ampSustain;
}

void WavetableSynthDevice::setAmpSustain(float s)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyAmpSustain().toStdString()); synthParameter) {
        synthParameter->get().setValue(s);
        syncParameters();
        emit dataChanged();
    }
}

float WavetableSynthDevice::ampRelease() const
{
    return m_ampRelease;
}

void WavetableSynthDevice::setAmpRelease(float r)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyAmpRelease().toStdString()); synthParameter) {
        synthParameter->get().setValue(r);
        syncParameters();
        emit dataChanged();
    }
}

// Mod EG
float WavetableSynthDevice::modAttack() const
{
    return m_modAttack;
}

void WavetableSynthDevice::setModAttack(float a)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyModAttack().toStdString()); synthParameter) {
        synthParameter->get().setValue(a);
        syncParameters();
        emit dataChanged();
    }
}

float WavetableSynthDevice::modDecay() const
{
    return m_modDecay;
}

void WavetableSynthDevice::setModDecay(float d)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyModDecay().toStdString()); synthParameter) {
        synthParameter->get().setValue(d);
        syncParameters();
        emit dataChanged();
    }
}

float WavetableSynthDevice::modInt() const
{
    return m_modInt;
}

void WavetableSynthDevice::setModInt(float intensity)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyModIntensity().toStdString()); synthParameter) {
        synthParameter->get().setValue(intensity);
        syncParameters();
        emit dataChanged();
    }
}

WavetableSynthDevice::ModTarget WavetableSynthDevice::modTarget() const
{
    return m_modTarget;
}

void WavetableSynthDevice::setModTarget(ModTarget target)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyModTarget().toStdString()); synthParameter) {
        synthParameter->get().setFromXml(static_cast<int>(target));
        syncParameters();
        emit dataChanged();
    }
}

// LFO
Lfo::Waveform WavetableSynthDevice::lfoWaveform() const
{
    return m_lfoWaveform;
}

void WavetableSynthDevice::setLfoWaveform(Lfo::Waveform wave)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyLfoWaveform().toStdString()); synthParameter) {
        synthParameter->get().setFromXml(static_cast<int>(wave));
        syncParameters();
        emit dataChanged();
    }
}

Lfo::Mode WavetableSynthDevice::lfoMode() const
{
    return m_lfoMode;
}

void WavetableSynthDevice::setLfoMode(Lfo::Mode mode)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyLfoMode().toStdString()); synthParameter) {
        synthParameter->get().setFromXml(static_cast<int>(mode));
        syncParameters();
        emit dataChanged();
    }
}

float WavetableSynthDevice::lfoRate() const
{
    return m_lfoRate;
}

void WavetableSynthDevice::setLfoRate(float rate)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyLfoRate().toStdString()); synthParameter) {
        synthParameter->get().setValue(rate);
        syncParameters();
        emit dataChanged();
    }
}

float WavetableSynthDevice::lfoInt() const
{
    return m_lfoInt;
}

void WavetableSynthDevice::setLfoInt(float intensity)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyLfoIntensity().toStdString()); synthParameter) {
        synthParameter->get().setValue(intensity);
        syncParameters();
        emit dataChanged();
    }
}

WavetableSynthDevice::LfoTarget WavetableSynthDevice::lfoTarget() const
{
    return m_lfoTarget;
}

void WavetableSynthDevice::setLfoTarget(LfoTarget target)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyLfoTarget().toStdString()); synthParameter) {
        synthParameter->get().setFromXml(static_cast<int>(target));
        syncParameters();
        emit dataChanged();
    }
}

// LFO 2
Lfo::Waveform WavetableSynthDevice::lfo2Waveform() const
{
    return m_lfo2Waveform;
}

void WavetableSynthDevice::setLfo2Waveform(Lfo::Waveform wave)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyLfo2Waveform().toStdString()); synthParameter) {
        synthParameter->get().setFromXml(static_cast<int>(wave));
        syncParameters();
        emit dataChanged();
    }
}

Lfo::Mode WavetableSynthDevice::lfo2Mode() const
{
    return m_lfo2Mode;
}

void WavetableSynthDevice::setLfo2Mode(Lfo::Mode mode)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyLfo2Mode().toStdString()); synthParameter) {
        synthParameter->get().setFromXml(static_cast<int>(mode));
        syncParameters();
        emit dataChanged();
    }
}

float WavetableSynthDevice::lfo2Rate() const
{
    return m_lfo2Rate;
}

void WavetableSynthDevice::setLfo2Rate(float rate)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyLfo2Rate().toStdString()); synthParameter) {
        synthParameter->get().setValue(rate);
        syncParameters();
        emit dataChanged();
    }
}

float WavetableSynthDevice::lfo2Int() const
{
    return m_lfo2Int;
}

void WavetableSynthDevice::setLfo2Int(float intensity)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyLfo2Intensity().toStdString()); synthParameter) {
        synthParameter->get().setValue(intensity);
        syncParameters();
        emit dataChanged();
    }
}

WavetableSynthDevice::LfoTarget WavetableSynthDevice::lfo2Target() const
{
    return m_lfo2Target;
}

void WavetableSynthDevice::setLfo2Target(LfoTarget target)
{
    if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyLfo2Target().toStdString()); synthParameter) {
        synthParameter->get().setFromXml(static_cast<int>(target));
        syncParameters();
        emit dataChanged();
    }
}

// Voice / Global
WavetableSynthDevice::VoiceMode WavetableSynthDevice::voiceMode() const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    return m_voiceMode;
}

void WavetableSynthDevice::setVoiceMode(VoiceMode mode)
{
    bool changed = false;
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyVoiceMode().toStdString()); synthParameter) {
            synthParameter->get().setFromXml(static_cast<int>(mode));
            syncParameters();
            changed = true;
        }
    }
    if (changed) {
        emit dataChanged();
    }
}

float WavetableSynthDevice::voiceDepth() const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    return m_voiceDepth;
}

void WavetableSynthDevice::setVoiceDepth(float depth)
{
    bool changed = false;
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyVoiceDepth().toStdString()); synthParameter) {
            synthParameter->get().setValue(depth);
            syncParameters();
            changed = true;
        }
    }
    if (changed) {
        emit dataChanged();
    }
}

float WavetableSynthDevice::panSpread() const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    return m_panSpread;
}

void WavetableSynthDevice::setPanSpread(float spread)
{
    bool changed = false;
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyPanSpread().toStdString()); synthParameter) {
            synthParameter->get().setValue(spread);
            syncParameters();
            changed = true;
        }
    }
    if (changed) {
        emit dataChanged();
    }
}

float WavetableSynthDevice::portamento() const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    return m_portamento;
}

void WavetableSynthDevice::setPortamento(float p)
{
    bool changed = false;
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyPortamento().toStdString()); synthParameter) {
            synthParameter->get().setValue(p);
            syncParameters();
            changed = true;
        }
    }
    if (changed) {
        emit dataChanged();
    }
}

int WavetableSynthDevice::wavetableIndex() const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    return m_wavetableIndex;
}

void WavetableSynthDevice::setWavetableIndex(int index)
{
    bool changed = false;
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        if (const auto synthParameter = parameter(Constants::NahdXml::xmlKeyWavetableIndex().toStdString()); synthParameter) {
            synthParameter->get().setFromXml(index);
            syncParameters();
            changed = true;
        }
    }
    if (changed) {
        emit dataChanged();
    }
}

std::vector<std::string> WavetableSynthDevice::wavetableNames() const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    std::vector<std::string> names;
    for (const auto & wt : m_wavetables) {
        names.push_back(wt->name());
    }
    return names;
}

int WavetableSynthDevice::pitchBendRange() const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    return m_pitchBendRange;
}

void WavetableSynthDevice::setPitchBendRange(int range)
{
    bool changed = false;
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        m_pitchBendRange = range;
        syncParameters();
        changed = true;
    }
    if (changed) {
        emit dataChanged();
    }
}

} // namespace noteahead
