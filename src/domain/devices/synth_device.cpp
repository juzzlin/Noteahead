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

#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <cmath>
#include <algorithm>

namespace noteahead {

SynthDevice::Oscillator::Oscillator()
{
    addParameter(Parameter { Constants::NahdXml::xmlKeyWaveform().toStdString(), 0.5f, 0, 2, 1 }); // Saw as default
    addParameter(Parameter { Constants::NahdXml::xmlKeyLevel().toStdString(), 1.0f, 0, 100, 100 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyDetune().toStdString(), 0.5f, -100, 100, 0 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyOctave().toStdString(), 0.333f, -1, 2, 0 }); // 0.333f internal -> 0 in [-1, 2]
}

SynthDevice::SynthDevice()
{
    addParameter(Parameter { Constants::NahdXml::xmlKeyKeyAssignMode().toStdString(), 1.0f, 0, 2, 2 }); // Poly default
    addParameter(Parameter { Constants::NahdXml::xmlKeyPulseWidth().toStdString(), 0.5f, 0, 100, 50 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyDetune().toStdString(), 0.0f, 0, 100, 0 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyFreqModAmount().toStdString(), 0.0f, 0, 100, 0 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyFreqModSource().toStdString(), 0.0f, 0, 1, 0 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyCutoff().toStdString(), 0.5f, 0, 100, 50 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyResonance().toStdString(), 0.0f, 0, 100, 0 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyEnvAmount().toStdString(), 0.5f, 0, 100, 50 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyKeyTrack().toStdString(), 0.0f, 0, 100, 0 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyFilterModAmount().toStdString(), 0.1f, 0, 100, 10 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyVolume().toStdString(), 1.0f, 0, 100, 100 });
    
    addParameter(Parameter { Constants::NahdXml::xmlKeyMgFrequency().toStdString() + "1", 0.5f, 0, 100, 50 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyMgFrequency().toStdString() + "2", 0.2f, 0, 100, 20 });

    addParameter(Parameter { "vca" + Constants::NahdXml::xmlKeyAttack().toStdString(), 0.1f, 0, 100, 10 });
    addParameter(Parameter { "vca" + Constants::NahdXml::xmlKeyDecay().toStdString(), 0.2f, 0, 100, 20 });
    addParameter(Parameter { "vca" + Constants::NahdXml::xmlKeySustain().toStdString(), 1.0f, 0, 100, 100 });
    addParameter(Parameter { "vca" + Constants::NahdXml::xmlKeyRelease().toStdString(), 0.2f, 0, 100, 20 });

    addParameter(Parameter { "vcf" + Constants::NahdXml::xmlKeyAttack().toStdString(), 0.1f, 0, 100, 10 });
    addParameter(Parameter { "vcf" + Constants::NahdXml::xmlKeyDecay().toStdString(), 0.2f, 0, 100, 20 });
    addParameter(Parameter { "vcf" + Constants::NahdXml::xmlKeySustain().toStdString(), 1.0f, 0, 100, 100 });
    addParameter(Parameter { "vcf" + Constants::NahdXml::xmlKeyRelease().toStdString(), 0.2f, 0, 100, 20 });

    for (auto && osc : m_oscParams) {
        osc = std::make_unique<Oscillator>();
    }

    syncParameters();
}

SynthDevice::~SynthDevice() = default;

std::string SynthDevice::name() const
{
    return Constants::synthDeviceName().toStdString();
}

std::string SynthDevice::category() const
{
    return Constants::NahdXml::xmlValueSynths().toStdString();
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

void SynthDevice::processMidiCc(uint8_t /*controller*/, uint8_t /*value*/, uint8_t /*channel*/)
{
}

void SynthDevice::processMidiAllNotesOff()
{
    const std::lock_guard<std::mutex> lock(m_mutex);
    m_heldNotes.clear();
    for (size_t i = 0; i < m_oscillators.size(); i++) {
        m_oscillatorActive[i] = false;
    }
    m_vcaEnvelope.reset();
    m_vcfEnvelope.reset();
    m_filter.reset();
}

void SynthDevice::processAudio(float * output, uint32_t nFrames, uint32_t sampleRate)
{
    const std::lock_guard<std::mutex> lock(m_mutex);

    m_vcaEnvelope.setSampleRate(sampleRate);
    m_vcfEnvelope.setSampleRate(sampleRate);
    m_filter.setSampleRate(sampleRate);
    m_mg1.setSampleRate(sampleRate);
    m_mg2.setSampleRate(sampleRate);

    m_vcaEnvelope.setAttackTime(m_vcaAttack);
    m_vcaEnvelope.setDecayTime(m_vcaDecay);
    m_vcaEnvelope.setSustainLevel(m_vcaSustain);
    m_vcaEnvelope.setReleaseTime(m_vcaRelease);

    m_vcfEnvelope.setAttackTime(m_vcfAttack);
    m_vcfEnvelope.setDecayTime(m_vcfDecay);
    m_vcfEnvelope.setSustainLevel(m_vcfSustain);
    m_vcfEnvelope.setReleaseTime(m_vcfRelease);

    m_filter.setResonance(m_filterResonance);
    m_mg1.setFrequency(m_mg1Frequency * 10.0);
    m_mg2.setFrequency(m_mg2Frequency * 10.0);

    for (size_t j = 0; j < m_oscillators.size(); j++) {
        m_oscillators[j].setSampleRate(sampleRate);
        m_oscillators[j].setWaveform(m_oscParams[j]->waveform);
        m_oscillators[j].setPulseWidth(m_pulseWidth);
    }

    const float globalVolumeScale = m_volume * static_cast<float>(m_velocityFactor);

    for (uint32_t i = 0; i < nFrames; i++) {
        const double mg1Val = m_mg1.nextSample();
        const double mg2Val = m_mg2.nextSample();
        const double vcfEnv = m_vcfEnvelope.nextSample();
        const double vcaEnv = m_vcaEnvelope.nextSample();
        
        const double freqModVal = (m_freqModSource == FreqModSource::Mg1) ? mg1Val : vcfEnv;
        updateOscillatorFrequencies(static_cast<double>(sampleRate), freqModVal);

        double mix = 0.0;
        for (size_t j = 0; j < m_oscillators.size(); j++) {
            if (m_oscillatorActive[j]) {
                mix += m_oscillators[j].nextSample() * m_oscParams[j]->level;
            }
        }

        double cutoffMod = (vcfEnv * m_filterEnvAmount) + (mg2Val * m_filterModAmount);
        
        if (!m_heldNotes.empty()) {
            cutoffMod += (m_heldNotes.back() - 60.0) / 127.0 * m_filterKeyTrack;
        }

        m_filter.setCutoff(std::clamp(m_filterCutoff + cutoffMod, 0.0, 1.0));

        float filtered = m_filter.process(static_cast<float>(mix));
        float finalSample = filtered * static_cast<float>(vcaEnv) * globalVolumeScale;

        output[i * 2] += finalSample;
        output[i * 2 + 1] += finalSample;
    }
}

void SynthDevice::reset()
{
    const std::lock_guard<std::mutex> lock(m_mutex);
    ParameterContainer::reset();
    for (auto && osc : m_oscParams) {
        if (osc) {
            osc->reset();
        }
    }
    syncParameters();
    m_heldNotes.clear();
    for (auto && active : m_oscillatorActive) {
        active = false;
    }
    emit dataChanged();
}

void SynthDevice::updateOscillatorFrequencies(double /*sampleRate*/, double modValue)
{
    const double pitchModSemitones = modValue * m_freqModAmount * 12.0;

    for (size_t i = 0; i < m_oscillators.size(); i++) {
        if (m_oscillatorActive[i]) {
            const double oscTuneSemitones = m_oscParams[i]->tune;
            const double spread = static_cast<double>(i) * m_detune;
            const double totalSemitones = (m_oscParams[i]->octave * 12.0) + spread + oscTuneSemitones + pitchModSemitones;
            m_oscillators[i].setFrequency(m_oscillatorBaseFreqs[i] * std::pow(2.0, totalSemitones / 12.0));
        }
    }
}

void SynthDevice::handleNoteOn(uint8_t note, uint8_t velocity)
{
    m_heldNotes.push_back(note);
    m_velocityFactor = velocity / 127.0;

    if (m_keyAssignMode == KeyAssignMode::Unison) {
        for (size_t i = 0; i < m_oscillators.size(); i++) {
            m_oscillatorNotes[i] = note;
            m_oscillatorBaseFreqs[i] = midiNoteToFreq(note);
            m_oscillatorActive[i] = true;
        }
        m_vcaEnvelope.trigger();
        m_vcfEnvelope.trigger();
    } else if (m_keyAssignMode == KeyAssignMode::Poly) {
        int oscIndex = -1;
        for (size_t i = 0; i < m_oscillators.size(); i++) {
            if (!m_oscillatorActive[i]) {
                oscIndex = static_cast<int>(i);
                break;
            }
        }

        if (oscIndex == -1) {
            oscIndex = m_polyNextOsc;
            m_polyNextOsc = (m_polyNextOsc + 1) % static_cast<int>(m_oscillators.size());
        }

        m_oscillatorNotes[oscIndex] = note;
        m_oscillatorBaseFreqs[oscIndex] = midiNoteToFreq(note);
        m_oscillatorActive[oscIndex] = true;
        
        if (m_heldNotes.size() == 1) {
            m_vcaEnvelope.trigger();
            m_vcfEnvelope.trigger();
        }
    } else if (m_keyAssignMode == KeyAssignMode::UnisonShare) {
        const size_t numHeld = m_heldNotes.size();
        for (size_t i = 0; i < m_oscillators.size(); i++) {
            m_oscillatorNotes[i] = m_heldNotes[i % numHeld];
            m_oscillatorBaseFreqs[i] = midiNoteToFreq(m_oscillatorNotes[i]);
            m_oscillatorActive[i] = true;
        }
        if (numHeld == 1) {
            m_vcaEnvelope.trigger();
            m_vcfEnvelope.trigger();
        }
    }
}

void SynthDevice::handleNoteOff(uint8_t note)
{
    auto it = std::find(m_heldNotes.begin(), m_heldNotes.end(), note);
    if (it != m_heldNotes.end()) {
        m_heldNotes.erase(it);
    }

    if (m_heldNotes.empty()) {
        m_vcaEnvelope.release();
        m_vcfEnvelope.release();
    } else {
        if (m_keyAssignMode == KeyAssignMode::UnisonShare) {
            const size_t numHeld = m_heldNotes.size();
            for (size_t i = 0; i < m_oscillators.size(); i++) {
                m_oscillatorNotes[i] = m_heldNotes[i % numHeld];
                m_oscillatorBaseFreqs[i] = midiNoteToFreq(m_oscillatorNotes[i]);
            }
        }
        if (m_keyAssignMode == KeyAssignMode::Poly) {
             for (size_t i = 0; i < m_oscillators.size(); i++) {
                if (m_oscillatorNotes[i] == note) {
                    m_oscillatorActive[i] = false;
                }
            }
        }
    }
}

double SynthDevice::midiNoteToFreq(uint8_t note) const
{
    return 440.0 * std::pow(2.0, (note - 69.0) / 12.0);
}

SynthDevice::KeyAssignMode SynthDevice::keyAssignMode() const
{
    const std::lock_guard<std::mutex> lock(m_mutex);
    return m_keyAssignMode;
}

void SynthDevice::setKeyAssignMode(KeyAssignMode mode)
{
    const std::lock_guard<std::mutex> lock(m_mutex);
    if (auto p = parameter(Constants::NahdXml::xmlKeyKeyAssignMode().toStdString()); p) {
        p->get().setFromXml(static_cast<int>(mode));
        m_keyAssignMode = static_cast<KeyAssignMode>(p->get().xmlValue());
    }
    emit dataChanged();
}

PolyBLEPOscillator::Waveform SynthDevice::oscWaveform(int index) const
{
    const std::lock_guard<std::mutex> lock(m_mutex);
    return m_oscParams.at(index)->waveform;
}

void SynthDevice::setOscWaveform(int index, PolyBLEPOscillator::Waveform waveform)
{
    const std::lock_guard<std::mutex> lock(m_mutex);
    auto & osc = m_oscParams.at(index);
    if (auto p = osc->parameter(Constants::NahdXml::xmlKeyWaveform().toStdString()); p) {
        p->get().setFromXml(static_cast<int>(waveform));
        osc->waveform = static_cast<PolyBLEPOscillator::Waveform>(p->get().xmlValue());
    }
    emit dataChanged();
}

float SynthDevice::oscLevel(int index) const
{
    const std::lock_guard<std::mutex> lock(m_mutex);
    return m_oscParams.at(index)->level;
}

void SynthDevice::setOscLevel(int index, float level)
{
    const std::lock_guard<std::mutex> lock(m_mutex);
    auto & osc = m_oscParams.at(index);
    if (auto p = osc->parameter(Constants::NahdXml::xmlKeyLevel().toStdString()); p) {
        p->get().setValue(level);
        osc->level = p->get().value();
    }
    emit dataChanged();
}

float SynthDevice::oscTune(int index) const
{
    const std::lock_guard<std::mutex> lock(m_mutex);
    return m_oscParams.at(index)->tune;
}

void SynthDevice::setOscTune(int index, float tune)
{
    const std::lock_guard<std::mutex> lock(m_mutex);
    auto & osc = m_oscParams.at(index);
    if (auto p = osc->parameter(Constants::NahdXml::xmlKeyDetune().toStdString()); p) {
        p->get().setFromXml(static_cast<int>(tune));
        osc->tune = static_cast<float>(p->get().xmlValue());
    }
    emit dataChanged();
}

int SynthDevice::oscOctave(int index) const
{
    const std::lock_guard<std::mutex> lock(m_mutex);
    return m_oscParams.at(index)->octave;
}

void SynthDevice::setOscOctave(int index, int octave)
{
    const std::lock_guard<std::mutex> lock(m_mutex);
    auto & osc = m_oscParams.at(index);
    if (auto p = osc->parameter(Constants::NahdXml::xmlKeyOctave().toStdString()); p) {
        p->get().setFromXml(octave);
        osc->octave = p->get().xmlValue();
    }
    emit dataChanged();
}

float SynthDevice::pulseWidth() const
{
    const std::lock_guard<std::mutex> lock(m_mutex);
    return m_pulseWidth;
}

void SynthDevice::setPulseWidth(float pw)
{
    const std::lock_guard<std::mutex> lock(m_mutex);
    if (auto p = parameter(Constants::NahdXml::xmlKeyPulseWidth().toStdString()); p) {
        p->get().setValue(pw);
        m_pulseWidth = p->get().value();
    }
    emit dataChanged();
}

float SynthDevice::filterCutoff() const
{
    const std::lock_guard<std::mutex> lock(m_mutex);
    return m_filterCutoff;
}

void SynthDevice::setFilterCutoff(float cutoff)
{
    const std::lock_guard<std::mutex> lock(m_mutex);
    if (auto p = parameter(Constants::NahdXml::xmlKeyCutoff().toStdString()); p) {
        p->get().setValue(cutoff);
        m_filterCutoff = p->get().value();
    }
    emit dataChanged();
}

float SynthDevice::filterResonance() const
{
    const std::lock_guard<std::mutex> lock(m_mutex);
    return m_filterResonance;
}

void SynthDevice::setFilterResonance(float resonance)
{
    const std::lock_guard<std::mutex> lock(m_mutex);
    if (auto p = parameter(Constants::NahdXml::xmlKeyResonance().toStdString()); p) {
        p->get().setValue(resonance);
        m_filterResonance = p->get().value();
    }
    emit dataChanged();
}

float SynthDevice::filterEnvAmount() const
{
    const std::lock_guard<std::mutex> lock(m_mutex);
    return m_filterEnvAmount;
}

void SynthDevice::setFilterEnvAmount(float amount)
{
    const std::lock_guard<std::mutex> lock(m_mutex);
    if (auto p = parameter(Constants::NahdXml::xmlKeyEnvAmount().toStdString()); p) {
        p->get().setValue((amount + 1.0f) / 2.0f); // Map -1..1 to 0..1
        m_filterEnvAmount = (p->get().value() * 2.0f) - 1.0f;
    }
    emit dataChanged();
}

float SynthDevice::filterKeyTrack() const
{
    const std::lock_guard<std::mutex> lock(m_mutex);
    return m_filterKeyTrack;
}

void SynthDevice::setFilterKeyTrack(float track)
{
    const std::lock_guard<std::mutex> lock(m_mutex);
    if (auto p = parameter(Constants::NahdXml::xmlKeyKeyTrack().toStdString()); p) {
        p->get().setValue(track);
        m_filterKeyTrack = p->get().value();
    }
    emit dataChanged();
}

float SynthDevice::filterModAmount() const
{
    const std::lock_guard<std::mutex> lock(m_mutex);
    return m_filterModAmount;
}

void SynthDevice::setFilterModAmount(float amount)
{
    const std::lock_guard<std::mutex> lock(m_mutex);
    if (auto p = parameter(Constants::NahdXml::xmlKeyFilterModAmount().toStdString()); p) {
        p->get().setValue(amount);
        m_filterModAmount = p->get().value();
    }
    emit dataChanged();
}

float SynthDevice::volume() const
{
    const std::lock_guard<std::mutex> lock(m_mutex);
    return m_volume;
}

void SynthDevice::setVolume(float volume)
{
    const std::lock_guard<std::mutex> lock(m_mutex);
    if (auto p = parameter(Constants::NahdXml::xmlKeyVolume().toStdString()); p) {
        p->get().setValue(volume);
        m_volume = p->get().value();
    }
    emit dataChanged();
}

float SynthDevice::vcaAttack() const
{
    const std::lock_guard<std::mutex> lock(m_mutex);
    return m_vcaAttack;
}

void SynthDevice::setVcaAttack(float a)
{
    const std::lock_guard<std::mutex> lock(m_mutex);
    if (auto p = parameter("vca" + Constants::NahdXml::xmlKeyAttack().toStdString()); p) {
        p->get().setValue(a);
        m_vcaAttack = p->get().value();
    }
    emit dataChanged();
}

float SynthDevice::vcaDecay() const
{
    const std::lock_guard<std::mutex> lock(m_mutex);
    return m_vcaDecay;
}

void SynthDevice::setVcaDecay(float d)
{
    const std::lock_guard<std::mutex> lock(m_mutex);
    if (auto p = parameter("vca" + Constants::NahdXml::xmlKeyDecay().toStdString()); p) {
        p->get().setValue(d);
        m_vcaDecay = p->get().value();
    }
    emit dataChanged();
}

float SynthDevice::vcaSustain() const
{
    const std::lock_guard<std::mutex> lock(m_mutex);
    return m_vcaSustain;
}

void SynthDevice::setVcaSustain(float s)
{
    const std::lock_guard<std::mutex> lock(m_mutex);
    if (auto p = parameter("vca" + Constants::NahdXml::xmlKeySustain().toStdString()); p) {
        p->get().setValue(s);
        m_vcaSustain = p->get().value();
    }
    emit dataChanged();
}

float SynthDevice::vcaRelease() const
{
    const std::lock_guard<std::mutex> lock(m_mutex);
    return m_vcaRelease;
}

void SynthDevice::setVcaRelease(float r)
{
    const std::lock_guard<std::mutex> lock(m_mutex);
    if (auto p = parameter("vca" + Constants::NahdXml::xmlKeyRelease().toStdString()); p) {
        p->get().setValue(r);
        m_vcaRelease = p->get().value();
    }
    emit dataChanged();
}

float SynthDevice::vcfAttack() const
{
    const std::lock_guard<std::mutex> lock(m_mutex);
    return m_vcfAttack;
}

void SynthDevice::setVcfAttack(float a)
{
    const std::lock_guard<std::mutex> lock(m_mutex);
    if (auto p = parameter("vcf" + Constants::NahdXml::xmlKeyAttack().toStdString()); p) {
        p->get().setValue(a);
        m_vcfAttack = p->get().value();
    }
    emit dataChanged();
}

float SynthDevice::vcfDecay() const
{
    const std::lock_guard<std::mutex> lock(m_mutex);
    return m_vcfDecay;
}

void SynthDevice::setVcfDecay(float d)
{
    const std::lock_guard<std::mutex> lock(m_mutex);
    if (auto p = parameter("vcf" + Constants::NahdXml::xmlKeyDecay().toStdString()); p) {
        p->get().setValue(d);
        m_vcfDecay = p->get().value();
    }
    emit dataChanged();
}

float SynthDevice::vcfSustain() const
{
    const std::lock_guard<std::mutex> lock(m_mutex);
    return m_vcfSustain;
}

void SynthDevice::setVcfSustain(float s)
{
    const std::lock_guard<std::mutex> lock(m_mutex);
    if (auto p = parameter("vcf" + Constants::NahdXml::xmlKeySustain().toStdString()); p) {
        p->get().setValue(s);
        m_vcfSustain = p->get().value();
    }
    emit dataChanged();
}

float SynthDevice::vcfRelease() const
{
    const std::lock_guard<std::mutex> lock(m_mutex);
    return m_vcfRelease;
}

void SynthDevice::setVcfRelease(float r)
{
    const std::lock_guard<std::mutex> lock(m_mutex);
    if (auto p = parameter("vcf" + Constants::NahdXml::xmlKeyRelease().toStdString()); p) {
        p->get().setValue(r);
        m_vcfRelease = p->get().value();
    }
    emit dataChanged();
}

void SynthDevice::setVcaAdsr(float a, float d, float s, float r)
{
    setVcaAttack(a);
    setVcaDecay(d);
    setVcaSustain(s);
    setVcaRelease(r);
}

void SynthDevice::setVcfAdsr(float a, float d, float s, float r)
{
    setVcfAttack(a);
    setVcfDecay(d);
    setVcfSustain(s);
    setVcfRelease(r);
}

float SynthDevice::mg1Frequency() const
{
    const std::lock_guard<std::mutex> lock(m_mutex);
    return m_mg1Frequency;
}

void SynthDevice::setMg1Frequency(float freq)
{
    const std::lock_guard<std::mutex> lock(m_mutex);
    if (auto p = parameter(Constants::NahdXml::xmlKeyMgFrequency().toStdString() + "1"); p) {
        p->get().setValue(freq);
        m_mg1Frequency = p->get().value();
    }
    emit dataChanged();
}

float SynthDevice::mg2Frequency() const
{
    const std::lock_guard<std::mutex> lock(m_mutex);
    return m_mg2Frequency;
}

void SynthDevice::setMg2Frequency(float freq)
{
    const std::lock_guard<std::mutex> lock(m_mutex);
    if (auto p = parameter(Constants::NahdXml::xmlKeyMgFrequency().toStdString() + "2"); p) {
        p->get().setValue(freq);
        m_mg2Frequency = p->get().value();
    }
    emit dataChanged();
}

float SynthDevice::detune() const
{
    const std::lock_guard<std::mutex> lock(m_mutex);
    return m_detune;
}

void SynthDevice::setDetune(float detune)
{
    const std::lock_guard<std::mutex> lock(m_mutex);
    if (auto p = parameter(Constants::NahdXml::xmlKeyDetune().toStdString()); p) {
        p->get().setValue(detune);
        m_detune = p->get().value();
    }
    emit dataChanged();
}

float SynthDevice::freqModAmount() const
{
    const std::lock_guard<std::mutex> lock(m_mutex);
    return m_freqModAmount;
}

void SynthDevice::setFreqModAmount(float amount)
{
    const std::lock_guard<std::mutex> lock(m_mutex);
    if (auto p = parameter(Constants::NahdXml::xmlKeyFreqModAmount().toStdString()); p) {
        p->get().setValue(amount);
        m_freqModAmount = p->get().value();
    }
    emit dataChanged();
}

SynthDevice::FreqModSource SynthDevice::freqModSource() const
{
    const std::lock_guard<std::mutex> lock(m_mutex);
    return m_freqModSource;
}

void SynthDevice::setFreqModSource(FreqModSource source)
{
    const std::lock_guard<std::mutex> lock(m_mutex);
    if (auto p = parameter(Constants::NahdXml::xmlKeyFreqModSource().toStdString()); p) {
        p->get().setFromXml(static_cast<int>(source));
        m_freqModSource = static_cast<FreqModSource>(p->get().xmlValue());
    }
    emit dataChanged();
}

void SynthDevice::serializeToXml(QXmlStreamWriter & writer) const
{
    const std::lock_guard<std::mutex> lock(m_mutex);
    writer.writeStartElement(Constants::NahdXml::xmlKeyDevice());
    serializeAttributesToXml(writer);
    serializeParametersToXml(writer);

    for (size_t i = 0; i < m_oscParams.size(); i++) {
        writer.writeStartElement(Constants::NahdXml::xmlKeyOscillator());
        writer.writeAttribute(Constants::NahdXml::xmlKeyIndex(), QString::number(i));
        m_oscParams[i]->serializeParametersToXml(writer);
        writer.writeEndElement();
    }

    writer.writeEndElement(); // Device
}

void SynthDevice::deserializeFromXml(QXmlStreamReader & reader)
{
    const std::lock_guard<std::mutex> lock(m_mutex);
    deserializeAttributesFromXml(reader);

    while (reader.readNextStartElement()) {
        const auto name = reader.name();
        if (name == Constants::NahdXml::xmlKeyParameter()) {
            const auto paramName = reader.attributes().value(Constants::NahdXml::xmlKeyName()).toString().toStdString();
            const auto value = reader.attributes().value(Constants::NahdXml::xmlKeyValue()).toInt();
            if (auto p = parameter(paramName); p) {
                p->get().setFromXml(value);
            }
            reader.skipCurrentElement();
        } else if (name == Constants::NahdXml::xmlKeyOscillator()) {
            const auto index = Utils::Xml::readIntAttribute(reader, Constants::NahdXml::xmlKeyIndex(), false);
            if (index.has_value() && index.value() >= 0 && index.value() < static_cast<int>(m_oscParams.size())) {
                m_oscParams[index.value()]->deserializeParametersFromXml(reader);
            } else {
                reader.skipCurrentElement();
            }
        } else {
            reader.skipCurrentElement();
        }
    }

    syncParameters();
    emit dataChanged();
}

void SynthDevice::syncParameters()
{
    if (auto p = parameter(Constants::NahdXml::xmlKeyKeyAssignMode().toStdString()); p) m_keyAssignMode = static_cast<KeyAssignMode>(p->get().xmlValue());
    if (auto p = parameter(Constants::NahdXml::xmlKeyPulseWidth().toStdString()); p) m_pulseWidth = p->get().value();
    if (auto p = parameter(Constants::NahdXml::xmlKeyDetune().toStdString()); p) m_detune = p->get().value();
    if (auto p = parameter(Constants::NahdXml::xmlKeyFreqModAmount().toStdString()); p) m_freqModAmount = p->get().value();
    if (auto p = parameter(Constants::NahdXml::xmlKeyFreqModSource().toStdString()); p) m_freqModSource = static_cast<FreqModSource>(p->get().xmlValue());
    if (auto p = parameter(Constants::NahdXml::xmlKeyCutoff().toStdString()); p) m_filterCutoff = p->get().value();
    if (auto p = parameter(Constants::NahdXml::xmlKeyResonance().toStdString()); p) m_filterResonance = p->get().value();
    if (auto p = parameter(Constants::NahdXml::xmlKeyEnvAmount().toStdString()); p) m_filterEnvAmount = (p->get().value() * 2.0f) - 1.0f;
    if (auto p = parameter(Constants::NahdXml::xmlKeyKeyTrack().toStdString()); p) m_filterKeyTrack = p->get().value();
    if (auto p = parameter(Constants::NahdXml::xmlKeyFilterModAmount().toStdString()); p) m_filterModAmount = p->get().value();
    if (auto p = parameter(Constants::NahdXml::xmlKeyVolume().toStdString()); p) m_volume = p->get().value();
    
    if (auto p = parameter(Constants::NahdXml::xmlKeyMgFrequency().toStdString() + "1"); p) m_mg1Frequency = p->get().value();
    if (auto p = parameter(Constants::NahdXml::xmlKeyMgFrequency().toStdString() + "2"); p) m_mg2Frequency = p->get().value();

    if (auto p = parameter("vca" + Constants::NahdXml::xmlKeyAttack().toStdString()); p) m_vcaAttack = p->get().value();
    if (auto p = parameter("vca" + Constants::NahdXml::xmlKeyDecay().toStdString()); p) m_vcaDecay = p->get().value();
    if (auto p = parameter("vca" + Constants::NahdXml::xmlKeySustain().toStdString()); p) m_vcaSustain = p->get().value();
    if (auto p = parameter("vca" + Constants::NahdXml::xmlKeyRelease().toStdString()); p) m_vcaRelease = p->get().value();

    if (auto p = parameter("vcf" + Constants::NahdXml::xmlKeyAttack().toStdString()); p) m_vcfAttack = p->get().value();
    if (auto p = parameter("vcf" + Constants::NahdXml::xmlKeyDecay().toStdString()); p) m_vcfDecay = p->get().value();
    if (auto p = parameter("vcf" + Constants::NahdXml::xmlKeySustain().toStdString()); p) m_vcfSustain = p->get().value();
    if (auto p = parameter("vcf" + Constants::NahdXml::xmlKeyRelease().toStdString()); p) m_vcfRelease = p->get().value();

    for (auto && osc : m_oscParams) {
        if (auto p = osc->parameter(Constants::NahdXml::xmlKeyWaveform().toStdString()); p) osc->waveform = static_cast<PolyBLEPOscillator::Waveform>(p->get().xmlValue());
        if (auto p = osc->parameter(Constants::NahdXml::xmlKeyLevel().toStdString()); p) osc->level = p->get().value();
        if (auto p = osc->parameter(Constants::NahdXml::xmlKeyDetune().toStdString()); p) osc->tune = static_cast<float>(p->get().xmlValue());
        if (auto p = osc->parameter(Constants::NahdXml::xmlKeyOctave().toStdString()); p) osc->octave = p->get().xmlValue();
    }
}

} // namespace noteahead
