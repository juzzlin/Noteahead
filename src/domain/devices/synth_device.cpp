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
#include "synth_presets.hpp"
#include "../../common/constants.hpp"
#include "../../common/utils.hpp"

#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <cmath>
#include <algorithm>

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

    if (phaseSync) {
        vco1.sync(0.0);
        vco2.sync(0.0);
    }
    ampEg.trigger();
    modEg.trigger();
}

void SynthDevice::Voice::release()
{
    ampEg.release();
    modEg.release();
}

SynthDevice::SynthDevice()
{
    // Initialize Parameters
    addParameter(Parameter { "vco1" + Constants::NahdXml::xmlKeyWaveform().toStdString(), 0.5f, 0, 2, 1 });
    addParameter(Parameter { "vco1" + Constants::NahdXml::xmlKeyOctave().toStdString(), 0.333f, -1, 2, 0 });
    addParameter(Parameter { "vco1" + Constants::NahdXml::xmlKeyPitch().toStdString(), 0.5f, -100, 100, 0 });
    addParameter(Parameter { "vco1" + Constants::NahdXml::xmlKeyShape().toStdString(), 0.0f, 0, 100, 0 });
    addParameter(Parameter { "vco1" + Constants::NahdXml::xmlKeySync().toStdString(), 1.0f, 0, 1, 1 });

    addParameter(Parameter { "vco2" + Constants::NahdXml::xmlKeyWaveform().toStdString(), 0.5f, 0, 2, 1 });
    addParameter(Parameter { "vco2" + Constants::NahdXml::xmlKeyOctave().toStdString(), 0.333f, -1, 2, 0 });
    addParameter(Parameter { "vco2" + Constants::NahdXml::xmlKeyPitch().toStdString(), 0.5f, -100, 100, 0 });
    addParameter(Parameter { "vco2" + Constants::NahdXml::xmlKeyShape().toStdString(), 0.0f, 0, 100, 0 });
    addParameter(Parameter { "vco2" + Constants::NahdXml::xmlKeySync().toStdString(), 1.0f, 0, 1, 1 });

    addParameter(Parameter { "mix" + Constants::NahdXml::xmlKeyLevel().toStdString() + "1", 1.0f, 0, 100, 100 });
    addParameter(Parameter { "mix" + Constants::NahdXml::xmlKeyLevel().toStdString() + "2", 0.0f, 0, 100, 0 });

    addParameter(Parameter { "lpf" + Constants::NahdXml::xmlKeyCutoff().toStdString(), 1.0f, 0, 100, 100 });
    addParameter(Parameter { "lpf" + Constants::NahdXml::xmlKeyResonance().toStdString(), 0.0f, 0, 100, 0 });
    addParameter(Parameter { "hpf" + Constants::NahdXml::xmlKeyCutoff().toStdString(), 0.0f, 0, 100, 0 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyKeyTrack().toStdString(), 0.0f, 0, 100, 0 });

    addParameter(Parameter { "amp" + Constants::NahdXml::xmlKeyAttack().toStdString(), 0.1f, 0, 100, 10 });
    addParameter(Parameter { "amp" + Constants::NahdXml::xmlKeyDecay().toStdString(), 0.2f, 0, 100, 20 });
    addParameter(Parameter { "amp" + Constants::NahdXml::xmlKeySustain().toStdString(), 1.0f, 0, 100, 100 });
    addParameter(Parameter { "amp" + Constants::NahdXml::xmlKeyReleaseTime().toStdString(), 0.2f, 0, 100, 20 });

    addParameter(Parameter { "mod" + Constants::NahdXml::xmlKeyAttack().toStdString(), 0.1f, 0, 100, 10 });
    addParameter(Parameter { "mod" + Constants::NahdXml::xmlKeyDecay().toStdString(), 0.2f, 0, 100, 20 });
    addParameter(Parameter { "mod" + Constants::NahdXml::xmlKeyIntensity().toStdString(), 0.0f, 0, 100, 0 });
    addParameter(Parameter { "mod" + Constants::NahdXml::xmlKeyTarget().toStdString(), 1.0f, 0, 2, 2 }); // Cutoff default

    addParameter(Parameter { Constants::NahdXml::xmlKeyVoiceMode().toStdString(), 0.0f, 0, 1, 0 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyVoiceDepth().toStdString(), 0.0f, 0, 100, 0 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyPortamento().toStdString(), 0.0f, 0, 100, 0 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyPanSpread().toStdString(), 0.0f, 0, 100, 0 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyVolume().toStdString(), 1.0f, 0, 100, 100 });

    addParameter(Parameter { Constants::NahdXml::xmlKeyDelayType().toStdString(), 0.0f, 0, 5, 0 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyDelayTime().toStdString(), 0.5f, 0, 2000, 500 }); // 0..2 seconds in ms
    addParameter(Parameter { Constants::NahdXml::xmlKeyDelayFeedback().toStdString(), 0.3f, 0, 100, 30 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyDelayDepth().toStdString(), 0.5f, 0, 100, 50 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyDelayMix().toStdString(), 0.0f, 0, 100, 0 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyDelaySync().toStdString(), 0.0f, 0, 1, 0 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyDelaySyncDivision().toStdString(), 0.25f, 0, 100, 25 });

    for (auto && voice : m_voices) {
        voice.lpf.setMode(CascadedSVF::Mode::LowPass);
        voice.hpf.setMode(CascadedSVF::Mode::HighPass);
    }

    m_manualPanSpread = m_panSpread;
    m_manualMasterVolume = m_masterVolume;
    m_manualLpfCutoff = m_lpfCutoff;
    m_manualHpfCutoff = m_hpfCutoff;

    syncParameters();
}

SynthDevice::~SynthDevice() = default;

std::string SynthDevice::name() const { return "Notealogue"; }
std::string SynthDevice::category() const { return Constants::NahdXml::xmlValueSynths().toStdString(); }

void SynthDevice::processAudio(float * output, uint32_t nFrames, uint32_t sampleRate)
{
    const std::lock_guard<std::mutex> lock(m_mutex);

    std::vector<float> localBuffer(nFrames * 2, 0.0f);

    for (auto && voice : m_voices) {
        if (!voice.active) continue;

        voice.vco1.setSampleRate(sampleRate);
        voice.vco2.setSampleRate(sampleRate);
        voice.lpf.setSampleRate(sampleRate);
        voice.hpf.setSampleRate(sampleRate);
        voice.ampEg.setSampleRate(sampleRate);
        voice.modEg.setSampleRate(sampleRate);

        voice.lpf.setResonance(m_lpfResonance);

        // Portamento constant (simple one-pole)
        const double portamentoCoeff = m_portamento > 0 ? 1.0 - std::pow(0.001, 1.0 / (m_portamento * sampleRate)) : 1.0;

        for (uint32_t i = 0; i < nFrames; i++) {
            // Update glide frequency
            voice.glideFrequency += (voice.frequency - voice.glideFrequency) * portamentoCoeff;

            const double ampEnv = voice.ampEg.nextSample();
            const double modEnv = voice.modEg.nextSample() * m_modInt;

            // Pitch modulation
            double pitchMod1 = (m_modTarget == ModTarget::Pitch1) ? modEnv : 0.0;
            double pitchMod2 = (m_modTarget == ModTarget::Pitch2) ? modEnv : 0.0;

            // VCO Frequencies
            const double vco1Freq = voice.glideFrequency * std::pow(2.0, (m_vco1Octave * 12.0 + m_vco1Pitch + pitchMod1 * 12.0) / 12.0);
            const double vco2Freq = voice.glideFrequency * std::pow(2.0, (m_vco2Octave * 12.0 + m_vco2Pitch + pitchMod2 * 12.0) / 12.0);

            voice.vco1.setFrequency(vco1Freq);
            voice.vco2.setFrequency(vco2Freq);
            voice.vco1.setShape(m_vco1Shape);
            voice.vco2.setShape(m_vco2Shape);

            // VCO2 Hard Sync to VCO1
            double oldPhase = voice.vco1.phase();
            double vco1Val = voice.vco1.nextSample();
            if (m_vco2Sync && voice.vco1.phase() < oldPhase) {
                voice.vco2.sync(0.0);
            }
            double vco2Val = voice.vco2.nextSample();

            double mix = (vco1Val * m_mixVco1) + (vco2Val * m_mixVco2);
            mix *= 0.6; // Safety headroom

            // Filter
            double cutoffMod = (m_modTarget == ModTarget::Cutoff) ? modEnv : 0.0;
            cutoffMod += (voice.note - 60.0) / 127.0 * m_filterKeyTrack;
            
            voice.lpf.setCutoff(std::clamp(m_lpfCutoff + cutoffMod, 0.0, 1.0));
            voice.hpf.setCutoff(m_hpfCutoff);

            float filtered = voice.hpf.process(voice.lpf.process(static_cast<float>(mix)));
            float finalSample = filtered * static_cast<float>(ampEnv) * m_masterVolume * 0.7f; // Global gain reduction

            localBuffer[i * 2] += finalSample * (1.0f - voice.pan);
            localBuffer[i * 2 + 1] += finalSample * voice.pan;
        }

        if (voice.ampEg.state() == ADSREnvelope::State::Idle) {
            voice.active = false;
        }
    }

    // Apply global FX (Delay)
    for (uint32_t i = 0; i < nFrames; i++) {
        float l = localBuffer[i * 2];
        float r = localBuffer[i * 2 + 1];
        m_delay.process(l, r, sampleRate);
        output[i * 2] += l;
        output[i * 2 + 1] += r;
    }
}

void SynthDevice::processMidiNoteOn(uint8_t note, uint8_t velocity)
{
    const std::lock_guard<std::mutex> lock(m_mutex);
    handleNoteOn(note, velocity);
}

void SynthDevice::processMidiNoteOff(uint8_t note)
{
    const std::lock_guard<std::mutex> lock(m_mutex);
    handleNoteOff(note);
}

void SynthDevice::processMidiCc(uint8_t controller, uint8_t value, uint8_t /*channel*/)
{
    std::lock_guard<std::mutex> lock { m_mutex };

    if (controller == 121) { // Reset All Controllers
        m_panSpread = m_manualPanSpread;
        m_masterVolume = m_manualMasterVolume;
        m_lpfCutoff = m_manualLpfCutoff;
        m_hpfCutoff = m_manualHpfCutoff;

        if (auto p = parameter(Constants::NahdXml::xmlKeyPanSpread().toStdString()); p) p->get().setValue(m_panSpread);
        if (auto p = parameter(Constants::NahdXml::xmlKeyVolume().toStdString()); p) p->get().setValue(m_masterVolume);
        if (auto p = parameter("lpf" + Constants::NahdXml::xmlKeyCutoff().toStdString()); p) p->get().setValue(m_lpfCutoff);
        if (auto p = parameter("hpf" + Constants::NahdXml::xmlKeyCutoff().toStdString()); p) p->get().setValue(m_hpfCutoff);

        syncParameters();
        emit dataChanged();
        return;
    }

    const float val = static_cast<float>(value) / 127.0f;

    if (controller == 7) { // Volume
        m_masterVolume = val;
        if (auto p = parameter(Constants::NahdXml::xmlKeyVolume().toStdString()); p) p->get().setValue(val);
        emit dataChanged();
    } else if (controller == 10) { // Panning (using for Pan Spread)
        m_panSpread = val;
        if (auto p = parameter(Constants::NahdXml::xmlKeyPanSpread().toStdString()); p) p->get().setValue(val);
        emit dataChanged();
    } else if (controller == 74) { // Cutoff
        m_lpfCutoff = val;
        if (auto p = parameter("lpf" + Constants::NahdXml::xmlKeyCutoff().toStdString()); p) p->get().setValue(val);
        emit dataChanged();
    } else if (controller == 81) { // HPF Cutoff
        m_hpfCutoff = val;
        if (auto p = parameter("hpf" + Constants::NahdXml::xmlKeyCutoff().toStdString()); p) p->get().setValue(val);
        emit dataChanged();
    }
}
void SynthDevice::processMidiAllNotesOff()
{
    const std::lock_guard<std::mutex> lock(m_mutex);
    for (auto && voice : m_voices) voice.reset();
    m_delay.reset();
}

void SynthDevice::setBpm(float bpm)
{
    const std::lock_guard<std::mutex> lock(m_mutex);
    m_delay.setBpm(bpm);
}

void SynthDevice::reset()
{
    const std::lock_guard<std::mutex> lock(m_mutex);
    ParameterContainer::reset();
    for (auto && voice : m_voices) voice.reset();
    m_delay.reset();
    m_polyNextVoice = 0;
    syncParameters();
}

void SynthDevice::handleNoteOn(uint8_t note, uint8_t velocity)
{
    (void)velocity;
    double freq = midiNoteToFreq(note);
    float spread = m_panSpread * 0.5f;

    if (m_voiceMode == VoiceMode::Poly) {
        // Round robin
        m_voices[m_polyNextVoice].trigger(note, freq, 0.5f + (m_polyNextVoice % 2 == 0 ? spread : -spread), m_vco1Sync);
        m_polyNextVoice = (m_polyNextVoice + 1) % 4;
    } else {
        // Unison
        for (int i = 0; i < 4; i++) {
            double detune = (i - 1.5) * m_voiceDepth * 0.1; // Spread detune
            m_voices[i].trigger(note, freq * std::pow(2.0, detune / 12.0), 0.5f + (i < 2 ? -spread : spread), m_vco1Sync);
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
    if (auto p = parameter("vco1" + Constants::NahdXml::xmlKeyWaveform().toStdString()); p) m_vco1Waveform = static_cast<PolyBLEPOscillator::Waveform>(p->get().xmlValue());
    if (auto p = parameter("vco1" + Constants::NahdXml::xmlKeyOctave().toStdString()); p) m_vco1Octave = p->get().xmlValue();
    if (auto p = parameter("vco1" + Constants::NahdXml::xmlKeyPitch().toStdString()); p) m_vco1Pitch = static_cast<float>(p->get().xmlValue());
    if (auto p = parameter("vco1" + Constants::NahdXml::xmlKeyShape().toStdString()); p) m_vco1Shape = p->get().value();
    if (auto p = parameter("vco1" + Constants::NahdXml::xmlKeySync().toStdString()); p) m_vco1Sync = p->get().xmlValue() > 0;

    if (auto p = parameter("vco2" + Constants::NahdXml::xmlKeyWaveform().toStdString()); p) m_vco2Waveform = static_cast<PolyBLEPOscillator::Waveform>(p->get().xmlValue());
    if (auto p = parameter("vco2" + Constants::NahdXml::xmlKeyOctave().toStdString()); p) m_vco2Octave = p->get().xmlValue();
    if (auto p = parameter("vco2" + Constants::NahdXml::xmlKeyPitch().toStdString()); p) m_vco2Pitch = static_cast<float>(p->get().xmlValue());
    if (auto p = parameter("vco2" + Constants::NahdXml::xmlKeyShape().toStdString()); p) m_vco2Shape = p->get().value();
    if (auto p = parameter("vco2" + Constants::NahdXml::xmlKeySync().toStdString()); p) m_vco2Sync = p->get().xmlValue() > 0;

    if (auto p = parameter("mix" + Constants::NahdXml::xmlKeyLevel().toStdString() + "1"); p) m_mixVco1 = p->get().value();
    if (auto p = parameter("mix" + Constants::NahdXml::xmlKeyLevel().toStdString() + "2"); p) m_mixVco2 = p->get().value();

    if (auto p = parameter("lpf" + Constants::NahdXml::xmlKeyCutoff().toStdString()); p) m_lpfCutoff = p->get().value();
    if (auto p = parameter("lpf" + Constants::NahdXml::xmlKeyResonance().toStdString()); p) m_lpfResonance = p->get().value();
    if (auto p = parameter("hpf" + Constants::NahdXml::xmlKeyCutoff().toStdString()); p) m_hpfCutoff = p->get().value();
    if (auto p = parameter(Constants::NahdXml::xmlKeyKeyTrack().toStdString()); p) m_filterKeyTrack = p->get().value();

    if (auto p = parameter("amp" + Constants::NahdXml::xmlKeyAttack().toStdString()); p) m_ampAttack = p->get().value();
    if (auto p = parameter("amp" + Constants::NahdXml::xmlKeyDecay().toStdString()); p) m_ampDecay = p->get().value();
    if (auto p = parameter("amp" + Constants::NahdXml::xmlKeySustain().toStdString()); p) m_ampSustain = p->get().value();
    if (auto p = parameter("amp" + Constants::NahdXml::xmlKeyReleaseTime().toStdString()); p) m_ampRelease = p->get().value();

    if (auto p = parameter("mod" + Constants::NahdXml::xmlKeyAttack().toStdString()); p) m_modAttack = p->get().value();
    if (auto p = parameter("mod" + Constants::NahdXml::xmlKeyDecay().toStdString()); p) m_modDecay = p->get().value();
    if (auto p = parameter("mod" + Constants::NahdXml::xmlKeyIntensity().toStdString()); p) m_modInt = p->get().value();
    if (auto p = parameter("mod" + Constants::NahdXml::xmlKeyTarget().toStdString()); p) m_modTarget = static_cast<ModTarget>(p->get().xmlValue());

    if (auto p = parameter(Constants::NahdXml::xmlKeyVoiceMode().toStdString()); p) m_voiceMode = static_cast<VoiceMode>(p->get().xmlValue());
    if (auto p = parameter(Constants::NahdXml::xmlKeyVoiceDepth().toStdString()); p) m_voiceDepth = p->get().value();
    if (auto p = parameter(Constants::NahdXml::xmlKeyPortamento().toStdString()); p) m_portamento = p->get().value();
    if (auto p = parameter(Constants::NahdXml::xmlKeyPanSpread().toStdString()); p) m_panSpread = p->get().value();
    if (auto p = parameter(Constants::NahdXml::xmlKeyVolume().toStdString()); p) m_masterVolume = p->get().value();

    if (auto p = parameter(Constants::NahdXml::xmlKeyDelayType().toStdString()); p) m_delayType = static_cast<DelayEffect::Type>(p->get().xmlValue());
    if (auto p = parameter(Constants::NahdXml::xmlKeyDelayTime().toStdString()); p) m_delayTime = p->get().value() * 2.0f; 
    if (auto p = parameter(Constants::NahdXml::xmlKeyDelayFeedback().toStdString()); p) m_delayFeedback = p->get().value();
    if (auto p = parameter(Constants::NahdXml::xmlKeyDelayDepth().toStdString()); p) m_delayDepth = p->get().value();
    if (auto p = parameter(Constants::NahdXml::xmlKeyDelayMix().toStdString()); p) m_delayMix = p->get().value();
    if (auto p = parameter(Constants::NahdXml::xmlKeyDelaySync().toStdString()); p) m_delaySync = p->get().xmlValue() > 0;
    if (auto p = parameter(Constants::NahdXml::xmlKeyDelaySyncDivision().toStdString()); p) m_delaySyncDivision = p->get().value();

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
        voice.ampEg.setAttackTime(m_ampAttack);
        voice.ampEg.setDecayTime(m_ampDecay);
        voice.ampEg.setSustainLevel(m_ampSustain);
        voice.ampEg.setReleaseTime(m_ampRelease);
        voice.modEg.setAttackTime(m_modAttack);
        voice.modEg.setDecayTime(m_modDecay);
        voice.modEg.setSustainLevel(0.0); // Mod EG is AD only
        voice.modEg.setReleaseTime(m_modDecay);
    }
}

void SynthDevice::serializeToXml(QXmlStreamWriter & writer) const
{
    const std::lock_guard<std::mutex> lock(m_mutex);
    writer.writeStartElement(Constants::NahdXml::xmlKeyDevice());
    serializeAttributesToXml(writer);
    serializeParametersToXml(writer);
    writer.writeEndElement();
}

void SynthDevice::deserializeFromXml(QXmlStreamReader & reader)
{
    const std::lock_guard<std::mutex> lock(m_mutex);
    deserializeAttributesFromXml(reader);
    while (reader.readNextStartElement()) {
        if (reader.name() == Constants::NahdXml::xmlKeyParameter()) {
            const auto paramName = reader.attributes().value(Constants::NahdXml::xmlKeyName()).toString().toStdString();
            const auto value = reader.attributes().value(Constants::NahdXml::xmlKeyValue()).toInt();
            if (auto p = parameter(paramName); p) p->get().setFromXml(value);
            reader.skipCurrentElement();
        } else {
            reader.skipCurrentElement();
        }
    }
    syncParameters();
}

void SynthDevice::loadPreset(int index)
{
    const std::lock_guard<std::mutex> lock(m_mutex);
    
    const auto& presets = SynthPresets::presets();
    if (index < 0 || index >= static_cast<int>(presets.size())) return;

    ParameterContainer::reset();

    for (auto && [name, val] : presets[index].parameters) {
        if (auto p = parameter(name); p) {
            if (name.find("waveform") != std::string::npos || name.find("octave") != std::string::npos || 
                name.find("sync") != std::string::npos || name.find("target") != std::string::npos ||
                name.find("Mode") != std::string::npos || name.find("pitch") != std::string::npos ||
                name.find("delayType") != std::string::npos) {
                p->get().setFromXml(static_cast<int>(val));
            } else {
                p->get().setValue(val);
            }
        }
    }

    syncParameters();
    emit dataChanged();
}

// Accessors (VCO1)
PolyBLEPOscillator::Waveform SynthDevice::vco1Waveform() const { return m_vco1Waveform; }
void SynthDevice::setVco1Waveform(PolyBLEPOscillator::Waveform wave) { if (auto p = parameter("vco1" + Constants::NahdXml::xmlKeyWaveform().toStdString()); p) { p->get().setFromXml(static_cast<int>(wave)); syncParameters(); emit dataChanged(); } }
int SynthDevice::vco1Octave() const { return m_vco1Octave; }
void SynthDevice::setVco1Octave(int octave) { if (auto p = parameter("vco1" + Constants::NahdXml::xmlKeyOctave().toStdString()); p) { p->get().setFromXml(octave); syncParameters(); emit dataChanged(); } }
float SynthDevice::vco1Pitch() const { return m_vco1Pitch; }
void SynthDevice::setVco1Pitch(float pitch) { if (auto p = parameter("vco1" + Constants::NahdXml::xmlKeyPitch().toStdString()); p) { p->get().setFromXml(static_cast<int>(pitch)); syncParameters(); emit dataChanged(); } }
float SynthDevice::vco1Shape() const { return m_vco1Shape; }
void SynthDevice::setVco1Shape(float shape) { if (auto p = parameter("vco1" + Constants::NahdXml::xmlKeyShape().toStdString()); p) { p->get().setValue(shape); syncParameters(); emit dataChanged(); } }
bool SynthDevice::vco1Sync() const { return m_vco1Sync; }
void SynthDevice::setVco1Sync(bool sync) { if (auto p = parameter("vco1" + Constants::NahdXml::xmlKeySync().toStdString()); p) { p->get().setFromXml(sync ? 1 : 0); syncParameters(); emit dataChanged(); } }

// Accessors (VCO2)
PolyBLEPOscillator::Waveform SynthDevice::vco2Waveform() const { return m_vco2Waveform; }
void SynthDevice::setVco2Waveform(PolyBLEPOscillator::Waveform wave) { if (auto p = parameter("vco2" + Constants::NahdXml::xmlKeyWaveform().toStdString()); p) { p->get().setFromXml(static_cast<int>(wave)); syncParameters(); emit dataChanged(); } }
int SynthDevice::vco2Octave() const { return m_vco2Octave; }
void SynthDevice::setVco2Octave(int octave) { if (auto p = parameter("vco2" + Constants::NahdXml::xmlKeyOctave().toStdString()); p) { p->get().setFromXml(octave); syncParameters(); emit dataChanged(); } }
float SynthDevice::vco2Pitch() const { return m_vco2Pitch; }
void SynthDevice::setVco2Pitch(float pitch) { if (auto p = parameter("vco2" + Constants::NahdXml::xmlKeyPitch().toStdString()); p) { p->get().setFromXml(static_cast<int>(pitch)); syncParameters(); emit dataChanged(); } }
float SynthDevice::vco2Shape() const { return m_vco2Shape; }
void SynthDevice::setVco2Shape(float shape) { if (auto p = parameter("vco2" + Constants::NahdXml::xmlKeyShape().toStdString()); p) { p->get().setValue(shape); syncParameters(); emit dataChanged(); } }
bool SynthDevice::vco2Sync() const { return m_vco2Sync; }
void SynthDevice::setVco2Sync(bool sync) { if (auto p = parameter("vco2" + Constants::NahdXml::xmlKeySync().toStdString()); p) { p->get().setFromXml(sync ? 1 : 0); syncParameters(); emit dataChanged(); } }

// Mixer
float SynthDevice::mixVco1() const { return m_mixVco1; }
void SynthDevice::setMixVco1(float level) { if (auto p = parameter("mix" + Constants::NahdXml::xmlKeyLevel().toStdString() + "1"); p) { p->get().setValue(level); syncParameters(); emit dataChanged(); } }
float SynthDevice::mixVco2() const { return m_mixVco2; }
void SynthDevice::setMixVco2(float level) { if (auto p = parameter("mix" + Constants::NahdXml::xmlKeyLevel().toStdString() + "2"); p) { p->get().setValue(level); syncParameters(); emit dataChanged(); } }

// Filter
float SynthDevice::lpfCutoff() const { return m_lpfCutoff; }
void SynthDevice::setLpfCutoff(float cutoff) { if (auto p = parameter("lpf" + Constants::NahdXml::xmlKeyCutoff().toStdString()); p) { p->get().setValue(cutoff); m_manualLpfCutoff = p->get().value(); syncParameters(); emit dataChanged(); } }
float SynthDevice::lpfResonance() const { return m_lpfResonance; }
void SynthDevice::setLpfResonance(float resonance) { if (auto p = parameter("lpf" + Constants::NahdXml::xmlKeyResonance().toStdString()); p) { p->get().setValue(resonance); syncParameters(); emit dataChanged(); } }
float SynthDevice::hpfCutoff() const { return m_hpfCutoff; }
void SynthDevice::setHpfCutoff(float cutoff) { if (auto p = parameter("hpf" + Constants::NahdXml::xmlKeyCutoff().toStdString()); p) { p->get().setValue(cutoff); m_manualHpfCutoff = p->get().value(); syncParameters(); emit dataChanged(); } }
float SynthDevice::filterKeyTrack() const { return m_filterKeyTrack; }
void SynthDevice::setFilterKeyTrack(float track) { if (auto p = parameter(Constants::NahdXml::xmlKeyKeyTrack().toStdString()); p) { p->get().setValue(track); syncParameters(); emit dataChanged(); } }

// Amp EG
float SynthDevice::ampAttack() const { return m_ampAttack; }
void SynthDevice::setAmpAttack(float a) { if (auto p = parameter("amp" + Constants::NahdXml::xmlKeyAttack().toStdString()); p) { p->get().setValue(a); syncParameters(); emit dataChanged(); } }
float SynthDevice::ampDecay() const { return m_ampDecay; }
void SynthDevice::setAmpDecay(float d) { if (auto p = parameter("amp" + Constants::NahdXml::xmlKeyDecay().toStdString()); p) { p->get().setValue(d); syncParameters(); emit dataChanged(); } }
float SynthDevice::ampSustain() const { return m_ampSustain; }
void SynthDevice::setAmpSustain(float s) { if (auto p = parameter("amp" + Constants::NahdXml::xmlKeySustain().toStdString()); p) { p->get().setValue(s); syncParameters(); emit dataChanged(); } }
float SynthDevice::ampRelease() const { return m_ampRelease; }
void SynthDevice::setAmpRelease(float r) { if (auto p = parameter("amp" + Constants::NahdXml::xmlKeyReleaseTime().toStdString()); p) { p->get().setValue(r); syncParameters(); emit dataChanged(); } }

// Mod EG
float SynthDevice::modAttack() const { return m_modAttack; }
void SynthDevice::setModAttack(float a) { if (auto p = parameter("mod" + Constants::NahdXml::xmlKeyAttack().toStdString()); p) { p->get().setValue(a); syncParameters(); emit dataChanged(); } }
float SynthDevice::modDecay() const { return m_modDecay; }
void SynthDevice::setModDecay(float d) { if (auto p = parameter("mod" + Constants::NahdXml::xmlKeyDecay().toStdString()); p) { p->get().setValue(d); syncParameters(); emit dataChanged(); } }
float SynthDevice::modInt() const { return m_modInt; }
void SynthDevice::setModInt(float intensity) { if (auto p = parameter("mod" + Constants::NahdXml::xmlKeyIntensity().toStdString()); p) { p->get().setValue(intensity); syncParameters(); emit dataChanged(); } }
SynthDevice::ModTarget SynthDevice::modTarget() const { return m_modTarget; }
void SynthDevice::setModTarget(ModTarget target) { if (auto p = parameter("mod" + Constants::NahdXml::xmlKeyTarget().toStdString()); p) { p->get().setFromXml(static_cast<int>(target)); syncParameters(); emit dataChanged(); } }

// Voice / Global
SynthDevice::VoiceMode SynthDevice::voiceMode() const { return m_voiceMode; }
void SynthDevice::setVoiceMode(VoiceMode mode) { if (auto p = parameter(Constants::NahdXml::xmlKeyVoiceMode().toStdString()); p) { p->get().setFromXml(static_cast<int>(mode)); syncParameters(); emit dataChanged(); } }
float SynthDevice::voiceDepth() const { return m_voiceDepth; }
void SynthDevice::setVoiceDepth(float depth) { if (auto p = parameter(Constants::NahdXml::xmlKeyVoiceDepth().toStdString()); p) { p->get().setValue(depth); syncParameters(); emit dataChanged(); } }
float SynthDevice::portamento() const { return m_portamento; }
void SynthDevice::setPortamento(float val) { if (auto p = parameter(Constants::NahdXml::xmlKeyPortamento().toStdString()); p) { p->get().setValue(val); syncParameters(); emit dataChanged(); } }
float SynthDevice::panSpread() const { return m_panSpread; }
void SynthDevice::setPanSpread(float spread) { if (auto p = parameter(Constants::NahdXml::xmlKeyPanSpread().toStdString()); p) { p->get().setValue(spread); m_manualPanSpread = p->get().value(); syncParameters(); emit dataChanged(); } }
float SynthDevice::masterVolume() const { return m_masterVolume; }
void SynthDevice::setMasterVolume(float vol) { if (auto p = parameter(Constants::NahdXml::xmlKeyVolume().toStdString()); p) { p->get().setValue(vol); m_manualMasterVolume = p->get().value(); syncParameters(); emit dataChanged(); } }

// Delay Accessors
DelayEffect::Type SynthDevice::delayType() const { return m_delayType; }
void SynthDevice::setDelayType(DelayEffect::Type type) { if (auto p = parameter(Constants::NahdXml::xmlKeyDelayType().toStdString()); p) { p->get().setFromXml(static_cast<int>(type)); syncParameters(); emit dataChanged(); } }
float SynthDevice::delayTime() const { return m_delayTime; }
void SynthDevice::setDelayTime(float time) { if (auto p = parameter(Constants::NahdXml::xmlKeyDelayTime().toStdString()); p) { p->get().setValue(time / 2.0f); syncParameters(); emit dataChanged(); } }
float SynthDevice::delayFeedback() const { return m_delayFeedback; }
void SynthDevice::setDelayFeedback(float fb) { if (auto p = parameter(Constants::NahdXml::xmlKeyDelayFeedback().toStdString()); p) { p->get().setValue(fb); syncParameters(); emit dataChanged(); } }
float SynthDevice::delayDepth() const { return m_delayDepth; }
void SynthDevice::setDelayDepth(float depth) { if (auto p = parameter(Constants::NahdXml::xmlKeyDelayDepth().toStdString()); p) { p->get().setValue(depth); syncParameters(); emit dataChanged(); } }
float SynthDevice::delayMix() const { return m_delayMix; }
void SynthDevice::setDelayMix(float mix) { if (auto p = parameter(Constants::NahdXml::xmlKeyDelayMix().toStdString()); p) { p->get().setValue(mix); syncParameters(); emit dataChanged(); } }
bool SynthDevice::delaySync() const { return m_delaySync; }
void SynthDevice::setDelaySync(bool sync) { if (auto p = parameter(Constants::NahdXml::xmlKeyDelaySync().toStdString()); p) { p->get().setFromXml(sync ? 1 : 0); syncParameters(); emit dataChanged(); } }
float SynthDevice::delaySyncDivision() const { return m_delaySyncDivision; }
void SynthDevice::setDelaySyncDivision(float division) { if (auto p = parameter(Constants::NahdXml::xmlKeyDelaySyncDivision().toStdString()); p) { p->get().setValue(division); syncParameters(); emit dataChanged(); } }

} // namespace noteahead
