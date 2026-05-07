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

namespace noteahead {

void AdsrEnvelope::setAttackTime(double seconds)
{
    m_attackTime = std::max(0.000001, seconds);
    calculateSteps();
}

void AdsrEnvelope::setDecayTime(double seconds)
{
    m_decayTime = std::max(0.000001, seconds);
    calculateSteps();
}

void AdsrEnvelope::setSustainLevel(double level)
{
    m_sustainLevel = std::clamp(level, 0.0, 1.0);
    calculateSteps();
}

void AdsrEnvelope::setReleaseTime(double seconds)
{
    m_releaseTime = std::max(0.000001, seconds);
    calculateSteps();
}

void AdsrEnvelope::setSampleRate(double sampleRate)
{
    if (std::abs(m_sampleRate - sampleRate) < 0.1) {
        return;
    }
    DspComponent::setSampleRate(sampleRate);
    calculateSteps();
}

void AdsrEnvelope::trigger()
{
    m_state = State::Attack;
}

void AdsrEnvelope::release()
{
    if (m_state != State::Idle) {
        m_state = State::Release;
    }
}

void AdsrEnvelope::reset()
{
    m_state = State::Idle;
    m_currentLevel = 0.0;
}

double AdsrEnvelope::nextSample()
{
    switch (m_state) {
    case State::Idle:
        m_currentLevel = 0.0;
        break;
    case State::Attack:
        m_currentLevel += m_attackStep;
        if (m_currentLevel >= 1.0) {
            m_currentLevel = 1.0;
            m_state = State::Decay;
        }
        break;
    case State::Decay:
        m_currentLevel -= m_decayStep;
        if (m_currentLevel <= m_sustainLevel) {
            m_currentLevel = m_sustainLevel;
            m_state = State::Sustain;
        }
        break;
    case State::Sustain:
        m_currentLevel = m_sustainLevel;
        break;
    case State::Release:
        m_currentLevel -= m_releaseStep;
        if (m_currentLevel <= 0.0) {
            m_currentLevel = 0.0;
            m_state = State::Idle;
        }
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

void AdsrEnvelope::calculateSteps()
{
    m_attackStep = 1.0 / (m_attackTime * m_sampleRate);
    m_decayStep = (1.0 - m_sustainLevel) / (m_decayTime * m_sampleRate);
    m_releaseStep = 1.0 / (m_releaseTime * m_sampleRate);
}

} // namespace noteahead
