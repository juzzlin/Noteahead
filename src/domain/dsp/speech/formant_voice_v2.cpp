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

#include "formant_voice_v2.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace noteahead {

namespace {

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

double logLerp(double from, double to, double t)
{
    return std::exp2(lerp(std::log2(std::max(1.0, from)), std::log2(std::max(1.0, to)), t));
}

bool isTractShape(PhonemeType type)
{
    return type == PhonemeType::Vowel || type == PhonemeType::Nasal || type == PhonemeType::Liquid;
}

bool isNoiseConsonant(const PhonemeSpec & spec)
{
    return spec.type == PhonemeType::Plosive || (spec.type == PhonemeType::Fricative && spec.name != "HH");
}

double diphthongGlide(double progress)
{
    if (progress <= 0.25) {
        return 0.0;
    }
    if (progress >= 0.80) {
        return 1.0;
    }
    const double u = (progress - 0.25) / 0.55;
    return u * u * (3.0 - 2.0 * u);
}

} // namespace

void FormantVoiceV2::Resonator::configure(double frequency, double bandwidth, double sampleRate)
{
    if (sampleRate <= 0.0) {
        return;
    }
    const double f = std::clamp(frequency, 20.0, sampleRate * 0.42);
    const double bw = std::clamp(bandwidth, 20.0, sampleRate * 0.42);
    const double r = std::exp(-std::numbers::pi * bw / sampleRate);
    c = -(r * r);
    b = 2.0 * r * std::cos(2.0 * std::numbers::pi * f / sampleRate);
    a = 1.0 - b - c;
}

double FormantVoiceV2::Resonator::process(double input)
{
    const double y = a * input + b * y1 + c * y2;
    y2 = y1;
    y1 = std::abs(y) < 1.0e-15 ? 0.0 : y;
    return y;
}

void FormantVoiceV2::Resonator::reset()
{
    y1 = 0.0;
    y2 = 0.0;
}

void FormantVoiceV2::NasalPoleZero::configure(double fnp, double bnp, double fnz, double bnz, double sampleRate)
{
    if (sampleRate <= 0.0) {
        return;
    }
    const double fp = std::clamp(fnp, 20.0, sampleRate * 0.42);
    const double bpBw = std::clamp(bnp, 20.0, sampleRate * 0.42);
    const double fz = std::clamp(std::max(fnz, fp), 20.0, sampleRate * 0.42);
    const double bzBw = std::clamp(bnz, 20.0, sampleRate * 0.42);

    const double rp = std::exp(-std::numbers::pi * bpBw / sampleRate);
    const double cp = -(rp * rp);
    const double bp = 2.0 * rp * std::cos(2.0 * std::numbers::pi * fp / sampleRate);
    const double ap = 1.0 - bp - cp;

    const double rz = std::exp(-std::numbers::pi * bzBw / sampleRate);
    const double cz = -(rz * rz);
    const double bz = 2.0 * rz * std::cos(2.0 * std::numbers::pi * fz / sampleRate);
    const double az = 1.0 - bz - cz;

    const double gain = ap / std::max(1.0e-9, az);
    b0 = gain;
    b1 = -gain * bz;
    b2 = -gain * cz;
    a1 = bp;
    a2 = cp;
}

double FormantVoiceV2::NasalPoleZero::process(double input)
{
    const double y = b0 * input + s1;
    s1 = b1 * input + a1 * y + s2;
    s2 = b2 * input + a2 * y;
    if (std::abs(s1) < 1.0e-15) {
        s1 = 0.0;
    }
    if (std::abs(s2) < 1.0e-15) {
        s2 = 0.0;
    }
    return y;
}

void FormantVoiceV2::NasalPoleZero::reset()
{
    s1 = 0.0;
    s2 = 0.0;
}

void FormantVoiceV2::Biquad::configurePeaking(double fc, double gainDb, double q, double sampleRate)
{
    if (sampleRate <= 0.0) {
        return;
    }
    const double clampedFc = std::clamp(fc, 20.0, sampleRate * 0.44);
    const double clampedQ = std::max(0.2, q);
    const double aGain = std::pow(10.0, gainDb / 40.0);
    const double w0 = 2.0 * std::numbers::pi * clampedFc / sampleRate;
    const double cw = std::cos(w0);
    const double alpha = std::sin(w0) / (2.0 * clampedQ);
    const double invA0 = 1.0 / (1.0 + alpha / aGain);
    b0 = (1.0 + alpha * aGain) * invA0;
    b1 = (-2.0 * cw) * invA0;
    b2 = (1.0 - alpha * aGain) * invA0;
    a1 = (-2.0 * cw) * invA0;
    a2 = (1.0 - alpha / aGain) * invA0;
}

void FormantVoiceV2::Biquad::configureHighPass(double fc, double q, double sampleRate)
{
    if (sampleRate <= 0.0) {
        return;
    }
    const double clampedFc = std::clamp(fc, 20.0, sampleRate * 0.44);
    const double w0 = 2.0 * std::numbers::pi * clampedFc / sampleRate;
    const double cw = std::cos(w0);
    const double alpha = std::sin(w0) / (2.0 * std::max(0.2, q));
    const double invA0 = 1.0 / (1.0 + alpha);
    b0 = 0.5 * (1.0 + cw) * invA0;
    b1 = -(1.0 + cw) * invA0;
    b2 = b0;
    a1 = (-2.0 * cw) * invA0;
    a2 = (1.0 - alpha) * invA0;
}

void FormantVoiceV2::Biquad::configureLowPass(double fc, double q, double sampleRate)
{
    if (sampleRate <= 0.0) {
        return;
    }
    const double clampedFc = std::clamp(fc, 20.0, sampleRate * 0.44);
    const double w0 = 2.0 * std::numbers::pi * clampedFc / sampleRate;
    const double cw = std::cos(w0);
    const double alpha = std::sin(w0) / (2.0 * std::max(0.2, q));
    const double invA0 = 1.0 / (1.0 + alpha);
    b0 = 0.5 * (1.0 - cw) * invA0;
    b1 = (1.0 - cw) * invA0;
    b2 = b0;
    a1 = (-2.0 * cw) * invA0;
    a2 = (1.0 - alpha) * invA0;
}

void FormantVoiceV2::Biquad::configureAllPass(double fc, double radius, double sampleRate)
{
    if (sampleRate <= 0.0) {
        return;
    }
    const double clampedFc = std::clamp(fc, 20.0, sampleRate * 0.44);
    const double r = std::clamp(radius, 0.0, 0.90);
    const double theta = 2.0 * std::numbers::pi * clampedFc / sampleRate;
    const double c1 = -2.0 * r * std::cos(theta);
    const double c2 = r * r;
    b0 = c2;
    b1 = c1;
    b2 = 1.0;
    a1 = c1;
    a2 = c2;
}

double FormantVoiceV2::Biquad::process(double input)
{
    const double y = b0 * input + s1;
    s1 = b1 * input - a1 * y + s2;
    s2 = b2 * input - a2 * y;
    if (std::abs(s1) < 1.0e-15) {
        s1 = 0.0;
    }
    if (std::abs(s2) < 1.0e-15) {
        s2 = 0.0;
    }
    return y;
}

void FormantVoiceV2::Biquad::reset()
{
    s1 = 0.0;
    s2 = 0.0;
}

FormantVoiceV2::FormantVoiceV2()
{
    for (auto & filter : m_fricationFormants) {
        filter.setMode(CascadedSvf::Mode::BandPass);
        filter.setOrder(2);
    }
    for (auto & filter : m_previousFricationFormants) {
        filter.setMode(CascadedSvf::Mode::BandPass);
        filter.setOrder(2);
    }
}

void FormantVoiceV2::configureTractPostFilters()
{
    if (m_sampleRate <= 0.0 || std::abs(m_formantShift - m_configuredFilterShift) <= 1.0e-5) {
        return;
    }
    m_configuredFilterShift = m_formantShift;
    m_hpc[0].configurePeaking(5200.0 * m_formantShift, 14.5, 0.95, m_sampleRate);
    m_hpc[1].configurePeaking(6000.0 * m_formantShift, 13.5, 1.05, m_sampleRate);
    m_cascadeCeiling.configureLowPass(std::min(7500.0 * m_formantShift, m_sampleRate * 0.42), 0.7071, m_sampleRate);

    // Diffuse closure-phase harmonic alignment without changing magnitude response
    m_allPass[0].configureAllPass(380.0 * m_formantShift, 0.72, m_sampleRate);
    m_allPass[1].configureAllPass(920.0 * m_formantShift, 0.72, m_sampleRate);
    m_allPass[2].configureAllPass(2100.0 * m_formantShift, 0.70, m_sampleRate);
}

void FormantVoiceV2::setSampleRate(double sampleRate)
{
    DspComponent::setSampleRate(sampleRate);

    m_glottis.setSampleRate(sampleRate);
    m_radDecay = sampleRate > 0.0 ? std::exp(-2.0 * std::numbers::pi * 80.0 / sampleRate) : 0.99;
    m_radScale = sampleRate > 0.0 ? sampleRate / 48000.0 : 1.0;

    m_glottalNoiseHp.configureHighPass(1100.0, 0.7071, sampleRate);
    m_glottalNoiseLp.configureLowPass(4800.0, 0.7071, sampleRate);
    m_aspirationHp.configureHighPass(450.0, 0.7071, sampleRate);
    m_aspirationLp.configureLowPass(4600.0, 0.7071, sampleRate);

    m_configuredFilterShift = 0.0;
    configureTractPostFilters();

    setSourceRolloff(m_sourceRolloff);
    setSibilance(m_sibilance);

    for (auto & filter : m_fricationFormants) {
        filter.setSampleRate(sampleRate);
    }
    for (auto & filter : m_previousFricationFormants) {
        filter.setSampleRate(sampleRate);
    }

    m_levelCoefficient = smoothingCoefficient(LevelGlideTime, sampleRate);
    m_voicingAttackCoeff = smoothingCoefficient(VoicingAttackTime, sampleRate);
    m_voicingReleaseCoeff = smoothingCoefficient(VoicingReleaseTime, sampleRate);
    m_glideCoefficient = smoothingCoefficient(m_glideTime, sampleRate / ControlBlockFrames);
    m_occlusionGlideCoeff = smoothingCoefficient(OcclusionGlideTime, sampleRate / ControlBlockFrames);
    m_seamCoefficient = smoothingCoefficient(SeamFadeTime, sampleRate);
    m_burstAttack = smoothingCoefficient(PlosiveBurstAttackTime, sampleRate);
    m_burstDecay = sampleRate > 0.0 ? std::exp(-1.0 / (PlosiveBurstDecayTime * sampleRate)) : 0.0;
    m_aspirationDecay = sampleRate > 0.0 ? std::exp(-1.0 / (AspirationDecayTime * sampleRate)) : 0.0;
}

void FormantVoiceV2::setFrequency(double frequency)
{
    m_frequency = std::max(1.0, frequency);
    m_glottis.setFrequency(m_frequency);
}

void FormantVoiceV2::setGlideTime(double seconds)
{
    m_glideTime = std::max(0.0, seconds);
    m_glideCoefficient = smoothingCoefficient(m_glideTime, m_sampleRate / ControlBlockFrames);
}

void FormantVoiceV2::setFormantShift(double shift)
{
    m_formantShift = std::max(0.1, shift);
    configureTractPostFilters();
}

void FormantVoiceV2::setSourceRolloff(double frequency)
{
    m_sourceRolloff = std::max(0.0, frequency);
    if (m_sampleRate > 0.0 && m_sourceRolloff > 0.0) {
        m_sourceRolloffFilter.calculate(std::min(m_sourceRolloff, m_sampleRate * 0.45), m_sampleRate);
    }
}

void FormantVoiceV2::setBreathiness(double breathiness)
{
    m_breathiness = std::clamp(breathiness, 0.0, 1.0);
    m_effectiveBreathiness = m_breathiness;
}

void FormantVoiceV2::setOpenQuotient(double openQuotient)
{
    m_openQuotient = std::clamp(openQuotient, 0.0, 1.0);
    m_effectiveOpenQuotient = m_openQuotient;
    m_glottis.setOpenQuotient(m_openQuotient);
}

void FormantVoiceV2::setVoicePerturbation(double perturbation)
{
    m_basePerturbation = std::max(0.0, perturbation);
    m_glottis.setJitter(m_basePerturbation * JitterRange);
    m_glottis.setShimmer(m_basePerturbation * ShimmerRange);
}

void FormantVoiceV2::setSpeedQuotient(double speedQuotient)
{
    m_glottis.setSpeedQuotient(speedQuotient);
}

void FormantVoiceV2::setReturnPhase(double returnPhase)
{
    m_returnPhase = std::clamp(returnPhase, 0.002, 0.12);
    m_glottis.setReturnPhase(m_returnPhase);
}

void FormantVoiceV2::setSubharmonic(double subharmonic)
{
    m_baseSubharmonic = std::clamp(subharmonic, 0.0, 1.0);
    m_glottis.setSubharmonic(m_baseSubharmonic);
}

void FormantVoiceV2::setPhraseDynamics(double amount)
{
    m_phraseDynamics = std::clamp(amount, 0.0, 1.0);
}

void FormantVoiceV2::setUtteranceProgress(double progress)
{
    m_utteranceProgress = std::clamp(progress, 0.0, 1.0);
}

void FormantVoiceV2::setConsonantLevel(double level)
{
    m_consonantLevel = std::max(0.0, level);
}

void FormantVoiceV2::setSibilance(double sibilance)
{
    m_sibilance = std::clamp(sibilance, 0.0, 1.0);
    if (m_sampleRate > 0.0) {
        const double corner = MinNoiseRolloffFrequency + (MaxNoiseRolloffFrequency - MinNoiseRolloffFrequency) * m_sibilance;
        for (auto & filter : m_noiseRolloff) {
            filter.calculate(std::min(corner, m_sampleRate * 0.45), m_sampleRate);
        }
    }
}

void FormantVoiceV2::setVoiceCharacter(const VoiceCharacter & character)
{
    m_character = character;
}

std::array<FormantVoiceV2::FormantPair, 3> FormantVoiceV2::consonantLocus(std::string_view name, const PhonemeSpec * adjacentVowel)
{
    const bool isNasal = name == "M" || name == "N" || name == "NG";
    const double nasalF1 = isNasal ? 400.0 : 220.0;
    const double nasalBw = isNasal ? 160.0 : 120.0;

    if (name == "P" || name == "B" || name == "M") {
        return { { { nasalF1, nasalBw }, { 750.0, 130.0 }, { 2100.0, 180.0 } } };
    }
    if (name == "F" || name == "V") {
        return { { { 280.0, 130.0 }, { 950.0, 140.0 }, { 2200.0, 180.0 } } };
    }
    if (name == "TH" || name == "DH") {
        return { { { 280.0, 130.0 }, { 1500.0, 140.0 }, { 2500.0, 180.0 } } };
    }
    if (name == "T" || name == "D" || name == "N" || name == "S" || name == "Z") {
        return { { { nasalF1, nasalBw }, { 1750.0, 130.0 }, { 2650.0, 180.0 } } };
    }
    if (name == "SH" || name == "ZH") {
        return { { { 260.0, 130.0 }, { 1950.0, 140.0 }, { 2350.0, 180.0 } } };
    }
    if (name == "K" || name == "G" || name == "NG") {
        const double adjacentF2 = (adjacentVowel && isTractShape(adjacentVowel->type)) ? adjacentVowel->formants[1].frequency : 1200.0;
        const bool front = adjacentF2 >= 1500.0;
        return { {
          { nasalF1, nasalBw },
          { front ? 2300.0 : 1250.0, 140.0 },
          { front ? 2650.0 : 2250.0, 180.0 },
        } };
    }
    if (name == "HH") {
        if (adjacentVowel && isTractShape(adjacentVowel->type)) {
            return { {
              { adjacentVowel->formants[0].frequency, adjacentVowel->formants[0].bandwidth + 120.0 },
              { adjacentVowel->formants[1].frequency, adjacentVowel->formants[1].bandwidth },
              { adjacentVowel->formants[2].frequency, adjacentVowel->formants[2].bandwidth },
            } };
        }
        return { { { 500.0, 190.0 }, { 1500.0, 120.0 }, { 2500.0, 180.0 } } };
    }

    return { { { 500.0, 80.0 }, { 1500.0, 110.0 }, { 2500.0, 160.0 } } };
}

FormantVoiceV2::NasalSpec FormantVoiceV2::nasalTargetsFor(const PhonemeSpec & spec, const PhonemeSpec * next, double progress)
{
    if (spec.name == "M") {
        return { 260.0, 90.0, 1000.0, 220.0 };
    }
    if (spec.name == "N") {
        return { 260.0, 90.0, 1700.0, 280.0 };
    }
    if (spec.name == "NG") {
        return { 260.0, 90.0, 2600.0, 340.0 };
    }
    // Anticipatory nasalization late in a vowel before a nasal consonant
    if (spec.type == PhonemeType::Vowel && next && next->type == PhonemeType::Nasal && progress > 0.65) {
        const double blend = std::clamp((progress - 0.65) / 0.35, 0.0, 1.0);
        return { 260.0, 90.0, logLerp(260.0, 420.0, blend), lerp(90.0, 140.0, blend) };
    }
    return { 260.0, 90.0, 260.0, 90.0 };
}

bool FormantVoiceV2::isPhraseFinalStop() const
{
    return m_phoneme && m_phoneme->type == PhonemeType::Plosive && (!m_nextPhoneme || m_nextPhoneme->type == PhonemeType::Silence);
}

bool FormantVoiceV2::isReleased() const
{
    return m_phoneme != nullptr;
}

double FormantVoiceV2::closureFraction() const
{
    if (!m_phoneme) {
        return UnvoicedClosureFraction;
    }
    if (m_phonemeSeconds <= 0.0) {
        return m_phoneme->voicing > 0.0 ? VoicedClosureFraction : UnvoicedClosureFraction;
    }
    double release = shouldAspirate() ? AspiratedReleaseSeconds : UnaspiratedReleaseSeconds;
    if (isPhraseFinalStop()) {
        release = FinalReleaseSeconds;
    }
    return std::clamp(1.0 - release / m_phonemeSeconds, MinimumClosureFraction, MaximumClosureFraction);
}

bool FormantVoiceV2::shouldAspirate() const
{
    if (!m_phoneme || m_phoneme->type != PhonemeType::Plosive || m_phoneme->voicing > 0.0 || m_afterUnvoicedFricative) {
        return false;
    }
    if (isPhraseFinalStop()) {
        return true;
    }
    return m_nextPhoneme
      && (m_nextPhoneme->type == PhonemeType::Vowel
          || m_nextPhoneme->type == PhonemeType::Nasal
          || m_nextPhoneme->type == PhonemeType::Liquid);
}

double FormantVoiceV2::voicingTargetFor(const PhonemeSpec & spec) const
{
    switch (spec.type) {
    case PhonemeType::Silence:
        return 0.0;
    case PhonemeType::Vowel: {
        if (spec.name == "AX") {
            return 0.68;
        }
        const double openEff = 0.88 + 0.14 * std::clamp((spec.formants[0].frequency - 270.0) / 460.0, 0.0, 1.0);
        return (m_stressed ? 1.0 : 0.86) * openEff;
    }
    case PhonemeType::Liquid:
        return (spec.name == "DX" ? 0.38 : 0.62) * m_consonantLevel;
    case PhonemeType::Nasal:
        return 0.66 * m_consonantLevel;
    case PhonemeType::Fricative:
        return spec.voicing > 0.0 ? 0.35 * m_consonantLevel : 0.0;
    case PhonemeType::Plosive:
        return spec.voicing > 0.0 ? 0.22 * m_consonantLevel : 0.0;
    }
    return 0.0;
}

double FormantVoiceV2::fricationTargetFor(const PhonemeSpec & spec) const
{
    if (spec.type == PhonemeType::Fricative && spec.name != "HH") {
        return spec.amplitude * m_consonantLevel * (m_sibilance * SibilanceLevelRange) * FricationMakeupGain;
    }
    return 0.0;
}

double FormantVoiceV2::aspirationTargetFor(const PhonemeSpec & spec) const
{
    if (spec.name == "HH") {
        return m_consonantLevel * (m_sibilance * SibilanceLevelRange) * HhAspirationLevel;
    }
    return 0.0;
}

std::array<FormantVoiceV2::FormantPair, 3> FormantVoiceV2::effectiveCascadeTargets() const
{
    if (!m_phoneme || m_phoneme->type == PhonemeType::Silence) {
        return m_currentCascade;
    }

    const double glottalBwLoss = std::max(0.0, m_effectiveOpenQuotient - 0.5) * 110.0;
    const double pitchBw = std::max(0.0, m_frequency - 120.0) * 0.05;

    auto shapeFromSpec = [&](const PhonemeSpec & spec, double progress) {
        std::array<FormantPair, 3> out {};
        for (size_t i = 0; i < out.size(); i++) {
            out[i] = { spec.formants[i].frequency, spec.formants[i].bandwidth };
        }
        if (spec.glideTo.has_value()) {
            const auto & dest = *spec.glideTo;
            const double glide = diphthongGlide(progress);
            for (size_t i = 0; i < out.size(); i++) {
                out[i].frequency = logLerp(out[i].frequency, dest[i].frequency, glide);
                out[i].bandwidth = lerp(out[i].bandwidth, dest[i].bandwidth, glide);
            }
        }
        if (spec.type == PhonemeType::Vowel) {
            out[0].bandwidth = 54.0 + 0.038 * out[0].frequency + glottalBwLoss + pitchBw;
            out[1].bandwidth = 56.0 + 0.022 * out[1].frequency + 0.5 * glottalBwLoss;
            out[2].bandwidth = 120.0 + 0.045 * out[2].frequency;
        }
        return out;
    };

    std::array<FormantPair, 3> targets {};
    if (m_phoneme->type == PhonemeType::Vowel || m_phoneme->type == PhonemeType::Liquid) {
        if (m_phoneme->name == "L") {
            const bool codaDarkL = !m_nextPhoneme || m_nextPhoneme->type != PhonemeType::Vowel;
            targets = codaDarkL
              ? std::array<FormantPair, 3> { { { 450.0, 90.0 }, { 850.0, 120.0 }, { 2650.0, 180.0 } } }
              : std::array<FormantPair, 3> { { { 400.0, 70.0 }, { 1250.0, 100.0 }, { 2600.0, 160.0 } } };
        } else {
            targets = shapeFromSpec(*m_phoneme, m_progress);
        }

        // Unstressed vowel centralization (Lindblom)
        if (m_phoneme->type == PhonemeType::Vowel && !m_stressed && m_phoneme->name != "AX") {
            targets[0].frequency = logLerp(targets[0].frequency, 500.0, 0.24);
            targets[1].frequency = logLerp(targets[1].frequency, 1500.0, 0.24);
            targets[2].frequency = logLerp(targets[2].frequency, 2500.0, 0.24);
            for (auto & pair : targets) {
                pair.bandwidth *= 1.15;
            }
        }

        // VC closing transition into a following consonant
        if (m_nextPhoneme && (isNoiseConsonant(*m_nextPhoneme) || m_nextPhoneme->type == PhonemeType::Nasal) && m_progress > 0.55) {
            const double blend = std::clamp((m_progress - 0.55) / 0.45, 0.0, 1.0) * 0.75;
            const auto locus = consonantLocus(m_nextPhoneme->name, m_phoneme);
            for (size_t i = 0; i < targets.size(); i++) {
                targets[i].frequency = logLerp(targets[i].frequency, locus[i].frequency, blend);
                targets[i].bandwidth = lerp(targets[i].bandwidth, locus[i].bandwidth, blend);
            }
        } else if (m_phoneme->type == PhonemeType::Vowel && !m_phoneme->glideTo.has_value()
                   && m_nextPhoneme && m_nextPhoneme->type == PhonemeType::Liquid && m_progress > 0.60) {
            const double blend = std::clamp((m_progress - 0.60) / 0.40, 0.0, 1.0) * 0.35;
            const auto liquid = (m_nextPhoneme->name == "L")
              ? std::array<FormantPair, 3> { { { 450.0, 90.0 }, { 850.0, 120.0 }, { 2650.0, 180.0 } } }
              : shapeFromSpec(*m_nextPhoneme, 0.0);
            for (size_t i = 0; i < targets.size(); i++) {
                targets[i].frequency = logLerp(targets[i].frequency, liquid[i].frequency, blend);
                targets[i].bandwidth = lerp(targets[i].bandwidth, liquid[i].bandwidth, blend);
            }
        }
    } else if (m_phoneme->type == PhonemeType::Plosive) {
        if (m_aspirating && m_nextPhoneme && isTractShape(m_nextPhoneme->type)) {
            targets = shapeFromSpec(*m_nextPhoneme, 0.0);
        } else {
            const auto * context = (m_nextPhoneme && isTractShape(m_nextPhoneme->type)) ? m_nextPhoneme : m_lastVowel;
            targets = consonantLocus(m_phoneme->name, context);
        }
    } else if (m_phoneme->name == "HH") {
        targets = consonantLocus(m_phoneme->name, m_nextPhoneme);
    } else {
        const auto * context = (m_nextPhoneme && isTractShape(m_nextPhoneme->type)) ? m_nextPhoneme : m_lastVowel;
        targets = consonantLocus(m_phoneme->name, context);
    }

    if (m_phoneme->type != PhonemeType::Vowel) {
        targets[0].bandwidth += glottalBwLoss;
    }

    targets[1].frequency *= m_character.f2Stretch;
    targets[2].frequency *= m_character.f3Stretch;

    if (m_character.formantWobbleDepth > 0.0) {
        const double phase = 2.0 * std::numbers::pi * m_wobblePhase;
        targets[0].frequency = std::max(80.0, targets[0].frequency * (1.0 + m_character.formantWobbleDepth * std::sin(phase)));
        targets[1].frequency = std::max(250.0, targets[1].frequency * (1.0 + m_character.formantWobbleDepth * std::sin(phase + 2.1)));
        targets[2].frequency = std::max(600.0, targets[2].frequency * (1.0 + m_character.formantWobbleDepth * std::sin(phase + 4.2)));
    }

    for (auto & pair : targets) {
        pair.frequency *= m_formantShift;
        pair.bandwidth *= m_formantShift * m_character.bandwidthScale;
    }

    if (m_phoneme->type == PhonemeType::Vowel && m_frequency > 0.85 * targets[0].frequency) {
        const double limit = targets[1].frequency - 120.0 * m_formantShift;
        targets[0].frequency = std::min(std::max(targets[0].frequency, m_frequency * 1.05), std::max(targets[0].frequency, limit));
    }

    return targets;
}

FormantVoiceV2::NasalSpec FormantVoiceV2::effectiveNasalTargets() const
{
    if (!m_phoneme || m_phoneme->type == PhonemeType::Silence) {
        return m_currentNasal;
    }
    NasalSpec nasal = nasalTargetsFor(*m_phoneme, m_nextPhoneme, m_progress);
    nasal.fnp *= m_formantShift;
    nasal.bnp *= m_formantShift;
    nasal.fnz *= m_formantShift;
    nasal.bnz *= m_formantShift;
    return nasal;
}

void FormantVoiceV2::applyCascadeTargets(const std::array<FormantPair, 3> & targets, const NasalSpec & nasal, bool immediate, bool fastClosure)
{
    const double coeff = fastClosure ? m_occlusionGlideCoeff : m_glideCoefficient;
    for (size_t i = 0; i < targets.size(); i++) {
        if (immediate || m_currentCascade[i].frequency <= 0.0) {
            m_currentCascade[i] = targets[i];
        } else {
            m_currentCascade[i].frequency = logLerp(m_currentCascade[i].frequency, targets[i].frequency, coeff);
            m_currentCascade[i].bandwidth = lerp(m_currentCascade[i].bandwidth, targets[i].bandwidth, coeff);
        }
    }

    if (immediate || m_currentNasal.fnp <= 0.0) {
        m_currentNasal = nasal;
    } else {
        m_currentNasal.fnp = logLerp(m_currentNasal.fnp, nasal.fnp, coeff);
        m_currentNasal.bnp = lerp(m_currentNasal.bnp, nasal.bnp, coeff);
        m_currentNasal.fnz = logLerp(m_currentNasal.fnz, nasal.fnz, coeff);
        m_currentNasal.bnz = lerp(m_currentNasal.bnz, nasal.bnz, coeff);
    }
}

void FormantVoiceV2::tuneFricationFilters(const FormantTargets & targets)
{
    if (m_sampleRate <= 0.0) {
        return;
    }
    const double maxFrequency = std::min(20000.0, m_sampleRate * 0.49);
    const double range = std::log2(maxFrequency / 20.0);
    for (size_t i = 0; i < m_fricationFormants.size(); i++) {
        const double frequency = targets[i].frequency * m_formantShift;
        const double bandwidth = targets[i].bandwidth * m_formantShift;
        m_fricationFormants[i].setCutoff(std::log2(std::max(20.0, frequency) / 20.0) / range);
        const double q = frequency / std::max(1.0, bandwidth);
        m_fricationFormants[i].setResonance(std::clamp(1.0 - 1.0 / (2.0 * q), 0.0, 0.99));
    }
}

void FormantVoiceV2::setPhoneme(const PhonemeSpec & spec, const PhonemeSpec * next, double seconds)
{
    if (m_phoneme == &spec) {
        m_nextPhoneme = next;
        m_phonemeSeconds = seconds;
        return;
    }

    const bool firstPhoneme = m_phoneme == nullptr;
    const PhonemeSpec * prev = m_phoneme;
    const bool wasNoise = prev && isNoiseConsonant(*prev);
    const bool isNoise = isNoiseConsonant(spec);

    m_afterUnvoicedFricative = prev && prev->type == PhonemeType::Fricative && prev->voicing <= 0.0;
    if (prev && (prev->type == PhonemeType::Vowel || prev->type == PhonemeType::Liquid)) {
        m_lastVowel = prev;
    }

    m_phoneme = &spec;
    m_nextPhoneme = next;
    m_phonemeSeconds = seconds;
    m_progress = 0.0;
    m_plosiveReleased = false;
    m_phraseFinalRelease = false;
    m_aspirating = false;
    m_framesSinceRelease = 0;
    m_aspirationEnv = 0.0;

    if (spec.type != PhonemeType::Silence) {
        applyCascadeTargets(effectiveCascadeTargets(), effectiveNasalTargets(), firstPhoneme, false);
    }

    // Only noise consonants drive the parallel bank; crossfade only when jumping from one active
    // noise spectrum straight into a different one.
    if (isNoise) {
        if (wasNoise && m_fricationLevel > 0.001 && m_fricationSpec) {
            m_previousFricationFormants = m_fricationFormants;
            m_previousNoiseSpec = m_fricationSpec;
            m_previousFricationLevel = m_fricationLevel;
            m_seam = 0.0;
        }
        for (auto & filter : m_fricationFormants) {
            filter.reset();
        }
        m_fricationSpec = &spec;
        if (spec.name == "K" || spec.name == "G") {
            const auto * context = (next && isTractShape(next->type)) ? next : m_lastVowel;
            const double adjacentF2 = (context && isTractShape(context->type)) ? context->formants[1].frequency : 1200.0;
            const double scale = std::clamp(adjacentF2 / 1550.0, 0.72, 1.38);
            FormantTargets burstTargets { spec.formants };
            for (auto & target : burstTargets) {
                target.frequency *= scale;
                target.bandwidth *= scale;
            }
            tuneFricationFilters(burstTargets);
        } else {
            tuneFricationFilters(spec.formants);
        }
        m_fricationLevel = fricationTargetFor(spec);
    }

    if (firstPhoneme) {
        m_voicingTarget = voicingTargetFor(spec);
        m_fricationTarget = fricationTargetFor(spec);
        m_aspirationTarget = aspirationTargetFor(spec);
        updateControl();
    }
}

void FormantVoiceV2::setPhonemeStressed(bool stressed)
{
    m_stressed = stressed;
}

void FormantVoiceV2::setPhonemeProgress(double progress)
{
    m_progress = std::clamp(progress, 0.0, 1.0);
}

void FormantVoiceV2::updateControl()
{
    if (!m_phoneme) {
        m_voicingTarget = 0.0;
        m_fricationTarget = 0.0;
        m_aspirationTarget = 0.0;
        m_voicedAge = 0.0;
        return;
    }

    configureTractPostFilters();

    const double baseVoicingTarget = voicingTargetFor(*m_phoneme);
    if (baseVoicingTarget > 0.01) {
        m_voicedAge = std::min(1.0, m_voicedAge + static_cast<double>(ControlBlockFrames) / std::max(1.0, m_sampleRate));
    } else if (m_voicingLevel < 0.05) {
        m_voicedAge = 0.0;
    }

    // Soft breathy-to-modal onset and gentle phrase-final glottal relaxation
    const double onsetFade = (baseVoicingTarget > 0.01)
      ? (1.0 - std::clamp(m_voicedAge / VoicingOnsetSeconds, 0.0, 1.0))
      : 0.0;
    const double finalFactor = (m_phraseDynamics > 0.0 && m_utteranceProgress > PhraseFinalStart)
      ? std::clamp((m_utteranceProgress - PhraseFinalStart) / (1.0 - PhraseFinalStart), 0.0, 1.0) * m_phraseDynamics
      : 0.0;

    m_effectiveOpenQuotient = std::clamp(m_openQuotient + 0.14 * onsetFade + 0.14 * finalFactor, 0.25, 0.85);
    const double effRa = std::clamp(m_returnPhase + 0.022 * onsetFade + 0.028 * finalFactor, 0.002, 0.12);
    const double lowPitchWeight = std::clamp((180.0 - m_frequency) / 80.0, 0.0, 1.0);
    const double effSub = std::clamp(m_baseSubharmonic + 0.14 * finalFactor * lowPitchWeight, 0.0, 1.0);
    const double effPerturb = m_basePerturbation + 0.25 * finalFactor;

    m_glottis.setOpenQuotient(m_effectiveOpenQuotient);
    m_glottis.setReturnPhase(effRa);
    m_glottis.setSubharmonic(effSub);
    m_glottis.setJitter(effPerturb * JitterRange);
    m_glottis.setShimmer(effPerturb * ShimmerRange);
    m_effectiveBreathiness = std::clamp(m_breathiness + 0.08 * onsetFade + 0.10 * finalFactor, 0.0, 1.0);

    if (m_character.formantWobbleHz > 0.0 && m_sampleRate > 0.0) {
        m_wobblePhase += m_character.formantWobbleHz * (static_cast<double>(ControlBlockFrames) / m_sampleRate);
        m_wobblePhase -= std::floor(m_wobblePhase);
    }

    const int stepDiv = std::max(1, m_character.steppedControlDiv);
    const bool updateTract = (m_controlStepSkip == 0);
    m_controlStepSkip = (m_controlStepSkip + 1) % stepDiv;

    if (updateTract) {
        const bool fastClosure = m_phoneme->type == PhonemeType::Plosive && !m_plosiveReleased;
        applyCascadeTargets(effectiveCascadeTargets(), effectiveNasalTargets(), false, fastClosure);

        m_nasalPoleZero.configure(m_currentNasal.fnp, m_currentNasal.bnp, m_currentNasal.fnz, m_currentNasal.bnz, m_sampleRate);

        const double f1 = m_currentCascade[0].frequency;
        const double f2 = std::max(m_currentCascade[1].frequency, f1 + 120.0 * m_formantShift);
        const double f3 = std::max(m_currentCascade[2].frequency, f2 + 150.0 * m_formantShift);
        m_cascadeFormants[0].configure(f1, m_currentCascade[0].bandwidth, m_sampleRate);
        m_cascadeFormants[1].configure(f2, m_currentCascade[1].bandwidth, m_sampleRate);
        m_cascadeFormants[2].configure(f3, m_currentCascade[2].bandwidth, m_sampleRate);
        for (size_t i = 0; i < UpperCascadeFormants.size(); i++) {
            m_cascadeFormants[3 + i].configure(
              UpperCascadeFormants[i].frequency * m_formantShift,
              UpperCascadeFormants[i].bandwidth * m_formantShift * m_character.bandwidthScale,
              m_sampleRate);
        }
    }

    double syllableArc = 1.0;
    if (m_phraseDynamics > 0.0 && m_phoneme->type == PhonemeType::Vowel) {
        const double arch = std::sin(std::numbers::pi * m_progress);
        syllableArc = 1.0 + m_phraseDynamics * (0.06 * arch - 0.05 * (1.0 - arch));
    }

    m_voicingTarget = baseVoicingTarget * syllableArc * (1.0 - 0.42 * finalFactor);
    m_fricationTarget = fricationTargetFor(*m_phoneme);
    m_aspirationTarget = aspirationTargetFor(*m_phoneme);

    if (m_phoneme->type == PhonemeType::Plosive) {
        if (!m_plosiveReleased && isReleased() && m_progress >= closureFraction()) {
            m_plosiveReleased = true;
            m_phraseFinalRelease = isPhraseFinalStop();
            m_burstRise = 0.0;
            m_burstFall = 1.0;
            m_framesSinceRelease = 0;
        }
        if (m_plosiveReleased && !m_aspirating && shouldAspirate()
            && static_cast<double>(m_framesSinceRelease) > PlosiveBurstSeconds * m_sampleRate) {
            m_aspirating = true;
            const double aspScale = m_phraseFinalRelease ? FinalAspirationScale : 1.0;
            m_aspirationEnv = PlosiveAspirationLevel * aspScale * m_consonantLevel;
        }
    }
}

double FormantVoiceV2::parallelBankOutput(std::array<CascadedSvf, 3> & filters, const PhonemeSpec & spec, double input)
{
    const bool skipVoiceBarBand = spec.type == PhonemeType::Fricative && spec.voicing > 0.0;
    double sum = 0.0;
    for (size_t i = 0; i < filters.size(); i++) {
        const double output = filters[i].process(input);
        if (skipVoiceBarBand && i == 0) {
            continue;
        }
        const auto & target = spec.formants[i];
        const double polarity = (i % 2 == 0) ? 1.0 : -1.0;
        const double normalization = target.bandwidth / std::max(1.0, target.frequency);
        sum += polarity * target.amplitude * normalization * output;
    }
    return sum;
}

double FormantVoiceV2::nextSample()
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

    if (m_character.fmWarbleHz > 0.0 && m_character.fmWarbleSemitones > 0.0) {
        m_fmPhase += m_character.fmWarbleHz / m_sampleRate;
        m_fmPhase -= std::floor(m_fmPhase);
        const double warbleSemitones = std::sin(2.0 * std::numbers::pi * m_fmPhase) * m_character.fmWarbleSemitones;
        m_glottis.setFrequency(m_frequency * std::pow(2.0, warbleSemitones / 12.0));
    }

    // Evaluate glottal flow first so aspiration tracks instantaneous glottal openness
    const double glottalFlow = m_glottis.nextSample();

    const double rawNoise = m_noise(m_rng) * NoiseSourceLevel;
    double fricationNoise = rawNoise;
    for (auto & filter : m_noiseRolloff) {
        filter.process(fricationNoise);
        fricationNoise = filter.lowPass();
    }

    const double voicingCoeff = (m_voicingTarget > m_voicingLevel) ? m_voicingAttackCoeff : m_voicingReleaseCoeff;
    m_voicingLevel += (m_voicingTarget - m_voicingLevel) * voicingCoeff;
    m_fricationLevel += (m_fricationTarget - m_fricationLevel) * m_levelCoefficient;
    m_aspirationLevel += (m_aspirationTarget - m_aspirationLevel) * m_levelCoefficient;
    m_seam += (1.0 - m_seam) * m_seamCoefficient;

    const double glottalAir = m_glottalNoiseLp.process(m_glottalNoiseHp.process(rawNoise));
    const double breathAir = m_aspirationLp.process(m_aspirationHp.process(rawNoise));

    const double glottalChink = m_voicingLevel * m_effectiveBreathiness * glottalAir * m_glottis.openness() * GlottalChinkLevel;
    double breathAspiration = m_aspirationLevel * breathAir;
    if (m_character.whisperMix > 0.0) {
        breathAspiration += m_voicingLevel * m_character.whisperMix * breathAir * WhisperNoiseLevel;
    }

    if (m_phoneme->type == PhonemeType::Plosive) {
        if (m_plosiveReleased) {
            m_framesSinceRelease++;
            m_burstRise += (1.0 - m_burstRise) * m_burstAttack;
            m_burstFall *= m_burstDecay;
            if (m_burstFall < 1.0e-6) {
                m_burstFall = 0.0;
            }
            if (m_aspirating) {
                breathAspiration += m_aspirationEnv * breathAir;
                m_aspirationEnv *= m_aspirationDecay;
            }
        }
    }

    // First-difference lip radiation (+6 dB/oct across speech bands) on voiced glottal flow;
    // turbulent glottal chink and unvoiced aspiration already enter as radiated noise.
    const double whisperAtten = 1.0 - std::clamp(m_character.whisperMix, 0.0, 1.0);
    const double voicedFlow = glottalFlow * m_voicingLevel * whisperAtten;
    double glottalExcitation = (voicedFlow - m_radDecay * m_radPrev) * m_radScale;
    m_radPrev = voicedFlow;

    if (m_character.subOctaveMix > 0.0) {
        m_subPhase += (0.5 * m_frequency) / m_sampleRate;
        m_subPhase -= std::floor(m_subPhase);
        double subRaw = 0.0;
        if (m_subPhase < 0.48) {
            subRaw = 0.5 * (1.0 - std::cos(std::numbers::pi * m_subPhase / 0.48));
        } else if (m_subPhase < 0.60) {
            subRaw = std::cos(0.5 * std::numbers::pi * (m_subPhase - 0.48) / 0.12);
        }
        const double subScaled = subRaw * 0.55 * std::clamp(220.0 / std::max(30.0, m_frequency), 0.5, 4.0);
        const double subDiff = (subScaled - m_radDecay * m_subPrev) * m_radScale;
        m_subPrev = subScaled;
        glottalExcitation += subDiff * (m_voicingLevel * whisperAtten * m_character.subOctaveMix * 1.4);
    }

    if (m_sourceRolloff > 0.0) {
        m_sourceRolloffFilter.process(glottalExcitation);
        glottalExcitation = m_sourceRolloffFilter.lowPass();
    }

    // Voiced excitation and shaped turbulence through cascade tract + 48 kHz higher-pole correction + phase dispersion
    double tractSignal = m_nasalPoleZero.process(glottalExcitation + glottalChink + breathAspiration);
    for (auto & resonator : m_cascadeFormants) {
        tractSignal = resonator.process(tractSignal);
    }
    for (auto & stage : m_hpc) {
        tractSignal = stage.process(tractSignal);
    }
    tractSignal = m_cascadeCeiling.process(tractSignal);
    if (!m_character.bypassPhaseDispersion) {
        for (auto & stage : m_allPass) {
            tractSignal = stage.process(tractSignal);
        }
    }
    double cascadeOut = tractSignal * CascadeMakeupGain;

    if (m_character.combDelayMs > 0.0 && m_character.combMix > 0.0) {
        const size_t delaySamples = std::clamp(
          static_cast<size_t>(std::lround(m_character.combDelayMs * 0.001 * m_sampleRate)),
          size_t { 1 },
          m_combBuffer.size() - 1);
        const size_t readIndex = (m_combWriteIndex + m_combBuffer.size() - delaySamples) % m_combBuffer.size();
        const double delayed = m_combBuffer[readIndex];
        m_combBuffer[m_combWriteIndex] = cascadeOut + delayed * std::clamp(m_character.combFeedback, 0.0, 0.75);
        m_combWriteIndex = (m_combWriteIndex + 1) % m_combBuffer.size();
        cascadeOut = cascadeOut * (1.0 - m_character.combMix) + (cascadeOut + delayed) * 0.5 * m_character.combMix;
    }

    // Parallel broadband turbulence / stop burst
    if (m_phoneme->type != PhonemeType::Plosive && m_burstFall > 0.0) {
        m_burstRise += (1.0 - m_burstRise) * m_burstAttack;
        m_burstFall *= m_burstDecay;
        if (m_burstFall < 1.0e-6) {
            m_burstFall = 0.0;
        }
    }
    const double finalScale = m_phraseFinalRelease ? FinalStopBurstScale : 1.0;
    const double burstOut = (m_fricationSpec && m_fricationSpec->type == PhonemeType::Plosive)
      ? m_fricationSpec->amplitude * m_consonantLevel * m_burstRise * m_burstFall * BurstMakeupGain * finalScale
      : 0.0;
    const double activeGain = isNoiseConsonant(*m_phoneme)
      ? (m_phoneme->type == PhonemeType::Plosive ? burstOut : m_fricationLevel)
      : std::max(burstOut, m_fricationLevel);
    double parallelOut = (m_fricationSpec && activeGain > 1.0e-6)
      ? parallelBankOutput(m_fricationFormants, *m_fricationSpec, isNoiseConsonant(*m_phoneme) ? fricationNoise : 0.0) * activeGain
      : 0.0;
    if (m_seam < 1.0 && m_previousNoiseSpec) {
        const double outgoing = parallelBankOutput(m_previousFricationFormants, *m_previousNoiseSpec, 0.0) * m_previousFricationLevel;
        parallelOut = outgoing + (parallelOut - outgoing) * m_seam;
    }

    double out = (cascadeOut + parallelOut) * SourceMakeupGain;

    if (m_character.ringModHz > 0.0 && m_character.ringModDepth > 0.0) {
        m_ringPhase += m_character.ringModHz / m_sampleRate;
        m_ringPhase -= std::floor(m_ringPhase);
        const double mod = std::sin(2.0 * std::numbers::pi * m_ringPhase);
        out *= (1.0 - m_character.ringModDepth) + m_character.ringModDepth * mod;
    }

    if (m_character.drive > 0.0) {
        const double g = 1.0 + m_character.drive;
        out = std::tanh(out * g) / std::tanh(g);
    }

    if (m_character.sampleHoldHz > 0.0) {
        m_shCounter += m_character.sampleHoldHz / m_sampleRate;
        if (m_shCounter >= 1.0) {
            m_shCounter -= std::floor(m_shCounter);
            m_shValue = out;
            if (m_character.bitCrushSteps > 0) {
                const double steps = static_cast<double>(m_character.bitCrushSteps);
                m_shValue = std::round(m_shValue * steps) / steps;
            }
        }
        out = m_shValue;
    }

    return out;
}

void FormantVoiceV2::reset()
{
    m_phoneme = nullptr;
    m_nextPhoneme = nullptr;
    m_lastVowel = nullptr;
    m_progress = 0.0;
    m_phonemeSeconds = 0.0;
    m_stressed = true;

    m_currentCascade = {};
    m_currentNasal = {};
    for (auto & resonator : m_cascadeFormants) {
        resonator.reset();
    }
    m_nasalPoleZero.reset();
    for (auto & stage : m_hpc) {
        stage.reset();
    }
    m_cascadeCeiling.reset();
    for (auto & stage : m_allPass) {
        stage.reset();
    }

    m_voicingLevel = 0.0;
    m_voicingTarget = 0.0;
    m_voicedAge = 0.0;
    m_utteranceProgress = 0.0;
    m_effectiveBreathiness = m_breathiness;
    m_effectiveOpenQuotient = m_openQuotient;
    m_fricationLevel = 0.0;
    m_fricationTarget = 0.0;
    m_previousFricationLevel = 0.0;
    m_aspirationLevel = 0.0;
    m_aspirationTarget = 0.0;

    m_afterUnvoicedFricative = false;
    m_plosiveReleased = false;
    m_phraseFinalRelease = false;
    m_aspirating = false;
    m_framesSinceRelease = 0;
    m_burstRise = 0.0;
    m_burstFall = 0.0;
    m_aspirationEnv = 0.0;

    m_seam = 1.0;
    m_fricationSpec = nullptr;
    m_previousNoiseSpec = nullptr;
    for (auto & filter : m_fricationFormants) {
        filter.reset();
    }
    for (auto & filter : m_previousFricationFormants) {
        filter.reset();
    }

    m_glottis.reset();
    m_glottis.setOpenQuotient(m_openQuotient);
    m_glottis.setReturnPhase(m_returnPhase);
    m_glottis.setSubharmonic(m_baseSubharmonic);
    m_glottis.setJitter(m_basePerturbation * JitterRange);
    m_glottis.setShimmer(m_basePerturbation * ShimmerRange);
    m_radPrev = 0.0;
    m_glottalNoiseHp.reset();
    m_glottalNoiseLp.reset();
    m_aspirationHp.reset();
    m_aspirationLp.reset();
    m_sourceRolloffFilter.reset();
    for (auto & filter : m_noiseRolloff) {
        filter.reset();
    }
    m_combBuffer.fill(0.0);
    m_combWriteIndex = 0;
    m_ringPhase = 0.0;
    m_fmPhase = 0.0;
    m_wobblePhase = 0.0;
    m_subPhase = 0.0;
    m_subPrev = 0.0;
    m_shCounter = 1.0;
    m_shValue = 0.0;
    m_controlStepSkip = 0;
    m_controlCounter = 0;
    m_rng.seed(RngSeed);
}

} // namespace noteahead
