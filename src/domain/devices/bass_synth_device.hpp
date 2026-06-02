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

#ifndef BASS_SYNTH_DEVICE_HPP
#define BASS_SYNTH_DEVICE_HPP

#include "../dsp/adsr_envelope.hpp"
#include "../dsp/cascaded_svf.hpp"
#include "../dsp/diode_ladder_filter.hpp"
#include "../dsp/oversampler.hpp"
#include "../dsp/poly_blep_oscillator.hpp"
#include "device.hpp"

#include <mutex>
#include <vector>

namespace noteahead {

class BassSynthDevice : public Device
{
public:
    explicit BassSynthDevice(std::string name);
    ~BassSynthDevice() override;

    std::string name() const override;
    std::string category() const override;
    std::string typeName() const override;
    std::string typeId() const override;

    std::vector<MidiCcController> availableMidiCcControllers() const override;

    static std::string typeIdString();

    void processMidiNoteOn(uint8_t note, uint8_t velocity) override;
    void processMidiNoteOff(uint8_t note) override;
    void processMidiCc(uint8_t controller, uint8_t value, uint8_t channel) override;
    void processMidiPitchBend(uint16_t value, uint8_t channel) override;
    void processMidiAllNotesOff() override;

    void processAudio(AudioContext & context) override;
    bool hasActiveAudio() const override;

    void reset() override;
    void resetAudio() override;

    void serializeToXml(QXmlStreamWriter & writer) const override;
    void deserializeFromXml(QXmlStreamReader & reader) override;

    // Parameter accessors
    PolyBlepOscillator::Waveform waveform() const;
    void setWaveform(PolyBlepOscillator::Waveform wave);
    float tuning() const;
    void setTuning(float tuning);

    float subLevel() const;
    void setSubLevel(float level);
    int subOctave() const;
    void setSubOctave(int octave);

    float lpfCutoff() const;
    void setLpfCutoff(float cutoff);
    float lpfResonance() const;
    void setLpfResonance(float resonance);
    float hpfCutoff() const;
    void setHpfCutoff(float cutoff);
    float envMod() const;
    void setEnvMod(float mod);
    float decay() const;
    void setDecay(float decay);

    float accent() const;
    void setAccent(float accent);
    float slide() const;
    void setSlide(float slide);

    float distDrive() const;
    void setDistDrive(float drive);
    float distTone() const;
    void setDistTone(float tone);
    float distLevel() const;
    void setDistLevel(float level);

protected:
    void syncParameters() override;

private:
    struct Voice
    {
        PolyBlepOscillator vco;
        PolyBlepOscillator sub;
        DiodeLadderFilter lpf;
        CascadedSvf hpf;
        AdsrEnvelope filterEg;
        AdsrEnvelope ampEg;

        uint8_t note { 0 };
        double frequency { 0.0 };
        double glideFrequency { 0.0 };
        bool active { false };
        bool accent { false };
        float velocity { 1.0f };

        void reset();
        void trigger(uint8_t note, double freq, float velocity, bool hasAccent, bool phaseSync);
        void release();
    };

    Voice m_voice;

    // Internal parameter storage
    PolyBlepOscillator::Waveform m_waveform { PolyBlepOscillator::Waveform::Saw };
    float m_tuning { 0.5f };
    float m_subLevel { 0.0f };
    int m_subOctave { 1 };

    float m_lpfCutoff { 0.5f };
    float m_lpfResonance { 0.0f };
    float m_hpfCutoff { 0.0f };
    float m_envMod { 0.5f };
    float m_decay { 0.5f };

    float m_accent { 0.5f };
    float m_slide { 0.0f };

    float m_distDrive { 0.0f };
    float m_distTone { 0.5f };
    float m_distLevel { 1.0f };

    float m_distLpState { 0.0f };

    double m_vcoBasePitchRatio { 1.0 };
    double m_subBasePitchRatio { 0.5 };
    double m_slideCoeff { 1.0 };
    uint32_t m_lastOversampledRate { 0 };

    Oversampler2x m_oversamplerL;
    Oversampler2x m_oversamplerR;

    uint16_t m_pitchBend { 8192 };
    int m_pitchBendRange { 2 };

    // Manual settings for CC reset
    float m_manualLpfCutoff { 0.5f };
    float m_manualHpfCutoff { 0.0f };

    void handleNoteOn(uint8_t note, uint8_t velocity);
    void handleNoteOff(uint8_t note);
    double midiNoteToFreq(uint8_t note) const;

    std::string m_name;
};

} // namespace noteahead

#endif // BASS_SYNTH_DEVICE_HPP
