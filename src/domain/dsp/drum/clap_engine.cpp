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

#include "clap_engine.hpp"

#include <algorithm>
#include <cmath>

namespace noteahead {

ClapEngine::ClapEngine()
{
    m_rng.seed(0);
    m_filter.setMode(CascadedSvf::Mode::HighPass);
}

void ClapEngine::trigger(float velocity)
{
    m_velocity = velocity;
    m_active = true;
    m_sampleCount = 0;
    m_tailEnv = 1.0f;
    m_attackEnv = 0.0f;
    m_filter.reset();
}

float ClapEngine::nextSample()
{
    if (!m_active) {
        return 0.0f;
    }

    float burstAmp = 0.0f;
    const double sr = sampleRate();

    // Three initial bursts
    const int burstInterval = static_cast<int>(0.01f * sr);
    const int burstDuration = static_cast<int>(0.005f * sr);

    for (int i = 0; i < 3; ++i) {
        int start = i * burstInterval;
        if (m_sampleCount >= start && m_sampleCount < start + burstDuration) {
            burstAmp = 1.0f - static_cast<float>(m_sampleCount - start) / burstDuration;
            break;
        }
    }

    // Main tail starts after bursts
    float tailAmp = 0.0f;
    const int tailStart = static_cast<int>(0.03f * sr);
    if (m_sampleCount >= tailStart) {
        tailAmp = m_tailEnv;
        const float decayRate = 1.0f - (1.0f / (std::max(0.01f, m_decay) * 0.2f * static_cast<float>(sr)));
        m_tailEnv *= decayRate;
    }

    const float noise = m_dist(m_rng);
    m_filter.setSampleRate(sr);
    m_filter.setCutoff(0.2f + m_tune * 0.5f);
    m_filter.setResonance(0.3f);

    const float out = m_filter.process(noise) * (burstAmp + tailAmp) * m_attackEnv * m_velocity;

    const float attackRate { 1.0f / (0.0005f * static_cast<float>(sampleRate())) };
    m_attackEnv = std::min(1.0f, m_attackEnv + attackRate);

    m_sampleCount++;

    if (m_sampleCount > tailStart && m_tailEnv < AmplitudeThreshold) {
        m_active = false;
    }

    return out;
}

bool ClapEngine::isActive() const
{
    return m_active;
}

void ClapEngine::reset()
{
    m_active = false;
    m_sampleCount = 0;
}

void ClapEngine::setTune(float tune)
{
    m_tune = tune;
}

void ClapEngine::setDecay(float decay)
{
    m_decay = decay;
}

} // namespace noteahead
