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

#ifndef FM_SYNTH_DEVICE_HPP
#define FM_SYNTH_DEVICE_HPP

#include "../effects/delay.hpp"

#include "../dsp/adsr_envelope.hpp"
#include "../dsp/cascaded_svf.hpp"
#include "../dsp/dc_blocker.hpp"
#include "../dsp/fm_operator.hpp"
#include "../dsp/lfo.hpp"
#include "../dsp/one_pole_filter.hpp"
#include "../dsp/upsampler.hpp"
#include "device.hpp"

#include <array>
#include <cstdint>
#include <map>
#include <optional>
#include <random>
#include <string>
#include <vector>

namespace noteahead {

//! A four-operator phase-modulation synth, in the shape of Yamaha's four-operator machines.
//!
//! Four operators, eight algorithms, feedback on the top operator of every one of them. What makes
//! it an FM synth rather than a subtractive one with sine oscillators is that each operator carries
//! its own envelope: the timbre is a moving thing, because the operators that shape it fade in and
//! out at their own rates while the note is held.
class FmSynthDevice : public Device
{
    Q_OBJECT

public:
    static constexpr int MaxVoices = 8;
    static constexpr size_t OperatorCount = 4;
    static constexpr size_t AlgorithmCount = 8;

    //! One algorithm: who modulates whom, and who is heard.
    //!
    //! Both fields are bit masks over the operators, bit 0 being operator 1. @c modulators[i] names
    //! the operators whose output is added to operator i's phase, and is always a subset of the
    //! operators above i -- the render order depends on it, so the table is asserted against that.
    struct Algorithm
    {
        std::array<uint8_t, OperatorCount> modulators;
        uint8_t carriers;
    };

    static const std::array<Algorithm, AlgorithmCount> & algorithms();
    static std::vector<std::string> algorithmNames();

    //! Operator that feeds back into itself, the same one in every algorithm: the top of the chain.
    static constexpr size_t FeedbackOperator = OperatorCount - 1;

    enum class ModTarget
    {
        Cutoff,
        Pitch,
        //! Scales every modulation path at once, which is the FM synth's brightness control.
        ModIndex,
        Feedback
    };

    enum class LfoTarget
    {
        Pitch,
        Cutoff,
        ModIndex,
        Volume,
        Resonance,
        Pan
    };

    //! Serialized as a raw ordinal, so this is append-only. The order matches SynthDevice's and
    //! WavetableSynthDevice's, including Mono sitting last rather than next to Poly.
    enum class VoiceMode
    {
        Poly,
        Unison,
        Dual,
        Supersaw,
        Drift,
        Mono
    };

    static bool isStacked(VoiceMode mode);

    static constexpr int SupersawVoices = 7;

    explicit FmSynthDevice(std::string name);
    ~FmSynthDevice() override;

    std::string name() const override;
    std::string category() const override;
    std::string typeName() const override;
    std::string typeId() const override;

    std::vector<MidiCcController> deviceMidiCcControllers() const override;

    static std::string typeIdString();

    void processMidiNoteOn(uint8_t note, uint8_t velocity) override;
    void processMidiNoteOff(uint8_t note) override;
    void processDeviceMidiCc(uint8_t controller, uint8_t value, uint8_t channel) override;
    void processMidiPitchBend(uint16_t value, uint8_t channel) override;
    void processMidiAllNotesOff() override;

    void processAudio(AudioContext & context) override;
    bool hasActiveAudio() const override;

    void setBpm(float bpm) override;

    void reset() override;
    void resetAudio() override;

    void serializeToXml(ProjectWriter & writer) const override;
    void deserializeFromXml(ProjectReader & reader) override;

    //! Loads factory preset @p index. A preset is the whole panel: everything it does not name
    //! goes back to its default first, so nothing carries over from the patch that was there.
    void loadPreset(int index);

    //! Loads a patch assembled at random. @p seed selects it, so the same seed gives the same one.
    void loadRandomPatch(uint32_t seed);

    //! Frequency voice @p index is currently gliding towards, or 0 if it has never been triggered.
    double voiceGlideFrequency(size_t index) const;

    //! Frequency ratio the given ratio setting stands for. The grid is the DX one: a half, and then
    //! the whole numbers. Anything but a whole ratio puts the sidebands off the harmonic series,
    //! which is where the bells and the metal come from.
    static double ratioForSetting(int setting);
    static constexpr int MaxRatioSetting = 31;

    // Operators
    FmOperator::Waveform operatorWaveform(size_t index) const;
    void setOperatorWaveform(size_t index, FmOperator::Waveform waveform);
    int operatorRatio(size_t index) const;
    void setOperatorRatio(size_t index, int setting);
    float operatorDetune(size_t index) const;
    void setOperatorDetune(size_t index, float detune);
    float operatorLevel(size_t index) const;
    void setOperatorLevel(size_t index, float level);
    float operatorVelocitySensitivity(size_t index) const;
    void setOperatorVelocitySensitivity(size_t index, float sensitivity);
    float operatorKeyScale(size_t index) const;
    void setOperatorKeyScale(size_t index, float keyScale);
    float operatorAttack(size_t index) const;
    void setOperatorAttack(size_t index, float a);
    float operatorDecay(size_t index) const;
    void setOperatorDecay(size_t index, float d);
    float operatorSustain(size_t index) const;
    void setOperatorSustain(size_t index, float s);

    // Algorithm
    int algorithm() const;
    void setAlgorithm(int index);
    float feedback() const;
    void setFeedback(float feedback);

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
    float ampVelocitySensitivity() const;
    void setAmpVelocitySensitivity(float sensitivity);

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

    // Delay
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
    void setDelayFeedbackLpf(float cutoff);
    float delayFeedbackHpf() const;
    void setDelayFeedbackHpf(float cutoff);

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

private:
    struct Voice
    {
        std::array<FmOperator, OperatorCount> operators;
        CascadedSvf lpf;
        CascadedSvf hpf;
        AdsrEnvelope ampEg;
        AdsrEnvelope modEg;
        Lfo lfo;
        Lfo lfo2;
        //! The waveforms are zero-mean already, so this has nothing to do on an ordinary patch.
        //! It is here for the paths that can still manufacture an offset out of symmetric inputs:
        //! a feedback loop turned up far enough to go chaotic, and the filters' own nonlinearity.
        //! Taken off the carrier sum, before the filters and before the amp envelope.
        DcBlocker dcBlocker;

        uint8_t note { 0 };
        uint64_t triggerId { 0 };
        bool active { false };
        float velocity { 1.0f };
        double frequency { 0.0 };
        double glideFrequency { 0.0 };
        float pan { 0.5f };
        double driftPhase { 0.0 };
        double driftRate { 0.2 };
        OnePoleFilter damping;

        void reset();
        void trigger(uint8_t note, double freq, float pan, float velocity, uint64_t triggerId, double startPhase = 0.0);

        //! Lets the note go.
        //!
        //! The operators are deliberately not released with it: they hold wherever their own
        //! envelopes have reached, and the amp envelope alone fades the voice out. The note
        //! therefore fades as the sound it currently is.
        //!
        //! Two envelopes in series on one tail cannot both be right. Released together, the shorter
        //! silently wins and the Amp EG's release does nothing; released only on the modulators,
        //! the sidebands collapse at note off and a rich patch drops to a bare sine before it
        //! fades. An operator's decay and sustain still move the timbre for as long as the note is
        //! held, which is where FM's movement actually lives -- a modulator set to no sustain has
        //! faded out long before the key is released.
        void release();
    };

    std::vector<Voice> m_voices;
    size_t m_polyNextVoice { 0 };
    size_t m_dualNextPair { 0 };
    std::optional<size_t> m_monoPanSlot;
    uint64_t m_nextTriggerId { 1 };

    static constexpr uint32_t RngSeed { 2003 };
    mutable std::mt19937 m_rng { RngSeed };
    mutable std::uniform_real_distribution<double> m_phaseDist { 0.0, 1.0 };

    // Internal parameter storage
    std::array<int, OperatorCount> m_operatorWaveform {};
    std::array<int, OperatorCount> m_operatorRatio {};
    std::array<float, OperatorCount> m_operatorDetune {};
    std::array<float, OperatorCount> m_operatorLevel {};
    std::array<float, OperatorCount> m_operatorVelocitySensitivity {};
    std::array<float, OperatorCount> m_operatorKeyScale {};
    std::array<float, OperatorCount> m_operatorAttack {};
    std::array<float, OperatorCount> m_operatorDecay {};
    std::array<float, OperatorCount> m_operatorSustain {};
    //! The ratio setting turned into the multiplier it stands for, once per parameter change.
    std::array<double, OperatorCount> m_operatorRatioValue {};
    //! Detune knob turned into a frequency multiplier, likewise.
    std::array<double, OperatorCount> m_operatorDetuneRatio {};

    int m_algorithm { 0 };
    float m_feedback { 0.0f };

    float m_lpfCutoff { 1.0f };
    float m_lpfResonance { 0.0f };
    float m_hpfCutoff { 0.0f };

    float m_ampAttack { 0.1f };
    float m_ampDecay { 0.2f };
    float m_ampSustain { 1.0f };
    float m_ampRelease { 0.2f };
    float m_ampCurve { 0.0f };
    float m_ampVelocitySensitivity { 1.0f };

    float m_modAttack { 0.1f };
    float m_modDecay { 0.2f };
    float m_modSustain { 0.0f };
    float m_modInt { 0.5f };
    double m_modDepth { 0.0 };
    ModTarget m_modTarget { ModTarget::ModIndex };
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

    //! The same delay the Synth carries, and for the same reason: an FM patch is dry by nature, and
    //! the sound most people picture is one with a delay on it. It runs after the voices are summed
    //! and downsampled.
    Delay m_delay;
    Delay::Type m_delayType { Delay::Type::Stereo };
    float m_delayTime { 0.5f };
    float m_delayFeedback { 0.3f };
    float m_delayDepth { 0.5f };
    float m_delayMix { 0.0f };
    bool m_delaySync { false };
    float m_delaySyncDivision { 0.25f };

    uint16_t m_pitchBend { 8192 };
    int m_pitchBendRange { 2 };

    void handleNoteOn(uint8_t note, uint8_t velocity);
    void handleNoteOff(uint8_t note);
    void applyPreset(const std::map<std::string, float> & values);
    void handleMonoNoteOn(uint8_t note, double frequency, float velocity);
    void releaseVoicesAbove(size_t count);
    double midiNoteToFreq(uint8_t note) const;
    void syncParameters() override;

    struct ModulationValues
    {
        double ampEnvelope { 0.0 };
        double modEnvelope { 0.0 };
        double lfoValue { 0.0 };
        double lfo2Value { 0.0 };
        double cutoffMod { 0.0 };
        double pitchMod { 0.0 };
        //! Multiplier on every modulation path, never negative: a negative index would invert the
        //! modulator rather than quieten it, which is not what a brightness control does.
        double indexMod { 1.0 };
        double feedbackMod { 0.0 };
        double resonanceMod { 0.0 };
        double panMod { 0.0 };
        double volumeMod { 0.0 };
    };

    ModulationValues calculateModulation(Voice & voice) const;
    float generateVoiceSample(Voice & voice, const ModulationValues & mods, double oversampledRate, double pbRatio);

    void prepareForProcessing(AudioContext & context);
    void updateVoiceParameters(Voice & voice, uint32_t oversampledRate, size_t index);
    //! Level operator @p op of @p voice runs at, once its velocity sensitivity and key scaling have
    //! been applied. Depends on the note, so it cannot be folded into syncParameters().
    float scaledOperatorLevel(const Voice & voice, size_t op) const;
    int voicesPerNote() const;
    double voiceDetuneSemitones(size_t index) const;
    float voiceLevel(size_t index) const;
    float voiceStackNormalization() const;
    float voiceSpreadPan(size_t slot) const;
    double voiceDampingHz(size_t index) const;

    void renderVoice(Voice & voice, AudioContext & context, uint8_t oversampleFactor, uint32_t oversampledRate, double portamentoCoeff, double pbRatio, size_t index);

    std::string m_name;

    std::vector<float> m_oversampledBuffer;
    Decimator m_downsamplerL;
    Decimator m_downsamplerR;
};

} // namespace noteahead

#endif // FM_SYNTH_DEVICE_HPP
