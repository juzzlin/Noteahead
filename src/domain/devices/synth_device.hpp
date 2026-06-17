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

#include "../dsp/adsr_envelope.hpp"
#include "../dsp/cascaded_svf.hpp"
#include "../dsp/lfo.hpp"
#include "../dsp/multi_engine.hpp"
#include "../dsp/oversampler.hpp"
#include "../dsp/poly_blep_oscillator.hpp"
#include "../effects/delay_effect.hpp"
#include "device.hpp"
#include "synth_presets.hpp"

#include <mutex>
#include <random>
#include <vector>

namespace noteahead {

class ProjectReader;
class ProjectWriter;

class SynthDevice : public Device
{
public:
    static constexpr int MaxVoices = 6;

    enum class VoiceMode
    {
        Poly,
        Unison
    };

    enum class ModTarget
    {
        Pitch1,
        Pitch2,
        Pitch3,
        Cutoff
    };

    enum class LfoTarget
    {
        Pitch,
        Shape,
        Cutoff,
        Volume,
        Resonance,
        Pan
    };

    explicit SynthDevice(std::string name);
    ~SynthDevice() override;

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
    void processMidiProgramChange(uint8_t program, uint8_t channel) override;
    void processMidiAllNotesOff() override;

    void processAudio(AudioContext & context) override;
    bool hasActiveAudio() const override;

    void setBpm(float bpm) override;

    void reset() override;
    void resetAudio() override;

    double voiceGlideFrequency(size_t index) const;

    void serializeToXml(ProjectWriter & writer) const override;
    void deserializeFromXml(ProjectReader & reader) override;

    // Parameter accessors (VCO1)
    PolyBlepOscillator::Waveform vco1Waveform() const;
    void setVco1Waveform(PolyBlepOscillator::Waveform wave);
    int vco1Octave() const;
    void setVco1Octave(int octave);
    float vco1Pitch() const;
    void setVco1Pitch(float pitch);
    float vco1Shape() const;
    void setVco1Shape(float shape);
    bool vco1Sync() const;
    void setVco1Sync(bool sync);

    // Parameter accessors (VCO2)
    PolyBlepOscillator::Waveform vco2Waveform() const;
    void setVco2Waveform(PolyBlepOscillator::Waveform wave);
    int vco2Octave() const;
    void setVco2Octave(int octave);
    float vco2Pitch() const;
    void setVco2Pitch(float pitch);
    float vco2Shape() const;
    void setVco2Shape(float shape);
    bool vco2Sync() const;
    void setVco2Sync(bool sync);

    // Parameter accessors (VCO3)
    PolyBlepOscillator::Waveform vco3Waveform() const;
    void setVco3Waveform(PolyBlepOscillator::Waveform wave);
    int vco3Octave() const;
    void setVco3Octave(int octave);
    float vco3Pitch() const;
    void setVco3Pitch(float pitch);
    float vco3Shape() const;
    void setVco3Shape(float shape);
    bool vco3Sync() const;
    void setVco3Sync(bool sync);

    // Multi Engine
    MultiEngine::Type multiType() const;
    void setMultiType(MultiEngine::Type type);
    float multiShape() const;
    void setMultiShape(float shape);
    float multiLevel() const;
    void setMultiLevel(float level);
    float multiKeyTrack() const;
    void setMultiKeyTrack(float keyTrack);

    // Mixer
    float mixVco1() const;
    void setMixVco1(float level);
    float mixVco2() const;
    void setMixVco2(float level);
    float mixVco3() const;
    void setMixVco3(float level);

    // Filter
    float lpfCutoff() const;
    void setLpfCutoff(float cutoff);
    float lpfResonance() const;
    void setLpfResonance(float resonance);
    float hpfCutoff() const;
    void setHpfCutoff(float cutoff);
    float filterKeyTrack() const;
    void setFilterKeyTrack(float track);

    // Amp EG
    float ampAttack() const;
    void setAmpAttack(float a);
    float ampDecay() const;
    void setAmpDecay(float d);
    float ampSustain() const;
    void setAmpSustain(float s);
    float ampRelease() const;
    void setAmpRelease(float r);
    float ampVelocitySensitivity() const;
    void setAmpVelocitySensitivity(float sensitivity);

    // Mod EG
    float modAttack() const;
    void setModAttack(float a);
    float modDecay() const;
    void setModDecay(float d);
    float modInt() const;
    void setModInt(float intensity);
    ModTarget modTarget() const;
    void setModTarget(ModTarget target);

    // Lfo
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

    // Voice / Global
    VoiceMode voiceMode() const;
    void setVoiceMode(VoiceMode mode);
    float voiceDepth() const;
    void setVoiceDepth(float depth);
    float portamento() const;
    void setPortamento(float p);
    float panSpread() const;
    void setPanSpread(float spread);
    int pitchBendRange() const;
    void setPitchBendRange(int range);
    float currentPitchBendOffset() const;
    void setPan(float pan) override;
    void setVolume(float vol) override;
    float gain() const;
    void setGain(float gain) override;

    // Oscillator drift
    float oscillatorDrift() const;
    void setOscillatorDrift(float drift);

    // Cross modulation (VCO1 -> VCO2 audio-rate FM)
    float crossModDepth() const;
    void setCrossModDepth(float depth);

    // Delay parameters
    DelayEffect::Type delayType() const;
    void setDelayType(DelayEffect::Type type);
    float delayTime() const;
    void setDelayTime(float time);
    float delayFeedback() const;
    void setDelayFeedback(float fb);
    float delayDepth() const;
    void setDelayDepth(float depth);
    float delayMix() const;
    void setDelayMix(float mix);
    bool delaySync() const;
    void setDelaySync(bool sync);
    float delaySyncDivision() const;
    void setDelaySyncDivision(float division);
    float delayFeedbackLpf() const;
    void setFeedbackLpf(float cutoff);
    float delayFeedbackHpf() const;
    void setFeedbackHpf(float cutoff);

    void loadPreset(int bank, int index);
    void setUserPresets(const UserPresets & presets);

private:
    struct Voice
    {
        PolyBlepOscillator vco1;
        PolyBlepOscillator vco2;
        PolyBlepOscillator vco3;
        MultiEngine multi;
        CascadedSvf lpf;
        CascadedSvf hpf;
        AdsrEnvelope ampEg;
        AdsrEnvelope modEg;
        Lfo lfo;

        uint8_t note { 0 };
        uint64_t triggerId { 0 };
        double frequency { 0.0 };
        double glideFrequency { 0.0 };
        bool active { false };
        float pan { 0.5f };
        float velocity { 1.0f };
        double driftPhase { 0.0 };
        double driftRate { 0.2 };

        void reset();
        void trigger(uint8_t note, double freq, float pan, float velocity, bool phaseSync, uint64_t triggerId);
        void triggerRandomized(uint8_t note, double freq, float pan, float velocity, double randomPhase, uint64_t triggerId);
        void release();
    };

    std::vector<Voice> m_voices;
    size_t m_polyNextVoice = 0;

    mutable std::mt19937 m_rng { 0 };
    mutable std::uniform_real_distribution<double> m_phaseDist { 0.0, 1.0 };

    // Internal parameter storage
    PolyBlepOscillator::Waveform m_vco1Waveform { PolyBlepOscillator::Waveform::Saw };
    int m_vco1Octave { 0 };
    float m_vco1Pitch { 0.5f };
    float m_vco1Shape { 0.0f };
    bool m_vco1Sync { false };

    PolyBlepOscillator::Waveform m_vco2Waveform { PolyBlepOscillator::Waveform::Saw };
    int m_vco2Octave { 0 };
    float m_vco2Pitch { 0.5f };
    float m_vco2Shape { 0.0f };
    bool m_vco2Sync { false };

    PolyBlepOscillator::Waveform m_vco3Waveform { PolyBlepOscillator::Waveform::Saw };
    int m_vco3Octave { 0 };
    float m_vco3Pitch { 0.5f };
    float m_vco3Shape { 0.0f };
    bool m_vco3Sync { false };

    MultiEngine::Type m_multiType { MultiEngine::Type::Low };
    float m_multiShape { 0.5f };
    float m_multiLevel { 0.0f };
    float m_multiKeyTrack { 0.0f };

    float m_mixVco1 { 1.0f };
    float m_mixVco2 { 0.0f };
    float m_mixVco3 { 0.0f };

    float m_lpfCutoff { 1.0f };
    float m_lpfResonance { 0.0f };
    float m_hpfCutoff { 0.0f };
    float m_filterKeyTrack { 0.0f };

    float m_ampAttack { 0.1f };
    float m_ampDecay { 0.2f };
    float m_ampSustain { 1.0f };
    float m_ampRelease { 0.2f };
    float m_ampVelocitySensitivity { 1.0f };

    float m_modAttack { 0.1f };
    float m_modDecay { 0.2f };
    float m_modInt { 0.0f };
    ModTarget m_modTarget { ModTarget::Cutoff };

    Lfo::Waveform m_lfoWaveform { Lfo::Waveform::Triangle };
    Lfo::Mode m_lfoMode { Lfo::Mode::Normal };
    float m_lfoRate { 0.5f };
    float m_lfoInt { 0.0f };
    LfoTarget m_lfoTarget { LfoTarget::Pitch };

    VoiceMode m_voiceMode { VoiceMode::Poly };
    uint64_t m_nextTriggerId { 1 };
    float m_voiceDepth { 0.0f };
    float m_portamento { 0.0f };
    float m_panSpread { 0.0f };
    int m_pitchBendRange { 2 };
    uint16_t m_pitchBend { 8192 };

    // Manual settings for CC reset
    float m_manualLpfCutoff { 1.0f };
    float m_manualLpfResonance { 0.0f };
    float m_manualHpfCutoff { 0.0f };

    float m_oscillatorDrift { 0.0f };
    float m_crossModDepth { 0.0f };

    DelayEffect m_delay;
    DelayEffect::Type m_delayType { DelayEffect::Type::Stereo };
    float m_delayTime { 0.5f };
    float m_delayFeedback { 0.3f };
    float m_delayDepth { 0.5f };
    float m_delayMix { 0.0f };
    bool m_delaySync { false };
    float m_delaySyncDivision { 0.25f };

    int m_currentBank = 0;
    UserPresets m_userPresets;

    Oversampler2x m_oversamplerL;
    Oversampler2x m_oversamplerR;

    std::vector<float> m_oversampledBuffer;

    double m_vco1BasePitchRatio { 1.0 };
    double m_vco2BasePitchRatio { 1.0 };
    double m_vco3BasePitchRatio { 1.0 };

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
        double shapeMod { 0.0 };
        double vco1PitchMod { 0.0 };
        double vco2PitchMod { 0.0 };
        double vco3PitchMod { 0.0 };
        double resonanceMod { 0.0 };
        double panMod { 0.0 };
        double volumeMod { 0.0 };
    };

    ModulationValues calculateModulation(Voice & voice) const;
    float generateVoiceSample(Voice & voice, const ModulationValues & mods, double oversampledRate, double pbRatio);

    void prepareForProcessing(AudioContext & context);
    void updateVoiceParameters(Voice & voice, uint32_t oversampledRate, size_t index);
    void renderVoice(Voice & voice, AudioContext & context, uint32_t oversampledRate, double portamentoCoeff, double pbRatio, size_t index);
    void applyGlobalEffects(AudioContext & context);

    std::string m_name;
};

} // namespace noteahead

#endif // SYNTH_DEVICE_HPP
