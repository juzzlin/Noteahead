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

#include "adsr_envelope.hpp"

#include <algorithm>
#include <cmath>

namespace noteahead {

namespace {

constexpr double MinimumSegmentTime { 0.000001 };

} // namespace

void AdsrEnvelope::setAttackTime(double seconds)
{
    m_attackTime = std::max(MinimumSegmentTime, seconds);
    updatePhaseStep();
}

void AdsrEnvelope::setDecayTime(double seconds)
{
    m_decayTime = std::max(MinimumSegmentTime, seconds);
    updatePhaseStep();
}

void AdsrEnvelope::setSustainLevel(double level)
{
    m_sustainLevel = std::clamp(level, 0.0, 1.0);
    // A decay is on its way to the sustain level, so moving it moves the target out from under the
    // running segment.
    if (m_state == State::Decay) {
        m_segmentTarget = m_sustainLevel;
    }
    updatePhaseStep();
}

void AdsrEnvelope::setReleaseTime(double seconds)
{
    m_releaseTime = std::max(MinimumSegmentTime, seconds);
    updatePhaseStep();
}

void AdsrEnvelope::setCurve(double curve)
{
    m_curve = std::clamp(curve, 0.0, 1.0);
}

void AdsrEnvelope::setSampleRate(double sampleRate)
{
    if (std::abs(m_sampleRate - sampleRate) < 0.1) {
        return;
    }
    DspComponent::setSampleRate(sampleRate);
    updatePhaseStep();
}

void AdsrEnvelope::trigger()
{
    beginSegment(State::Attack);
}

void AdsrEnvelope::release()
{
    if (m_state != State::Idle) {
        beginSegment(State::Release);
    }
}

void AdsrEnvelope::reset()
{
    m_state = State::Idle;
    m_currentLevel = 0.0;
    m_segmentStart = 0.0;
    m_segmentTarget = 0.0;
    m_phase = 0.0;
    m_phaseStep = 0.0;
}

double AdsrEnvelope::nextSample()
{
    switch (m_state) {
    case State::Idle:
        m_currentLevel = 0.0;
        break;
    case State::Attack:
    case State::Decay:
    case State::Release:
        m_phase += m_phaseStep;
        if (m_phase >= 1.0) {
            m_phase = 1.0;
            m_currentLevel = m_segmentTarget;
            if (m_state == State::Attack) {
                beginSegment(State::Decay);
            } else if (m_state == State::Decay) {
                m_state = State::Sustain;
            } else {
                m_state = State::Idle;
                m_currentLevel = 0.0;
            }
        } else {
            const double shaped = shape(m_phase);
            // The attack rises into its target and the other two fall away from their start, so the
            // same shaping function bends one concave and the other convex.
            m_currentLevel = m_state == State::Attack
              ? m_segmentStart + (m_segmentTarget - m_segmentStart) * shaped
              : m_segmentTarget + (m_segmentStart - m_segmentTarget) * (1.0 - shaped);
        }
        break;
    case State::Sustain:
        m_currentLevel = m_sustainLevel;
        break;
    }
    return m_currentLevel;
}

double AdsrEnvelope::value() const
{
    return m_currentLevel;
}

AdsrEnvelope::State AdsrEnvelope::state() const
{
    return m_state;
}

bool AdsrEnvelope::isActive() const
{
    return m_state != State::Idle;
}

bool AdsrEnvelope::isSilent() const
{
    if (m_state == State::Idle) {
        return true;
    }
    // Attack starts from zero and is on its way up, so a low level there means nothing yet.
    return m_state != State::Attack && m_currentLevel <= SilenceThreshold;
}

void AdsrEnvelope::beginSegment(State state)
{
    m_state = state;
    m_segmentStart = m_currentLevel;
    m_phase = 0.0;

    switch (state) {
    case State::Attack:
        m_segmentTarget = 1.0;
        break;
    case State::Decay:
        m_segmentTarget = m_sustainLevel;
        break;
    case State::Release:
        m_segmentTarget = 0.0;
        break;
    default:
        m_segmentTarget = m_currentLevel;
        break;
    }

    updatePhaseStep();
}

double AdsrEnvelope::segmentDuration(State state) const
{
    switch (state) {
    case State::Attack:
        return m_attackTime * std::max(0.0, 1.0 - m_segmentStart);
    case State::Decay:
        return m_decayTime;
    case State::Release:
        return m_releaseTime * std::clamp(m_segmentStart, 0.0, 1.0);
    default:
        return 0.0;
    }
}

double AdsrEnvelope::shape(double phase) const
{
    const double curvature = m_curve * MaxCurvature;
    if (curvature < 1.0e-6) {
        return phase;
    }
    return std::expm1(-curvature * phase) / std::expm1(-curvature);
}

void AdsrEnvelope::updatePhaseStep()
{
    // A segment with nowhere to travel is over on the next sample rather than never: that is what
    // the fixed-step envelope did when its step came out zero.
    const double samples = segmentDuration(m_state) * m_sampleRate;
    m_phaseStep = samples > 1.0 ? 1.0 / samples : 1.0;
}

} // namespace noteahead
