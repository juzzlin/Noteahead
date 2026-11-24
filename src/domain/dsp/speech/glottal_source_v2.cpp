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

#include "glottal_source_v2.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace noteahead {

namespace {

constexpr double ReferenceFrequency = 220.0;
constexpr double MinFrequencyScale = 0.25;
constexpr double MaxFrequencyScale = 4.0;
constexpr double FrequencyScaleGlideTime = 0.010;

//! Matches Rosenberg's pi / 2 peak closing slope factor for compatible cascade drive levels.
constexpr double LfGainScale = std::numbers::pi * 0.5;

} // namespace

GlottalSourceV2::GlottalSourceV2()
{
    updateLfParameters();
    updateFrequencyScale();
}

void GlottalSourceV2::setSampleRate(double sampleRate)
{
    DspComponent::setSampleRate(sampleRate);
    m_frequencyScaleCoefficient = sampleRate > 0.0 ? 1.0 - std::exp(-1.0 / (FrequencyScaleGlideTime * sampleRate)) : 1.0;
    updatePhaseStep();
    updateLfParametersIfNeeded();
}

void GlottalSourceV2::setFrequency(double frequency)
{
    m_frequency = std::max(1.0, frequency);
    updatePhaseStep();
    updateLfParametersIfNeeded();
    updateFrequencyScale();
}

void GlottalSourceV2::setOpenQuotient(double openQuotient)
{
    const double clamped = std::clamp(openQuotient, MinOpenQuotient, MaxOpenQuotient);
    if (clamped != m_openQuotient) {
        m_openQuotient = clamped;
        m_lfDirty = true;
        updateLfParametersIfNeeded();
    }
}

void GlottalSourceV2::setSpeedQuotient(double speedQuotient)
{
    const double clamped = std::clamp(speedQuotient, MinSpeedQuotient, MaxSpeedQuotient);
    if (clamped != m_speedQuotient) {
        m_speedQuotient = clamped;
        m_lfDirty = true;
        updateLfParametersIfNeeded();
    }
}

void GlottalSourceV2::setReturnPhase(double returnPhase)
{
    const double clamped = std::clamp(returnPhase, MinReturnPhase, MaxReturnPhase);
    if (clamped != m_returnPhase) {
        m_returnPhase = clamped;
        m_lfDirty = true;
        updateLfParametersIfNeeded();
    }
}

void GlottalSourceV2::setJitter(double jitter)
{
    m_jitter = std::clamp(jitter, 0.0, 0.5);
}

void GlottalSourceV2::setShimmer(double shimmer)
{
    m_shimmer = std::clamp(shimmer, 0.0, 0.9);
}

void GlottalSourceV2::setSubharmonic(double subharmonic)
{
    m_subharmonic = std::clamp(subharmonic, 0.0, 1.0);
}

void GlottalSourceV2::updatePhaseStep()
{
    m_phaseStep = m_sampleRate > 0.0 ? m_frequency / m_sampleRate : 0.0;
}

void GlottalSourceV2::updateFrequencyScale()
{
    m_frequencyScaleTarget = std::clamp(ReferenceFrequency / m_frequency, MinFrequencyScale, MaxFrequencyScale);
    if (!m_started) {
        m_frequencyScale = m_frequencyScaleTarget;
    }
}

void GlottalSourceV2::updateLfParametersIfNeeded()
{
    const double tr = 1.0 - m_openQuotient;
    const double ta = std::clamp(std::max(m_returnPhase, AntiAliasingTaFactor * m_phaseStep), MinReturnPhase, MaxReturnFraction * tr);
    if (m_lfDirty || std::abs(ta - m_effectiveTa) > 1.0e-5) {
        updateLfParameters();
    }
}

void GlottalSourceV2::updateLfParameters()
{
    m_te = m_openQuotient;
    const double tr = 1.0 - m_te;
    m_tp = m_te * m_speedQuotient / (1.0 + m_speedQuotient);
    m_wg = std::numbers::pi / m_tp;
    m_effectiveTa = std::clamp(std::max(m_returnPhase, AntiAliasingTaFactor * m_phaseStep), MinReturnPhase, MaxReturnFraction * tr);
    m_lfDirty = false;

    // Solve epsilon * ta = 1 - exp(-epsilon * tr) via x = epsilon * tr
    const double k = m_effectiveTa / tr;
    double x = 1.0 / k;
    for (int iter = 0; iter < 10; iter++) {
        const double e = x > 50.0 ? 0.0 : std::exp(-x);
        const double f = 1.0 - e - k * x;
        const double df = e - k;
        if (std::abs(df) > 1.0e-12) {
            x = std::max(1.0e-6, x - f / df);
        }
    }

    m_epsilon = x / tr;
    m_expNegEpsTr = x > 50.0 ? 0.0 : std::exp(-x);
    m_returnNorm = 1.0 / std::max(1.0e-12, 1.0 - m_expNegEpsTr);
    const double returnFlowAtTe = m_returnNorm * ((1.0 - m_expNegEpsTr) / m_epsilon - tr * m_expNegEpsTr);

    // Match Ug_open(te; alpha) to returnFlowAtTe so the period has zero net area drift
    const double sinTe = std::sin(m_wg * m_te);
    const double cosTe = std::cos(m_wg * m_te);
    const auto openFlowAtTe = [&](double alpha) {
        const double denom = -sinTe * (alpha * alpha + m_wg * m_wg);
        const double expNeg = std::exp(std::clamp(-alpha * m_te, -80.0, 80.0));
        return (alpha * sinTe - m_wg * cosTe + m_wg * expNeg) / std::max(1.0e-12, denom);
    };

    double lo = -40.0;
    double hi = 60.0;
    while (openFlowAtTe(lo) < returnFlowAtTe && lo > -320.0) {
        lo *= 2.0;
    }
    while (openFlowAtTe(hi) > returnFlowAtTe && hi < 480.0) {
        hi *= 2.0;
    }
    for (int iter = 0; iter < 48; iter++) {
        const double mid = 0.5 * (lo + hi);
        if (openFlowAtTe(mid) > returnFlowAtTe) {
            lo = mid;
        } else {
            hi = mid;
        }
    }

    m_alpha = 0.5 * (lo + hi);
    m_invOpenDenom = 1.0 / std::max(1.0e-12, -sinTe * (m_alpha * m_alpha + m_wg * m_wg));
    m_wgExpNegAlphaTe = m_wg * std::exp(std::clamp(-m_alpha * m_te, -80.0, 80.0));

    constexpr int QuadratureSteps = 64;
    double sum = 0.0;
    for (int i = 0; i < QuadratureSteps; i++) {
        sum += lfFlow((static_cast<double>(i) + 0.5) / static_cast<double>(QuadratureSteps));
    }
    m_meanFlow = std::max(1.0e-6, sum / static_cast<double>(QuadratureSteps));
}

double GlottalSourceV2::lfFlow(double phase) const
{
    if (phase <= 0.0 || phase >= 1.0) {
        return 0.0;
    }
    if (phase <= m_te) {
        const double e = std::exp(m_alpha * (phase - m_te));
        const double w = m_wg * phase;
        const double flow = m_invOpenDenom * (e * (m_alpha * std::sin(w) - m_wg * std::cos(w)) + m_wgExpNegAlphaTe);
        return std::max(0.0, flow);
    }

    const double dt = phase - m_te;
    const double e = (m_epsilon * dt > 50.0) ? 0.0 : std::exp(-m_epsilon * dt);
    const double flow = m_returnNorm * ((e - m_expNegEpsTr) / m_epsilon - m_expNegEpsTr * (1.0 - phase));
    return std::max(0.0, flow);
}

void GlottalSourceV2::beginPeriod()
{
    m_periodScale = 1.0 + m_jitter * m_deviation(m_rng);
    m_amplitude = 1.0 + m_shimmer * m_deviation(m_rng);
    m_weakPeriod = !m_weakPeriod;
    if (m_weakPeriod) {
        m_amplitude *= 1.0 - m_subharmonic;
    }
}

double GlottalSourceV2::nextSample()
{
    m_started = true;
    m_frequencyScale += (m_frequencyScaleTarget - m_frequencyScale) * m_frequencyScaleCoefficient;

    const double opening = lfFlow(m_phase);
    m_openness = opening / m_meanFlow;
    const double flow = opening * m_amplitude * LfGainScale * m_frequencyScale;

    m_phase += m_phaseStep / std::max(0.5, m_periodScale);
    if (m_phase >= 1.0) {
        m_phase -= std::floor(m_phase);
        beginPeriod();
    }

    return flow;
}

double GlottalSourceV2::openness() const
{
    return m_openness;
}

void GlottalSourceV2::reset()
{
    m_phase = 0.0;
    m_periodScale = 1.0;
    m_amplitude = 1.0;
    m_weakPeriod = false;
    m_openness = 1.0;
    m_frequencyScale = m_frequencyScaleTarget;
    m_started = false;
    m_rng.seed(0x51EEC4);
}

} // namespace noteahead
