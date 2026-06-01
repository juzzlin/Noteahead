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

#include "synth_device.hpp"

#include "../../common/constants.hpp"
#include "../../common/parameter_mapper.hpp"
#include "../../common/utils.hpp"
#include "../../infra/midi/midi_cc_mapping.hpp"
#include "synth_presets.hpp"

#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <algorithm>
#include <cmath>

namespace noteahead {

void SynthDevice::Voice::reset()
{
    active = false;
    vco1.sync(0.0);
    vco2.sync(0.0);
    vco3.sync(0.0);
    lpf.reset();
    hpf.reset();
    ampEg.reset();
    modEg.reset();
    lfo.reset();
    glideFrequency = 0.0;
}

void SynthDevice::Voice::trigger(uint8_t n, double freq, float p, float v, bool phaseSync)
{
    note = n;
    frequency = freq;
    if (glideFrequency == 0.0) {
        glideFrequency = freq;
    }
    pan = p;
    velocity = v;
    active = true;

    lfo.reset();

    if (phaseSync) {
        vco1.sync(0.0);
        vco2.sync(0.0);
        vco3.sync(0.0);
    }
    ampEg.trigger();
    modEg.trigger();
}

void SynthDevice::Voice::triggerRandomized(uint8_t n, double freq, float p, float v, double randomPhase)
{
    note = n;
    frequency = freq;
    if (glideFrequency == 0.0) {
        glideFrequency = freq;
    }
    pan = p;
    velocity = v;
    active = true;

    lfo.reset();

    vco1.sync(randomPhase);
    vco2.sync(std::fmod(randomPhase + 0.33, 1.0));
    vco3.sync(std::fmod(randomPhase + 0.66, 1.0));
    ampEg.trigger();
    modEg.trigger();
}

void SynthDevice::Voice::release()
{
    ampEg.release();
    modEg.release();
}

SynthDevice::SynthDevice(std::string name)
  : m_name { std::move(name) }
{
    m_voices.resize(MaxVoices);

    // Initialize Parameters
    addParameter(Parameter { Constants::NahdXml::xmlKeySynthVco1Waveform().toStdString(), 1.0f, 0, 3, 1, 1, Parameter::Type::Discrete });
    addParameter(Parameter { Constants::NahdXml::xmlKeySynthVco1Octave().toStdString(), 0.0f, -1, 2, 0, 1, Parameter::Type::Discrete });
    addParameter(Parameter { Constants::NahdXml::xmlKeySynthVco1Pitch().toStdString(), 0.5f, -2400, 2400, 0 });
    addParameter(Parameter { Constants::NahdXml::xmlKeySynthVco1Shape().toStdString(), 0.0f, 0, 100, 0 });
    addParameter(Parameter { Constants::NahdXml::xmlKeySynthVco1Sync().toStdString(), 0.0f, 0, 1, 0, 1, Parameter::Type::Boolean });

    addParameter(Parameter { Constants::NahdXml::xmlKeySynthVco2Waveform().toStdString(), 1.0f, 0, 3, 1, 1, Parameter::Type::Discrete });
    addParameter(Parameter { Constants::NahdXml::xmlKeySynthVco2Octave().toStdString(), 0.0f, -1, 2, 0, 1, Parameter::Type::Discrete });
    addParameter(Parameter { Constants::NahdXml::xmlKeySynthVco2Pitch().toStdString(), 0.5f, -2400, 2400, 0 });
    addParameter(Parameter { Constants::NahdXml::xmlKeySynthVco2Shape().toStdString(), 0.0f, 0, 100, 0 });
    addParameter(Parameter { Constants::NahdXml::xmlKeySynthVco2Sync().toStdString(), 0.0f, 0, 1, 0, 1, Parameter::Type::Boolean });

    addParameter(Parameter { Constants::NahdXml::xmlKeySynthVco3Waveform().toStdString(), 1.0f, 0, 3, 1, 1, Parameter::Type::Discrete });
    addParameter(Parameter { Constants::NahdXml::xmlKeySynthVco3Octave().toStdString(), 0.0f, -1, 2, 0, 1, Parameter::Type::Discrete });
    addParameter(Parameter { Constants::NahdXml::xmlKeySynthVco3Pitch().toStdString(), 0.5f, -2400, 2400, 0 });
    addParameter(Parameter { Constants::NahdXml::xmlKeySynthVco3Shape().toStdString(), 0.0f, 0, 100, 0 });
    addParameter(Parameter { Constants::NahdXml::xmlKeySynthVco3Sync().toStdString(), 0.0f, 0, 1, 0, 1, Parameter::Type::Boolean });

    addParameter(Parameter { Constants::NahdXml::xmlKeySynthMultiMode().toStdString(), 1.0f, 0, 3, 1, 1, Parameter::Type::Discrete }); // Low default
    addParameter(Parameter { Constants::NahdXml::xmlKeySynthMultiShape().toStdString(), 0.5f, 0, 100, 50 });
    addParameter(Parameter { Constants::NahdXml::xmlKeySynthMultiLevel().toStdString(), 0.0f, 0, 100, 0 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyMultiKeyTrack().toStdString(), 0.0f, 0, 100, 0 });

    addParameter(Parameter { Constants::NahdXml::xmlKeySynthMixLevel1().toStdString(), 1.0f, 0, 100, 100 });
    addParameter(Parameter { Constants::NahdXml::xmlKeySynthMixLevel2().toStdString(), 0.0f, 0, 100, 0 });
    addParameter(Parameter { Constants::NahdXml::xmlKeySynthMixLevel3().toStdString(), 0.0f, 0, 100, 0 });

    addParameter(Parameter { Constants::NahdXml::xmlKeySynthLpfCutoff().toStdString(), 1.0f, 0, 100, 100 });
    addParameter(Parameter { Constants::NahdXml::xmlKeySynthLpfResonance().toStdString(), 0.0f, 0, 100, 0 });
    addParameter(Parameter { Constants::NahdXml::xmlKeySynthHpfCutoff().toStdString(), 0.0f, 0, 100, 0 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyKeyTrack().toStdString(), 0.0f, 0, 100, 0 });

    addParameter(Parameter { Constants::NahdXml::xmlKeySynthAmpAttack().toStdString(), 0.5f, 0, 100, 50 });
    addParameter(Parameter { Constants::NahdXml::xmlKeySynthAmpDecay().toStdString(), 0.34f, 0, 100, 34 });
    addParameter(Parameter { Constants::NahdXml::xmlKeySynthAmpSustain().toStdString(), 1.0f, 0, 100, 100 });
    addParameter(Parameter { Constants::NahdXml::xmlKeySynthAmpRelease().toStdString(), 0.48f, 0, 100, 48 });

    addParameter(Parameter { Constants::NahdXml::xmlKeySynthModAttack().toStdString(), 0.5f, 0, 100, 50 });
    addParameter(Parameter { Constants::NahdXml::xmlKeySynthModDecay().toStdString(), 0.34f, 0, 100, 34 });
    addParameter(Parameter { Constants::NahdXml::xmlKeySynthModIntensity().toStdString(), 0.5f, -100, 100, 0 });
    addParameter(Parameter { Constants::NahdXml::xmlKeySynthModTarget().toStdString(), 3.0f, 0, 3, 3, 1, Parameter::Type::Discrete }); // Cutoff default

    addParameter(Parameter { Constants::NahdXml::xmlKeySynthLfoWaveform().toStdString(), 1.0f, 0, 3, 1, 1, Parameter::Type::Discrete }); // Tri default
    addParameter(Parameter { Constants::NahdXml::xmlKeySynthLfoMode().toStdString(), 0.0f, 0, 2, 0, 1, Parameter::Type::Discrete }); // Normal default
    addParameter(Parameter { Constants::NahdXml::xmlKeySynthLfoRate().toStdString(), 0.5f, 0, 100, 50 });
    addParameter(Parameter { Constants::NahdXml::xmlKeySynthLfoIntensity().toStdString(), 0.5f, -100, 100, 0 });
    addParameter(Parameter { Constants::NahdXml::xmlKeySynthLfoTarget().toStdString(), 0.0f, 0, 2, 0, 1, Parameter::Type::Discrete }); // Pitch default

    addParameter(Parameter { Constants::NahdXml::xmlKeyVoiceMode().toStdString(), 0.0f, 0, 1, 0, 1, Parameter::Type::Discrete });
    addParameter(Parameter { Constants::NahdXml::xmlKeyVoiceDepth().toStdString(), 0.0f, 0, 100, 0 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyPortamento().toStdString(), 0.0f, 0, 100, 0 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyPanSpread().toStdString(), 0.0f, 0, 100, 0 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyPitchBendRange().toStdString(), 2.0f, 0, 24, 2, 1, Parameter::Type::Discrete });

    addParameter(Parameter { Constants::NahdXml::xmlKeyDelayType().toStdString(), 0.0f, 0, 2, 0, 1, Parameter::Type::Discrete });

    addParameter(Parameter { Constants::NahdXml::xmlKeyDelayTime().toStdString(), 0.5f, 0, 10000, 500 }); // 0..10 seconds in ms
    addParameter(Parameter { Constants::NahdXml::xmlKeyDelayFeedback().toStdString(), 0.3f, 0, 100, 30 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyDelayDepth().toStdString(), 0.5f, 0, 100, 50 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyDelayMix().toStdString(), 0.0f, 0, 100, 0 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyDelaySync().toStdString(), 0.0f, 0, 1, 0, 1, Parameter::Type::Boolean });
    addParameter(Parameter { Constants::NahdXml::xmlKeyDelaySyncDivision().toStdString(), 0.25f, 0, 100, 25 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyDelayFeedbackLpf().toStdString(), 1.0f, 0, 100, 100 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyDelayFeedbackHpf().toStdString(), 0.0f, 0, 100, 0 });

    for (auto && voice : m_voices) {
        voice.lpf.setMode(CascadedSvf::Mode::LowPass);
        voice.hpf.setMode(CascadedSvf::Mode::HighPass);
    }

    setManualPan(panInternal());
    setManualVolume(volumeInternal());
    setManualGain(gainInternal());
    m_manualLpfCutoff = m_lpfCutoff;
    m_manualHpfCutoff = m_hpfCutoff;

    SynthDevice::syncParameters();
}

SynthDevice::~SynthDevice() = default;

std::string SynthDevice::name() const
{
    return m_name;
}

std::string SynthDevice::category() const
{
    return Constants::NahdXml::xmlValueSynths().toStdString();
}

std::string SynthDevice::typeName() const
{
    return Constants::synthDeviceName().toStdString();
}

std::string SynthDevice::typeId() const
{
    return typeIdString();
}

std::vector<MidiCcController> SynthDevice::availableMidiCcControllers() const
{
    using namespace MidiCcMapping;
    return {
        { static_cast<uint8_t>(Controller::ChannelVolumeMSB), "Volume" },
        { static_cast<uint8_t>(Controller::PanMSB), "Pan" },
        { static_cast<uint8_t>(Controller::SoundController5), "LPF" },
        { static_cast<uint8_t>(Controller::GeneralPurpose6), "HPF" }
    };
}

void SynthDevice::processAudio(AudioContext & context)
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };

    prepareForProcessing(context);

    const uint32_t oversampledRate = context.sampleRate * 2;

    const double portamentoTime = ParameterMapper::mapExponential(m_portamento, 0.01, 2.0);
    const double portamentoCoeff = m_portamento > 0 ? 1.0 - std::pow(0.001, 1.0 / (portamentoTime * oversampledRate)) : 1.0;
    const double pbOffset = (static_cast<double>(m_pitchBend) - 8192.0) / 8192.0 * m_pitchBendRange;
    const double pbRatio = std::exp2(pbOffset / 12.0);

    for (auto && voice : m_voices) {
        if (voice.active) {
            renderVoice(voice, context, oversampledRate, portamentoCoeff, pbRatio);
        }
    }

    applyGlobalEffects(context);
}

void SynthDevice::prepareForProcessing(AudioContext & context)
{
    setSampleRate(context.sampleRate);
    m_delay.setSampleRate(static_cast<double>(context.sampleRate));

    const size_t requiredSize = static_cast<size_t>(context.frameCount) * 4;
    if (m_oversampledBuffer.size() < requiredSize) {
        m_oversampledBuffer.resize(requiredSize);
    }
    std::fill(m_oversampledBuffer.begin(), m_oversampledBuffer.begin() + requiredSize, 0.0f);
}

void SynthDevice::renderVoice(Voice & voice, AudioContext & context, uint32_t oversampledRate, double portamentoCoeff, double pbRatio)
{
    updateVoiceParameters(voice, oversampledRate);

    const float gain = (1.0f / static_cast<float>(MaxVoices)) * linearGainInternal() * voice.velocity;
    const float panL = (1.0f - voice.pan) * (1.0f - panInternal()) * 2.0f;
    const float panR = voice.pan * panInternal() * 2.0f;

    for (uint32_t i = 0; i < context.frameCount; i++) {
        for (int os = 0; os < 2; os++) {
            voice.glideFrequency += (voice.frequency - voice.glideFrequency) * portamentoCoeff;

            const ModulationValues mods = calculateModulation(voice);
            const float finalHighRateSample = generateVoiceSample(voice, mods, oversampledRate, pbRatio) * gain;

            m_oversampledBuffer[(i * 2 + os) * 2] += finalHighRateSample * panL;
            m_oversampledBuffer[(i * 2 + os) * 2 + 1] += finalHighRateSample * panR;
        }
    }

    if (voice.ampEg.state() == AdsrEnvelope::State::Idle) {
        voice.active = false;
    }
}

void SynthDevice::updateVoiceParameters(Voice & voice, uint32_t oversampledRate)
{
    voice.vco1.setSampleRate(oversampledRate);
    voice.vco2.setSampleRate(oversampledRate);
    voice.vco3.setSampleRate(oversampledRate);
    voice.multi.setSampleRate(oversampledRate);
    voice.lpf.setSampleRate(oversampledRate);
    voice.hpf.setSampleRate(oversampledRate);
    voice.ampEg.setSampleRate(oversampledRate);
    voice.modEg.setSampleRate(oversampledRate);
    voice.lfo.setSampleRate(oversampledRate);

    voice.lpf.setResonance(m_lpfResonance);
    voice.vco1.setWaveform(m_vco1Waveform);
    voice.vco2.setWaveform(m_vco2Waveform);
    voice.vco3.setWaveform(m_vco3Waveform);
    voice.vco1.setShape(m_vco1Shape);
    voice.vco2.setShape(m_vco2Shape);
    voice.vco3.setShape(m_vco3Shape);
    voice.multi.setType(m_multiType);
    voice.multi.setShape(m_multiShape);
    voice.multi.setKeyTrack(m_multiKeyTrack);
    voice.multi.setNote(voice.note);
}

void SynthDevice::applyGlobalEffects(AudioContext & context)
{
    for (uint32_t i = 0; i < context.frameCount; i++) {
        const float l0 = m_oversampledBuffer[i * 4];
        const float r0 = m_oversampledBuffer[i * 4 + 1];
        const float l1 = m_oversampledBuffer[i * 4 + 2];
        const float r1 = m_oversampledBuffer[i * 4 + 3];

        double l = static_cast<double>(m_oversamplerL.process(l0, l1));
        double r = static_cast<double>(m_oversamplerR.process(r0, r1));

        m_delay.process(l, r);

        context.buffer[i * 2] += l * volumeInternal();
        context.buffer[i * 2 + 1] += r * volumeInternal();
    }
}

bool SynthDevice::hasActiveAudio() const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    return std::ranges::any_of(m_voices, [](const auto & voice) { return voice.active; });
}

void SynthDevice::processMidiNoteOn(uint8_t note, uint8_t velocity)
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    handleNoteOn(note, velocity);
}

void SynthDevice::processMidiNoteOff(uint8_t note)
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    handleNoteOff(note);
}

void SynthDevice::processMidiCc(uint8_t controller, uint8_t value, uint8_t)
{
    using namespace MidiCcMapping;

    bool changed = false;
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        if (controller == static_cast<uint8_t>(Controller::ResetAllControllers)) {
            m_lpfCutoff = m_manualLpfCutoff;
            m_hpfCutoff = m_manualHpfCutoff;
            if (const auto p = parameter(Constants::NahdXml::xmlKeySynthLpfCutoff().toStdString()); p) {
                p->get().setValue(m_lpfCutoff);
            }
            if (const auto p = parameter(Constants::NahdXml::xmlKeySynthHpfCutoff().toStdString()); p) {
                p->get().setValue(m_hpfCutoff);
            }
            updatePanParameter(manualPanInternal(), false);
            updateVolumeParameter(manualVolumeInternal(), false);
            updateGainParameter(manualGainInternal(), false);
            changed = true;
        } else if (controller == static_cast<uint8_t>(Controller::BankSelectMSB)) {
            m_currentBank = std::clamp(static_cast<int>(value), 0, 1); // 0: Factory, 1: User
        } else {
            const float val = static_cast<float>(value) / 127.0f;
            if (controller == static_cast<uint8_t>(Controller::ChannelVolumeMSB)) {
                changed |= updateVolumeParameter(val, false);
            } else if (controller == static_cast<uint8_t>(Controller::PanMSB)) {
                changed |= updatePanParameter(val, false);
            } else if (controller == static_cast<uint8_t>(Controller::SoundController5)) { // Cutoff
                m_lpfCutoff = val;
                if (const auto p = parameter(Constants::NahdXml::xmlKeySynthLpfCutoff().toStdString()); p) {
                    p->get().setValue(val);
                    syncParameters();
                    changed = true;
                }
            } else if (controller == static_cast<uint8_t>(Controller::GeneralPurpose6)) { // HPF Cutoff
                m_hpfCutoff = val;
                if (const auto p = parameter(Constants::NahdXml::xmlKeySynthHpfCutoff().toStdString()); p) {
                    p->get().setValue(val);
                    syncParameters();
                    changed = true;
                }
            }
        }
    }

    if (changed) {
        emit dataChanged();
    }
}

void SynthDevice::processMidiAllNotesOff()
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    for (auto && voice : m_voices) {
        voice.reset();
    }
    m_delay.reset();
}

void SynthDevice::setBpm(float bpm)
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    m_delay.setBpm(static_cast<double>(bpm));
}

void SynthDevice::reset()
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    Device::reset();
    resetAudio();
    syncParameters();
}

void SynthDevice::resetAudio()
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    for (auto && voice : m_voices) {
        voice.reset();
    }
    m_delay.reset();
    m_oversamplerL.reset();
    m_oversamplerR.reset();
    m_polyNextVoice = 0;
}

double SynthDevice::voiceGlideFrequency(size_t index) const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    return index < m_voices.size() ? m_voices.at(index).glideFrequency : 0.0;
}

void SynthDevice::handleNoteOn(uint8_t note, uint8_t velocity)
{
    // First release any existing voices for this note to avoid multiple voices for same note in poly
    for (auto && voice : m_voices) {
        if (voice.active && voice.note == note) {
            voice.release();
        }
    }

    const double freq = midiNoteToFreq(note);
    const float vel = static_cast<float>(velocity) / 127.0f;

    if (m_voiceMode == VoiceMode::Poly) {
        std::optional<size_t> bestVoice;

        // 1. Try to find a voice already playing or releasing this note (Affinity)
        for (size_t i = 0; i < MaxVoices; i++) {
            if (m_voices.at(i).active && m_voices.at(i).note == note) {
                bestVoice = i;
                break;
            }
        }

        // 2. Try to find an idle voice (Round-Robin)
        if (!bestVoice) {
            for (size_t i = 0; i < MaxVoices; i++) {
                const size_t voiceIndex = (m_polyNextVoice + i) % MaxVoices;
                if (!m_voices.at(voiceIndex).active) {
                    bestVoice = voiceIndex;
                    m_polyNextVoice = (voiceIndex + 1) % MaxVoices;
                    break;
                }
            }
        }

        // 3. Steal the quietest voice (Round-Robin search for stealing candidate)
        if (!bestVoice) {
            float lowestAmp = 2.0f;
            for (size_t i = 0; i < MaxVoices; i++) {
                size_t voiceIndex = (m_polyNextVoice + i) % MaxVoices;
                const float currentAmp = static_cast<float>(m_voices.at(voiceIndex).ampEg.value());
                if (currentAmp < lowestAmp) {
                    lowestAmp = currentAmp;
                    bestVoice = voiceIndex;
                }
            }
            if (bestVoice) {
                m_polyNextVoice = (bestVoice.value() + 1) % MaxVoices;
            }
        }

        if (bestVoice) {
            // Reset glide if portamento is off
            if (m_portamento <= 0.001f) {
                m_voices.at(bestVoice.value()).glideFrequency = freq;
            }

            // Balanced Pan Spread (Voice-alternating distribution inspired by Behringer DeepMind)
            const float side = (bestVoice.value() % 2 == 0) ? -1.0f : 1.0f;
            const float depth = 1.0f - static_cast<float>(bestVoice.value() / 2) * (2.0f / static_cast<float>(MaxVoices));
            const float pan = 0.5f + (side * depth * m_panSpread * 0.5f);

            if (m_vco1Sync) {
                m_voices.at(bestVoice.value()).trigger(note, freq, pan, vel, true);
            } else {
                m_voices.at(bestVoice.value()).triggerRandomized(note, freq, pan, vel, m_phaseDist(m_rng));
            }
        }
    } else {
        // Unison
        for (size_t i = 0; i < MaxVoices; i++) {
            // Non-linear detune spread for better texture
            const double detuneAmount = (static_cast<double>(i) - (MaxVoices - 1) / 2.0) * std::pow(m_voiceDepth, 1.5) * 0.2;
            const double voiceFreq = freq * std::pow(2.0, detuneAmount / 12.0);

            if (m_portamento <= 0.001f) {
                m_voices.at(i).glideFrequency = voiceFreq;
            }

            const float side = (i % 2 == 0) ? -1.0f : 1.0f;
            const float depth = 1.0f - static_cast<float>(i / 2) * (2.0f / static_cast<float>(MaxVoices));
            const float pan = 0.5f + (side * depth * m_panSpread * 0.5f);

            if (m_vco1Sync) {
                m_voices.at(i).trigger(note, voiceFreq, pan, vel, true);
            } else {
                m_voices.at(i).triggerRandomized(note, voiceFreq, pan, vel, m_phaseDist(m_rng));
            }
        }
    }
}

void SynthDevice::handleNoteOff(uint8_t note)
{
    for (auto && voice : m_voices) {
        if (voice.active && voice.note == note) {
            voice.release();
        }
    }
}

double SynthDevice::midiNoteToFreq(uint8_t note) const
{
    return 440.0 * std::pow(2.0, (note - 69) / 12.0);
}

SynthDevice::ModulationValues SynthDevice::calculateModulation(Voice & voice) const
{
    ModulationValues mods;
    mods.ampEnvelope = voice.ampEg.nextSample();
    const double modEnv = voice.modEg.nextSample() * m_modInt;
    const double lfoVal = voice.lfo.nextSample() * m_lfoInt;

    mods.cutoffMod = (m_modTarget == ModTarget::Cutoff) ? modEnv : 0.0;
    if (m_lfoTarget == LfoTarget::Cutoff) {
        mods.cutoffMod += lfoVal;
    }

    mods.shapeMod = (m_lfoTarget == LfoTarget::Shape) ? lfoVal : 0.0;

    mods.vco1PitchMod = (m_modTarget == ModTarget::Pitch1) ? modEnv : 0.0;
    mods.vco2PitchMod = (m_modTarget == ModTarget::Pitch2) ? modEnv : 0.0;
    mods.vco3PitchMod = (m_modTarget == ModTarget::Pitch3) ? modEnv : 0.0;

    if (m_lfoTarget == LfoTarget::Pitch) {
        mods.vco1PitchMod += lfoVal;
        mods.vco2PitchMod += lfoVal;
        mods.vco3PitchMod += lfoVal;
    }

    return mods;
}

float SynthDevice::generateVoiceSample(Voice & voice, const ModulationValues & mods, double oversampledRate, double pbRatio)
{
    double vco1Freq = voice.glideFrequency * m_vco1BasePitchRatio * pbRatio;
    double vco2Freq = voice.glideFrequency * m_vco2BasePitchRatio * pbRatio;
    double vco3Freq = voice.glideFrequency * m_vco3BasePitchRatio * pbRatio;

    if (mods.vco1PitchMod != 0.0) {
        vco1Freq *= std::exp2(mods.vco1PitchMod);
    }
    if (mods.vco2PitchMod != 0.0) {
        vco2Freq *= std::exp2(mods.vco2PitchMod);
    }
    if (mods.vco3PitchMod != 0.0) {
        vco3Freq *= std::exp2(mods.vco3PitchMod);
    }

    double vco1Val = 0.0;
    double oldPhase1 = voice.vco1.phase();
    if (m_mixVco1 >= 0.001f) {
        voice.vco1.setFrequency(vco1Freq);
        voice.vco1.setShape(std::clamp(m_vco1Shape + mods.shapeMod, 0.0, 1.0));
        oldPhase1 = voice.vco1.phase();
        vco1Val = voice.vco1.nextSample();
    }

    double vco2Val = 0.0;
    double oldPhase2 = voice.vco2.phase();
    if (m_mixVco2 >= 0.001f) {
        voice.vco2.setFrequency(vco2Freq);
        voice.vco2.setShape(std::clamp(m_vco2Shape + mods.shapeMod, 0.0, 1.0));

        if (m_vco2Sync && voice.vco1.phase() < oldPhase1) {
            // Calculate fractional phase for VCO2 to maintain sync accuracy
            const double phaseStep1 = vco1Freq / oversampledRate;
            const double phaseStep2 = vco2Freq / oversampledRate;
            if (phaseStep1 > 0.0) {
                const double fraction = voice.vco1.phase() / phaseStep1;
                voice.vco2.sync(fraction * phaseStep2);
            } else {
                voice.vco2.sync(0.0);
            }
        }
        oldPhase2 = voice.vco2.phase();
        vco2Val = voice.vco2.nextSample();
    }

    double vco3Val = 0.0;
    if (m_mixVco3 >= 0.001f) {
        voice.vco3.setFrequency(vco3Freq);
        voice.vco3.setShape(std::clamp(m_vco3Shape + mods.shapeMod, 0.0, 1.0));

        // VCO3 Hard Sync to VCO2
        if (m_vco3Sync && voice.vco2.phase() < oldPhase2) {
            const double phaseStep2 = vco2Freq / oversampledRate;
            const double phaseStep3 = vco3Freq / oversampledRate;
            if (phaseStep2 > 0.0) {
                const double fraction = voice.vco2.phase() / phaseStep2;
                voice.vco3.sync(fraction * phaseStep3);
            } else {
                voice.vco3.sync(0.0);
            }
        }
        vco3Val = voice.vco3.nextSample();
    }

    double multiVal = 0.0;
    if (m_multiLevel >= 0.001f) {
        multiVal = voice.multi.nextSample();
    }

    const double mix = (vco1Val * m_mixVco1) + (vco2Val * m_mixVco2) + (vco3Val * m_mixVco3) + (multiVal * m_multiLevel);
    const double mixHeadroom = mix * 0.4; // Slightly more headroom for 3rd VCO + Multi engine

    // Filter
    const double cutoffMod = mods.cutoffMod + (voice.note - 60.0) / 127.0 * m_filterKeyTrack;

    voice.lpf.setCutoff(std::clamp(m_lpfCutoff + cutoffMod, 0.0, 1.0));
    voice.hpf.setCutoff(m_hpfCutoff);

    const float filtered = voice.hpf.process(voice.lpf.process(static_cast<float>(mixHeadroom)));
    return filtered * static_cast<float>(mods.ampEnvelope);
}

void SynthDevice::syncParameters()
{
    Device::syncParameters();
    if (const auto p = parameter(Constants::NahdXml::xmlKeySynthVco1Waveform().toStdString()); p)
        m_vco1Waveform = static_cast<PolyBlepOscillator::Waveform>(p->get().xmlValue());
    if (const auto p = parameter(Constants::NahdXml::xmlKeySynthVco1Octave().toStdString()); p)
        m_vco1Octave = p->get().xmlValue();
    if (const auto p = parameter(Constants::NahdXml::xmlKeySynthVco1Pitch().toStdString()); p)
        m_vco1Pitch = p->get().value();
    if (const auto p = parameter(Constants::NahdXml::xmlKeySynthVco1Shape().toStdString()); p)
        m_vco1Shape = p->get().value();
    if (const auto p = parameter(Constants::NahdXml::xmlKeySynthVco1Sync().toStdString()); p)
        m_vco1Sync = p->get().value() > 0.5f;

    const double vco1PitchOffset = ParameterMapper::mapCubicCentered(m_vco1Pitch * 2.0 - 1.0, -2400, 2400);
    m_vco1BasePitchRatio = std::pow(2.0, (m_vco1Octave * 12.0 + vco1PitchOffset / 100.0) / 12.0);

    if (const auto p = parameter(Constants::NahdXml::xmlKeySynthVco2Waveform().toStdString()); p)
        m_vco2Waveform = static_cast<PolyBlepOscillator::Waveform>(p->get().xmlValue());
    if (const auto p = parameter(Constants::NahdXml::xmlKeySynthVco2Octave().toStdString()); p)
        m_vco2Octave = p->get().xmlValue();
    if (const auto p = parameter(Constants::NahdXml::xmlKeySynthVco2Pitch().toStdString()); p)
        m_vco2Pitch = p->get().value();
    if (const auto p = parameter(Constants::NahdXml::xmlKeySynthVco2Shape().toStdString()); p)
        m_vco2Shape = p->get().value();
    if (const auto p = parameter(Constants::NahdXml::xmlKeySynthVco2Sync().toStdString()); p)
        m_vco2Sync = p->get().value() > 0.5f;

    const double vco2PitchOffset = ParameterMapper::mapCubicCentered(m_vco2Pitch * 2.0 - 1.0, -2400, 2400);
    m_vco2BasePitchRatio = std::pow(2.0, (m_vco2Octave * 12.0 + vco2PitchOffset / 100.0) / 12.0);

    if (const auto p = parameter(Constants::NahdXml::xmlKeySynthVco3Waveform().toStdString()); p)
        m_vco3Waveform = static_cast<PolyBlepOscillator::Waveform>(p->get().xmlValue());
    if (const auto p = parameter(Constants::NahdXml::xmlKeySynthVco3Octave().toStdString()); p)
        m_vco3Octave = p->get().xmlValue();
    if (const auto p = parameter(Constants::NahdXml::xmlKeySynthVco3Pitch().toStdString()); p)
        m_vco3Pitch = p->get().value();
    if (const auto p = parameter(Constants::NahdXml::xmlKeySynthVco3Shape().toStdString()); p)
        m_vco3Shape = p->get().value();
    if (const auto p = parameter(Constants::NahdXml::xmlKeySynthVco3Sync().toStdString()); p)
        m_vco3Sync = p->get().value() > 0.5f;

    const double vco3PitchOffset = ParameterMapper::mapCubicCentered(m_vco3Pitch * 2.0 - 1.0, -2400, 2400);
    m_vco3BasePitchRatio = std::pow(2.0, (m_vco3Octave * 12.0 + vco3PitchOffset / 100.0) / 12.0);

    if (const auto p = parameter(Constants::NahdXml::xmlKeySynthMultiMode().toStdString()); p)
        m_multiType = static_cast<MultiEngine::Type>(p->get().xmlValue());
    if (const auto p = parameter(Constants::NahdXml::xmlKeySynthMultiShape().toStdString()); p)
        m_multiShape = p->get().value();
    if (const auto p = parameter(Constants::NahdXml::xmlKeySynthMultiLevel().toStdString()); p)
        m_multiLevel = p->get().value();
    if (const auto p = parameter(Constants::NahdXml::xmlKeyMultiKeyTrack().toStdString()); p)
        m_multiKeyTrack = p->get().value();

    if (const auto p = parameter(Constants::NahdXml::xmlKeySynthMixLevel1().toStdString()); p)
        m_mixVco1 = p->get().value();
    if (const auto p = parameter(Constants::NahdXml::xmlKeySynthMixLevel2().toStdString()); p)
        m_mixVco2 = p->get().value();
    if (const auto p = parameter(Constants::NahdXml::xmlKeySynthMixLevel3().toStdString()); p)
        m_mixVco3 = p->get().value();

    if (const auto p = parameter(Constants::NahdXml::xmlKeySynthLpfCutoff().toStdString()); p)
        m_lpfCutoff = p->get().value();
    if (const auto p = parameter(Constants::NahdXml::xmlKeySynthLpfResonance().toStdString()); p)
        m_lpfResonance = p->get().value();
    if (const auto p = parameter(Constants::NahdXml::xmlKeySynthHpfCutoff().toStdString()); p)
        m_hpfCutoff = p->get().value();
    if (const auto p = parameter(Constants::NahdXml::xmlKeyKeyTrack().toStdString()); p)
        m_filterKeyTrack = p->get().value();

    if (const auto p = parameter(Constants::NahdXml::xmlKeySynthAmpAttack().toStdString()); p)
        m_ampAttack = p->get().value();
    if (const auto p = parameter(Constants::NahdXml::xmlKeySynthAmpDecay().toStdString()); p)
        m_ampDecay = p->get().value();
    if (const auto p = parameter(Constants::NahdXml::xmlKeySynthAmpSustain().toStdString()); p)
        m_ampSustain = p->get().value();
    if (const auto p = parameter(Constants::NahdXml::xmlKeySynthAmpRelease().toStdString()); p)
        m_ampRelease = p->get().value();

    if (const auto p = parameter(Constants::NahdXml::xmlKeySynthModAttack().toStdString()); p)
        m_modAttack = p->get().value();
    if (const auto p = parameter(Constants::NahdXml::xmlKeySynthModDecay().toStdString()); p)
        m_modDecay = p->get().value();

    if (const auto p = parameter(Constants::NahdXml::xmlKeySynthModIntensity().toStdString()); p)
        m_modInt = (p->get().value() - 0.5f) * 2.0f;
    if (const auto p = parameter(Constants::NahdXml::xmlKeySynthModTarget().toStdString()); p)
        m_modTarget = static_cast<ModTarget>(p->get().xmlValue());

    if (const auto p = parameter(Constants::NahdXml::xmlKeySynthLfoWaveform().toStdString()); p)
        m_lfoWaveform = static_cast<Lfo::Waveform>(p->get().xmlValue());
    if (const auto p = parameter(Constants::NahdXml::xmlKeySynthLfoMode().toStdString()); p)
        m_lfoMode = static_cast<Lfo::Mode>(p->get().xmlValue());
    if (const auto p = parameter(Constants::NahdXml::xmlKeySynthLfoRate().toStdString()); p)
        m_lfoRate = p->get().value();
    if (const auto p = parameter(Constants::NahdXml::xmlKeySynthLfoIntensity().toStdString()); p)
        m_lfoInt = (p->get().value() - 0.5f) * 2.0f;
    if (const auto p = parameter(Constants::NahdXml::xmlKeySynthLfoTarget().toStdString()); p)
        m_lfoTarget = static_cast<LfoTarget>(p->get().xmlValue());

    if (const auto p = parameter(Constants::NahdXml::xmlKeyVoiceMode().toStdString()); p)
        m_voiceMode = static_cast<VoiceMode>(p->get().xmlValue());
    if (const auto p = parameter(Constants::NahdXml::xmlKeyVoiceDepth().toStdString()); p)
        m_voiceDepth = p->get().value();
    if (const auto p = parameter(Constants::NahdXml::xmlKeyPortamento().toStdString()); p)
        m_portamento = p->get().value();
    if (const auto p = parameter(Constants::NahdXml::xmlKeyPanSpread().toStdString()); p)
        m_panSpread = p->get().value();
    if (const auto p = parameter(Constants::NahdXml::xmlKeyPitchBendRange().toStdString()); p)
        m_pitchBendRange = static_cast<int>(p->get().xmlValue());

    if (const auto p = parameter(Constants::NahdXml::xmlKeyDelayType().toStdString()); p)
        m_delayType = static_cast<DelayEffect::Type>(p->get().xmlValue());
    if (const auto p = parameter(Constants::NahdXml::xmlKeyDelayTime().toStdString()); p)
        m_delayTime = p->get().value() * 10.0f;
    if (const auto p = parameter(Constants::NahdXml::xmlKeyDelayFeedback().toStdString()); p)
        m_delayFeedback = p->get().value();
    if (const auto p = parameter(Constants::NahdXml::xmlKeyDelayDepth().toStdString()); p)
        m_delayDepth = p->get().value();
    if (const auto p = parameter(Constants::NahdXml::xmlKeyDelayMix().toStdString()); p)
        m_delayMix = p->get().value();
    if (const auto p = parameter(Constants::NahdXml::xmlKeyDelaySync().toStdString()); p)
        m_delaySync = p->get().value() > 0.5f;
    if (const auto p = parameter(Constants::NahdXml::xmlKeyDelaySyncDivision().toStdString()); p)
        m_delaySyncDivision = p->get().value();
    if (const auto p = parameter(Constants::NahdXml::xmlKeyDelayFeedbackLpf().toStdString()); p)
        m_delay.setFeedbackLpf(p->get().value());
    if (const auto p = parameter(Constants::NahdXml::xmlKeyDelayFeedbackHpf().toStdString()); p)
        m_delay.setFeedbackHpf(p->get().value());

    m_delay.setType(m_delayType);
    m_delay.setTime(m_delayTime);
    m_delay.setFeedback(m_delayFeedback);
    m_delay.setDepth(m_delayDepth);
    m_delay.setMix(m_delayMix);
    m_delay.setSync(m_delaySync);
    m_delay.setSyncDivision(m_delaySyncDivision);

    for (auto && voice : m_voices) {
        voice.vco1.setWaveform(m_vco1Waveform);
        voice.vco2.setWaveform(m_vco2Waveform);
        voice.vco3.setWaveform(m_vco3Waveform);
        voice.vco1.setShape(m_vco1Shape);
        voice.vco2.setShape(m_vco2Shape);
        voice.vco3.setShape(m_vco3Shape);

        voice.lfo.setWaveform(m_lfoWaveform);
        voice.lfo.setMode(m_lfoMode);

        double freq = 0.0;
        if (m_lfoMode == Lfo::Mode::BPM) {
            freq = (m_delay.bpm() / 60.0f) * (0.25f / std::max(0.0001f, m_lfoRate));
        } else {
            freq = static_cast<float>(ParameterMapper::mapLfoFrequency(m_lfoRate, 0.05, 20.0));
        }
        voice.lfo.setFrequency(freq);

        voice.ampEg.setAttackTime(ParameterMapper::mapExponential(m_ampAttack, 0.000001, 20.0));
        voice.ampEg.setDecayTime(ParameterMapper::mapExponential(m_ampDecay, 0.01, 60.0));
        voice.ampEg.setSustainLevel(m_ampSustain);
        voice.ampEg.setReleaseTime(ParameterMapper::mapExponential(m_ampRelease, 0.001, 60.0));
        voice.modEg.setAttackTime(ParameterMapper::mapExponential(m_modAttack, 0.000001, 20.0));
        voice.modEg.setDecayTime(ParameterMapper::mapExponential(m_modDecay, 0.01, 60.0));
        voice.modEg.setSustainLevel(0.0); // Mod EG is AD only
        voice.modEg.setReleaseTime(ParameterMapper::mapExponential(m_modDecay, 0.01, 60.0));
    }
}

void SynthDevice::serializeToXml(QXmlStreamWriter & writer) const
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

void SynthDevice::deserializeFromXml(QXmlStreamReader & reader)
{
    {
        const std::lock_guard<std::recursive_mutex> lock { mutex() };

        deserializeAttributesFromXml(reader);

        while (!reader.atEnd() && !reader.hasError()) {
            const auto token = reader.readNext();
            if (token == QXmlStreamReader::EndElement && reader.name() == Constants::NahdXml::xmlKeyDevice()) {
                break;
            }

            if (token == QXmlStreamReader::StartElement) {
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

        // Update manual fallback values for MIDI CC reset
        setManualPan(panInternal());
        setManualVolume(volumeInternal());
        setManualGain(gainInternal());
        m_manualLpfCutoff = m_lpfCutoff;
        m_manualHpfCutoff = m_hpfCutoff;
    }
    emit dataChanged();
}

void SynthDevice::processMidiPitchBend(uint16_t value, uint8_t)
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    m_pitchBend = value;
}

void SynthDevice::processMidiProgramChange(uint8_t program, uint8_t)
{
    loadPreset(m_currentBank, program);
}

void SynthDevice::loadPreset(int bank, int index)
{
    {
        const std::lock_guard<std::recursive_mutex> lock { mutex() };

        if (bank == 0) {
            const auto & presets = SynthPresets::presets();
            if (index < 0 || index >= static_cast<int>(presets.size())) {
                return;
            }
            reset();
            for (auto && [name, val] : presets[index].parameters) {
                if (const auto p = parameter(name); p) {
                    p->get().setValue(val);
                }
            }
        } else if (bank == 1) {
            if (m_userPresets.find(index) == m_userPresets.end()) {
                return;
            }
            const auto & preset = m_userPresets.at(index);
            reset();
            for (auto && [name, val] : preset.parameters) {
                if (const auto p = parameter(name); p) {
                    p->get().setValue(val);
                }
            }
        } else {
            return;
        }

        syncParameters();

        // Update manual fallback values for MIDI CC reset to match the new preset
        setManualPan(panInternal());
        setManualVolume(volumeInternal());
        setManualGain(gainInternal());
        m_manualLpfCutoff = m_lpfCutoff;
        m_manualHpfCutoff = m_hpfCutoff;
    }

    emit dataChanged();
}

void SynthDevice::setUserPresets(const UserPresets & presets)
{
    {
        const std::lock_guard<std::recursive_mutex> lock(mutex());
        m_userPresets = presets;
    }
}

// Accessors (VCO1)
PolyBlepOscillator::Waveform SynthDevice::vco1Waveform() const
{
    return m_vco1Waveform;
}

void SynthDevice::setVco1Waveform(PolyBlepOscillator::Waveform wave)
{
    bool changed = false;
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        if (const auto p = parameter(Constants::NahdXml::xmlKeySynthVco1Waveform().toStdString()); p) {
            p->get().setFromXml(static_cast<int>(wave));
            syncParameters();
            changed = true;
        }
    }
    if (changed)
        emit dataChanged();
}

int SynthDevice::vco1Octave() const
{
    return m_vco1Octave;
}

void SynthDevice::setVco1Octave(int octave)
{
    bool changed = false;
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        if (const auto p = parameter(Constants::NahdXml::xmlKeySynthVco1Octave().toStdString()); p) {
            p->get().setFromXml(octave);
            syncParameters();
            changed = true;
        }
    }
    if (changed)
        emit dataChanged();
}

float SynthDevice::vco1Pitch() const
{
    return m_vco1Pitch;
}

void SynthDevice::setVco1Pitch(float pitch)
{
    bool changed = false;
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        if (const auto p = parameter(Constants::NahdXml::xmlKeySynthVco1Pitch().toStdString()); p) {
            p->get().setValue(pitch);
            syncParameters();
            changed = true;
        }
    }
    if (changed)
        emit dataChanged();
}

float SynthDevice::vco1Shape() const
{
    return m_vco1Shape;
}

void SynthDevice::setVco1Shape(float shape)
{
    bool changed = false;
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        if (const auto p = parameter(Constants::NahdXml::xmlKeySynthVco1Shape().toStdString()); p) {
            p->get().setValue(shape);
            syncParameters();
            changed = true;
        }
    }
    if (changed)
        emit dataChanged();
}

bool SynthDevice::vco1Sync() const
{
    return m_vco1Sync;
}

void SynthDevice::setVco1Sync(bool sync)
{
    bool changed = false;
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        if (const auto p = parameter(Constants::NahdXml::xmlKeySynthVco1Sync().toStdString()); p) {
            p->get().setValue(sync ? 1.0f : 0.0f);
            syncParameters();
            changed = true;
        }
    }
    if (changed)
        emit dataChanged();
}

// Accessors (VCO2)
PolyBlepOscillator::Waveform SynthDevice::vco2Waveform() const
{
    return m_vco2Waveform;
}

void SynthDevice::setVco2Waveform(PolyBlepOscillator::Waveform wave)
{
    bool changed = false;
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        if (const auto p = parameter(Constants::NahdXml::xmlKeySynthVco2Waveform().toStdString()); p) {
            p->get().setFromXml(static_cast<int>(wave));
            syncParameters();
            changed = true;
        }
    }
    if (changed)
        emit dataChanged();
}

int SynthDevice::vco2Octave() const
{
    return m_vco2Octave;
}

void SynthDevice::setVco2Octave(int octave)
{
    bool changed = false;
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        if (const auto p = parameter(Constants::NahdXml::xmlKeySynthVco2Octave().toStdString()); p) {
            p->get().setFromXml(octave);
            syncParameters();
            changed = true;
        }
    }
    if (changed)
        emit dataChanged();
}

float SynthDevice::vco2Pitch() const
{
    return m_vco2Pitch;
}

void SynthDevice::setVco2Pitch(float pitch)
{
    bool changed = false;
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        if (const auto p = parameter(Constants::NahdXml::xmlKeySynthVco2Pitch().toStdString()); p) {
            p->get().setValue(pitch);
            syncParameters();
            changed = true;
        }
    }
    if (changed)
        emit dataChanged();
}

float SynthDevice::vco2Shape() const
{
    return m_vco2Shape;
}

void SynthDevice::setVco2Shape(float shape)
{
    bool changed = false;
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        if (const auto p = parameter(Constants::NahdXml::xmlKeySynthVco2Shape().toStdString()); p) {
            p->get().setValue(shape);
            syncParameters();
            changed = true;
        }
    }
    if (changed)
        emit dataChanged();
}

bool SynthDevice::vco2Sync() const
{
    return m_vco2Sync;
}

void SynthDevice::setVco2Sync(bool sync)
{
    bool changed = false;
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        if (const auto p = parameter(Constants::NahdXml::xmlKeySynthVco2Sync().toStdString()); p) {
            p->get().setValue(sync ? 1.0f : 0.0f);
            syncParameters();
            changed = true;
        }
    }
    if (changed)
        emit dataChanged();
}

// Multi Engine
MultiEngine::Type SynthDevice::multiType() const
{
    return m_multiType;
}

void SynthDevice::setMultiType(MultiEngine::Type type)
{
    bool changed = false;
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        if (const auto p = parameter(Constants::NahdXml::xmlKeySynthMultiMode().toStdString()); p) {
            p->get().setFromXml(static_cast<int>(type));
            syncParameters();
            changed = true;
        }
    }
    if (changed)
        emit dataChanged();
}

float SynthDevice::multiShape() const
{
    return m_multiShape;
}

void SynthDevice::setMultiShape(float shape)
{
    bool changed = false;
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        if (const auto p = parameter(Constants::NahdXml::xmlKeySynthMultiShape().toStdString()); p) {
            p->get().setValue(shape);
            syncParameters();
            changed = true;
        }
    }
    if (changed)
        emit dataChanged();
}

float SynthDevice::multiLevel() const
{
    return m_multiLevel;
}

void SynthDevice::setMultiLevel(float level)
{
    bool changed = false;
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        if (const auto p = parameter(Constants::NahdXml::xmlKeySynthMultiLevel().toStdString()); p) {
            p->get().setValue(level);
            syncParameters();
            changed = true;
        }
    }
    if (changed)
        emit dataChanged();
}

float SynthDevice::multiKeyTrack() const
{
    return m_multiKeyTrack;
}

void SynthDevice::setMultiKeyTrack(float keyTrack)
{
    bool changed = false;
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        if (const auto p = parameter(Constants::NahdXml::xmlKeyMultiKeyTrack().toStdString()); p) {
            p->get().setValue(keyTrack);
            syncParameters();
            changed = true;
        }
    }
    if (changed)
        emit dataChanged();
}

// Mixer
float SynthDevice::mixVco1() const
{
    return m_mixVco1;
}

void SynthDevice::setMixVco1(float level)
{
    bool changed = false;
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        if (const auto p = parameter(Constants::NahdXml::xmlKeySynthMixLevel1().toStdString()); p) {
            p->get().setValue(level);
            syncParameters();
            changed = true;
        }
    }
    if (changed)
        emit dataChanged();
}

float SynthDevice::mixVco2() const
{
    return m_mixVco2;
}

void SynthDevice::setMixVco2(float level)
{
    bool changed = false;
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        if (const auto p = parameter(Constants::NahdXml::xmlKeySynthMixLevel2().toStdString()); p) {
            p->get().setValue(level);
            syncParameters();
            changed = true;
        }
    }
    if (changed)
        emit dataChanged();
}

// Filter
float SynthDevice::lpfCutoff() const
{
    return m_lpfCutoff;
}

void SynthDevice::setLpfCutoff(float cutoff)
{
    bool changed = false;
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        if (const auto p = parameter(Constants::NahdXml::xmlKeySynthLpfCutoff().toStdString()); p) {
            p->get().setValue(cutoff);
            m_manualLpfCutoff = p->get().value();
            syncParameters();
            changed = true;
        }
    }
    if (changed)
        emit dataChanged();
}

float SynthDevice::lpfResonance() const
{
    return m_lpfResonance;
}

void SynthDevice::setLpfResonance(float resonance)
{
    bool changed = false;
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        if (const auto p = parameter(Constants::NahdXml::xmlKeySynthLpfResonance().toStdString()); p) {
            p->get().setValue(resonance);
            syncParameters();
            changed = true;
        }
    }
    if (changed)
        emit dataChanged();
}

float SynthDevice::hpfCutoff() const
{
    return m_hpfCutoff;
}

void SynthDevice::setHpfCutoff(float cutoff)
{
    bool changed = false;
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        if (const auto p = parameter(Constants::NahdXml::xmlKeySynthHpfCutoff().toStdString()); p) {
            p->get().setValue(cutoff);
            m_manualHpfCutoff = p->get().value();
            syncParameters();
            changed = true;
        }
    }
    if (changed)
        emit dataChanged();
}

float SynthDevice::filterKeyTrack() const
{
    return m_filterKeyTrack;
}

void SynthDevice::setFilterKeyTrack(float track)
{
    bool changed = false;
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        if (const auto p = parameter(Constants::NahdXml::xmlKeyKeyTrack().toStdString()); p) {
            p->get().setValue(track);
            syncParameters();
            changed = true;
        }
    }
    if (changed)
        emit dataChanged();
}

// Amp EG
float SynthDevice::ampAttack() const
{
    return m_ampAttack;
}

void SynthDevice::setAmpAttack(float a)
{
    bool changed = false;
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        if (const auto p = parameter(Constants::NahdXml::xmlKeySynthAmpAttack().toStdString()); p) {
            p->get().setValue(a);
            syncParameters();
            changed = true;
        }
    }
    if (changed)
        emit dataChanged();
}

float SynthDevice::ampDecay() const
{
    return m_ampDecay;
}

void SynthDevice::setAmpDecay(float d)
{
    bool changed = false;
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        if (const auto p = parameter(Constants::NahdXml::xmlKeySynthAmpDecay().toStdString()); p) {
            p->get().setValue(d);
            syncParameters();
            changed = true;
        }
    }
    if (changed)
        emit dataChanged();
}

float SynthDevice::ampSustain() const
{
    return m_ampSustain;
}

void SynthDevice::setAmpSustain(float s)
{
    bool changed = false;
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        if (const auto p = parameter(Constants::NahdXml::xmlKeySynthAmpSustain().toStdString()); p) {
            p->get().setValue(s);
            syncParameters();
            changed = true;
        }
    }
    if (changed)
        emit dataChanged();
}

float SynthDevice::ampRelease() const
{
    return m_ampRelease;
}

void SynthDevice::setAmpRelease(float r)
{
    bool changed = false;
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        if (const auto p = parameter(Constants::NahdXml::xmlKeySynthAmpRelease().toStdString()); p) {
            p->get().setValue(r);
            syncParameters();
            changed = true;
        }
    }
    if (changed)
        emit dataChanged();
}

// Mod EG
float SynthDevice::modAttack() const
{
    return m_modAttack;
}

void SynthDevice::setModAttack(float a)
{
    bool changed = false;
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        if (const auto p = parameter(Constants::NahdXml::xmlKeySynthModAttack().toStdString()); p) {
            p->get().setValue(a);
            syncParameters();
            changed = true;
        }
    }
    if (changed)
        emit dataChanged();
}

float SynthDevice::modDecay() const
{
    return m_modDecay;
}

void SynthDevice::setModDecay(float d)
{
    bool changed = false;
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        if (const auto p = parameter(Constants::NahdXml::xmlKeySynthModDecay().toStdString()); p) {
            p->get().setValue(d);
            syncParameters();
            changed = true;
        }
    }
    if (changed)
        emit dataChanged();
}

float SynthDevice::modInt() const
{
    return m_modInt;
}

void SynthDevice::setModInt(float intensity)
{
    bool changed = false;
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        if (const auto p = parameter(Constants::NahdXml::xmlKeySynthModIntensity().toStdString()); p) {
            p->get().setValue(intensity);
            syncParameters();
            changed = true;
        }
    }
    if (changed)
        emit dataChanged();
}

SynthDevice::ModTarget SynthDevice::modTarget() const
{
    return m_modTarget;
}

void SynthDevice::setModTarget(ModTarget target)
{
    bool changed = false;
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        if (const auto p = parameter(Constants::NahdXml::xmlKeySynthModTarget().toStdString()); p) {
            p->get().setFromXml(static_cast<int>(target));
            syncParameters();
            changed = true;
        }
    }
    if (changed)
        emit dataChanged();
}

// Lfo
Lfo::Waveform SynthDevice::lfoWaveform() const
{
    return m_lfoWaveform;
}

void SynthDevice::setLfoWaveform(Lfo::Waveform wave)
{
    bool changed = false;
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        if (const auto p = parameter(Constants::NahdXml::xmlKeySynthLfoWaveform().toStdString()); p) {
            p->get().setFromXml(static_cast<int>(wave));
            syncParameters();
            changed = true;
        }
    }
    if (changed)
        emit dataChanged();
}

Lfo::Mode SynthDevice::lfoMode() const
{
    return m_lfoMode;
}

void SynthDevice::setLfoMode(Lfo::Mode mode)
{
    bool changed = false;
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        if (const auto p = parameter(Constants::NahdXml::xmlKeySynthLfoMode().toStdString()); p) {
            p->get().setFromXml(static_cast<int>(mode));
            syncParameters();
            changed = true;
        }
    }
    if (changed)
        emit dataChanged();
}

float SynthDevice::lfoRate() const
{
    return m_lfoRate;
}

void SynthDevice::setLfoRate(float rate)
{
    bool changed = false;
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        if (const auto p = parameter(Constants::NahdXml::xmlKeySynthLfoRate().toStdString()); p) {
            p->get().setValue(rate);
            syncParameters();
            changed = true;
        }
    }
    if (changed)
        emit dataChanged();
}

float SynthDevice::lfoInt() const
{
    return m_lfoInt;
}

void SynthDevice::setLfoInt(float intensity)
{
    bool changed = false;
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        if (const auto p = parameter(Constants::NahdXml::xmlKeySynthLfoIntensity().toStdString()); p) {
            p->get().setValue(intensity);
            syncParameters();
            changed = true;
        }
    }
    if (changed)
        emit dataChanged();
}

SynthDevice::LfoTarget SynthDevice::lfoTarget() const
{
    return m_lfoTarget;
}

void SynthDevice::setLfoTarget(LfoTarget target)
{
    bool changed = false;
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        if (const auto p = parameter(Constants::NahdXml::xmlKeySynthLfoTarget().toStdString()); p) {
            p->get().setFromXml(static_cast<int>(target));
            syncParameters();
            changed = true;
        }
    }
    if (changed)
        emit dataChanged();
}

// Voice / Global
SynthDevice::VoiceMode SynthDevice::voiceMode() const
{
    return m_voiceMode;
}

void SynthDevice::setVoiceMode(VoiceMode mode)
{
    bool changed = false;
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        if (const auto p = parameter(Constants::NahdXml::xmlKeyVoiceMode().toStdString()); p) {
            p->get().setFromXml(static_cast<int>(mode));
            syncParameters();
            changed = true;
        }
    }
    if (changed)
        emit dataChanged();
}

float SynthDevice::voiceDepth() const
{
    return m_voiceDepth;
}

void SynthDevice::setVoiceDepth(float depth)
{
    bool changed = false;
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        if (const auto p = parameter(Constants::NahdXml::xmlKeyVoiceDepth().toStdString()); p) {
            p->get().setValue(depth);
            syncParameters();
            changed = true;
        }
    }
    if (changed)
        emit dataChanged();
}

float SynthDevice::portamento() const
{
    return m_portamento;
}

void SynthDevice::setPortamento(float val)
{
    bool changed = false;
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        if (const auto p = parameter(Constants::NahdXml::xmlKeyPortamento().toStdString()); p) {
            p->get().setValue(val);
            syncParameters();
            changed = true;
        }
    }
    if (changed)
        emit dataChanged();
}

float SynthDevice::panSpread() const
{
    return m_panSpread;
}

void SynthDevice::setPanSpread(float spread)
{
    bool changed = false;
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        if (const auto p = parameter(Constants::NahdXml::xmlKeyPanSpread().toStdString()); p) {
            p->get().setValue(spread);
            syncParameters();
            changed = true;
        }
    }
    if (changed)
        emit dataChanged();
}

int SynthDevice::pitchBendRange() const
{
    return m_pitchBendRange;
}

void SynthDevice::setPitchBendRange(int range)
{
    bool changed = false;
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        if (const auto p = parameter(Constants::NahdXml::xmlKeyPitchBendRange().toStdString()); p) {
            p->get().setValue(static_cast<float>(range));
            syncParameters();
            changed = true;
        }
    }
    if (changed)
        emit dataChanged();
}

float SynthDevice::currentPitchBendOffset() const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    return (static_cast<float>(m_pitchBend) - 8192.0f) / 8192.0f * m_pitchBendRange;
}

void SynthDevice::setPan(float val)
{
    Device::setPan(val);
}

void SynthDevice::setVolume(float vol)
{
    Device::setVolume(vol);
}

float SynthDevice::gain() const
{
    return Device::gain();
}

void SynthDevice::setGain(float val)
{
    Device::setGain(val);
}

// Delay Accessors
DelayEffect::Type SynthDevice::delayType() const
{
    return m_delayType;
}

void SynthDevice::setDelayType(DelayEffect::Type type)
{
    bool changed = false;
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        if (const auto p = parameter(Constants::NahdXml::xmlKeyDelayType().toStdString()); p) {
            p->get().setFromXml(static_cast<int>(type));
            syncParameters();
            changed = true;
        }
    }
    if (changed)
        emit dataChanged();
}

float SynthDevice::delayTime() const
{
    return m_delayTime;
}

void SynthDevice::setDelayTime(float time)
{
    bool changed = false;
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        if (const auto p = parameter(Constants::NahdXml::xmlKeyDelayTime().toStdString()); p) {
            p->get().setValue(time);
            syncParameters();
            changed = true;
        }
    }
    if (changed)
        emit dataChanged();
}

float SynthDevice::delayFeedback() const
{
    return m_delayFeedback;
}

void SynthDevice::setDelayFeedback(float fb)
{
    bool changed = false;
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        if (const auto p = parameter(Constants::NahdXml::xmlKeyDelayFeedback().toStdString()); p) {
            p->get().setValue(fb);
            syncParameters();
            changed = true;
        }
    }
    if (changed)
        emit dataChanged();
}

float SynthDevice::delayDepth() const
{
    return m_delayDepth;
}

void SynthDevice::setDelayDepth(float depth)
{
    bool changed = false;
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        if (const auto p = parameter(Constants::NahdXml::xmlKeyDelayDepth().toStdString()); p) {
            p->get().setValue(depth);
            syncParameters();
            changed = true;
        }
    }
    if (changed)
        emit dataChanged();
}

float SynthDevice::delayMix() const
{
    return m_delayMix;
}

void SynthDevice::setDelayMix(float mix)
{
    bool changed = false;
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        if (const auto p = parameter(Constants::NahdXml::xmlKeyDelayMix().toStdString()); p) {
            p->get().setValue(mix);
            syncParameters();
            changed = true;
        }
    }
    if (changed)
        emit dataChanged();
}

bool SynthDevice::delaySync() const
{
    return m_delaySync;
}

void SynthDevice::setDelaySync(bool sync)
{
    bool changed = false;
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        if (const auto p = parameter(Constants::NahdXml::xmlKeyDelaySync().toStdString()); p) {
            p->get().setValue(sync ? 1.0f : 0.0f);
            syncParameters();
            changed = true;
        }
    }
    if (changed)
        emit dataChanged();
}

float SynthDevice::delaySyncDivision() const
{
    return m_delaySyncDivision;
}

void SynthDevice::setDelaySyncDivision(float division)
{
    bool changed = false;
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        if (const auto p = parameter(Constants::NahdXml::xmlKeyDelaySyncDivision().toStdString()); p) {
            p->get().setValue(division);
            syncParameters();
            changed = true;
        }
    }
    if (changed)
        emit dataChanged();
}

float SynthDevice::delayFeedbackLpf() const
{
    return m_delay.feedbackLpf();
}

void SynthDevice::setFeedbackLpf(float cutoff)
{
    bool changed = false;
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        if (const auto p = parameter(Constants::NahdXml::xmlKeyDelayFeedbackLpf().toStdString()); p) {
            p->get().setValue(cutoff);
            syncParameters();
            changed = true;
        }
    }
    if (changed)
        emit dataChanged();
}

float SynthDevice::delayFeedbackHpf() const
{
    return m_delay.feedbackHpf();
}

void SynthDevice::setFeedbackHpf(float cutoff)
{
    bool changed = false;
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        if (const auto p = parameter(Constants::NahdXml::xmlKeyDelayFeedbackHpf().toStdString()); p) {
            p->get().setValue(cutoff);
            syncParameters();
            changed = true;
        }
    }
    if (changed)
        emit dataChanged();
}

// Accessors (VCO3)
PolyBlepOscillator::Waveform SynthDevice::vco3Waveform() const
{
    return m_vco3Waveform;
}

void SynthDevice::setVco3Waveform(PolyBlepOscillator::Waveform wave)
{
    bool changed = false;
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        if (const auto p = parameter(Constants::NahdXml::xmlKeySynthVco3Waveform().toStdString()); p) {
            p->get().setFromXml(static_cast<int>(wave));
            syncParameters();
            changed = true;
        }
    }
    if (changed)
        emit dataChanged();
}

int SynthDevice::vco3Octave() const
{
    return m_vco3Octave;
}

void SynthDevice::setVco3Octave(int octave)
{
    bool changed = false;
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        if (const auto p = parameter(Constants::NahdXml::xmlKeySynthVco3Octave().toStdString()); p) {
            p->get().setFromXml(octave);
            syncParameters();
            changed = true;
        }
    }
    if (changed)
        emit dataChanged();
}

float SynthDevice::vco3Pitch() const
{
    return m_vco3Pitch;
}

void SynthDevice::setVco3Pitch(float pitch)
{
    bool changed = false;
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        if (const auto p = parameter(Constants::NahdXml::xmlKeySynthVco3Pitch().toStdString()); p) {
            p->get().setValue(pitch);
            syncParameters();
            changed = true;
        }
    }
    if (changed)
        emit dataChanged();
}

float SynthDevice::vco3Shape() const
{
    return m_vco3Shape;
}

void SynthDevice::setVco3Shape(float shape)
{
    bool changed = false;
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        if (const auto p = parameter(Constants::NahdXml::xmlKeySynthVco3Shape().toStdString()); p) {
            p->get().setValue(shape);
            syncParameters();
            changed = true;
        }
    }
    if (changed)
        emit dataChanged();
}

bool SynthDevice::vco3Sync() const
{
    return m_vco3Sync;
}

void SynthDevice::setVco3Sync(bool sync)
{
    bool changed = false;
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        if (const auto p = parameter(Constants::NahdXml::xmlKeySynthVco3Sync().toStdString()); p) {
            p->get().setValue(sync ? 1.0f : 0.0f);
            syncParameters();
            changed = true;
        }
    }
    if (changed)
        emit dataChanged();
}

float SynthDevice::mixVco3() const
{
    return m_mixVco3;
}

void SynthDevice::setMixVco3(float level)
{
    bool changed = false;
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        if (const auto p = parameter(Constants::NahdXml::xmlKeySynthMixLevel3().toStdString()); p) {
            p->get().setValue(level);
            syncParameters();
            changed = true;
        }
    }
    if (changed)
        emit dataChanged();
}

} // namespace noteahead
