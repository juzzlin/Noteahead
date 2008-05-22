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

#ifndef DOMAIN_DEVICES_JU_SYNTH_08_DEVICE_HPP
#define DOMAIN_DEVICES_JU_SYNTH_08_DEVICE_HPP

#include "../dsp/adsr_envelope.hpp"
#include "../dsp/dc_blocker.hpp"
#include "../dsp/lfo.hpp"
#include "../dsp/one_pole_filter.hpp"
#include "../dsp/ota_ladder_filter.hpp"
#include "../dsp/upsampler.hpp"
#include "device.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace noteahead {

class JuSynth08Device : public Device
{
    Q_OBJECT

public:
    static constexpr size_t MaxVoices { 8 };
    static constexpr size_t Oversampling { 2 };

    enum class VoiceMode
    {
        Poly = 0,
        Solo = 1,
        Unison = 2,
    };

    enum class Vco1Waveform
    {
        Triangle = 0,
        Saw = 1,
        Square = 2,
        Sine = 3,
    };

    enum class Vco2Waveform
    {
        Saw = 0,
        Square = 1,
        Triangle = 2,
        Noise = 3,
    };

    enum class Vco1Range
    {
        Feet16 = 0,
        Feet8 = 1,
        Feet4 = 2,
        Feet2 = 3,
        LowFreq = 4,
    };

    enum class Vco2Range
    {
        Feet16 = 0,
        Feet8 = 1,
        Feet4 = 2,
        Feet2 = 3,
        LowFreq = 4,
    };

    enum class VcfSlope
    {
        Slope12dB = 0,
        Slope24dB = 1,
    };

    enum class VcaMode
    {
        Env2 = 0,
        Gate = 1,
    };

    enum class PwmSource
    {
        Manual = 0,
        Lfo = 1,
        Env1 = 2,
    };

    enum class PitchEnvTarget
    {
        Vco1 = 0,
        Vco2 = 1,
        Both = 2,
    };

    explicit JuSynth08Device(const std::string & name = "");
    ~JuSynth08Device() override;

    std::string name() const override;
    std::string category() const override;
    std::string typeName() const override;
    std::string typeId() const override;

    std::vector<MidiCcController> deviceMidiCcControllers() const override;

    static std::string typeIdString();

    void processDeviceMidiNoteOn(uint8_t note, uint8_t velocity) override;
    void processDeviceMidiNoteOff(uint8_t note) override;
    void processDeviceMidiCc(uint8_t controller, uint8_t value, uint8_t channel) override;
    void processMidiPitchBend(uint16_t value, uint8_t channel) override;
    void processMidiProgramChange(uint8_t program, uint8_t channel) override;
    void processMidiAllNotesOff() override;

    void processAudio(AudioContext & context) override;
    bool hasActiveAudio() const override;
    void advanceIdle(const AudioContext & context) override;

    void reset() override;
    void resetAudio() override;
    void syncParameters() override;

    void serializeToXml(ProjectWriter & writer) const override;
    void deserializeFromXml(ProjectReader & reader) override;

    void loadPreset(int index);

    // Getters and Setters
    float lfoRate() const;
    void setLfoRate(float rate);

    float lfoDelay() const;
    void setLfoDelay(float delay);

    int lfoWaveform() const;
    void setLfoWaveform(int waveform);

    float pitchLfoDepth() const;
    void setPitchLfoDepth(float depth);

    float pitchEnvDepth() const;
    void setPitchEnvDepth(float depth);

    int pitchEnvTarget() const;
    void setPitchEnvTarget(int target);

    float pulseWidth() const;
    void setPulseWidth(float width);

    int pwmSource() const;
    void setPwmSource(int source);

    float crossMod() const;
    void setCrossMod(float amount);

    int vco1Waveform() const;
    void setVco1Waveform(int waveform);

    int vco1Range() const;
    void setVco1Range(int range);

    float vco1Level() const;
    void setVco1Level(float level);

    int vco2Waveform() const;
    void setVco2Waveform(int waveform);

    int vco2Range() const;
    void setVco2Range(int range);

    float vco2FineTune() const;
    void setVco2FineTune(float tune);

    bool sync() const;
    void setSync(bool enable);

    float vco2Level() const;
    void setVco2Level(float level);

    float noiseLevel() const;
    void setNoiseLevel(float level);

    float hpfCutoff() const;
    void setHpfCutoff(float cutoff);

    int vcfSlope() const;
    void setVcfSlope(int slope);

    float lpfCutoff() const;
    void setLpfCutoff(float cutoff);

    float lpfResonance() const;
    void setLpfResonance(float resonance);

    float envMod() const;
    void setEnvMod(float mod);

    bool envPolarity() const;
    void setEnvPolarity(bool positive);

    float filterLfoDepth() const;
    void setFilterLfoDepth(float depth);

    float keyTrack() const;
    void setKeyTrack(float track);

    int vcaMode() const;
    void setVcaMode(int mode);

    float velocitySensitivity() const;
    void setVelocitySensitivity(float sens);

    float vcaLfoDepth() const;
    void setVcaLfoDepth(float depth);

    float env1Attack() const;
    void setEnv1Attack(float val);

    float env1Decay() const;
    void setEnv1Decay(float val);

    float env1Sustain() const;
    void setEnv1Sustain(float val);

    float env1Release() const;
    void setEnv1Release(float val);

    bool env1KeyFollow() const;
    void setEnv1KeyFollow(bool val);

    float env2Attack() const;
    void setEnv2Attack(float val);

    float env2Decay() const;
    void setEnv2Decay(float val);

    float env2Sustain() const;
    void setEnv2Sustain(float val);

    float env2Release() const;
    void setEnv2Release(float val);

    bool env2KeyFollow() const;
    void setEnv2KeyFollow(bool val);

    float delayTime() const;
    void setDelayTime(float val);

    float delayFeedback() const;
    void setDelayFeedback(float val);

    float delayLevel() const;
    void setDelayLevel(float val);

    int voiceMode() const;
    void setVoiceMode(int mode);

    float unisonDetune() const;
    void setUnisonDetune(float detune);

    float portamento() const;
    void setPortamento(float port);

    int pitchBendRange() const;
    void setPitchBendRange(int range);

    float panSpread() const;
    void setPanSpread(float spread);

private:
    struct Voice
    {
        bool active { false };
        bool gated { false };
        uint8_t note { 0 };
        double velocity { 0.0 };
        uint64_t startedAt { 0 };

        double frequency { 440.0 };
        double glideFrequency { 440.0 };
        double detuneCents { 0.0 };
        double pan { 0.5 };

        double phaseVco1 { 0.0 };
        double phaseVco2 { 0.0 };

        OtaLadderFilter vcf;
        OnePoleFilter hpf;
        AdsrEnvelope env1;
        AdsrEnvelope env2;
        AdsrEnvelope gate;

        uint32_t noiseState { 0x12345678 };

        void reset()
        {
            active = false;
            gated = false;
            note = 0;
            velocity = 0.0;
            startedAt = 0;
            frequency = 440.0;
            glideFrequency = 440.0;
            detuneCents = 0.0;
            pan = 0.5;
            phaseVco1 = 0.0;
            phaseVco2 = 0.0;
            vcf.reset();
            hpf.reset();
            env1.reset();
            env2.reset();
            gate.reset();
            gate.setAttackTime(0.001);
            gate.setDecayTime(0.001);
            gate.setSustainLevel(1.0);
            gate.setReleaseTime(0.005);
        }
    };

    void applyPreset(int index, bool authored);
    void setupParameters();
    void updateSampleRateDependencies();
    void handleNoteOn(uint8_t note, uint8_t velocity);
    void handleNoteOff(uint8_t note);
    size_t allocateVoice(uint8_t note);
    double rangeMultiplier(int range) const;
    double voiceSpreadPan(size_t voiceIndex) const;

    std::string m_name;
    uint16_t m_pitchBend { 8192 };
    int m_pitchBendRange { 2 };

    // Synth Parameters
    float m_lfoRate { 0.2f };
    float m_lfoDelay { 0.0f };
    int m_lfoWaveform { 0 };

    float m_pitchLfoDepth { 0.0f };
    float m_pitchEnvDepth { 0.0f };
    PitchEnvTarget m_pitchEnvTarget { PitchEnvTarget::Both };
    float m_pulseWidth { 0.5f };
    PwmSource m_pwmSource { PwmSource::Manual };
    float m_crossMod { 0.0f };

    Vco1Waveform m_vco1Waveform { Vco1Waveform::Saw };
    Vco1Range m_vco1Range { Vco1Range::Feet8 };
    float m_vco1Level { 0.8f };

    Vco2Waveform m_vco2Waveform { Vco2Waveform::Saw };
    Vco2Range m_vco2Range { Vco2Range::Feet8 };
    float m_vco2FineTune { 0.0f };
    bool m_sync { false };
    float m_vco2Level { 0.8f };

    float m_noiseLevel { 0.0f };
    float m_hpfCutoff { 0.0f };

    VcfSlope m_vcfSlope { VcfSlope::Slope24dB };
    float m_lpfCutoff { 1.0f };
    float m_lpfResonance { 0.0f };
    float m_envMod { 0.0f };
    bool m_envPolarity { false }; // false = positive, true = inverted
    float m_filterLfoDepth { 0.0f };
    float m_keyTrack { 1.0f };

    VcaMode m_vcaMode { VcaMode::Env2 };
    float m_velocitySensitivity { 0.0f };
    float m_vcaLfoDepth { 0.0f };

    float m_env1Attack { 0.01f };
    float m_env1Decay { 0.3f };
    float m_env1Sustain { 0.5f };
    float m_env1Release { 0.2f };
    bool m_env1KeyFollow { false };

    float m_env2Attack { 0.01f };
    float m_env2Decay { 0.3f };
    float m_env2Sustain { 1.0f };
    float m_env2Release { 0.2f };
    bool m_env2KeyFollow { false };

    float m_delayTime { 0.3f };
    float m_delayFeedback { 0.3f };
    float m_delayLevel { 0.0f };

    VoiceMode m_voiceMode { VoiceMode::Poly };
    float m_unisonDetune { 0.2f };
    float m_portamento { 0.0f };
    float m_panSpread { 0.5f };

    // DSP Engine State
    std::array<Voice, MaxVoices> m_voices;
    uint64_t m_noteCounter { 0 };
    size_t m_nextVoice { 0 };
    Lfo m_lfo;
    double m_lfoDelayElapsed { 0.0 };

    std::array<DcBlocker, 2> m_dcBlockers;
    std::array<Decimator, 2> m_decimators;

    std::vector<float> m_delayBufferL;
    std::vector<float> m_delayBufferR;
    size_t m_delayWritePos { 0 };

    size_t m_tailFrames { 0 };
};

} // namespace noteahead

#endif // DOMAIN_DEVICES_JU_SYNTH_08_DEVICE_HPP
