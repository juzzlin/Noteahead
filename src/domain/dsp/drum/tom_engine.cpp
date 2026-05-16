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

#include "tom_engine.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace noteahead {

TomEngine::TomEngine() = default;
void TomEngine::trigger(float velocity)
{
    m_retriggerOffset = m_lastOut;
    m_velocity = velocity;
    m_ampEnv = 1.0f;
    m_attackEnv = 0.0f;
    m_pitchEnv = 1.0f;
    m_phase = 0.0;
    m_active = true;
}

float TomEngine::nextSample()
{
    if (!m_active) {
        m_lastOut = 0.0f;
        return 0.0f;
    }

    const double baseFreq { 80.0 + (m_tune * 150.0) };
    const double sweepFreq { baseFreq + (m_pitchDepth * 200.0 * m_pitchEnv) };

    float out { static_cast<float>(std::sin(m_phase * 2.0 * std::numbers::pi)) * m_ampEnv * m_attackEnv * m_velocity * 0.7f };

    // Apply re-trigger offset to smooth out discontinuities
    out += m_retriggerOffset;
    m_retriggerOffset *= 0.95f;

    const double phaseStep { sweepFreq / sampleRate() };
    m_phase += phaseStep;
    if (m_phase >= 1.0) {
        m_phase -= 1.0;
    }

    const float ampDecayRate { 1.0f - (1.0f / (std::max(0.001f, m_decay) * 0.8f * static_cast<float>(sampleRate()))) };
    const float pitchDecayRate { 1.0f - (1.0f / (std::max(0.001f, m_pitchDecay * 0.1f) * static_cast<float>(sampleRate()))) };

    const float attackRate { 1.0f / (0.0005f * static_cast<float>(sampleRate())) };
    m_attackEnv = std::min(1.0f, m_attackEnv + attackRate);

    m_ampEnv *= ampDecayRate;
    m_pitchEnv *= pitchDecayRate;

    if (m_ampEnv < AmplitudeThreshold && std::abs(out) < AmplitudeThreshold) {
        m_active = false;
        m_ampEnv = 0.0f;
        m_retriggerOffset = 0.0f;
    }

    m_lastOut = out;
    return out;
}

void TomEngine::reset()
{
    m_active = false;
    m_ampEnv = 0.0f;
    m_lastOut = 0.0f;
    m_retriggerOffset = 0.0f;
}

bool TomEngine::isActive() const
{
    return m_active;
}

void TomEngine::setTune(float tune)
{
    m_tune = tune;
}

void TomEngine::setDecay(float decay)
{
    m_decay = decay;
}

void TomEngine::setPitchDepth(float depth)
{
    m_pitchDepth = depth;
}

void TomEngine::setPitchDecay(float decay)
{
    m_pitchDecay = decay;
}

} // namespace noteahead
