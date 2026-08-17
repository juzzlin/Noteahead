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

#include "../dsp/adsr_envelope.hpp"
#include "../dsp/cascaded_svf.hpp"
#include "../dsp/lfo.hpp"
#include "../dsp/one_pole_filter.hpp"
#include "../dsp/upsampler.hpp"
#include "../dsp/wavetable_oscillator.hpp"
#include "device.hpp"

#include <map>
#include <mutex>
#include <optional>
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
        Osc2Pos,
        Volume,
        Resonance,
        Pan
    };

    //! Serialized as a raw ordinal, so this is append-only: inserting a value would silently change
    //! the voice mode of every project saved before the change. The order matches SynthDevice's.
    enum class VoiceMode
    {
        Poly,
        Unison,
        Dual,
        //! JP-8000 style: unevenly spaced detune around a centre voice held at full level, so the
        //! stack keeps a solid core pitch instead of equal voices smearing it.
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

    //! Voices a supersaw stack takes. One short of the pool: the JP-8000 arrangement is seven saws.
    static constexpr int SupersawVoices = 7;

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

    void serializeToXml(ProjectWriter & writer) const override;
    void deserializeFromXml(ProjectReader & reader) override;

    //! Frequency voice @p index is currently gliding towards, or 0 if it has never been triggered.
    //! Exposed so tests can read the detune a voice mode hands out without rendering audio.
    double voiceGlideFrequency(size_t index) const;

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

    // LFO 1
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

    // LFO 2
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

    // Wavetable selection
    int wavetableIndex() const;
    void setWavetableIndex(int index);
    std::vector<std::string> wavetableNames() const;

    //! Builds the given set into the shared cache if it isn't there yet. Call this off the audio
    //! path before selecting a set: the selection itself runs under the device lock, and building
    //! a set there would hold the audio thread off for as long as it takes.
    static void prepareWavetable(int index);

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
        Lfo lfo2;

        uint8_t note { 0 };
        uint64_t triggerId { 0 };
        bool active { false };
        float velocity { 1.0f };
        double frequency { 0.0 };
        double glideFrequency { 0.0 };
        float pan { 0.5f };
        //! Drift mode's per-voice wander. The rates are set apart at construction so that no two
        //! voices ever settle into a steady beat.
        double driftPhase { 0.0 };
        double driftRate { 0.2 };
        //! Takes the edge off the outer voices of a wide stack. Idle in the modes that do not stack.
        OnePoleFilter damping;

        void reset();
        //! @p startPhase spreads unison voices apart. Voices started at the same phase stay
        //! correlated and simply sum, which is a level jump rather than the thickening unison is
        //! for; poly needs no spread and leaves it at zero.
        void trigger(uint8_t note, double freq, float pan, float velocity, uint64_t triggerId, double startPhase = 0.0);
        void release();
    };

    std::vector<Voice> m_voices;
    size_t m_polyNextVoice { 0 };
    size_t m_dualNextPair { 0 };
    //! Pan slot the sounding mono note took, so a mono line still travels across the field.
    std::optional<size_t> m_monoPanSlot;
    uint64_t m_nextTriggerId { 1 };

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
    //! Compensation for drawing one noise sample per clock at the oversampled rate.
    float m_noiseOversampleGain { 1.0f };

    float m_lpfCutoff { 1.0f };
    float m_lpfResonance { 0.0f };
    float m_hpfCutoff { 0.0f };

    float m_ampAttack { 0.01f };
    float m_ampDecay { 0.1f };
    float m_ampSustain { 1.0f };
    float m_ampRelease { 0.1f };
    float m_ampCurve { 0.0f };

    float m_modAttack { 0.01f };
    float m_modDecay { 0.1f };
    float m_modSustain { 0.0f };
    float m_modInt { 0.5f };
    //! m_modInt is the knob position, this is the depth it stands for once tapered.
    double m_modDepth { 0.0 };
    ModTarget m_modTarget { ModTarget::Cutoff };
    float m_modCurve { 0.0f };

    Lfo::Waveform m_lfoWaveform { Lfo::Waveform::Triangle };
    Lfo::Mode m_lfoMode { Lfo::Mode::Normal };
    float m_lfoRate { 0.5f };
    float m_lfoInt { 0.5f };
    double m_lfoDepth { 0.0 };
    LfoTarget m_lfoTarget { LfoTarget::Pitch };

    Lfo::Waveform m_lfo2Waveform { Lfo::Waveform::Triangle };
    Lfo::Mode m_lfo2Mode { Lfo::Mode::Normal };
    float m_lfo2Rate { 0.5f };
    float m_lfo2Int { 0.5f };
    double m_lfo2Depth { 0.0 };
    LfoTarget m_lfo2Target { LfoTarget::Pitch };

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
        double lfo2Value { 0.0 };
        double cutoffMod { 0.0 };
        double osc1PosMod { 0.0 };
        double osc2PosMod { 0.0 };
        double resonanceMod { 0.0 };
        double panMod { 0.0 };
        double volumeMod { 0.0 };
    };

    ModulationValues calculateModulation(Voice & voice) const;
    float generateVoiceSample(Voice & voice, const ModulationValues & mods, double oversampledRate, double pbRatio);

    void prepareForProcessing(AudioContext & context);
    void updateVoiceParameters(Voice & voice, uint32_t oversampledRate, size_t index);
    //! Voices the current voice mode spends on a single note. Poly and mono play one, dual pairs
    //! them up, and the stacked modes take the lot.
    int voicesPerNote() const;
    //! Detune of the stack's voice @p index, in semitones. Zero for the modes that do not stack.
    double voiceDetuneSemitones(size_t index) const;
    //! Level weighting the current voice mode gives this voice. Only Supersaw uses anything but
    //! unity: its centre saw and its side saws follow different curves as the detune opens up.
    float voiceLevel(size_t index) const;
    //! Gain that keeps one note at the same loudness whatever the voice mode.
    float voiceStackNormalization() const;
    //! Pan for the voice occupying @p slot, spread out from the centre by the pan spread knob.
    float voiceSpreadPan(size_t slot) const;
    //! Corner for the voice's damping filter, or 0 when the mode damps nothing.
    double voiceDampingHz(size_t index) const;

    void handleMonoNoteOn(uint8_t note, double frequency, float velocity);
    //! Silences the voices past the end of the current stack, which a mode change can leave ringing.
    void releaseVoicesAbove(size_t count);

    void renderVoice(Voice & voice, AudioContext & context, uint8_t oversampleFactor, uint32_t oversampledRate, double portamentoCoeff, double pbRatio, size_t index);

    std::string m_name;

    std::vector<float> m_oversampledBuffer;
    Decimator m_downsamplerL;
    Decimator m_downsamplerR;
};

} // namespace noteahead

#endif // WAVETABLE_SYNTH_DEVICE_HPP
