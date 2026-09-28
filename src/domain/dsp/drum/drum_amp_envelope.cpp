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

#include "drum_amp_envelope.hpp"

#include <algorithm>
#include <cmath>

namespace noteahead {

void DrumAmpEnvelope::setAttackTime(double seconds)
{
    m_attackTime = std::max(MinimumSegmentTime, seconds);
    updatePhaseStep();
}

void DrumAmpEnvelope::setHoldTime(double seconds)
{
    m_holdTime = std::max(0.0, seconds);
    updatePhaseStep();
}

void DrumAmpEnvelope::setDecayTime(double seconds)
{
    m_decayTime = std::max(MinimumSegmentTime, seconds);
    updatePhaseStep();
}

void DrumAmpEnvelope::setCurve(double curve)
{
    m_curve = std::clamp(curve, 0.0, 1.0);
}

void DrumAmpEnvelope::setSampleRate(double sampleRate)
{
    if (std::abs(m_sampleRate - sampleRate) < 0.1) {
        return;
    }
    DspComponent::setSampleRate(sampleRate);
    updatePhaseStep();
}

void DrumAmpEnvelope::trigger()
{
    // Nothing sounding: the attack has the whole range to travel and starts now.
    if (m_currentLevel <= ChokeThreshold) {
        beginSegment(State::Attack);
        return;
    }

    // Otherwise the tail is walked to zero first, and nextSample() begins the attack when it lands.
    beginSegment(State::Choke);
}

void DrumAmpEnvelope::reset()
{
    m_state = State::Idle;
    m_currentLevel = 0.0;
    m_segmentStart = 0.0;
    m_phase = 0.0;
    m_phaseStep = 0.0;
}

double DrumAmpEnvelope::nextSample()
{
    switch (m_state) {
    case State::Idle:
        m_currentLevel = 0.0;
        break;
    case State::Choke:
        m_phase += m_phaseStep;
        if (m_phase >= 1.0) {
            m_currentLevel = 0.0;
            beginSegment(State::Attack);
        } else {
            // Straight down rather than shaped: this is not a decay anybody asked to hear, it is
            // the shortest honest way from where the level stands to zero.
            m_currentLevel = m_segmentStart * (1.0 - m_phase);
        }
        break;
    case State::Hold:
        m_currentLevel = 1.0;
        m_phase += m_phaseStep;
        if (m_phase >= 1.0) {
            beginSegment(State::Decay);
        }
        break;
    case State::Attack:
    case State::Decay:
        m_phase += m_phaseStep;
        if (m_phase >= 1.0) {
            if (m_state == State::Attack) {
                m_currentLevel = 1.0;
                beginSegment(State::Hold);
            } else {
                m_currentLevel = 0.0;
                m_state = State::Idle;
            }
        } else {
            const double shaped = shape(m_phase);
            // The attack rises into the top and the decay falls away from where it started, so the
            // one shaping function bends one concave and the other convex.
            m_currentLevel = m_state == State::Attack
              ? m_segmentStart + (1.0 - m_segmentStart) * shaped
              : m_segmentStart * (1.0 - shaped);
        }
        break;
    }
    return m_currentLevel;
}

double DrumAmpEnvelope::value() const
{
    return m_currentLevel;
}

DrumAmpEnvelope::State DrumAmpEnvelope::state() const
{
    return m_state;
}

bool DrumAmpEnvelope::isActive() const
{
    return m_state != State::Idle;
}

double DrumAmpEnvelope::segmentDuration(State state) const
{
    switch (state) {
    case State::Choke:
        return ChokeSeconds;
    case State::Attack:
        // What is left to climb, so a retrigger part-way up takes the shorter path rather than the
        // full attack time from where it already is.
        return m_attackTime * std::max(0.0, 1.0 - m_segmentStart);
    case State::Hold:
        return m_holdTime;
    case State::Decay:
        return m_decayTime * std::clamp(m_segmentStart, 0.0, 1.0);
    default:
        return 0.0;
    }
}

void DrumAmpEnvelope::beginSegment(State state)
{
    m_state = state;
    m_segmentStart = m_currentLevel;
    m_phase = 0.0;
    updatePhaseStep();
}

void DrumAmpEnvelope::updatePhaseStep()
{
    // A segment with nowhere to travel is over on the next sample rather than never, which is what
    // lets a zero hold fall straight through into the decay.
    const double samples = segmentDuration(m_state) * m_sampleRate;
    m_phaseStep = samples > 1.0 ? 1.0 / samples : 1.0;
}

double DrumAmpEnvelope::shape(double phase) const
{
    const double curvature = m_curve * MaxCurvature;
    if (curvature < 1.0e-6) {
        return phase;
    }
    return std::expm1(-curvature * phase) / std::expm1(-curvature);
}

} // namespace noteahead
