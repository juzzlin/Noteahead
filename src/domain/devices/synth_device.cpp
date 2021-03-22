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
#include "../../common/utils.hpp"
#include "../../infra/midi/midi_cc_mapping.hpp"
#include "synth_presets.hpp"

#include <algorithm>
#include <cmath>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

namespace noteahead {

void SynthDevice::Voice::reset()
{
    active = false;
    vco1.sync(0.0);
    vco2.sync(0.0);
    lpf.reset();
    hpf.reset();
    ampEg.reset();
    modEg.reset();
    lfo.reset();
    glideFrequency = 0.0;
}

void SynthDevice::Voice::trigger(uint8_t n, double freq, float p, bool phaseSync)
{
    note = n;
    frequency = freq;
    if (glideFrequency == 0.0) glideFrequency = freq;
    pan = p;
    active = true;

    lpf.reset();
    hpf.reset();
    lfo.reset();

    if (phaseSync) {
        vco1.sync(0.0);
        vco2.sync(0.0);
        multi.reset();
    }
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
    addParameter(Parameter { Constants::NahdXml::xmlKeySynthVco1Waveform().toStdString(), 1.0f, 0, 2, 1, 1, Parameter::Type::Discrete });
    addParameter(Parameter { Constants::NahdXml::xmlKeySynthVco1Octave().toStdString(), 0.0f, -1, 2, 0, 1, Parameter::Type::Discrete });
    addParameter(Parameter { Constants::NahdXml::xmlKeySynthVco1Pitch().toStdString(), 0.0f, -2400, 2400, 0, 1, Parameter::Type::Discrete });
    addParameter(Parameter { Constants::NahdXml::xmlKeySynthVco1Shape().toStdString(), 0.0f, 0, 100, 0 });
    addParameter(Parameter { Constants::NahdXml::xmlKeySynthVco1Sync().toStdString(), 0.0f, 0, 1, 0, 1, Parameter::Type::Boolean });

    addParameter(Parameter { Constants::NahdXml::xmlKeySynthVco2Waveform().toStdString(), 1.0f, 0, 2, 1, 1, Parameter::Type::Discrete });
    addParameter(Parameter { Constants::NahdXml::xmlKeySynthVco2Octave().toStdString(), 0.0f, -1, 2, 0, 1, Parameter::Type::Discrete });
    addParameter(Parameter { Constants::NahdXml::xmlKeySynthVco2Pitch().toStdString(), 0.0f, -2400, 2400, 0, 1, Parameter::Type::Discrete });
    addParameter(Parameter { Constants::NahdXml::xmlKeySynthVco2Shape().toStdString(), 0.0f, 0, 100, 0 });
    addParameter(Parameter { Constants::NahdXml::xmlKeySynthVco2Sync().toStdString(), 0.0f, 0, 1, 0, 1, Parameter::Type::Boolean });

    addParameter(Parameter { Constants::NahdXml::xmlKeySynthMultiMode().toStdString(), 1.0f, 0, 3, 1, 1, Parameter::Type::Discrete }); // Low default
    addParameter(Parameter { Constants::NahdXml::xmlKeySynthMultiShape().toStdString(), 0.5f, 0, 100, 50 });
    addParameter(Parameter { Constants::NahdXml::xmlKeySynthMultiLevel().toStdString(), 0.0f, 0, 100, 0 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyMultiKeyTrack().toStdString(), 0.0f, 0, 100, 0 });

    addParameter(Parameter { Constants::NahdXml::xmlKeySynthMixLevel1().toStdString(), 1.0f, 0, 100, 100 });
    addParameter(Parameter { Constants::NahdXml::xmlKeySynthMixLevel2().toStdString(), 0.0f, 0, 100, 0 });

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
    addParameter(Parameter { Constants::NahdXml::xmlKeySynthModTarget().toStdString(), 2.0f, 0, 2, 2, 1, Parameter::Type::Discrete }); // Cutoff default

    addParameter(Parameter { Constants::NahdXml::xmlKeySynthLfoWaveform().toStdString(), 1.0f, 0, 2, 1, 1, Parameter::Type::Discrete }); // Tri default
    addParameter(Parameter { Constants::NahdXml::xmlKeySynthLfoMode().toStdString(), 0.0f, 0, 2, 0, 1, Parameter::Type::Discrete }); // Normal default
    addParameter(Parameter { Constants::NahdXml::xmlKeySynthLfoRate().toStdString(), 0.5f, 0, 100, 50 });
    addParameter(Parameter { Constants::NahdXml::xmlKeySynthLfoIntensity().toStdString(), 0.5f, -100, 100, 0 });
    addParameter(Parameter { Constants::NahdXml::xmlKeySynthLfoTarget().toStdString(), 0.0f, 0, 2, 0, 1, Parameter::Type::Discrete }); // Pitch default

    addParameter(Parameter { Constants::NahdXml::xmlKeyVoiceMode().toStdString(), 0.0f, 0, 1, 0, 1, Parameter::Type::Discrete });
    addParameter(Parameter { Constants::NahdXml::xmlKeyVoiceDepth().toStdString(), 0.0f, 0, 100, 0 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyPortamento().toStdString(), 0.0f, 0, 100, 0 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyPanSpread().toStdString(), 0.0f, 0, 100, 0 });

    addParameter(Parameter { Constants::NahdXml::xmlKeyDelayType().toStdString(), 0.0f, 0, 5, 0, 1, Parameter::Type::Discrete });
    addParameter(Parameter { Constants::NahdXml::xmlKeyDelayTime().toStdString(), 0.5f, 0, 10000, 500 }); // 0..10 seconds in ms
    addParameter(Parameter { Constants::NahdXml::xmlKeyDelayFeedback().toStdString(), 0.3f, 0, 100, 30 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyDelayDepth().toStdString(), 0.5f, 0, 100, 50 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyDelayMix().toStdString(), 0.0f, 0, 100, 0 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyDelaySync().toStdString(), 0.0f, 0, 1, 0, 1, Parameter::Type::Boolean });
    addParameter(Parameter { Constants::NahdXml::xmlKeyDelaySyncDivision().toStdString(), 0.25f, 0, 100, 25 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyDelayFeedbackLpf().toStdString(), 1.0f, 0, 100, 100 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyDelayFeedbackHpf().toStdString(), 0.0f, 0, 100, 0 });

    for (auto && voice : m_voices) {
        voice.lpf.setMode(CascadedSVF::Mode::LowPass);
        voice.hpf.setMode(CascadedSVF::Mode::HighPass);
    }

    setManualPan(panInternal());
    setManualVolume(volumeInternal());
    setManualGain(gainInternal());
    m_manualLpfCutoff = m_lpfCutoff;
    m_manualHpfCutoff = m_hpfCutoff;

    syncParameters();
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

std::string SynthDevice::typeId() const
{
    return "26f5a47e-4786-11f1-92b0-0b3f3bef9f74";
}

void SynthDevice::processAudio(float * output, uint32_t nFrames, uint32_t sampleRate)
{
    setSampleRate(sampleRate);
    const uint32_t oversampledRate { sampleRate * 2 };
    const std::lock_guard<std::recursive_mutex> lock(mutex());

    std::vector<float> oversampledBuffer(nFrames * 4, 0.0f);

    for (auto && voice : m_voices) {
        if (!voice.active) continue;

        voice.vco1.setSampleRate(oversampledRate);
        voice.vco2.setSampleRate(oversampledRate);
        voice.multi.setSampleRate(oversampledRate);
        voice.lpf.setSampleRate(oversampledRate);
        voice.hpf.setSampleRate(oversampledRate);
        voice.ampEg.setSampleRate(oversampledRate);
        voice.modEg.setSampleRate(oversampledRate);
        voice.lfo.setSampleRate(oversampledRate);

        voice.lpf.setResonance(m_lpfResonance);
        voice.vco1.setWaveform(m_vco1Waveform);
        voice.vco2.setWaveform(m_vco2Waveform);
        voice.vco1.setShape(m_vco1Shape);
        voice.vco2.setShape(m_vco2Shape);
        voice.multi.setType(m_multiType);
        voice.multi.setShape(m_multiShape);
        voice.multi.setKeyTrack(m_multiKeyTrack);
        voice.multi.setNote(voice.note);

        // Portamento constant (simple one-pole)
        const double portamentoCoeff { m_portamento > 0 ? 1.0 - std::pow(0.001, 1.0 / (m_portamento * oversampledRate)) : 1.0 };

        for (uint32_t i { 0 }; i < nFrames; i++) {
            // Run voice generation twice for 2x oversampling
            for (int os { 0 }; os < 2; os++) {
                // Update glide frequency
                voice.glideFrequency += (voice.frequency - voice.glideFrequency) * portamentoCoeff;

                const double ampEnv { voice.ampEg.nextSample() };
                const double modEnv { voice.modEg.nextSample() * m_modInt };
                const double lfoVal { voice.lfo.nextSample() * m_lfoInt };

                // Modulation
                double cutoffMod { (m_modTarget == ModTarget::Cutoff) ? modEnv : 0.0 };
                if (m_lfoTarget == LfoTarget::Cutoff) cutoffMod += lfoVal;

                const double shapeMod { (m_lfoTarget == LfoTarget::Shape) ? lfoVal : 0.0 };

                // VCO Frequencies
                double p1 { (m_modTarget == ModTarget::Pitch1) ? modEnv : 0.0 };
                double p2 { (m_modTarget == ModTarget::Pitch2) ? modEnv : 0.0 };
                if (m_lfoTarget == LfoTarget::Pitch) {
                    p1 += lfoVal;
                    p2 += lfoVal;
                }

                // Note: Pitch parameters are in cents (-2400..2400)
                const double vco1PitchOffset = m_vco1Pitch;
                const double vco2PitchOffset = m_vco2Pitch;

                const double vco1Freq { voice.glideFrequency * std::pow(2.0, (m_vco1Octave * 12.0 + vco1PitchOffset / 100.0 + p1 * 12.0) / 12.0) };
                const double vco2Freq { voice.glideFrequency * std::pow(2.0, (m_vco2Octave * 12.0 + vco2PitchOffset / 100.0 + p2 * 12.0) / 12.0) };

                voice.vco1.setFrequency(vco1Freq);
                voice.vco2.setFrequency(vco2Freq);
                voice.vco1.setShape(std::clamp(m_vco1Shape + shapeMod, 0.0, 1.0));
                voice.vco2.setShape(std::clamp(m_vco2Shape + shapeMod, 0.0, 1.0));

                // VCO2 Hard Sync to VCO1
                const double oldPhase { voice.vco1.phase() };
                const double vco1Val { voice.vco1.nextSample() };
                if (m_vco2Sync && voice.vco1.phase() < oldPhase) {
                    voice.vco2.sync(0.0);
                }
                const double vco2Val { voice.vco2.nextSample() };
                const double multiVal { voice.multi.nextSample() };

                const double mix { (vco1Val * m_mixVco1) + (vco2Val * m_mixVco2) + (multiVal * m_multiLevel) };
                const double mixHeadroom { mix * 0.45 }; // Extra headroom for Multi engine

                // Filter
                cutoffMod += (voice.note - 60.0) / 127.0 * m_filterKeyTrack;
                
                voice.lpf.setCutoff(std::clamp(m_lpfCutoff + cutoffMod, 0.0, 1.0));
                voice.hpf.setCutoff(m_hpfCutoff);

                const float filtered { voice.hpf.process(voice.lpf.process(static_cast<float>(mixHeadroom))) };
                const float finalHighRateSample { filtered * static_cast<float>(ampEnv) * (1.0f / static_cast<float>(MaxVoices)) * linearGainInternal() };

                oversampledBuffer[(i * 2 + os) * 2] += finalHighRateSample * (1.0f - voice.pan) * (1.0f - panInternal()) * 2.0f;
                oversampledBuffer[(i * 2 + os) * 2 + 1] += finalHighRateSample * voice.pan * panInternal() * 2.0f;
            }
        }

        if (voice.ampEg.state() == ADSREnvelope::State::Idle) {
            voice.active = false;
        }
    }

    // Apply global FX (Delay) and downsample
    for (uint32_t i { 0 }; i < nFrames; i++) {
        const float l0 { oversampledBuffer[i * 4] };
        const float r0 { oversampledBuffer[i * 4 + 1] };
        const float l1 { oversampledBuffer[i * 4 + 2] };
        const float r1 { oversampledBuffer[i * 4 + 3] };

        // Soft-clip at high rate and then downsample
        float l { m_oversamplerL.process(std::tanh(l0), std::tanh(l1)) };
        float r { m_oversamplerR.process(std::tanh(r0), std::tanh(r1)) };

        m_delay.process(l, r);

        // Final volume after all processing
        l *= volumeInternal();
        r *= volumeInternal();

        output[i * 2] += l;
        output[i * 2 + 1] += r;
    }
}

void SynthDevice::processMidiNoteOn(uint8_t note, uint8_t velocity)
{
    const std::lock_guard<std::recursive_mutex> lock(mutex());
    handleNoteOn(note, velocity);
}

void SynthDevice::processMidiNoteOff(uint8_t note)
{
    const std::lock_guard<std::recursive_mutex> lock(mutex());
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

            if (auto p = parameter(Constants::NahdXml::xmlKeySynthLpfCutoff().toStdString()); p) p->get().setValue(m_lpfCutoff);
            if (auto p = parameter(Constants::NahdXml::xmlKeySynthHpfCutoff().toStdString()); p) p->get().setValue(m_hpfCutoff);

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
                if (auto p = parameter(Constants::NahdXml::xmlKeySynthLpfCutoff().toStdString()); p) {
                    p->get().setValue(val);
                    syncParameters();
                    changed = true;
                }
            } else if (controller == static_cast<uint8_t>(Controller::GeneralPurpose6)) { // HPF Cutoff
                m_hpfCutoff = val;
                if (auto p = parameter(Constants::NahdXml::xmlKeySynthHpfCutoff().toStdString()); p) {
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
    {
        const std::lock_guard<std::recursive_mutex> lock(mutex());
        for (auto && voice : m_voices) voice.reset();
        m_delay.reset();
    }
}

void SynthDevice::setBpm(float bpm)
{
    {
        const std::lock_guard<std::recursive_mutex> lock(mutex());
        m_delay.setBpm(bpm);
    }
}

void SynthDevice::reset()
{
    {
        const std::lock_guard<std::recursive_mutex> lock(mutex());
        Device::reset();
        for (auto && voice : m_voices) voice.reset();
        m_delay.reset();
        m_oversamplerL.reset();
        m_oversamplerR.reset();
        m_polyNextVoice = 0;
        syncParameters();
    }
}

double SynthDevice::voiceGlideFrequency(int index) const
{
    const std::lock_guard<std::recursive_mutex> lock(mutex());
    if (index >= 0 && index < static_cast<int>(m_voices.size())) {
        return m_voices.at(index).glideFrequency;
    }
    return 0.0;
}

void SynthDevice::handleNoteOn(uint8_t note, uint8_t)
{
    // First release any existing voices for this note to avoid multiple voices for same note in poly
    for (auto && voice : m_voices) {
        if (voice.active && voice.note == note) {
            voice.release();
        }
    }

    double freq = midiNoteToFreq(note);

    if (m_voiceMode == VoiceMode::Poly) {
        int bestVoice = -1;

        // 1. Try to find a voice already playing or releasing this note (Affinity)
        for (int i = 0; i < MaxVoices; i++) {
            if (m_voices.at(i).active && m_voices.at(i).note == note) {
                bestVoice = i;
                break;
            }
        }

        // 2. Try to find an idle voice (Round-Robin)
        if (bestVoice == -1) {
            for (int i = 0; i < MaxVoices; i++) {
                int idx = (m_polyNextVoice + i) % MaxVoices;
                if (!m_voices.at(idx).active) {
                    bestVoice = idx;
                    m_polyNextVoice = (idx + 1) % MaxVoices;
                    break;
                }
            }
        }

        // 3. Steal the quietest voice (Round-Robin search for stealing candidate)
        if (bestVoice == -1) {
            float lowestAmp = 2.0f;
            for (int i = 0; i < MaxVoices; i++) {
                int idx = (m_polyNextVoice + i) % MaxVoices;
                float currentAmp = static_cast<float>(m_voices.at(idx).ampEg.value());
                if (currentAmp < lowestAmp) {
                    lowestAmp = currentAmp;
                    bestVoice = idx;
                }
            }
            if (bestVoice != -1) {
                m_polyNextVoice = (bestVoice + 1) % MaxVoices;
            }
        }

        if (bestVoice != -1) {
            // Reset glide if portamento is off
            if (m_portamento == 0.0f) {
                m_voices.at(bestVoice).glideFrequency = freq;
            }

            // Balanced Pan Spread (Voice-alternating distribution inspired by Behringer DeepMind)
            const float side = (bestVoice % 2 == 0) ? -1.0f : 1.0f;
            const float depth = 1.0f - static_cast<float>(bestVoice / 2) * (2.0f / static_cast<float>(MaxVoices));
            const float pan = 0.5f + (side * depth * m_panSpread * 0.5f);

            m_voices.at(bestVoice).trigger(note, freq, pan, m_vco1Sync);
        }
    } else {
        // Unison
        for (int i = 0; i < MaxVoices; i++) {
            // Non-linear detune spread for better texture
            const double detuneAmount = (i - (MaxVoices - 1) / 2.0) * std::pow(m_voiceDepth, 1.5) * 0.2;

            const float side = (i % 2 == 0) ? -1.0f : 1.0f;
            const float depth = 1.0f - static_cast<float>(i / 2) * (2.0f / static_cast<float>(MaxVoices));
            const float pan = 0.5f + (side * depth * m_panSpread * 0.5f);

            m_voices.at(i).trigger(note, freq * std::pow(2.0, detuneAmount / 12.0), pan, m_vco1Sync);
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

void SynthDevice::syncParameters()
{
    Device::syncParameters();
    if (auto p = parameter(Constants::NahdXml::xmlKeySynthVco1Waveform().toStdString()); p) m_vco1Waveform = static_cast<PolyBLEPOscillator::Waveform>(p->get().xmlValue());
    if (auto p = parameter(Constants::NahdXml::xmlKeySynthVco1Octave().toStdString()); p) m_vco1Octave = p->get().xmlValue();
    if (auto p = parameter(Constants::NahdXml::xmlKeySynthVco1Pitch().toStdString()); p) m_vco1Pitch = p->get().xmlValue();
    if (auto p = parameter(Constants::NahdXml::xmlKeySynthVco1Shape().toStdString()); p) m_vco1Shape = p->get().value();
    if (auto p = parameter(Constants::NahdXml::xmlKeySynthVco1Sync().toStdString()); p) m_vco1Sync = p->get().value() > 0.5f;

    if (auto p = parameter(Constants::NahdXml::xmlKeySynthVco2Waveform().toStdString()); p) m_vco2Waveform = static_cast<PolyBLEPOscillator::Waveform>(p->get().xmlValue());
    if (auto p = parameter(Constants::NahdXml::xmlKeySynthVco2Octave().toStdString()); p) m_vco2Octave = p->get().xmlValue();
    if (auto p = parameter(Constants::NahdXml::xmlKeySynthVco2Pitch().toStdString()); p) m_vco2Pitch = p->get().xmlValue();
    if (auto p = parameter(Constants::NahdXml::xmlKeySynthVco2Shape().toStdString()); p) m_vco2Shape = p->get().value();
    if (auto p = parameter(Constants::NahdXml::xmlKeySynthVco2Sync().toStdString()); p) m_vco2Sync = p->get().value() > 0.5f;

    if (auto p = parameter(Constants::NahdXml::xmlKeySynthMultiMode().toStdString()); p) m_multiType = static_cast<MultiEngine::Type>(p->get().xmlValue());
    if (auto p = parameter(Constants::NahdXml::xmlKeySynthMultiShape().toStdString()); p) m_multiShape = p->get().value();
    if (auto p = parameter(Constants::NahdXml::xmlKeySynthMultiLevel().toStdString()); p) m_multiLevel = p->get().value();
    if (auto p = parameter(Constants::NahdXml::xmlKeyMultiKeyTrack().toStdString()); p) m_multiKeyTrack = p->get().value();

    if (auto p = parameter(Constants::NahdXml::xmlKeySynthMixLevel1().toStdString()); p) m_mixVco1 = p->get().value();
    if (auto p = parameter(Constants::NahdXml::xmlKeySynthMixLevel2().toStdString()); p) m_mixVco2 = p->get().value();

    if (auto p = parameter(Constants::NahdXml::xmlKeySynthLpfCutoff().toStdString()); p) m_lpfCutoff = p->get().value();
    if (auto p = parameter(Constants::NahdXml::xmlKeySynthLpfResonance().toStdString()); p) m_lpfResonance = p->get().value();
    if (auto p = parameter(Constants::NahdXml::xmlKeySynthHpfCutoff().toStdString()); p) m_hpfCutoff = p->get().value();
    if (auto p = parameter(Constants::NahdXml::xmlKeyKeyTrack().toStdString()); p) m_filterKeyTrack = p->get().value();

    if (auto p = parameter(Constants::NahdXml::xmlKeySynthAmpAttack().toStdString()); p) m_ampAttack = p->get().value();
    if (auto p = parameter(Constants::NahdXml::xmlKeySynthAmpDecay().toStdString()); p) m_ampDecay = p->get().value();
    if (auto p = parameter(Constants::NahdXml::xmlKeySynthAmpSustain().toStdString()); p) m_ampSustain = p->get().value();
    if (auto p = parameter(Constants::NahdXml::xmlKeySynthAmpRelease().toStdString()); p) m_ampRelease = p->get().value();

    if (auto p = parameter(Constants::NahdXml::xmlKeySynthModAttack().toStdString()); p) m_modAttack = p->get().value();
    if (auto p = parameter(Constants::NahdXml::xmlKeySynthModDecay().toStdString()); p) m_modDecay = p->get().value();

    const auto mapAttack = [](float x) { return 0.000001f * std::pow(20.0f / 0.000001f, x); };
    const auto mapDecay = [](float x) { return 0.01f * std::pow(60.0f / 0.01f, x); };
    const auto mapRelease = [](float x) { return 0.001f * std::pow(60.0f / 0.001f, x); };

    if (auto p = parameter(Constants::NahdXml::xmlKeySynthModIntensity().toStdString()); p) m_modInt = (p->get().value() - 0.5f) * 2.0f;
    if (auto p = parameter(Constants::NahdXml::xmlKeySynthModTarget().toStdString()); p) m_modTarget = static_cast<ModTarget>(p->get().xmlValue());

    if (auto p = parameter(Constants::NahdXml::xmlKeySynthLfoWaveform().toStdString()); p) m_lfoWaveform = static_cast<LFO::Waveform>(p->get().xmlValue());
    if (auto p = parameter(Constants::NahdXml::xmlKeySynthLfoMode().toStdString()); p) m_lfoMode = static_cast<LFO::Mode>(p->get().xmlValue());
    if (auto p = parameter(Constants::NahdXml::xmlKeySynthLfoRate().toStdString()); p) m_lfoRate = p->get().value();
    if (auto p = parameter(Constants::NahdXml::xmlKeySynthLfoIntensity().toStdString()); p) m_lfoInt = (p->get().value() - 0.5f) * 2.0f;
    if (auto p = parameter(Constants::NahdXml::xmlKeySynthLfoTarget().toStdString()); p) m_lfoTarget = static_cast<LfoTarget>(p->get().xmlValue());

    if (auto p = parameter(Constants::NahdXml::xmlKeyVoiceMode().toStdString()); p) m_voiceMode = static_cast<VoiceMode>(p->get().xmlValue());
    if (auto p = parameter(Constants::NahdXml::xmlKeyVoiceDepth().toStdString()); p) m_voiceDepth = p->get().value();
    if (auto p = parameter(Constants::NahdXml::xmlKeyPortamento().toStdString()); p) m_portamento = p->get().value();
    if (auto p = parameter(Constants::NahdXml::xmlKeyPanSpread().toStdString()); p) m_panSpread = p->get().value();

    if (auto p = parameter(Constants::NahdXml::xmlKeyDelayType().toStdString()); p) m_delayType = static_cast<DelayEffect::Type>(p->get().xmlValue());
    if (auto p = parameter(Constants::NahdXml::xmlKeyDelayTime().toStdString()); p) m_delayTime = p->get().value() * 10.0f; 
    if (auto p = parameter(Constants::NahdXml::xmlKeyDelayFeedback().toStdString()); p) m_delayFeedback = p->get().value();
    if (auto p = parameter(Constants::NahdXml::xmlKeyDelayDepth().toStdString()); p) m_delayDepth = p->get().value();
    if (auto p = parameter(Constants::NahdXml::xmlKeyDelayMix().toStdString()); p) m_delayMix = p->get().value();
    if (auto p = parameter(Constants::NahdXml::xmlKeyDelaySync().toStdString()); p) m_delaySync = p->get().value() > 0.5f;
    if (auto p = parameter(Constants::NahdXml::xmlKeyDelaySyncDivision().toStdString()); p) m_delaySyncDivision = p->get().value();
    if (auto p = parameter(Constants::NahdXml::xmlKeyDelayFeedbackLpf().toStdString()); p) m_delay.setFeedbackLpf(p->get().value());
    if (auto p = parameter(Constants::NahdXml::xmlKeyDelayFeedbackHpf().toStdString()); p) m_delay.setFeedbackHpf(p->get().value());

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
        voice.vco1.setShape(m_vco1Shape);
        voice.vco2.setShape(m_vco2Shape);
        
        voice.lfo.setWaveform(m_lfoWaveform);
        voice.lfo.setMode(m_lfoMode);
        
        float freq = 0.0f;
        if (m_lfoMode == LFO::Mode::BPM) {
            // Map 0..1 rate to BPM divisions (approx)
            freq = (m_delay.bpm() / 60.0f) * (m_lfoRate * 4.0f + 0.25f);
        } else {
            freq = std::pow(20.0f, m_lfoRate) - 0.95f; // 0.05 to 20Hz
        }
        voice.lfo.setFrequency(freq);

        voice.ampEg.setAttackTime(mapAttack(m_ampAttack));
        voice.ampEg.setDecayTime(mapDecay(m_ampDecay));
        voice.ampEg.setSustainLevel(m_ampSustain);
        voice.ampEg.setReleaseTime(mapRelease(m_ampRelease));
        voice.modEg.setAttackTime(mapAttack(m_modAttack));
        voice.modEg.setDecayTime(mapDecay(m_modDecay));
        voice.modEg.setSustainLevel(0.0); // Mod EG is AD only
        voice.modEg.setReleaseTime(mapDecay(m_modDecay));
    }
}

void SynthDevice::serializeToXml(QXmlStreamWriter & writer) const
{
    const std::lock_guard<std::recursive_mutex> lock(mutex());
    writer.writeStartElement(Constants::NahdXml::xmlKeyDevice());
    serializeAttributesToXml(writer);
    serializeParametersToXml(writer);
    writer.writeEndElement();
}

void SynthDevice::deserializeFromXml(QXmlStreamReader & reader)
{
    {
        const std::lock_guard<std::recursive_mutex> lock(mutex());
        deserializeAttributesFromXml(reader);
        deserializeParametersFromXml(reader);
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

void SynthDevice::processMidiProgramChange(uint8_t program, uint8_t)
{
    loadPreset(m_currentBank, program);
}

void SynthDevice::loadPreset(int bank, int index)
{
    {
        const std::lock_guard<std::recursive_mutex> lock(mutex());

        if (bank == 0) {
            const auto& presets = SynthPresets::presets();
            if (index < 0 || index >= static_cast<int>(presets.size())) return;

            Device::reset();

            for (auto && [name, val] : presets[index].parameters) {
                if (auto p = parameter(name); p) {
                    p->get().setValue(val);
                }
            }
        } else if (bank == 1) {
            if (m_userPresets.find(index) == m_userPresets.end()) return;
            const auto& preset = m_userPresets.at(index);

            Device::reset();

            for (auto && [name, val] : preset.parameters) {
                if (auto p = parameter(name); p) {
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
PolyBLEPOscillator::Waveform SynthDevice::vco1Waveform() const { return m_vco1Waveform; }
void SynthDevice::setVco1Waveform(PolyBLEPOscillator::Waveform wave) { bool changed = false; { std::lock_guard<std::recursive_mutex> lock { mutex() }; if (auto p = parameter(Constants::NahdXml::xmlKeySynthVco1Waveform().toStdString()); p) { p->get().setFromXml(static_cast<int>(wave)); syncParameters(); changed = true; } } if (changed) emit dataChanged(); }
int SynthDevice::vco1Octave() const { return m_vco1Octave; }
void SynthDevice::setVco1Octave(int octave) { bool changed = false; { std::lock_guard<std::recursive_mutex> lock { mutex() }; if (auto p = parameter(Constants::NahdXml::xmlKeySynthVco1Octave().toStdString()); p) { p->get().setFromXml(octave); syncParameters(); changed = true; } } if (changed) emit dataChanged(); }
int SynthDevice::vco1Pitch() const { return m_vco1Pitch; }
void SynthDevice::setVco1Pitch(int pitch) { bool changed = false; { std::lock_guard<std::recursive_mutex> lock { mutex() }; if (auto p = parameter(Constants::NahdXml::xmlKeySynthVco1Pitch().toStdString()); p) { p->get().setFromXml(pitch); syncParameters(); changed = true; } } if (changed) emit dataChanged(); }
float SynthDevice::vco1Shape() const { return m_vco1Shape; }
void SynthDevice::setVco1Shape(float shape) { bool changed = false; { std::lock_guard<std::recursive_mutex> lock { mutex() }; if (auto p = parameter(Constants::NahdXml::xmlKeySynthVco1Shape().toStdString()); p) { p->get().setValue(shape); syncParameters(); changed = true; } } if (changed) emit dataChanged(); }
bool SynthDevice::vco1Sync() const { return m_vco1Sync; }
void SynthDevice::setVco1Sync(bool sync) { bool changed = false; { std::lock_guard<std::recursive_mutex> lock { mutex() }; if (auto p = parameter(Constants::NahdXml::xmlKeySynthVco1Sync().toStdString()); p) { p->get().setValue(sync ? 1.0f : 0.0f); syncParameters(); changed = true; } } if (changed) emit dataChanged(); }

// Accessors (VCO2)
PolyBLEPOscillator::Waveform SynthDevice::vco2Waveform() const { return m_vco2Waveform; }
void SynthDevice::setVco2Waveform(PolyBLEPOscillator::Waveform wave) { bool changed = false; { std::lock_guard<std::recursive_mutex> lock { mutex() }; if (auto p = parameter(Constants::NahdXml::xmlKeySynthVco2Waveform().toStdString()); p) { p->get().setFromXml(static_cast<int>(wave)); syncParameters(); changed = true; } } if (changed) emit dataChanged(); }
int SynthDevice::vco2Octave() const { return m_vco2Octave; }
void SynthDevice::setVco2Octave(int octave) { bool changed = false; { std::lock_guard<std::recursive_mutex> lock { mutex() }; if (auto p = parameter(Constants::NahdXml::xmlKeySynthVco2Octave().toStdString()); p) { p->get().setFromXml(octave); syncParameters(); changed = true; } } if (changed) emit dataChanged(); }
int SynthDevice::vco2Pitch() const { return m_vco2Pitch; }
void SynthDevice::setVco2Pitch(int pitch) { bool changed = false; { std::lock_guard<std::recursive_mutex> lock { mutex() }; if (auto p = parameter(Constants::NahdXml::xmlKeySynthVco2Pitch().toStdString()); p) { p->get().setFromXml(pitch); syncParameters(); changed = true; } } if (changed) emit dataChanged(); }
float SynthDevice::vco2Shape() const { return m_vco2Shape; }
void SynthDevice::setVco2Shape(float shape) { bool changed = false; { std::lock_guard<std::recursive_mutex> lock { mutex() }; if (auto p = parameter(Constants::NahdXml::xmlKeySynthVco2Shape().toStdString()); p) { p->get().setValue(shape); syncParameters(); changed = true; } } if (changed) emit dataChanged(); }
bool SynthDevice::vco2Sync() const { return m_vco2Sync; }
void SynthDevice::setVco2Sync(bool sync) { bool changed = false; { std::lock_guard<std::recursive_mutex> lock { mutex() }; if (auto p = parameter(Constants::NahdXml::xmlKeySynthVco2Sync().toStdString()); p) { p->get().setValue(sync ? 1.0f : 0.0f); syncParameters(); changed = true; } } if (changed) emit dataChanged(); }

// Multi Engine
MultiEngine::Type SynthDevice::multiType() const { return m_multiType; }
void SynthDevice::setMultiType(MultiEngine::Type type) { bool changed = false; { std::lock_guard<std::recursive_mutex> lock { mutex() }; if (auto p = parameter(Constants::NahdXml::xmlKeySynthMultiMode().toStdString()); p) { p->get().setFromXml(static_cast<int>(type)); syncParameters(); changed = true; } } if (changed) emit dataChanged(); }
float SynthDevice::multiShape() const { return m_multiShape; }
void SynthDevice::setMultiShape(float shape) { bool changed = false; { std::lock_guard<std::recursive_mutex> lock { mutex() }; if (auto p = parameter(Constants::NahdXml::xmlKeySynthMultiShape().toStdString()); p) { p->get().setValue(shape); syncParameters(); changed = true; } } if (changed) emit dataChanged(); }
float SynthDevice::multiLevel() const { return m_multiLevel; }
void SynthDevice::setMultiLevel(float level) { bool changed = false; { std::lock_guard<std::recursive_mutex> lock { mutex() }; if (auto p = parameter(Constants::NahdXml::xmlKeySynthMultiLevel().toStdString()); p) { p->get().setValue(level); syncParameters(); changed = true; } } if (changed) emit dataChanged(); }
float SynthDevice::multiKeyTrack() const { return m_multiKeyTrack; }
void SynthDevice::setMultiKeyTrack(float keyTrack) { bool changed = false; { std::lock_guard<std::recursive_mutex> lock { mutex() }; if (auto p = parameter(Constants::NahdXml::xmlKeyMultiKeyTrack().toStdString()); p) { p->get().setValue(keyTrack); syncParameters(); changed = true; } } if (changed) emit dataChanged(); }

// Mixer
float SynthDevice::mixVco1() const { return m_mixVco1; }
void SynthDevice::setMixVco1(float level) { bool changed = false; { std::lock_guard<std::recursive_mutex> lock { mutex() }; if (auto p = parameter(Constants::NahdXml::xmlKeySynthMixLevel1().toStdString()); p) { p->get().setValue(level); syncParameters(); changed = true; } } if (changed) emit dataChanged(); }
float SynthDevice::mixVco2() const { return m_mixVco2; }
void SynthDevice::setMixVco2(float level) { bool changed = false; { std::lock_guard<std::recursive_mutex> lock { mutex() }; if (auto p = parameter(Constants::NahdXml::xmlKeySynthMixLevel2().toStdString()); p) { p->get().setValue(level); syncParameters(); changed = true; } } if (changed) emit dataChanged(); }

// Filter
float SynthDevice::lpfCutoff() const { return m_lpfCutoff; }
void SynthDevice::setLpfCutoff(float cutoff) { bool changed = false; { std::lock_guard<std::recursive_mutex> lock { mutex() }; if (auto p = parameter(Constants::NahdXml::xmlKeySynthLpfCutoff().toStdString()); p) { p->get().setValue(cutoff); m_manualLpfCutoff = p->get().value(); syncParameters(); changed = true; } } if (changed) emit dataChanged(); }
float SynthDevice::lpfResonance() const { return m_lpfResonance; }
void SynthDevice::setLpfResonance(float resonance) { bool changed = false; { std::lock_guard<std::recursive_mutex> lock { mutex() }; if (auto p = parameter(Constants::NahdXml::xmlKeySynthLpfResonance().toStdString()); p) { p->get().setValue(resonance); syncParameters(); changed = true; } } if (changed) emit dataChanged(); }
float SynthDevice::hpfCutoff() const { return m_hpfCutoff; }
void SynthDevice::setHpfCutoff(float cutoff) { bool changed = false; { std::lock_guard<std::recursive_mutex> lock { mutex() }; if (auto p = parameter(Constants::NahdXml::xmlKeySynthHpfCutoff().toStdString()); p) { p->get().setValue(cutoff); m_manualHpfCutoff = p->get().value(); syncParameters(); changed = true; } } if (changed) emit dataChanged(); }
float SynthDevice::filterKeyTrack() const { return m_filterKeyTrack; }
void SynthDevice::setFilterKeyTrack(float track) { bool changed = false; { std::lock_guard<std::recursive_mutex> lock { mutex() }; if (auto p = parameter(Constants::NahdXml::xmlKeyKeyTrack().toStdString()); p) { p->get().setValue(track); syncParameters(); changed = true; } } if (changed) emit dataChanged(); }

// Amp EG
float SynthDevice::ampAttack() const { return m_ampAttack; }
void SynthDevice::setAmpAttack(float a) { bool changed = false; { std::lock_guard<std::recursive_mutex> lock { mutex() }; if (auto p = parameter(Constants::NahdXml::xmlKeySynthAmpAttack().toStdString()); p) { p->get().setValue(a); syncParameters(); changed = true; } } if (changed) emit dataChanged(); }
float SynthDevice::ampDecay() const { return m_ampDecay; }
void SynthDevice::setAmpDecay(float d) { bool changed = false; { std::lock_guard<std::recursive_mutex> lock { mutex() }; if (auto p = parameter(Constants::NahdXml::xmlKeySynthAmpDecay().toStdString()); p) { p->get().setValue(d); syncParameters(); changed = true; } } if (changed) emit dataChanged(); }
float SynthDevice::ampSustain() const { return m_ampSustain; }
void SynthDevice::setAmpSustain(float s) { bool changed = false; { std::lock_guard<std::recursive_mutex> lock { mutex() }; if (auto p = parameter(Constants::NahdXml::xmlKeySynthAmpSustain().toStdString()); p) { p->get().setValue(s); syncParameters(); changed = true; } } if (changed) emit dataChanged(); }
float SynthDevice::ampRelease() const { return m_ampRelease; }
void SynthDevice::setAmpRelease(float r) { bool changed = false; { std::lock_guard<std::recursive_mutex> lock { mutex() }; if (auto p = parameter(Constants::NahdXml::xmlKeySynthAmpRelease().toStdString()); p) { p->get().setValue(r); syncParameters(); changed = true; } } if (changed) emit dataChanged(); }

// Mod EG
float SynthDevice::modAttack() const { return m_modAttack; }
void SynthDevice::setModAttack(float a) { bool changed = false; { std::lock_guard<std::recursive_mutex> lock { mutex() }; if (auto p = parameter(Constants::NahdXml::xmlKeySynthModAttack().toStdString()); p) { p->get().setValue(a); syncParameters(); changed = true; } } if (changed) emit dataChanged(); }
float SynthDevice::modDecay() const { return m_modDecay; }
void SynthDevice::setModDecay(float d) { bool changed = false; { std::lock_guard<std::recursive_mutex> lock { mutex() }; if (auto p = parameter(Constants::NahdXml::xmlKeySynthModDecay().toStdString()); p) { p->get().setValue(d); syncParameters(); changed = true; } } if (changed) emit dataChanged(); }
float SynthDevice::modInt() const { return m_modInt; }
void SynthDevice::setModInt(float intensity) { bool changed = false; { std::lock_guard<std::recursive_mutex> lock { mutex() }; if (auto p = parameter(Constants::NahdXml::xmlKeySynthModIntensity().toStdString()); p) { p->get().setValue(intensity); syncParameters(); changed = true; } } if (changed) emit dataChanged(); }
SynthDevice::ModTarget SynthDevice::modTarget() const { return m_modTarget; }
void SynthDevice::setModTarget(ModTarget target) { bool changed = false; { std::lock_guard<std::recursive_mutex> lock { mutex() }; if (auto p = parameter(Constants::NahdXml::xmlKeySynthModTarget().toStdString()); p) { p->get().setFromXml(static_cast<int>(target)); syncParameters(); changed = true; } } if (changed) emit dataChanged(); }

// LFO
LFO::Waveform SynthDevice::lfoWaveform() const { return m_lfoWaveform; }
void SynthDevice::setLfoWaveform(LFO::Waveform wave) { bool changed = false; { std::lock_guard<std::recursive_mutex> lock { mutex() }; if (auto p = parameter(Constants::NahdXml::xmlKeySynthLfoWaveform().toStdString()); p) { p->get().setFromXml(static_cast<int>(wave)); syncParameters(); changed = true; } } if (changed) emit dataChanged(); }
LFO::Mode SynthDevice::lfoMode() const { return m_lfoMode; }
void SynthDevice::setLfoMode(LFO::Mode mode) { bool changed = false; { std::lock_guard<std::recursive_mutex> lock { mutex() }; if (auto p = parameter(Constants::NahdXml::xmlKeySynthLfoMode().toStdString()); p) { p->get().setFromXml(static_cast<int>(mode)); syncParameters(); changed = true; } } if (changed) emit dataChanged(); }
float SynthDevice::lfoRate() const { return m_lfoRate; }
void SynthDevice::setLfoRate(float rate) { bool changed = false; { std::lock_guard<std::recursive_mutex> lock { mutex() }; if (auto p = parameter(Constants::NahdXml::xmlKeySynthLfoRate().toStdString()); p) { p->get().setValue(rate); syncParameters(); changed = true; } } if (changed) emit dataChanged(); }
float SynthDevice::lfoInt() const { return m_lfoInt; }
void SynthDevice::setLfoInt(float intensity) { bool changed = false; { std::lock_guard<std::recursive_mutex> lock { mutex() }; if (auto p = parameter(Constants::NahdXml::xmlKeySynthLfoIntensity().toStdString()); p) { p->get().setValue(intensity); syncParameters(); changed = true; } } if (changed) emit dataChanged(); }
SynthDevice::LfoTarget SynthDevice::lfoTarget() const { return m_lfoTarget; }
void SynthDevice::setLfoTarget(LfoTarget target) { bool changed = false; { std::lock_guard<std::recursive_mutex> lock { mutex() }; if (auto p = parameter(Constants::NahdXml::xmlKeySynthLfoTarget().toStdString()); p) { p->get().setFromXml(static_cast<int>(target)); syncParameters(); changed = true; } } if (changed) emit dataChanged(); }

// Voice / Global
SynthDevice::VoiceMode SynthDevice::voiceMode() const { return m_voiceMode; }
void SynthDevice::setVoiceMode(VoiceMode mode) { bool changed = false; { std::lock_guard<std::recursive_mutex> lock { mutex() }; if (auto p = parameter(Constants::NahdXml::xmlKeyVoiceMode().toStdString()); p) { p->get().setFromXml(static_cast<int>(mode)); syncParameters(); changed = true; } } if (changed) emit dataChanged(); }
float SynthDevice::voiceDepth() const { return m_voiceDepth; }
void SynthDevice::setVoiceDepth(float depth) { bool changed = false; { std::lock_guard<std::recursive_mutex> lock { mutex() }; if (auto p = parameter(Constants::NahdXml::xmlKeyVoiceDepth().toStdString()); p) { p->get().setValue(depth); syncParameters(); changed = true; } } if (changed) emit dataChanged(); }
float SynthDevice::portamento() const { return m_portamento; }
void SynthDevice::setPortamento(float val) { bool changed = false; { std::lock_guard<std::recursive_mutex> lock { mutex() }; if (auto p = parameter(Constants::NahdXml::xmlKeyPortamento().toStdString()); p) { p->get().setValue(val); syncParameters(); changed = true; } } if (changed) emit dataChanged(); }
float SynthDevice::panSpread() const { return m_panSpread; }
void SynthDevice::setPanSpread(float spread) { bool changed = false; { std::lock_guard<std::recursive_mutex> lock { mutex() }; if (auto p = parameter(Constants::NahdXml::xmlKeyPanSpread().toStdString()); p) { p->get().setValue(spread); syncParameters(); changed = true; } } if (changed) emit dataChanged(); }
void SynthDevice::setPan(float val) { Device::setPan(val); }
void SynthDevice::setVolume(float vol) { Device::setVolume(vol); }
float SynthDevice::gain() const { return Device::gain(); }
void SynthDevice::setGain(float val) { Device::setGain(val); }

// Delay Accessors
DelayEffect::Type SynthDevice::delayType() const { return m_delayType; }
void SynthDevice::setDelayType(DelayEffect::Type type) { bool changed = false; { std::lock_guard<std::recursive_mutex> lock { mutex() }; if (auto p = parameter(Constants::NahdXml::xmlKeyDelayType().toStdString()); p) { p->get().setFromXml(static_cast<int>(type)); syncParameters(); changed = true; } } if (changed) emit dataChanged(); }
float SynthDevice::delayTime() const { return m_delayTime; }
void SynthDevice::setDelayTime(float time) { bool changed = false; { std::lock_guard<std::recursive_mutex> lock { mutex() }; if (auto p = parameter(Constants::NahdXml::xmlKeyDelayTime().toStdString()); p) { p->get().setValue(time); syncParameters(); changed = true; } } if (changed) emit dataChanged(); }
float SynthDevice::delayFeedback() const { return m_delayFeedback; }
void SynthDevice::setDelayFeedback(float fb) { bool changed = false; { std::lock_guard<std::recursive_mutex> lock { mutex() }; if (auto p = parameter(Constants::NahdXml::xmlKeyDelayFeedback().toStdString()); p) { p->get().setValue(fb); syncParameters(); changed = true; } } if (changed) emit dataChanged(); }
float SynthDevice::delayDepth() const { return m_delayDepth; }
void SynthDevice::setDelayDepth(float depth) { bool changed = false; { std::lock_guard<std::recursive_mutex> lock { mutex() }; if (auto p = parameter(Constants::NahdXml::xmlKeyDelayDepth().toStdString()); p) { p->get().setValue(depth); syncParameters(); changed = true; } } if (changed) emit dataChanged(); }
float SynthDevice::delayMix() const { return m_delayMix; }
void SynthDevice::setDelayMix(float mix) { bool changed = false; { std::lock_guard<std::recursive_mutex> lock { mutex() }; if (auto p = parameter(Constants::NahdXml::xmlKeyDelayMix().toStdString()); p) { p->get().setValue(mix); syncParameters(); changed = true; } } if (changed) emit dataChanged(); }
bool SynthDevice::delaySync() const { return m_delaySync; }
void SynthDevice::setDelaySync(bool sync) { bool changed = false; { std::lock_guard<std::recursive_mutex> lock { mutex() }; if (auto p = parameter(Constants::NahdXml::xmlKeyDelaySync().toStdString()); p) { p->get().setValue(sync ? 1.0f : 0.0f); syncParameters(); changed = true; } } if (changed) emit dataChanged(); }
float SynthDevice::delaySyncDivision() const { return m_delaySyncDivision; }
void SynthDevice::setDelaySyncDivision(float division) { bool changed = false; { std::lock_guard<std::recursive_mutex> lock { mutex() }; if (auto p = parameter(Constants::NahdXml::xmlKeyDelaySyncDivision().toStdString()); p) { p->get().setValue(division); syncParameters(); changed = true; } } if (changed) emit dataChanged(); }
float SynthDevice::delayFeedbackLpf() const { return m_delay.feedbackLpf(); }
void SynthDevice::setFeedbackLpf(float cutoff) { bool changed = false; { std::lock_guard<std::recursive_mutex> lock { mutex() }; if (auto p = parameter(Constants::NahdXml::xmlKeyDelayFeedbackLpf().toStdString()); p) { p->get().setValue(cutoff); syncParameters(); changed = true; } } if (changed) emit dataChanged(); }
float SynthDevice::delayFeedbackHpf() const { return m_delay.feedbackHpf(); }
void SynthDevice::setFeedbackHpf(float cutoff) { bool changed = false; { std::lock_guard<std::recursive_mutex> lock { mutex() }; if (auto p = parameter(Constants::NahdXml::xmlKeyDelayFeedbackHpf().toStdString()); p) { p->get().setValue(cutoff); syncParameters(); changed = true; } } if (changed) emit dataChanged(); }

} // namespace noteahead
