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

#include "formant_voice.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace noteahead {

namespace {

//! One-pole coefficient reaching most of the way to a target in the given time.
double smoothingCoefficient(double seconds, double rate)
{
    if (seconds <= 0.0 || rate <= 0.0) {
        return 1.0;
    }
    return 1.0 - std::exp(-1.0 / (seconds * rate));
}

double lerp(double from, double to, double t)
{
    return from + (to - from) * t;
}

} // namespace

FormantVoice::FormantVoice()
{
    m_glottis.setWaveform(PolyBlepOscillator::Waveform::Saw);

    for (auto & filter : m_formants) {
        filter.setMode(CascadedSvf::Mode::BandPass);
        filter.setOrder(2);
    }
    for (auto & filter : m_previousFormants) {
        filter.setMode(CascadedSvf::Mode::BandPass);
        filter.setOrder(2);
    }
    for (auto & filter : m_upperFormants) {
        filter.setMode(CascadedSvf::Mode::BandPass);
        filter.setOrder(2);
    }
}

void FormantVoice::setSampleRate(double sampleRate)
{
    DspComponent::setSampleRate(sampleRate);

    m_glottis.setSampleRate(sampleRate);
    m_tilt.calculate(std::min(TiltFrequency, sampleRate * 0.45), sampleRate);
    setSourceRolloff(m_sourceRolloff);
    setSibilance(m_sibilance);

    for (auto & filter : m_formants) {
        filter.setSampleRate(sampleRate);
    }
    for (auto & filter : m_previousFormants) {
        filter.setSampleRate(sampleRate);
    }
    for (auto & filter : m_upperFormants) {
        filter.setSampleRate(sampleRate);
    }

    m_levelCoefficient = smoothingCoefficient(LevelGlideTime, sampleRate);
    m_glideCoefficient = smoothingCoefficient(m_glideTime, sampleRate / ControlBlockFrames);
    m_seamCoefficient = smoothingCoefficient(SeamFadeTime, sampleRate);
    m_burstAttack = smoothingCoefficient(PlosiveBurstAttackTime, sampleRate);
    m_burstDecay = sampleRate > 0.0 ? std::exp(-1.0 / (PlosiveBurstDecayTime * sampleRate)) : 0.0;
    m_aspirationDecay = sampleRate > 0.0 ? std::exp(-1.0 / (AspirationDecayTime * sampleRate)) : 0.0;
}

void FormantVoice::setFrequency(double frequency)
{
    m_glottis.setFrequency(frequency);
}

bool FormantVoice::isReleased() const
{
    return m_nextPhoneme && m_nextPhoneme->type != PhonemeType::Silence;
}

double FormantVoice::levelOf(const PhonemeSpec & spec) const
{
    if (spec.type == PhonemeType::Silence) {
        return 0.0;
    }
    const double consonant = spec.type == PhonemeType::Vowel ? 1.0 : m_consonantLevel;
    // Fricatives answer to Sibilance as well, which is the control for exactly them.
    const double sibilance = spec.type == PhonemeType::Fricative ? m_sibilance * SibilanceLevelRange : 1.0;
    return spec.amplitude * consonant * sibilance;
}

double FormantVoice::closureFraction() const
{
    if (!m_phoneme) {
        return UnvoicedClosureFraction;
    }
    if (m_phonemeSeconds <= 0.0) {
        return m_phoneme->voicing > 0.0 ? VoicedClosureFraction : UnvoicedClosureFraction;
    }
    const double release = shouldAspirate() ? AspiratedReleaseSeconds : UnaspiratedReleaseSeconds;
    return std::clamp(1.0 - release / m_phonemeSeconds, MinimumClosureFraction, MaximumClosureFraction);
}

bool FormantVoice::shouldAspirate() const
{
    // A stop after /s/ is not aspirated -- "stop" against "top" is the textbook pair -- and letting
    // one aspirate there put the breath immediately after the fricative's own noise, which read as
    // one long hiss rather than as two consonants.
    return m_phoneme
      && m_phoneme->type == PhonemeType::Plosive
      && m_phoneme->voicing <= 0.0
      && !m_afterUnvoicedFricative
      && m_nextPhoneme
      && (m_nextPhoneme->type == PhonemeType::Vowel
          || m_nextPhoneme->type == PhonemeType::Nasal
          || m_nextPhoneme->type == PhonemeType::Liquid);
}

void FormantVoice::setPhoneme(const PhonemeSpec & spec, const PhonemeSpec * next, double seconds)
{
    if (m_phoneme == &spec) {
        m_nextPhoneme = next;
        m_phonemeSeconds = seconds;
        return;
    }

    // Whether a phoneme's formants describe a shape the vocal tract was actually in. Vowels, nasals
    // and liquids do. A plosive's are the spectrum of its release burst and a fricative's are the
    // centres of its noise bands, neither of which the tract ever passed through on the way.
    const auto isTractShape = [](PhonemeType type) {
        return type != PhonemeType::Plosive && type != PhonemeType::Fricative;
    };

    const bool wasNoise = m_phoneme && !isTractShape(m_phoneme->type);
    const bool isNoise = !isTractShape(spec.type);

    // Read before the state is cleared below: aspiration has already walked the bank to this
    // phoneme's own shape, so there is no seam left to cover -- and covering one that is not there
    // would cut the breath off from the voice it leads into, which is the join it exists to make.
    const bool alreadyThere = m_aspirating && m_nextPhoneme == &spec;

    m_afterUnvoicedFricative = m_phoneme && m_phoneme->type == PhonemeType::Fricative && m_phoneme->voicing <= 0.0;
    m_phoneme = &spec;
    m_nextPhoneme = next;
    m_phonemeSeconds = seconds;
    m_progress = 0.0;
    m_plosiveReleased = false;
    m_aspirating = false;
    m_framesSinceRelease = 0;

    // So a transition to or from one of those is not an articulatory movement and is not glided.
    // Sweeping the bank between /s/, whose bands sit at 5.5 and 7.5 kHz, and a vowel two octaves
    // below measured as a step of 30% of full scale mid-glide -- a resonator swept that far while it
    // is still ringing chirps, and the longer the glide the worse it got. It is not what a listener
    // uses to hear the fricative either. Everything between two tract shapes still glides, which is
    // where the consonant cues actually live.
    const bool snap = (isNoise || wasNoise) && !alreadyThere;
    const FormantTargets outgoing = m_currentTargets;
    applyTargets(effectiveTargets(), snap);

    if (snap) {
        // Hand the outgoing shape its own resonators, coefficients and state intact, and start the
        // incoming one from rest.
        m_previousFormants = m_formants;
        m_previousTargets = outgoing;
        m_previousLevel = m_level;
        // The seam is the crossfade, so the incoming side needs no glide of its own: it starts at
        // its own level rather than sliding down from whatever preceded it.
        m_level = levelOf(spec);
        m_seam = 0.0;
        for (auto & filter : m_formants) {
            filter.reset();
        }
    }
}

void FormantVoice::setPhonemeProgress(double progress)
{
    m_progress = std::clamp(progress, 0.0, 1.0);
}

void FormantVoice::setGlideTime(double seconds)
{
    m_glideTime = std::max(0.0, seconds);
    m_glideCoefficient = smoothingCoefficient(m_glideTime, m_sampleRate / ControlBlockFrames);
}

void FormantVoice::setFormantShift(double shift)
{
    m_formantShift = std::max(0.1, shift);
}

void FormantVoice::setSourceRolloff(double frequency)
{
    m_sourceRolloff = std::max(0.0, frequency);
    if (m_sampleRate > 0.0 && m_sourceRolloff > 0.0) {
        m_sourceRolloffFilter.calculate(std::min(m_sourceRolloff, m_sampleRate * 0.45), m_sampleRate);
    }
}

void FormantVoice::setBreathiness(double breathiness)
{
    m_breathiness = std::clamp(breathiness, 0.0, 1.0);
}

void FormantVoice::setConsonantLevel(double level)
{
    m_consonantLevel = std::max(0.0, level);
}

void FormantVoice::setSibilance(double sibilance)
{
    m_sibilance = std::clamp(sibilance, 0.0, 1.0);
    if (m_sampleRate > 0.0) {
        const double corner = MinNoiseRolloffFrequency + (MaxNoiseRolloffFrequency - MinNoiseRolloffFrequency) * m_sibilance;
        for (auto & filter : m_noiseRolloff) {
            filter.calculate(std::min(corner, m_sampleRate * 0.45), m_sampleRate);
        }
    }
}

FormantTargets FormantVoice::effectiveTargets() const
{
    if (!m_phoneme) {
        return {};
    }

    // Once the burst is over, the tract is already in position for what comes next and the breath is
    // shaped by that, not by the stop.
    if (m_aspirating && m_nextPhoneme) {
        FormantTargets aspirated = m_nextPhoneme->formants;
        for (auto & target : aspirated) {
            target.frequency *= m_formantShift;
        }
        return aspirated;
    }

    FormantTargets targets = m_phoneme->formants;

    // A diphthong is a movement, so where it is aimed depends on how far through it we are.
    if (m_phoneme->glideTo.has_value()) {
        const auto & destination = m_phoneme->glideTo.value();
        for (size_t i = 0; i < targets.size(); i++) {
            targets[i].frequency = lerp(targets[i].frequency, destination[i].frequency, m_progress);
            targets[i].bandwidth = lerp(targets[i].bandwidth, destination[i].bandwidth, m_progress);
            targets[i].amplitude = lerp(targets[i].amplitude, destination[i].amplitude, m_progress);
        }
    }

    for (auto & target : targets) {
        target.frequency *= m_formantShift;
        // Bandwidth moves with the frequency, or shifting the formants up would narrow them: a
        // shorter tract has higher formants, not sharper ones, and holding the bandwidth fixed
        // raises their Q instead.
        target.bandwidth *= m_formantShift;
    }

    return targets;
}

void FormantVoice::applyTargets(const FormantTargets & targets, bool immediate)
{
    for (size_t i = 0; i < targets.size(); i++) {
        if (immediate || m_currentTargets[i].frequency <= 0.0) {
            m_currentTargets[i] = targets[i];
            continue;
        }
        // Frequencies glide in log space: that is how a listener hears a formant move, and an
        // octave near F1 must not take longer to cross than an octave near F3.
        const double logNow = std::log2(m_currentTargets[i].frequency);
        const double logTarget = std::log2(targets[i].frequency);
        m_currentTargets[i].frequency = std::exp2(lerp(logNow, logTarget, m_glideCoefficient));
        m_currentTargets[i].bandwidth = lerp(m_currentTargets[i].bandwidth, targets[i].bandwidth, m_glideCoefficient);
        m_currentTargets[i].amplitude = lerp(m_currentTargets[i].amplitude, targets[i].amplitude, m_glideCoefficient);
    }
}

void FormantVoice::updateControl()
{
    if (!m_phoneme) {
        m_levelTarget = 0.0;
        return;
    }

    applyTargets(effectiveTargets(), false);

    const double maxFrequency = std::min(20000.0, m_sampleRate * 0.49);
    const double range = std::log2(maxFrequency / 20.0);
    for (size_t i = 0; i < m_formants.size(); i++) {
        const auto & target = m_currentTargets[i];
        m_formants[i].setCutoff(std::log2(std::max(20.0, target.frequency) / 20.0) / range);
        const double q = target.frequency / std::max(1.0, target.bandwidth);
        m_formants[i].setResonance(std::clamp(1.0 - 1.0 / (2.0 * q), 0.0, 0.99));
    }

    // Aspiration swells towards the level of the phoneme it is leading into rather than holding the
    // stop's own, so the breath grows into the voice instead of stopping and being replaced by it.
    m_levelTarget = m_aspirating && m_nextPhoneme ? levelOf(*m_nextPhoneme) : levelOf(*m_phoneme);

    if (m_phoneme->type == PhonemeType::Plosive) {
        if (!m_plosiveReleased && isReleased() && m_progress >= closureFraction()) {
            m_plosiveReleased = true;
            m_burstRise = 0.0;
            m_burstFall = 1.0;
            m_framesSinceRelease = 0;
        }
        if (m_plosiveReleased && !m_aspirating && shouldAspirate()
            && static_cast<double>(m_framesSinceRelease) > PlosiveBurstSeconds * m_sampleRate) {
            m_aspirating = true;
            m_articulation = AspirationLevel;
        }
    }
}

double FormantVoice::upperFormantOutput(double input)
{
    if (m_tractAmount <= 0.0001) {
        // Still has to run, or it re-enters with stale state when the voice comes back.
        for (auto & filter : m_upperFormants) {
            filter.process(input);
        }
        return 0.0;
    }

    const double maxFrequency = std::min(20000.0, m_sampleRate * 0.49);
    const double range = std::log2(maxFrequency / 20.0);

    double sum = 0.0;
    for (size_t i = 0; i < m_upperFormants.size(); i++) {
        const auto & target = UpperFormants[i];
        const double frequency = std::min(target.frequency * m_formantShift, maxFrequency);
        m_upperFormants[i].setCutoff(std::log2(std::max(20.0, frequency) / 20.0) / range);
        const double q = frequency / target.bandwidth;
        m_upperFormants[i].setResonance(std::clamp(1.0 - 1.0 / (2.0 * q), 0.0, 0.99));
        // Continues the alternating polarity of the three below: F4 is the fourth, so it is negative.
        const double polarity = (i % 2 == 0) ? -1.0 : 1.0;
        sum += polarity * target.amplitude * (target.bandwidth / frequency) * m_upperFormants[i].process(input);
    }
    return sum * m_tractAmount;
}

double FormantVoice::formantBankOutput(std::array<CascadedSvf, 3> & filters, const FormantTargets & targets, double input)
{
    double sum = 0.0;
    for (size_t i = 0; i < filters.size(); i++) {
        const auto & target = targets[i];
        // Adjacent resonances are summed with alternating polarity. Summed in phase they cancel
        // between the peaks, which measured on FormantFilterBank as 15 - 28 dB notches where a real
        // vowel has valleys of 5 - 10 dB.
        const double polarity = (i % 2 == 0) ? 1.0 : -1.0;
        // Each band peaks at its own Q, so dividing by that leaves the table's amplitudes in charge.
        const double normalization = target.bandwidth / std::max(1.0, target.frequency);
        sum += polarity * target.amplitude * normalization * filters[i].process(input);
    }
    return sum;
}

double FormantVoice::nextSample()
{
    if (m_sampleRate <= 0.0 || !m_phoneme) {
        return 0.0;
    }

    if (m_controlCounter == 0) {
        updateControl();
    }
    if (++m_controlCounter >= ControlBlockFrames) {
        m_controlCounter = 0;
    }

    // Glottal pulse and lip radiation in one filter -- see TiltFrequency. Leaving the radiation out
    // is not a subtlety: it costs 6 dB per octave across the whole vowel space, which measured as F2
    // sitting some 33 dB under F1 on /i/ where it belongs nearer 19, and left the close vowels tens
    // of dB louder than the open ones. F2 is where most of a vowel's identity is, so burying it is
    // not a tone preference -- it is the difference between speech and a hum.
    m_tilt.process(m_glottis.nextSample());
    double voiced = m_tilt.highPass();
    if (m_sourceRolloff > 0.0) {
        m_sourceRolloffFilter.process(voiced);
        voiced = m_sourceRolloffFilter.lowPass();
    }

    double noise = m_noise(m_rng) * NoiseSourceLevel;
    for (auto & filter : m_noiseRolloff) {
        filter.process(noise);
        noise = filter.lowPass();
    }

    const double voicing = m_phoneme->voicing;
    const double source = voicing * voiced + (1.0 - voicing) * noise + voicing * m_breathiness * noise;

    m_level += (m_levelTarget - m_level) * m_levelCoefficient;
    m_seam += (1.0 - m_seam) * m_seamCoefficient;

    const double tractTarget = (m_phoneme->type == PhonemeType::Vowel
                                || m_phoneme->type == PhonemeType::Nasal
                                || m_phoneme->type == PhonemeType::Liquid)
      ? 1.0
      : 0.0;
    m_tractAmount += (tractTarget - m_tractAmount) * m_levelCoefficient;

    if (m_phoneme->type == PhonemeType::Plosive) {
        if (m_plosiveReleased) {
            m_framesSinceRelease++;
        }
        if (m_aspirating) {
            // The burst is spent; what is left is breath, dying away as the voice takes over.
            m_articulation *= m_aspirationDecay;
        } else if (m_plosiveReleased) {
            m_burstRise += (1.0 - m_burstRise) * m_burstAttack;
            m_burstFall *= m_burstDecay;
            m_articulation = m_burstRise * m_burstFall;
        } else {
            // The closure. Silent for a voiceless stop; for a voiced one the folds keep going
            // behind it, and that buzz is the only thing that tells B from P.
            m_articulation += (VoiceBarLevel * voicing - m_articulation) * m_levelCoefficient;
        }
    } else {
        m_articulation += (1.0 - m_articulation) * m_levelCoefficient;
    }

    double shaped = formantBankOutput(m_formants, m_currentTargets, source) * m_level;
    if (m_seam < 1.0) {
        // The outgoing shape is fed silence, not the new phoneme's source. It is a tract position
        // being left behind, so what it should do is ring down -- and driving it with the source
        // that replaced it is not merely wrong but loud: noise excites a narrow resonator far
        // harder than a harmonic source does, so a vowel's bank hit with frication blared for the
        // four milliseconds of the crossfade. Every fricative began with a spike nearly three times
        // its own steady level, and a stop's closure with one three hundred times its own, which is
        // what made an /s/ sound like it was being clipped.
        const double outgoing = formantBankOutput(m_previousFormants, m_previousTargets, 0.0) * m_previousLevel;
        shaped = outgoing + (shaped - outgoing) * m_seam;
    }
    shaped += upperFormantOutput(source) * m_level;

    return shaped * SourceMakeupGain * m_articulation;
}

void FormantVoice::reset()
{
    m_phoneme = nullptr;
    m_progress = 0.0;
    m_currentTargets = {};
    m_level = 0.0;
    m_levelTarget = 0.0;
    m_previousLevel = 0.0;
    m_plosiveReleased = false;
    m_aspirating = false;
    m_afterUnvoicedFricative = false;
    m_framesSinceRelease = 0;
    m_phonemeSeconds = 0.0;
    m_nextPhoneme = nullptr;
    m_articulation = 1.0;
    m_burstRise = 0.0;
    m_burstFall = 0.0;
    m_seam = 1.0;
    m_controlCounter = 0;

    m_glottis.reset();
    m_tilt.reset();
    m_sourceRolloffFilter.reset();
    for (auto & filter : m_noiseRolloff) {
        filter.reset();
    }

    for (auto & filter : m_formants) {
        filter.reset();
    }
    for (auto & filter : m_previousFormants) {
        filter.reset();
    }
    for (auto & filter : m_upperFormants) {
        filter.reset();
    }
    m_previousTargets = {};
    m_tractAmount = 0.0;

    m_rng.seed(RngSeed);
}

} // namespace noteahead
