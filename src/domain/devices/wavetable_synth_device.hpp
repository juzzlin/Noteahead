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

#ifndef WAVETABLE_SYNTH_DEVICE_HPP
#define WAVETABLE_SYNTH_DEVICE_HPP

#include "domain/devices/device.hpp"
#include "domain/dsp/adsr_envelope.hpp"
#include "domain/dsp/cascaded_svf.hpp"
#include "domain/dsp/lfo.hpp"
#include "domain/dsp/oversampler.hpp"
#include "domain/dsp/wavetable_oscillator.hpp"

#include <mutex>
#include <random>
#include <vector>

namespace noteahead {

class WavetableSynthDevice : public Device
{
    Q_OBJECT

public:
    static constexpr int MaxVoices = 8;

    enum class ModTarget
    {
        Cutoff,
        Pitch1,
        Pitch2,
        Osc1Pos,
        Osc2Pos
    };

    enum class LfoTarget
    {
        Pitch,
        Cutoff,
        Osc1Pos,
        Osc2Pos
    };

    enum class VoiceMode
    {
        Poly,
        Unison
    };

    explicit WavetableSynthDevice(std::string name);
    ~WavetableSynthDevice() override;

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

    void setBpm(float bpm) override;

    void reset() override;
    void resetAudio() override;

    void serializeToXml(QXmlStreamWriter & writer) const override;
    void deserializeFromXml(QXmlStreamReader & reader) override;

    // Voice / Global
    VoiceMode voiceMode() const;
    void setVoiceMode(VoiceMode mode);
    float voiceDepth() const;
    void setVoiceDepth(float depth);
    float panSpread() const;
    void setPanSpread(float spread);
    float portamento() const;
    void setPortamento(float p);
    int pitchBendRange() const;
    void setPitchBendRange(int range);

    // Parameter accessors (Osc 1)
    float osc1Pos() const;
    void setOsc1Pos(float pos);
    int osc1Octave() const;
    void setOsc1Octave(int octave);
    float osc1Pitch() const;
    void setOsc1Pitch(float pitch);
    float osc1Level() const;
    void setOsc1Level(float level);

    // Parameter accessors (Osc 2)
    float osc2Pos() const;
    void setOsc2Pos(float pos);
    int osc2Octave() const;
    void setOsc2Octave(int octave);
    float osc2Pitch() const;
    void setOsc2Pitch(float pitch);
    float osc2Level() const;
    void setOsc2Level(float level);

    // Noise
    float noiseLevel() const;
    void setNoiseLevel(float level);

    // Filter
    float lpfCutoff() const;
    void setLpfCutoff(float cutoff);
    float lpfResonance() const;
    void setLpfResonance(float resonance);
    float hpfCutoff() const;
    void setHpfCutoff(float cutoff);

    // Amp EG
    float ampAttack() const;
    void setAmpAttack(float a);
    float ampDecay() const;
    void setAmpDecay(float d);
    float ampSustain() const;
    void setAmpSustain(float s);
    float ampRelease() const;
    void setAmpRelease(float r);

    // Mod EG
    float modAttack() const;
    void setModAttack(float a);
    float modDecay() const;
    void setModDecay(float d);
    float modInt() const;
    void setModInt(float intensity);
    ModTarget modTarget() const;
    void setModTarget(ModTarget target);

    // LFO
    Lfo::Waveform lfoWaveform() const;
    void setLfoWaveform(Lfo::Waveform wave);
    Lfo::Mode lfoMode() const;
    void setLfoMode(Lfo::Mode mode);
    float lfoRate() const;
    void setLfoRate(float rate);
    float lfoInt() const;
    void setLfoInt(float intensity);
    LfoTarget lfoTarget() const;
    void setLfoTarget(LfoTarget target);

    // Wavetable selection
    int wavetableIndex() const;
    void setWavetableIndex(int index);
    std::vector<std::string> wavetableNames() const;

private:
    struct Voice
    {
        WavetableOscillator osc1;
        WavetableOscillator osc2;
        CascadedSvf lpf;
        CascadedSvf hpf;
        AdsrEnvelope ampEg;
        AdsrEnvelope modEg;
        Lfo lfo;

        uint8_t note { 0 };
        uint64_t triggerId { 0 };
        bool active { false };
        float velocity { 1.0f };
        double frequency { 0.0 };
        double glideFrequency { 0.0 };
        float pan { 0.5f };

        void reset();
        void trigger(uint8_t note, double freq, float pan, float velocity, uint64_t triggerId);
        void release();
    };

    std::vector<Voice> m_voices;
    size_t m_polyNextVoice { 0 };
    uint64_t m_nextTriggerId { 1 };

    Wavetable::WavetableList m_wavetables;
    int m_wavetableIndex { 0 };

    mutable std::mt19937 m_rng { 42 };
    mutable std::uniform_real_distribution<float> m_noiseDist { -1.0f, 1.0f };
    mutable std::uniform_real_distribution<double> m_phaseDist { 0.0, 1.0 };

    // Internal parameter storage
    float m_osc1Pos { 0.0f };
    int m_osc1Octave { 0 };
    float m_osc1Pitch { 0.5f };
    float m_osc1Level { 1.0f };

    float m_osc2Pos { 0.5f };
    int m_osc2Octave { 0 };
    float m_osc2Pitch { 0.5f };
    float m_osc2Level { 0.0f };

    float m_noiseLevel { 0.0f };

    float m_lpfCutoff { 1.0f };
    float m_lpfResonance { 0.0f };
    float m_hpfCutoff { 0.0f };

    float m_ampAttack { 0.01f };
    float m_ampDecay { 0.1f };
    float m_ampSustain { 1.0f };
    float m_ampRelease { 0.1f };

    float m_modAttack { 0.01f };
    float m_modDecay { 0.1f };
    float m_modInt { 0.0f };
    ModTarget m_modTarget { ModTarget::Cutoff };

    Lfo::Waveform m_lfoWaveform { Lfo::Waveform::Triangle };
    Lfo::Mode m_lfoMode { Lfo::Mode::Normal };
    float m_lfoRate { 0.5f };
    float m_lfoInt { 0.0f };
    LfoTarget m_lfoTarget { LfoTarget::Pitch };

    VoiceMode m_voiceMode { VoiceMode::Poly };
    float m_voiceDepth { 0.1f };
    float m_panSpread { 0.5f };
    float m_portamento { 0.0f };
    float m_bpm { 120.0f };

    uint16_t m_pitchBend { 8192 };
    int m_pitchBendRange { 2 };

    double m_osc1BasePitchRatio { 1.0 };
    double m_osc2BasePitchRatio { 1.0 };

    void handleNoteOn(uint8_t note, uint8_t velocity);
    void handleNoteOff(uint8_t note);
    double midiNoteToFreq(uint8_t note) const;
    void syncParameters() override;

    struct ModulationValues
    {
        double ampEnvelope { 0.0 };
        double modEnvelope { 0.0 };
        double lfoValue { 0.0 };
        double cutoffMod { 0.0 };
        double osc1PosMod { 0.0 };
        double osc2PosMod { 0.0 };
        double pitchMod { 0.0 };
    };

    ModulationValues calculateModulation(Voice & voice) const;
    float generateVoiceSample(Voice & voice, const ModulationValues & mods, double oversampledRate, double pbRatio);

    void prepareForProcessing(AudioContext & context);
    void updateVoiceParameters(Voice & voice, uint32_t oversampledRate);
    void renderVoice(Voice & voice, AudioContext & context, uint32_t oversampledRate, double portamentoCoeff, double pbRatio);

    std::string m_name;

    std::vector<float> m_oversampledBuffer;
    Oversampler2x m_oversamplerL;
    Oversampler2x m_oversamplerR;
};

} // namespace noteahead

#endif // WAVETABLE_SYNTH_DEVICE_HPP
