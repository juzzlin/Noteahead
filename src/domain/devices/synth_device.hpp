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

#ifndef SYNTH_DEVICE_HPP
#define SYNTH_DEVICE_HPP

#include "device.hpp"
#include "../dsp/adsr_envelope.hpp"
#include "../dsp/cascaded_svf.hpp"
#include "../dsp/lfo.hpp"
#include "../dsp/polyblep_oscillator.hpp"

#include <array>
#include <memory>
#include <mutex>
#include <vector>

class QXmlStreamReader;
class QXmlStreamWriter;

namespace noteahead {

class SynthDevice : public Device
{
public:
    enum class KeyAssignMode
    {
        Unison,
        UnisonShare,
        Poly
    };

    enum class FreqModSource
    {
        Mg1,
        VcfEg
    };

    SynthDevice();
    ~SynthDevice() override;

    std::string name() const override;
    std::string category() const override;

    void processMidiNoteOn(uint8_t note, uint8_t velocity) override;
    void processMidiNoteOff(uint8_t note) override;
    void processMidiCc(uint8_t controller, uint8_t value, uint8_t channel) override;
    void processMidiAllNotesOff() override;

    void processAudio(float * output, uint32_t nFrames, uint32_t sampleRate) override;

    void reset() override;

    void serializeToXml(QXmlStreamWriter & writer) const override;
    void deserializeFromXml(QXmlStreamReader & reader) override;

    struct Oscillator : public ParameterContainer
    {
        Oscillator();

        PolyBLEPOscillator::Waveform waveform = PolyBLEPOscillator::Waveform::Saw;
        float level = 1.0f;
        float tune = 0.0f;
        int octave = 0;
    };

    // Parameter accessors
    KeyAssignMode keyAssignMode() const;
    void setKeyAssignMode(KeyAssignMode mode);

    PolyBLEPOscillator::Waveform oscWaveform(int index) const;
    void setOscWaveform(int index, PolyBLEPOscillator::Waveform waveform);

    float oscLevel(int index) const;
    void setOscLevel(int index, float level);

    float oscTune(int index) const;
    void setOscTune(int index, float tune);

    int oscOctave(int index) const;
    void setOscOctave(int index, int octave);

    float pulseWidth() const;
    void setPulseWidth(float pw);

    float filterCutoff() const;
    void setFilterCutoff(float cutoff);

    float filterResonance() const;
    void setFilterResonance(float resonance);

    float filterEnvAmount() const;
    void setFilterEnvAmount(float amount);

    float filterKeyTrack() const;
    void setFilterKeyTrack(float track);

    float filterModAmount() const;
    void setFilterModAmount(float amount);

    float volume() const;
    void setVolume(float volume);

    float vcaAttack() const;
    void setVcaAttack(float a);
    float vcaDecay() const;
    void setVcaDecay(float d);
    float vcaSustain() const;
    void setVcaSustain(float s);
    float vcaRelease() const;
    void setVcaRelease(float r);

    float vcfAttack() const;
    void setVcfAttack(float a);
    float vcfDecay() const;
    void setVcfDecay(float d);
    float vcfSustain() const;
    void setVcfSustain(float s);
    float vcfRelease() const;
    void setVcfRelease(float r);

    void setVcaAdsr(float a, float d, float s, float r);
    void setVcfAdsr(float a, float d, float s, float r);

    float mg1Frequency() const;
    void setMg1Frequency(float freq);

    float mg2Frequency() const;
    void setMg2Frequency(float freq);

    float detune() const;
    void setDetune(float detune);

    float freqModAmount() const;
    void setFreqModAmount(float amount);

    FreqModSource freqModSource() const;
    void setFreqModSource(FreqModSource source);

private:
    KeyAssignMode m_keyAssignMode { KeyAssignMode::Poly };
    
    std::array<PolyBLEPOscillator, 4> m_oscillators;
    std::array<uint8_t, 4> m_oscillatorNotes {0, 0, 0, 0};
    std::array<double, 4> m_oscillatorBaseFreqs {0.0, 0.0, 0.0, 0.0};
    std::array<bool, 4> m_oscillatorActive {false, false, false, false};
    
    ADSREnvelope m_vcaEnvelope;
    ADSREnvelope m_vcfEnvelope;
    CascadedSVF m_filter;
    
    LFO m_mg1;
    LFO m_mg2;

    std::vector<uint8_t> m_heldNotes;
    int m_polyNextOsc = 0;

    mutable std::mutex m_mutex;

    std::array<std::unique_ptr<Oscillator>, 4> m_oscParams;

    float m_pulseWidth = 0.5f;
    float m_filterCutoff = 0.5f;
    float m_filterResonance = 0.0f;
    float m_filterEnvAmount = 0.0f;
    float m_filterKeyTrack = 0.0f;
    float m_filterModAmount = 0.1f;
    float m_volume = 1.0f;
    float m_vcaAttack = 0.1f;
    float m_vcaDecay = 0.2f;
    float m_vcaSustain = 1.0f;
    float m_vcaRelease = 0.2f;
    float m_vcfAttack = 0.1f;
    float m_vcfDecay = 0.2f;
    float m_vcfSustain = 1.0f;
    float m_vcfRelease = 0.2f;
    float m_mg1Frequency = 0.5f;
    float m_mg2Frequency = 0.2f;
    float m_detune = 0.0f;
    float m_freqModAmount = 0.0f;
    FreqModSource m_freqModSource = FreqModSource::Mg1;

    double m_velocityFactor = 1.0;

    void updateOscillatorFrequencies(double sampleRate, double modValue);
    void handleNoteOn(uint8_t note, uint8_t velocity);
    void handleNoteOff(uint8_t note);
    double midiNoteToFreq(uint8_t note) const;
    void syncParameters();
};

} // namespace noteahead

#endif // SYNTH_DEVICE_HPP
