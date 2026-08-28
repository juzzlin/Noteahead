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

#ifndef FORMANT_VOICE_HPP
#define FORMANT_VOICE_HPP

#include "../cascaded_svf.hpp"
#include "../dsp_component.hpp"
#include "../one_pole_filter.hpp"
#include "../poly_blep_oscillator.hpp"
#include "phoneme.hpp"

#include <array>
#include <random>

namespace noteahead {

//! A single speaking voice: a glottal source and a bank of three formant resonances that move
//! between the targets of whatever phoneme it has been handed.
//!
//! The source is the textbook source-filter chain: a sawtooth through a glottal tilt, so it falls at
//! about 12 dB per octave the way a glottal pulse train does, and then radiation from the lips,
//! which is a differentiator and gives 6 dB per octave back. Noise is summed in according to the
//! phoneme's voicing. The bank sums with alternating polarity and
//! normalises each band by its own Q, again as FormantFilterBank does and for the same measured
//! reason: summed in phase, adjacent resonances cancel between the peaks and leave notches far
//! deeper than a vowel has.
//!
//! What makes it intelligible rather than a vocoder stepping between presets is that the targets
//! are reached through a glide. Consonant identity lives mostly in the *direction* the formants
//! travel out of a closure, not in the steady state either side of it, so the transitions carry
//! more of the message than the targets do.
//!
//! Plosives are the exception and are exempt from the glide: a stop is a silence followed by a
//! burst, and gliding into one smears it into the neighbouring vowel until it stops reading as a
//! stop at all. The voice runs their closure and release itself, which is why it needs to be told
//! how far through the phoneme it is rather than only which phoneme it is.
class FormantVoice : public DspComponent
{
public:
    FormantVoice();

    void setSampleRate(double sampleRate) override;

    //! Fundamental of the voiced source, in Hz.
    void setFrequency(double frequency);

    //! The phoneme now being spoken. Handing over a different spec re-arms a plosive's burst;
    //! handing over the same one again changes nothing.
    //! The phoneme now being spoken, and the one that follows it.
    //!
    //! The voice needs to see ahead because an unvoiced stop is not finished when its burst is: it is
    //! followed by aspiration, and that aspiration is shaped by the *next* phoneme's formants, not by
    //! the stop's. It is the sound of breath passing through a tract that has already taken up the
    //! position for the coming vowel, which is why it leads into that vowel instead of sitting there
    //! as a click.
    //! \param seconds How long the sequencer has given this phoneme. Needed because what follows a
    //! stop's release lasts a fixed time in speech however long the syllable is: stretching a stop
    //! stretches the silence before the burst, not the breath after it.
    void setPhoneme(const PhonemeSpec & spec, const PhonemeSpec * next = nullptr, double seconds = 0.0);

    //! How far through the current phoneme, 0..1. Drives the diphthong glide and places a plosive's
    //! release, both of which need to know the duration the sequencer chose.
    void setPhonemeProgress(double progress);

    //! Seconds a formant takes to cover most of the distance to a new target. Short is robotic,
    //! long is slurred, and the useful range is narrow.
    void setGlideTime(double seconds);

    //! Vocal tract length as a multiplier on every formant frequency. 1 is the male voice the table
    //! is written for; above shifts the whole vowel space up, which reads as a smaller speaker.
    void setFormantShift(double shift);

    //! Corner of an extra rolloff on the voiced source, in Hz, or 0 for none.
    //!
    //! The glottal source's slope, which is one of the things separating a woman's voice from a
    //! man's alongside the shorter tract that setFormantShift() stands for: hers falls away faster,
    //! and that is why it reads as softer rather than merely higher.
    //!
    //! It has to be its own filter. The glottal tilt cannot do it: folded together with lip
    //! radiation the pair is a one-pole *high* pass, which is flat above its corner, so moving that
    //! corner changes the bottom of the spectrum and leaves the top alone -- measured, it moved the
    //! 2-6 kHz band by under 3%.
    void setSourceRolloff(double frequency);

    //! Aspiration mixed into the voiced phonemes.
    void setBreathiness(double breathiness);

    //! Level of everything that is not a vowel, relative to the vowels.
    void setConsonantLevel(double level);

    //! How prominent the frication is, 0 to 1, with 0.5 the level the phoneme table is written for.
    //!
    //! Moves the level and the brightness of the fricatives together, which is what a listener means
    //! by a sibilant being too much: it is both loud and bright or it is neither. A separate control
    //! because how much sibilance is right is a matter of taste and of what else is in the mix,
    //! rather than something a table can settle once.
    void setSibilance(double sibilance);

    double nextSample();

    void reset();

private:
    //! Frames between control updates. The formant glide and the filter coefficients move at this
    //! rate rather than per sample: a coefficient update costs a tan() per band, and at 0.7 ms the
    //! staircase is far below anything a formant transition does audibly.
    static constexpr int ControlBlockFrames { 32 };

    //! Corner of the glottal tilt, in Hz. A property of the pulse the folds make rather than of the
    //! note being sung, so it does not track pitch.
    //!
    //! The tilt and the lip radiation are one filter rather than two. A one-pole low pass followed
    //! by a differentiator is, to within a constant, exactly a one-pole *high* pass at the same
    //! corner -- s/(1 + s/w) = w * (s/w)/(1 + s/w) -- so the pair collapses into the high-pass tap
    //! of the filter that was already computing the low-pass one, and costs nothing extra.
    //!
    //! This is not only tidier. Differentiating explicitly means subtracting adjacent samples, which
    //! multiplies whatever one-sample step the oscillator leaves behind: polyBLEP is a first-order
    //! correction and does not remove the saw's edge entirely, and the difference amplified what was
    //! left into an audible tick once per period. Taking the high-pass tap never forms that
    //! difference, so there is nothing to amplify.
    static constexpr double TiltFrequency { 200.0 };

    //! The one place the device's output level is decided.
    //!
    //! Set against loudness rather than against peaks. Matching a SynthDevice peak for peak, which
    //! is what this did, leaves the device 10 dB quieter than it in practice: a held synth note has a
    //! crest factor near 5 dB and a spoken phrase near 13, because speech is mostly consonants and
    //! transitions. A fader is set by what a thing sounds like, so the phrase's RMS is what has to
    //! line up -- calibrated peak-to-peak it needed some 20 dB of gain before it sat in a mix at all.
    //!
    //! The cost is peaks about 6 dB above the other devices', which is bought cheaply: there is still
    //! close to 20 dB of headroom above them.
    static constexpr double SourceMakeupGain { 0.56 };

    //! The two formants above the three the phoneme table gives, fixed rather than per-phoneme.
    //!
    //! F4 and F5 are properties of the speaker's tract rather than of the vowel being said, so they
    //! do not move and are not worth a column in the table. They matter anyway: without them the
    //! voice has nothing at all above 3 kHz, and a sibilant then measured 18 dB above the vowels in
    //! the 3-6 kHz band and 29 dB above them in the 6-10 kHz band -- not because /s/ was loud, its
    //! overall level was right, but because it was the only thing in the phrase with any high end.
    //! That is what made it read as piercing, and no amount of turning it down would have fixed it
    //! without turning it into a lisp.
    static constexpr FormantTarget UpperFormants[] {
        { 3300.0, 250.0, 0.30 },
        { 4300.0, 350.0, 0.16 }
    };

    //! Range the Sibilance control moves the frication rolloff over, in Hz. Down at the bottom the
    //! hiss is dark and soft; at the top it is as bright as the phoneme table asks for.
    static constexpr double MinNoiseRolloffFrequency { 3500.0 };
    static constexpr double MaxNoiseRolloffFrequency { 11000.0 };

    //! Level range the same control spans, as a multiplier on the fricatives.
    static constexpr double SibilanceLevelRange { 2.0 };

    //! Corner of the rolloff on the frication noise, in Hz.
    //!
    //! Real frication falls away above its peak; white noise does not. Two poles here take a few dB
    //! off the top of a sibilant without touching the band its identity lives in, which is what is
    //! left of "piercing" once the voice has a high end of its own to sit against.
    static constexpr double NoiseRolloffFrequency { 8000.0 };

    //! Noise level relative to the glottal source, measured rather than derived.
    //!
    //! Re-measured after the source was recalibrated twice: the first value was chosen to stop the
    //! sibilants clipping at a time when the whole device was 20 dB too hot, and once the master
    //! makeup was corrected the noise had effectively been attenuated twice over. That left /s/ at
    //! 22 dB under a vowel where it belongs nearer 12, and the stop bursts further still -- which is
    //! to say the most information-bearing consonants in the language were the quietest thing the
    //! device made.
    //!
    //! The two excite the bank very differently: the glottal source only has energy at multiples of
    //! the fundamental, while noise fills every band continuously, and the fricative bands are
    //! several hundred hertz wide where a vowel's are seventy. Left at parity the sibilants and the
    //! stop bursts came out three to six times full scale while the vowels sat at half, which is
    //! not a balance any amount of table tuning could fix -- the table's amplitudes have to mean
    //! the same thing whichever source is driving them.
    static constexpr double NoiseSourceLevel { 0.30 };

    //! Seconds the phoneme's overall level takes to move. Long enough not to click at a boundary,
    //! short enough not to smear a stop into its vowel.
    static constexpr double LevelGlideTime { 0.008 };

    //! Share of a plosive spent in closure before the burst is released.
    //!
    //! Seconds between an aspirated stop's release and the end of the phoneme: the burst plus the
    //! breath after it. A fixed time rather than a share of the phoneme, because that is what it is
    //! in speech -- when it was a share, a /t/ stretched to 216 ms by stress and phrase-final
    //! lengthening came out with 120 ms of breath after it, and a word like "destroy" was more
    //! frication than voice.
    //!
    //! Kept short for a second reason: noise shaped by a vowel is only breath while it is brief. Run
    //! for the length of a fricative -- and 55 ms is the length of a fricative -- it stops being
    //! heard as the release of a stop and becomes one, so "great" came out as "gray" plus a /sh/.
    static constexpr double AspiratedReleaseSeconds { 0.040 };

    //! The same for a stop that is not aspirated. Voicing follows almost immediately, so all that
    //! has to fit is the burst.
    static constexpr double UnaspiratedReleaseSeconds { 0.012 };

    //! Fallbacks for when nobody said how long the phoneme is.
    static constexpr double UnvoicedClosureFraction { 0.45 };
    static constexpr double VoicedClosureFraction { 0.88 };

    //! Share of a stop that must remain closure, however short the phoneme. A stop with no silence
    //! before its burst is not a stop.
    static constexpr double MinimumClosureFraction { 0.35 };
    static constexpr double MaximumClosureFraction { 0.92 };

    //! Seconds the release burst takes to decay away. A real burst is a transient of a few
    //! milliseconds; anything longer is a ring, and a ring on filtered noise is a cymbal.
    static constexpr double PlosiveBurstDecayTime { 0.004 };

    //! Seconds after the release at which aspiration takes over from the burst.
    static constexpr double PlosiveBurstSeconds { 0.006 };

    //! Level the aspiration starts at, as a fraction of the level the following phoneme will reach.
    //!
    //! Well below the burst, which is a transient and has to stand out as one. At a third of the
    //! coming vowel the burst was only four dB above the breath behind it, and the two read as a
    //! single flat slab of noise some fifty milliseconds long -- a shaker hit, not a /t/.
    static constexpr double AspirationLevel { 0.12 };

    //! Seconds over which the aspiration dies away.
    //!
    //! It has to be spent by the time voicing starts, and not merely for tidiness. Noise drives a
    //! resonator far harder than a harmonic source does, because it has energy at the resonance
    //! rather than only at the harmonics either side of it -- so a bank left fully excited by breath
    //! when the voice arrives on top of it overshoots, and the vowel after a stop peaked at over
    //! twice the level of the same vowel said on its own.
    static constexpr double AspirationDecayTime { 0.022 };

    //! Seconds the release burst takes to reach full. Short enough to still read as a stop -- a real
    //! burst rises inside a millisecond -- but not the instantaneous step it was, which is a click
    //! in anybody's book.
    static constexpr double PlosiveBurstAttackTime { 0.0006 };

    //! Seconds a snapped seam is crossfaded over.
    //!
    //! There is no way to move a ringing resonator to a new shape without an audible seam. Sweeping
    //! it chirps, and the further and slower the sweep the worse: /s/ to a vowel two octaves below
    //! measured a step of 30% of full scale mid-glide. Snapping the coefficients under the ringing
    //! state steps just as badly, and ducking the output to hide the snap only moves the step to the
    //! duck. So the old shape is not moved at all -- it is left running on its own resonators and
    //! faded out while the new shape, starting from rest, is faded in. Both ends are continuous
    //! because neither one jumps.
    //!
    //! Short enough to still read as a consonant boundary, which is an abrupt event.
    static constexpr double SeamFadeTime { 0.004 };

    //! Level of the buzz heard through a voiced stop's closure. All that separates B, D and G from
    //! P, T and K, since they share a burst spectrum.
    static constexpr double VoiceBarLevel { 0.07 };

    static constexpr unsigned int RngSeed { 0xB0B };

    void updateControl();
    FormantTargets effectiveTargets() const;
    //! The level a phoneme is spoken at, before articulation.
    double levelOf(const PhonemeSpec & spec) const;

    //! Whether the stop being spoken is released at all. A stop at the end of a phrase is not: an
    //! English speaker forms the closure and simply stops there, and the consonant is carried by the
    //! closure and by the formant transition out of the vowel before it. Releasing one into silence
    //! leaves a bare burst with nothing after it, which is a click -- and a click through resonant
    //! bands is a cymbal, which is what a word ending in /d/ sounded like.
    bool isReleased() const;
    //! Where in the phoneme the burst falls, as a fraction.
    double closureFraction() const;

    //! Whether an unvoiced stop should breathe into what follows it. Only worth doing into a shape
    //! the tract actually holds: aspirating into another stop or a fricative is not a thing a mouth
    //! does, and there would be nothing for the breath to be shaped by.
    bool shouldAspirate() const;
    void applyTargets(const FormantTargets & targets, bool immediate);
    double formantBankOutput(std::array<CascadedSvf, 3> & filters, const FormantTargets & targets, double input);
    //! The fixed upper formants' contribution, continuing the bank's alternating polarity.
    double upperFormantOutput(double input);

    const PhonemeSpec * m_phoneme { nullptr };
    double m_progress { 0.0 };

    double m_glideTime { 0.03 };
    double m_formantShift { 1.0 };
    double m_sourceRolloff { 0.0 };
    double m_breathiness { 0.0 };
    double m_consonantLevel { 1.0 };
    double m_sibilance { 0.5 };

    //! Where the bank is now, as opposed to where the phoneme wants it. Frequencies are glided in
    //! log space, which is both how a listener hears a formant move and what the filter's own
    //! cutoff mapping is already in.
    FormantTargets m_currentTargets {};

    double m_level { 0.0 };
    double m_levelTarget { 0.0 };
    //! The level the outgoing shape is ringing down at. Held separately because a seam is a
    //! crossfade and each side of it has its own level: sharing one, the quiet phoneme after a loud
    //! one was rendered at the loud one's level until the glide caught up, so every fricative after
    //! a vowel opened with a spike near three times its own level. That is what "overshooting"
    //! sounded like, and no amount of shaping the fricative itself would have touched it.
    double m_previousLevel { 0.0 };
    double m_levelCoefficient { 0.0 };
    double m_glideCoefficient { 0.0 };

    double m_phonemeSeconds { 0.0 };
    bool m_afterUnvoicedFricative { false };
    bool m_plosiveReleased { false };
    //! Whether the bank has been handed over to the coming phoneme for the aspiration.
    bool m_aspirating { false };
    size_t m_framesSinceRelease { 0 };
    const PhonemeSpec * m_nextPhoneme { nullptr };
    //! The stop's own gain: down through the closure, instantly up at the release, decaying after
    //! it, and smoothly back to unity once the next phoneme takes over. Separate from the level
    //! because a release has to be instant while every other level move has to be smoothed.
    double m_articulation { 1.0 };
    //! The release burst's own shape: a fast rise multiplied by an exponential fall.
    double m_burstRise { 0.0 };
    double m_burstFall { 0.0 };
    double m_burstAttack { 0.0 };
    double m_burstDecay { 0.0 };
    double m_aspirationDecay { 0.0 };

    //! 0 fully on the outgoing shape, 1 fully on the incoming one.
    double m_seam { 1.0 };
    double m_seamCoefficient { 0.0 };

    PolyBlepOscillator m_glottis;
    //! Glottal tilt and lip radiation together: the high-pass tap is the source.
    OnePoleFilter m_tilt;
    OnePoleFilter m_sourceRolloffFilter;
    std::array<OnePoleFilter, 2> m_noiseRolloff;
    std::array<CascadedSvf, 3> m_formants;
    //! The fixed upper pair. Outside the seam crossfade because their shape never changes, so there
    //! is never a discontinuity in them to cover.
    std::array<CascadedSvf, 2> m_upperFormants;
    //! How much of a tract shape is being held, smoothed. The upper formants belong to a vocal tract
    //! and not to a hiss, so they fade out over a fricative rather than colouring its noise.
    double m_tractAmount { 0.0 };
    //! The shape being faded out across a seam, left on its own resonators with its own coefficients
    //! so that what it was doing continues uninterrupted while it goes.
    std::array<CascadedSvf, 3> m_previousFormants;
    FormantTargets m_previousTargets {};

    std::mt19937 m_rng { RngSeed };
    std::uniform_real_distribution<double> m_noise { -1.0, 1.0 };

    int m_controlCounter { 0 };
};

} // namespace noteahead

#endif // FORMANT_VOICE_HPP
