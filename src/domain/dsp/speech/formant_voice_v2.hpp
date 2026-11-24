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

#ifndef FORMANT_VOICE_V2_HPP
#define FORMANT_VOICE_V2_HPP

#include "../cascaded_svf.hpp"
#include "../dsp_component.hpp"
#include "../one_pole_filter.hpp"
#include "glottal_source_v2.hpp"
#include "phoneme.hpp"

#include <array>
#include <random>
#include <string_view>

namespace noteahead {

//! Second-generation speaking voice using a Klatt-style hybrid cascade/parallel vocal tract.
//!
//! Voiced glottal flow and aspiration excite a six-formant cascade chain (F1..F6) preceded by a
//! nasal pole-zero antiresonator, giving vowels and nasals a natural spectral envelope and nasal
//! zeros without spurious inter-formant notches. Turbulent fricatives and plosive bursts run in
//! parallel through broadband bandpass resonators.
//!
//! Unlike FormantVoice, the cascade tract never snaps to noise-band centres or resets at consonant
//! boundaries: every consonant supplies a place-of-articulation locus so CV and VC transitions
//! bend F1/F2/F3 smoothly into and out of adjacent vowels.
class FormantVoiceV2 : public DspComponent
{
public:
    //! Extra timbral character shaping for synthetic, robotic, monster, and creature voices.
    struct VoiceCharacter
    {
        double bandwidthScale { 1.0 };
        double f2Stretch { 1.0 };
        double f3Stretch { 1.0 };
        double ringModHz { 0.0 };
        double ringModDepth { 0.0 };
        double fmWarbleHz { 0.0 };
        double fmWarbleSemitones { 0.0 };
        double formantWobbleHz { 0.0 };
        double formantWobbleDepth { 0.0 };
        double combDelayMs { 0.0 };
        double combFeedback { 0.0 };
        double combMix { 0.0 };
        double drive { 0.0 };
        double whisperMix { 0.0 };
        double subOctaveMix { 0.0 };
        double sampleHoldHz { 0.0 };
        int bitCrushSteps { 0 };
        int steppedControlDiv { 1 };
        bool bypassPhaseDispersion { false };
    };

    FormantVoiceV2();

    void setSampleRate(double sampleRate) override;

    //! Fundamental of the voiced source, in Hz.
    void setFrequency(double frequency);

    //! Sets the current phoneme, upcoming phoneme for coarticulation/aspiration, and segment duration.
    void setPhoneme(const PhonemeSpec & spec, const PhonemeSpec * next = nullptr, double seconds = 0.0);

    //! Marks whether the current phoneme belongs to a stressed syllable (unstressed vowels centralize slightly).
    void setPhonemeStressed(bool stressed);

    //! Progress within the current phoneme, 0..1.
    void setPhonemeProgress(double progress);

    void setGlideTime(double seconds);
    void setFormantShift(double shift);
    void setSourceRolloff(double frequency);
    void setBreathiness(double breathiness);

    void setOpenQuotient(double openQuotient);
    void setVoicePerturbation(double perturbation);
    void setSpeedQuotient(double speedQuotient);
    void setReturnPhase(double returnPhase);
    void setSubharmonic(double subharmonic);
    void setPhraseDynamics(double amount);
    void setUtteranceProgress(double progress);

    void setConsonantLevel(double level);
    void setSibilance(double sibilance);
    void setVoiceCharacter(const VoiceCharacter & character);

    double nextSample();

    void reset();

private:
    struct FormantPair
    {
        double frequency { 500.0 };
        double bandwidth { 100.0 };
    };

    struct NasalSpec
    {
        double fnp { 260.0 };
        double bnp { 90.0 };
        double fnz { 260.0 };
        double bnz { 90.0 };
    };

    struct Resonator
    {
        double a { 1.0 };
        double b { 0.0 };
        double c { 0.0 };
        double y1 { 0.0 };
        double y2 { 0.0 };

        void configure(double frequency, double bandwidth, double sampleRate);
        double process(double input);
        void reset();
    };

    struct NasalPoleZero
    {
        double b0 { 1.0 };
        double b1 { 0.0 };
        double b2 { 0.0 };
        double a1 { 0.0 };
        double a2 { 0.0 };
        double s1 { 0.0 };
        double s2 { 0.0 };

        void configure(double fnp, double bnp, double fnz, double bnz, double sampleRate);
        double process(double input);
        void reset();
    };

    struct Biquad
    {
        double b0 { 1.0 };
        double b1 { 0.0 };
        double b2 { 0.0 };
        double a1 { 0.0 };
        double a2 { 0.0 };
        double s1 { 0.0 };
        double s2 { 0.0 };

        void configurePeaking(double fc, double gainDb, double q, double sampleRate);
        void configureHighPass(double fc, double q, double sampleRate);
        void configureLowPass(double fc, double q, double sampleRate);
        void configureAllPass(double fc, double radius, double sampleRate);
        double process(double input);
        void reset();
    };

    static constexpr int ControlBlockFrames { 32 };

    static constexpr double JitterRange { 0.012 };
    static constexpr double ShimmerRange { 0.05 };

    //! Calibrated against SpeechDevice so phrases sit at matching RMS with clear headroom.
    static constexpr double CascadeMakeupGain { 9.0 };
    static constexpr double FricationMakeupGain { 1.0 };
    static constexpr double BurstMakeupGain { 0.75 };
    static constexpr double FinalStopBurstScale { 0.52 };
    static constexpr double VoicedStopBurstScale { 0.45 };
    static constexpr double SourceMakeupGain { 0.80 };

    static constexpr double GlottalChinkLevel { 0.010 };
    static constexpr double HhAspirationLevel { 0.0024 };
    static constexpr double PlosiveAspirationLevel { 0.011 };
    static constexpr double WhisperNoiseLevel { 0.045 };
    static constexpr double FinalAspirationScale { 0.40 };

    static constexpr double MinNoiseRolloffFrequency { 3500.0 };
    static constexpr double MaxNoiseRolloffFrequency { 11000.0 };
    static constexpr double SibilanceLevelRange { 2.0 };
    static constexpr double NoiseSourceLevel { 0.30 };

    static constexpr double LevelGlideTime { 0.008 };
    static constexpr double VoicingAttackTime { 0.016 };
    static constexpr double VoicingReleaseTime { 0.010 };
    static constexpr double OcclusionGlideTime { 0.010 };
    static constexpr double VoicingOnsetSeconds { 0.022 };
    static constexpr double PhraseFinalStart { 0.72 };
    static constexpr double AspiratedReleaseSeconds { 0.040 };
    static constexpr double UnaspiratedReleaseSeconds { 0.012 };
    static constexpr double FinalReleaseSeconds { 0.014 };
    static constexpr double UnvoicedClosureFraction { 0.45 };
    static constexpr double VoicedClosureFraction { 0.88 };
    static constexpr double MinimumClosureFraction { 0.35 };
    static constexpr double MaximumClosureFraction { 0.92 };
    static constexpr double PlosiveBurstDecayTime { 0.0065 };
    static constexpr double PlosiveBurstSeconds { 0.005 };
    static constexpr double AspirationAttackTime { 0.003 };
    static constexpr double AspirationDecayTime { 0.022 };
    static constexpr double AspirationTailDecayTime { 0.006 };
    static constexpr double PlosiveBurstAttackTime { 0.0018 };
    static constexpr double SeamFadeTime { 0.004 };

    static constexpr std::array<FormantPair, 3> UpperCascadeFormants { {
      { 3500.0, 230.0 },
      { 4500.0, 280.0 },
      { 5500.0, 350.0 },
    } };

    static constexpr unsigned int RngSeed { 0xB0B2 };

    static std::array<FormantPair, 3> consonantLocus(std::string_view name, const PhonemeSpec * adjacentVowel);
    static NasalSpec nasalTargetsFor(const PhonemeSpec & spec, const PhonemeSpec * next, double progress);

    void updateControl();
    void configureTractPostFilters();
    std::array<FormantPair, 3> effectiveCascadeTargets() const;
    NasalSpec effectiveNasalTargets() const;
    double closureFraction() const;
    bool isPhraseFinalStop() const;
    bool isSoftStopRelease() const;
    bool isReleased() const;
    bool shouldAspirate() const;
    double voicingTargetFor(const PhonemeSpec & spec) const;
    double fricationTargetFor(const PhonemeSpec & spec) const;
    double aspirationTargetFor(const PhonemeSpec & spec) const;
    void applyCascadeTargets(const std::array<FormantPair, 3> & targets, const NasalSpec & nasal, bool immediate, bool fastClosure = false);
    double parallelBankOutput(std::array<CascadedSvf, 3> & filters, const PhonemeSpec & spec, double input);
    void tuneFricationFilters(const FormantTargets & targets);

    const PhonemeSpec * m_phoneme { nullptr };
    const PhonemeSpec * m_nextPhoneme { nullptr };
    const PhonemeSpec * m_lastVowel { nullptr };
    double m_progress { 0.0 };
    double m_phonemeSeconds { 0.0 };
    bool m_stressed { true };

    double m_configuredSampleRate { 0.0 };
    double m_frequency { 110.0 };
    double m_glideTime { 0.03 };
    double m_formantShift { 1.0 };
    double m_configuredFilterShift { 0.0 };
    double m_configuredWhisperMix { -1.0 };
    double m_sourceRolloff { 0.0 };
    double m_breathiness { 0.0 };
    double m_effectiveBreathiness { 0.0 };
    double m_openQuotient { 0.5 };
    double m_effectiveOpenQuotient { 0.5 };
    double m_returnPhase { 0.010 };
    double m_basePerturbation { 0.0 };
    double m_baseSubharmonic { 0.0 };
    double m_phraseDynamics { 0.0 };
    double m_utteranceProgress { 0.0 };
    double m_consonantLevel { 1.0 };
    double m_sibilance { 0.5 };

    std::array<FormantPair, 3> m_currentCascade {};
    NasalSpec m_currentNasal {};
    std::array<Resonator, 6> m_cascadeFormants {};
    NasalPoleZero m_nasalPoleZero {};
    std::array<Biquad, 2> m_hpc {};
    Biquad m_cascadeCeiling {};
    std::array<Biquad, 3> m_allPass {};

    double m_voicingLevel { 0.0 };
    double m_voicingTarget { 0.0 };
    double m_voicingAttackCoeff { 0.0 };
    double m_voicingReleaseCoeff { 0.0 };
    double m_voicedAge { 0.0 };
    double m_fricationLevel { 0.0 };
    double m_fricationTarget { 0.0 };
    double m_previousFricationLevel { 0.0 };
    double m_aspirationLevel { 0.0 };
    double m_aspirationTarget { 0.0 };
    double m_levelCoefficient { 0.0 };
    double m_glideCoefficient { 0.0 };
    double m_occlusionGlideCoeff { 0.0 };

    bool m_afterUnvoicedFricative { false };
    bool m_plosiveReleased { false };
    bool m_phraseFinalRelease { false };
    bool m_aspirating { false };
    size_t m_framesSinceRelease { 0 };
    double m_burstRise { 0.0 };
    double m_burstFall { 0.0 };
    double m_burstAttack { 0.0 };
    double m_burstDecay { 0.0 };
    double m_aspirationEnv { 0.0 };
    double m_aspirationCurrent { 0.0 };
    double m_aspirationAttackCoeff { 0.0 };
    double m_aspirationDecay { 0.0 };
    double m_aspirationTailDecay { 0.0 };

    double m_seam { 1.0 };
    double m_seamCoefficient { 0.0 };
    const PhonemeSpec * m_fricationSpec { nullptr };
    const PhonemeSpec * m_previousNoiseSpec { nullptr };
    std::array<CascadedSvf, 3> m_fricationFormants {};
    std::array<CascadedSvf, 3> m_previousFricationFormants {};

    GlottalSourceV2 m_glottis {};
    double m_radDecay { 0.99 };
    double m_radScale { 1.0 };
    double m_radPrev { 0.0 };
    Biquad m_glottalNoiseHp {};
    Biquad m_glottalNoiseLp {};
    Biquad m_aspirationHp {};
    Biquad m_aspirationLp {};
    OnePoleFilter m_sourceRolloffFilter {};
    std::array<OnePoleFilter, 2> m_noiseRolloff {};

    std::mt19937 m_rng { RngSeed };
    std::uniform_real_distribution<double> m_noise { -1.0, 1.0 };

    VoiceCharacter m_character {};
    std::array<double, 2048> m_combBuffer {};
    size_t m_combWriteIndex { 0 };
    double m_ringPhase { 0.0 };
    double m_fmPhase { 0.0 };
    double m_wobblePhase { 0.0 };
    double m_subPhase { 0.0 };
    double m_subPrev { 0.0 };
    double m_shCounter { 1.0 };
    double m_shValue { 0.0 };
    int m_controlStepSkip { 0 };

    int m_controlCounter { 0 };
};

} // namespace noteahead

#endif // FORMANT_VOICE_V2_HPP
