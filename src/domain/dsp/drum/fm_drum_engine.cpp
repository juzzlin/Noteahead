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

#include "fm_drum_engine.hpp"

#include <algorithm>
#include <cmath>

namespace noteahead {

namespace {

//! Never released, so this only ever matters to reset(); it is set short so that nothing lingers.
constexpr double EnvelopeReleaseSeconds { 0.005 };

void makeStruck(AdsrEnvelope & envelope)
{
    // Attack and decay only: no hold, nothing to sustain, and nothing to release from
    envelope.setHoldTime(0.0);
    envelope.setSustainLevel(0.0);
    envelope.setReleaseTime(EnvelopeReleaseSeconds);
}

} // namespace

FmDrumEngine::FmDrumEngine()
{
    m_rng.seed(0);
    m_ratios.fill(1.0);

    for (size_t op = 0; op < OperatorCount; op++) {
        auto & envelope = m_operators.at(op).envelope();
        makeStruck(envelope);
        envelope.setAttackTime(0.0);
        envelope.setDecayTime(0.3);
        envelope.setCurve(0.5);
        // A single sine until told otherwise: operator 1 heard, the rest silent
        m_operators.at(op).setLevel(op == 0 ? 1.0 : 0.0);
    }

    makeStruck(m_noiseEnvelope);
    m_noiseEnvelope.setAttackTime(0.0);
    m_noiseEnvelope.setDecayTime(0.1);
    m_noiseEnvelope.setCurve(0.5);

    m_noiseFilter.setOrder(2);
    m_noiseFilter.setMode(CascadedSvf::Mode::LowPass);
    m_noiseFilter.setCutoff(1.0);

    updatePitchEnvelopeCoefficient();
}

void FmDrumEngine::setSampleRate(double sampleRate)
{
    DspComponent::setSampleRate(sampleRate);
    for (auto && op : m_operators) {
        op.setSampleRate(sampleRate);
    }
    m_noiseEnvelope.setSampleRate(sampleRate);
    m_noiseFilter.setSampleRate(sampleRate);
    updatePitchEnvelopeCoefficient();
}

void FmDrumEngine::updatePitchEnvelopeCoefficient()
{
    const double rate = sampleRate();
    m_pitchEnvelopeCoefficient = rate > 0.0 ? std::exp(-1.0 / (std::max(0.0001, m_pitchEnvelopeDecay) * rate)) : 0.0;
}

void FmDrumEngine::trigger(float velocity)
{
    // Whatever the last hit was still putting out, blended away below rather than cut
    m_retriggerOffset = m_active ? m_lastOut : 0.0;

    const double clamped = std::clamp(static_cast<double>(velocity), 0.0, 1.0);
    m_levelScale = 1.0 - m_velocityToLevel + m_velocityToLevel * clamped;
    m_modulationScale = 1.0 - m_velocityToModulation + m_velocityToModulation * clamped;

    // Every hit starts from phase zero and the same noise, so a drum sounds the same each time
    for (auto && op : m_operators) {
        op.sync(0.0);
        op.trigger();
    }
    m_noiseEnvelope.trigger();
    m_noiseFilter.reset();
    m_noiseBank.reset();
    m_rng.seed(0);

    m_pitchEnvelope = 1.0;
    m_chokeGain = 1.0;
    m_stopping = false;
    m_active = true;
}

float FmDrumEngine::nextSample()
{
    if (!m_active) {
        m_lastOut = 0.0;
        return 0.0f;
    }

    const double rate = sampleRate();

    const double frequency = m_baseFrequency * std::exp2(m_pitchEnvelopeAmount * m_pitchEnvelope / 12.0);
    m_pitchEnvelope *= m_pitchEnvelopeCoefficient;

    // Generated at the base rate and interpolated up, so that its level in the audible band does not
    // depend on the oversampling factor. See BaseRateSource.
    m_noiseBank.setOversampleFactor(oversampleFactor());
    if (m_noiseBank.needsBaseSample()) {
        m_noiseBank.setBaseSample(m_dist(m_rng));
    }
    const double noise = m_noiseFilter.process(m_noiseBank.nextSample()) * m_noiseEnvelope.nextSample();

    // Operator 4 down to operator 1: the algorithm only ever routes an operator into a lower one, so
    // every modulator has produced this sample before the operator it modulates is asked for its own.
    const double modulationDepth = FmOperator::MaxIndexCycles * m_modulationScale;
    std::array<double, OperatorCount> outputs {};
    for (size_t reverse = 0; reverse < OperatorCount; reverse++) {
        const size_t op = OperatorCount - 1 - reverse;
        m_operators.at(op).setFrequency(frequency * m_ratios.at(op));

        double phaseMod = 0.0;
        if (const uint8_t sources = m_modulators.at(op); sources) {
            for (size_t source = op + 1; source < OperatorCount; source++) {
                if (sources & (1u << source)) {
                    phaseMod += outputs.at(source);
                }
            }
        }
        if (op == m_noiseTarget) {
            phaseMod += noise * m_noiseModulation;
        }

        outputs.at(op) = m_operators.at(op).nextSample(phaseMod * modulationDepth);
    }

    double mix = 0.0;
    int carrierCount = 0;
    for (size_t op = 0; op < OperatorCount; op++) {
        if (m_carriers & (1u << op)) {
            mix += outputs.at(op);
            carrierCount++;
        }
    }
    // Divided by the count rather than its root, as the FM Synth does: carriers struck together
    // from phase zero are correlated, and their amplitudes add.
    if (carrierCount > 1) {
        mix /= static_cast<double>(carrierCount);
    }

    double out = (mix + noise * m_noiseLevel) * m_levelScale * m_outputLevel * OutputGain;

    if (m_stopping) {
        m_chokeGain = std::max(0.0, m_chokeGain - 1.0 / (ChokeFadeSeconds * rate));
    }
    out *= m_chokeGain;

    out += m_retriggerOffset;
    const double blendStep = 1.0 / (RetriggerBlendSeconds * rate);
    m_retriggerOffset = m_retriggerOffset > 0.0 ? std::max(0.0, m_retriggerOffset - blendStep) : std::min(0.0, m_retriggerOffset + blendStep);

    if (!isAudible() && std::abs(m_retriggerOffset) < AmplitudeThreshold) {
        m_active = false;
        m_retriggerOffset = 0.0;
    }

    m_lastOut = out;
    return static_cast<float>(out);
}

bool FmDrumEngine::isAudible() const
{
    if (m_chokeGain <= 0.0) {
        return false;
    }
    if (m_noiseLevel > 0.0 && !m_noiseEnvelope.isSilent()) {
        return true;
    }
    for (size_t op = 0; op < OperatorCount; op++) {
        if ((m_carriers & (1u << op)) && m_operators.at(op).level() > 0.0 && !m_operators.at(op).isSilent()) {
            return true;
        }
    }
    return false;
}

bool FmDrumEngine::isActive() const
{
    return m_active;
}

void FmDrumEngine::reset()
{
    for (auto && op : m_operators) {
        op.reset();
    }
    m_noiseEnvelope.reset();
    m_noiseFilter.reset();
    m_noiseBank.reset();
    m_rng.seed(0);
    m_pitchEnvelope = 0.0;
    m_chokeGain = 1.0;
    m_stopping = false;
    m_active = false;
    m_lastOut = 0.0;
    m_retriggerOffset = 0.0;
}

void FmDrumEngine::stop()
{
    if (m_active) {
        m_stopping = true;
    }
}

void FmDrumEngine::setAlgorithm(const std::array<uint8_t, OperatorCount> & modulators, uint8_t carriers)
{
    m_modulators = modulators;
    m_carriers = carriers;
}

void FmDrumEngine::setBaseFrequency(double hz)
{
    m_baseFrequency = std::max(0.0, hz);
}

void FmDrumEngine::setOperatorRatio(size_t op, double ratio)
{
    m_ratios.at(op) = std::max(0.0, ratio);
}

void FmDrumEngine::setOperatorLevel(size_t op, double level)
{
    m_operators.at(op).setLevel(level);
}

void FmDrumEngine::setOperatorWaveform(size_t op, FmOperator::Waveform waveform)
{
    m_operators.at(op).setWaveform(waveform);
}

void FmDrumEngine::setOperatorAttack(size_t op, double seconds)
{
    m_operators.at(op).envelope().setAttackTime(std::max(0.0, seconds));
}

void FmDrumEngine::setOperatorDecay(size_t op, double seconds)
{
    m_operators.at(op).envelope().setDecayTime(std::max(0.0, seconds));
}

void FmDrumEngine::setOperatorCurve(size_t op, double curve)
{
    m_operators.at(op).envelope().setCurve(curve);
}

void FmDrumEngine::setFeedback(double feedback)
{
    m_feedback = std::clamp(feedback, 0.0, 1.0);
    m_operators.at(FeedbackOperator).setFeedback(m_feedback * FmOperator::MaxIndexCycles);
}

void FmDrumEngine::setPitchEnvelopeAmount(double semitones)
{
    m_pitchEnvelopeAmount = semitones;
}

void FmDrumEngine::setPitchEnvelopeDecay(double seconds)
{
    m_pitchEnvelopeDecay = std::max(0.0, seconds);
    updatePitchEnvelopeCoefficient();
}

void FmDrumEngine::setNoiseLevel(double level)
{
    m_noiseLevel = std::clamp(level, 0.0, 1.0);
}

void FmDrumEngine::setNoiseModulation(double amount)
{
    m_noiseModulation = std::clamp(amount, 0.0, 1.0);
}

void FmDrumEngine::setNoiseTarget(size_t op)
{
    m_noiseTarget = std::min(op, OperatorCount - 1);
}

void FmDrumEngine::setNoiseAttack(double seconds)
{
    m_noiseEnvelope.setAttackTime(std::max(0.0, seconds));
}

void FmDrumEngine::setNoiseDecay(double seconds)
{
    m_noiseEnvelope.setDecayTime(std::max(0.0, seconds));
}

void FmDrumEngine::setNoiseCurve(double curve)
{
    m_noiseEnvelope.setCurve(curve);
}

void FmDrumEngine::setNoiseFilterMode(NoiseFilterMode mode)
{
    switch (mode) {
    case NoiseFilterMode::LowPass:
        m_noiseFilter.setMode(CascadedSvf::Mode::LowPass);
        break;
    case NoiseFilterMode::BandPass:
        m_noiseFilter.setMode(CascadedSvf::Mode::BandPass);
        break;
    case NoiseFilterMode::HighPass:
        m_noiseFilter.setMode(CascadedSvf::Mode::HighPass);
        break;
    }
}

void FmDrumEngine::setNoiseCutoff(double cutoff)
{
    m_noiseFilter.setCutoff(cutoff);
}

void FmDrumEngine::setNoiseResonance(double resonance)
{
    m_noiseFilter.setResonance(resonance);
}

void FmDrumEngine::setVelocityToLevel(double amount)
{
    m_velocityToLevel = std::clamp(amount, 0.0, 1.0);
}

void FmDrumEngine::setVelocityToModulation(double amount)
{
    m_velocityToModulation = std::clamp(amount, 0.0, 1.0);
}

void FmDrumEngine::setOutputLevel(double level)
{
    m_outputLevel = std::clamp(level, 0.0, 1.0);
}

} // namespace noteahead
