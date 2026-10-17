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

#ifndef JU_SYNTH_DEVICE_HPP
#define JU_SYNTH_DEVICE_HPP

#include "../dsp/adsr_envelope.hpp"
#include "../dsp/cascaded_svf.hpp"
#include "../dsp/dc_blocker.hpp"
#include "../dsp/juno_chorus.hpp"
#include "../dsp/lfo.hpp"
#include "../dsp/ota_ladder_filter.hpp"
#include "../dsp/poly_blep_oscillator.hpp"
#include "../dsp/upsampler.hpp"
#include "device.hpp"

#include <array>
#include <string>
#include <vector>

namespace noteahead {

//! A six-voice polysynth after the Roland Juno-60 and JU-06A: one DCO per voice with saw, pulse,
//! sub and noise, a four-pole OTA ladder per voice on a single shared envelope, and the high pass
//! and the chorus on the sum of the voices, where the Juno has them.
//!
//! Laid out the way the Juno's panel is, left to right: LFO, DCO, HPF, VCF, VCA, ENV, Chorus.
class JuSynthDevice : public Device
{
public:
    static constexpr int MaxVoices = 6;

    //! Persisted as ordinals, so append only.
    enum class PwmSource
    {
        Lfo,
        Manual,
        Envelope
    };

    enum class VcaMode
    {
        Envelope,
        Gate
    };

    explicit JuSynthDevice(std::string name);
    ~JuSynthDevice() override;

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

    void loadPreset(int index);

    void processAudio(AudioContext & context) override;
    bool hasActiveAudio() const override;

    void reset() override;
    void resetAudio() override;

    void serializeToXml(ProjectWriter & writer) const override;
    void deserializeFromXml(ProjectReader & reader) override;

    // LFO
    float lfoRate() const;
    void setLfoRate(float rate);
    float lfoDelay() const;
    void setLfoDelay(float delay);

    // DCO
    //! 0 is 16', 1 is 8' and 2 is 4'.
    int range() const;
    void setRange(int range);
    float pitchLfoDepth() const;
    void setPitchLfoDepth(float depth);
    float pulseWidth() const;
    void setPulseWidth(float width);
    PwmSource pwmSource() const;
    void setPwmSource(PwmSource source);
    bool sawEnabled() const;
    void setSawEnabled(bool enabled);
    bool pulseEnabled() const;
    void setPulseEnabled(bool enabled);
    float subLevel() const;
    void setSubLevel(float level);
    float noiseLevel() const;
    void setNoiseLevel(float level);

    // HPF
    float hpfCutoff() const;
    void setHpfCutoff(float cutoff);

    // VCF
    float lpfCutoff() const;
    void setLpfCutoff(float cutoff);
    float lpfResonance() const;
    void setLpfResonance(float resonance);
    bool envInverted() const;
    void setEnvInverted(bool inverted);
    float envMod() const;
    void setEnvMod(float amount);
    float filterLfoDepth() const;
    void setFilterLfoDepth(float depth);
    float keyTrack() const;
    void setKeyTrack(float keyTrack);

    // VCA
    VcaMode vcaMode() const;
    void setVcaMode(VcaMode mode);
    float velocitySensitivity() const;
    void setVelocitySensitivity(float sensitivity);

    // ENV
    float attack() const;
    void setAttack(float attack);
    float decay() const;
    void setDecay(float decay);
    float sustain() const;
    void setSustain(float sustain);
    float release() const;
    void setRelease(float release);

    // Chorus
    JunoChorus::Mode chorusMode() const;
    void setChorusMode(JunoChorus::Mode mode);

    // Global
    float portamento() const;
    void setPortamento(float portamento);
    int pitchBendRange() const;
    void setPitchBendRange(int range);

protected:
    void syncParameters() override;

private:
    void applyPreset(int index, bool authored);

    struct Voice
    {
        PolyBlepOscillator saw;
        PolyBlepOscillator pulse;
        PolyBlepOscillator sub;
        OtaLadderFilter vcf;
        AdsrEnvelope env;
        //! The VCA's gate mode: on and off with only enough slope to keep the edges from clicking.
        AdsrEnvelope gate;
        uint32_t noiseState { 1 };

        uint8_t note { 0 };
        double frequency { 0.0 };
        double glideFrequency { 0.0 };
        float velocity { 1.0f };
        bool active { false };
        bool gated { false };
        //! When the voice last started a note, for choosing which to take when all six are busy.
        uint64_t startedAt { 0 };

        void reset();
    };

    void handleNoteOn(uint8_t note, uint8_t velocity);
    void handleNoteOff(uint8_t note);
    size_t allocateVoice(uint8_t note);
    void renderVoice(Voice & voice, size_t frameCount, uint8_t oversampleFactor, uint32_t oversampledRate, double portamentoCoeff, double pitchBendRatio);
    double nextNoise(Voice & voice) const;

    std::array<Voice, MaxVoices> m_voices;
    uint64_t m_noteCounter { 0 };

    //! One LFO for every voice, as on the Juno, and the delay before it comes in. The delay restarts
    //! when a key goes down with none held.
    Lfo m_lfo;
    double m_lfoDelayElapsed { 0.0 };
    //! The LFO for the block being rendered, one value per output frame, delay already applied.
    std::vector<double> m_lfoBlock;

    std::vector<float> m_oversampledBuffer;
    Decimator m_decimator;
    DcBlocker m_dcBlocker;
    CascadedSvf m_hpf;
    JunoChorus m_chorus;

    uint32_t m_tailFrames { 0 };
    uint16_t m_pitchBend { 8192 };

    float m_lfoRate { 0.5f };
    float m_lfoDelay { 0.0f };
    int m_range { 1 };
    float m_pitchLfoDepth { 0.0f };
    float m_pulseWidth { 0.0f };
    PwmSource m_pwmSource { PwmSource::Manual };
    bool m_sawEnabled { true };
    bool m_pulseEnabled { false };
    float m_subLevel { 0.0f };
    float m_noiseLevel { 0.0f };
    float m_hpfCutoff { 0.0f };
    float m_lpfCutoff { 1.0f };
    float m_lpfResonance { 0.0f };
    bool m_envInverted { false };
    float m_envMod { 0.0f };
    float m_filterLfoDepth { 0.0f };
    float m_keyTrack { 0.0f };
    VcaMode m_vcaMode { VcaMode::Envelope };
    float m_velocitySensitivity { 0.0f };
    float m_attack { 0.0f };
    float m_decay { 0.5f };
    float m_sustain { 1.0f };
    float m_release { 0.3f };
    JunoChorus::Mode m_chorusMode { JunoChorus::Mode::Off };
    float m_portamento { 0.0f };
    int m_pitchBendRange { 2 };

    std::string m_name;
};

} // namespace noteahead

#endif // JU_SYNTH_DEVICE_HPP
