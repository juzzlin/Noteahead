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
#include "../dsp/dc_blocker.hpp"
#include "../dsp/lfo.hpp"
#include "../dsp/multi_engine.hpp"
#include "../dsp/one_pole_filter.hpp"
#include "../dsp/poly_blep_oscillator.hpp"
#include "../dsp/upsampler.hpp"
#include "../effects/delay.hpp"
#include "device.hpp"
#include "synth_presets.hpp"

#include <array>
#include <mutex>
#include <optional>
#include <random>
#include <vector>

namespace noteahead {

class ProjectReader;
class ProjectWriter;

class SynthDevice : public Device
{
public:
    static constexpr int MaxVoices = 6;

    //! Serialized as a raw ordinal, so this is append-only: inserting a value would silently change
    //! the voice mode of every project saved before the change.
    enum class VoiceMode
    {
        Poly,
        Unison,
        Dual,
        //! JP-8000 style: unevenly spaced detune around a centre voice held at full level, so the
        //! stack keeps a solid core pitch instead of six equal voices smearing it.
        Supersaw,
        //! No fixed detune at all. Each voice wanders at its own slow rate, so no pair ever settles
        //! into a steady beat and the comb pattern that makes plain unison harsh never forms.
        Drift,
        //! Poly with a single voice: notes glide into each other instead of stacking. Appended last
        //! because the ordinal is persisted, so it cannot sit next to Poly where it belongs.
        Mono
    };

    //! Voice modes that spend every voice on a single note.
    static bool isStacked(VoiceMode mode);

    enum class ModTarget
    {
        Pitch1,
        Pitch2,
        Pitch3,
        Cutoff
    };

    //! Serialized as a raw ordinal, so this is append-only: inserting a value would silently change
    //! the LFO destination of every project saved before the change.
    enum class LfoTarget
    {
        //! All VCOs at once.
        Pitch,
        Shape,
        Cutoff,
        Volume,
        Resonance,
        Pan,
        //! Single-VCO pitch. Appended after Pan because the ordinal is persisted, so these cannot sit
        //! next to Pitch where they belong.
        Pitch1,
        Pitch2,
        Pitch3
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
    //! Level weighting the current voice mode gives this voice. Only Supersaw uses anything but
    //! unity, where the centre voice is held up and the outer ones give way.
    float voiceLevel(size_t index) const;

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
    float ampCurve() const;
    void setAmpCurve(float curve);

    // Mod EG
    float modAttack() const;
    void setModAttack(float a);
    float modDecay() const;
    void setModDecay(float d);
    float modSustain() const;
    void setModSustain(float sustain);
    float modInt() const;
    void setModInt(float intensity);
    ModTarget modTarget() const;
    void setModTarget(ModTarget target);
    float modCurve() const;
    void setModCurve(float curve);

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

    // Lfo 2
    Lfo::Waveform lfo2Waveform() const;
    void setLfo2Waveform(Lfo::Waveform wave);
    Lfo::Mode lfo2Mode() const;
    void setLfo2Mode(Lfo::Mode mode);
    float lfo2Rate() const;
    void setLfo2Rate(float rate);
    float lfo2Int() const;
    void setLfo2Int(float intensity);
    LfoTarget lfo2Target() const;
    void setLfo2Target(LfoTarget target);

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
    Delay::Type delayType() const;
    void setDelayType(Delay::Type type);
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

    //! Loads a preset as the user's own choice: it becomes the patch, and it is what gets saved.
    void loadPreset(int bank, int index);
    void setUserPresets(const UserPresets & presets);

private:
    //! Everything a note-on hands to a voice. Passed whole so the deferred path below can hold on to
    //! it for a millisecond and apply exactly what the immediate path would have.
    struct Trigger
    {
        uint8_t note { 0 };
        uint64_t triggerId { 0 };
        double frequency { 0.0 };
        float pan { 0.5f };
        float velocity { 1.0f };
        //! Whether the oscillators jump to the new pitch or glide to it from wherever the last note
        //! left them.
        bool resetGlide { true };
    };

    struct Voice
    {
        //! Starting phase of each oscillator, in VCO order.
        using Phases = std::array<double, 3>;

        PolyBlepOscillator vco1;
        PolyBlepOscillator vco2;
        PolyBlepOscillator vco3;
        MultiEngine multi;
        CascadedSvf lpf;
        CascadedSvf hpf;
        AdsrEnvelope ampEg;
        AdsrEnvelope modEg;
        Lfo lfo;
        Lfo lfo2;

        uint8_t note { 0 };
        uint64_t triggerId { 0 };
        double frequency { 0.0 };
        double glideFrequency { 0.0 };
        bool active { false };
        float pan { 0.5f };
        float velocity { 1.0f };
        double driftPhase { 0.0 };
        double driftRate { 0.2 };
        //! Rolls the top off the outer voices of a stacked mode, where the beating is roughest.
        OnePoleFilter unisonDamp;

        //! A note waiting for the voice to fade out before it resets anything under it. Set only by
        //! the synced path, and only when there is something to fade.
        std::optional<Trigger> pendingTrigger;
        //! Level of that fade, 1 down to 0 while a pending trigger waits.
        double declickGain { 1.0 };

        void reset();

        //! Phase Sync on: the note starts from a known oscillator phase and its own attack, every
        //! time. A voice that is still sounding is faded out first and the note applied at the
        //! bottom of that fade — resetting phase and envelopes under a signal that is not at zero is
        //! a click, and skipping the reset on such voices is what made the sync fire on some notes
        //! and not on others.
        void triggerSynced(const Trigger & trigger);

        //! Phase Sync off: a voice that had fallen silent starts from a random phase so a stack does
        //! not stand still, and one that is still producing audio is left running and simply
        //! re-attacked from where it is.
        void triggerFree(const Trigger & trigger, double randomPhase);

        void applyTrigger(const Trigger & trigger, std::optional<Phases> phases, bool restartEnvelopes);
        void cancelPendingTrigger();
        void release();
    };

    std::vector<Voice> m_voices;
    size_t m_polyNextVoice = 0;
    size_t m_dualNextPair { 0 };
    //! Pan-spread slot of the note Mono is currently sounding. Unset until the first note, so the
    //! line starts at the same position a poly patch would.
    std::optional<size_t> m_monoPanSlot;

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
    float m_ampCurve { 0.0f };

    float m_modAttack { 0.1f };
    float m_modDecay { 0.2f };
    float m_modSustain { 0.0f };
    float m_modInt { 0.0f };
    ModTarget m_modTarget { ModTarget::Cutoff };
    float m_modCurve { 0.0f };

    Lfo::Waveform m_lfoWaveform { Lfo::Waveform::Triangle };
    Lfo::Mode m_lfoMode { Lfo::Mode::Normal };
    float m_lfoRate { 0.5f };
    float m_lfoInt { 0.0f };
    LfoTarget m_lfoTarget { LfoTarget::Pitch };

    Lfo::Waveform m_lfo2Waveform { Lfo::Waveform::Triangle };
    Lfo::Mode m_lfo2Mode { Lfo::Mode::Normal };
    float m_lfo2Rate { 0.5f };
    float m_lfo2Int { 0.0f };
    LfoTarget m_lfo2Target { LfoTarget::Pitch };

    VoiceMode m_voiceMode { VoiceMode::Poly };
    uint64_t m_nextTriggerId { 1 };
    float m_voiceDepth { 0.0f };
    float m_portamento { 0.0f };
    float m_panSpread { 0.0f };
    int m_pitchBendRange { 2 };
    uint16_t m_pitchBend { 8192 };

    // Manual settings for CC reset

    float m_oscillatorDrift { 0.0f };
    float m_crossModDepth { 0.0f };

    Delay m_delay;
    Delay::Type m_delayType { Delay::Type::Stereo };
    float m_delayTime { 0.5f };
    float m_delayFeedback { 0.3f };
    float m_delayDepth { 0.5f };
    float m_delayMix { 0.0f };
    bool m_delaySync { false };
    float m_delaySyncDivision { 0.25f };

    int m_currentBank = 0;
    UserPresets m_userPresets;

    //! Oversampling factor of the block being rendered, so voices can compensate their noise.
    uint8_t m_oversampleFactor { 1 };
    Decimator m_downsamplerL;
    Decimator m_downsamplerR;

    //! Hard sync resets VCO2 mid-ramp, and cross mod moves the reset point every sample, so the
    //! truncated ramp has a mean that is both non-zero and wandering. Nothing downstream is
    //! guaranteed to remove it: the HPF is bypassed at cutoff zero.
    DcBlocker m_dcBlockerL;
    DcBlocker m_dcBlockerR;

    std::vector<float> m_oversampledBuffer;

    double m_vco1BasePitchRatio { 1.0 };
    double m_vco2BasePitchRatio { 1.0 };
    double m_vco3BasePitchRatio { 1.0 };

    void handleNoteOn(uint8_t note, uint8_t velocity);
    //! Starts a note on the given voice through whichever path the Phase Sync switch selects.
    void startVoice(Voice & voice, const Trigger & trigger);
    //! Mono voice allocation: one voice, envelopes retriggered per note, pitch and phase carried over.
    void handleMonoNoteOn(uint8_t note, double frequency, float velocity);
    void handleNoteOff(uint8_t note);
    double midiNoteToFreq(uint8_t note) const;
    //! \param authored Whether the preset is the user's choice or a program change from a song.
    //! A program change writes the live layer only, so stopping playback brings the patch back.
    void applyPreset(int bank, int index, bool authored);

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
    //! Voices the current voice mode spends on a single note. Poly plays one, dual pairs two and
    //! unison stacks the lot.
    int voicesPerNote() const;

    //! Detune of the given voice in semitones, for the current voice mode. Both the note-on and
    //! the per-block parameter update read it from here so the two can never disagree.
    double voiceDetuneSemitones(size_t index) const;
    //! Per-voice gain that keeps one note at the same level in every voice mode.
    float voiceStackNormalization() const;
    //! Position in the stereo field of the given pan-spread slot. Slots alternate sides and close in
    //! towards the centre, so a stack stays balanced however many of them are sounding.
    float voiceSpreadPan(size_t slot) const;
    //! Corner for the voice's damping filter, or 0 when the mode damps nothing.
    double voiceDampingHz(size_t index) const;

    void renderVoice(Voice & voice, AudioContext & context, uint8_t oversampleFactor, uint32_t oversampledRate, double portamentoCoeff, double pbRatio, size_t index);
    void applyGlobalEffects(AudioContext & context);

    std::string m_name;
};

} // namespace noteahead

#endif // SYNTH_DEVICE_HPP
