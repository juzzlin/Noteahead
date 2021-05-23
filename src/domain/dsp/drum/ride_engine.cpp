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

#include "ride_engine.hpp"

#include <algorithm>
#include <cmath>

namespace noteahead {

RideEngine::RideEngine()
{
    m_rng.seed(std::random_device {}());
    m_filter.setMode(CascadedSvf::Mode::HighPass);
}

void RideEngine::trigger(float velocity)
{
    m_velocity = velocity;
    m_active = true;
    m_filter.reset();
    m_ampEnv = 1.0f;
    for (auto && phase : m_phases) {
        phase = 0.0;
    }
}

float RideEngine::nextSample()
{
    if (!m_active) {
        return 0.0f;
    }

    const double sr { sampleRate() };
    const float noise { m_dist(m_rng) };

    const double baseFreq { 300.0 + m_tune * 500.0 };
    static constexpr std::array<double, 6> ratios { 1.0, 1.48, 1.92, 2.54, 3.41, 4.23 };

    double metallicSource = 0.0;
    for (size_t i = 0; i < 6; ++i) {
        const double freq = baseFreq * ratios[i];
        m_phases[i] += freq / sr;
        if (m_phases[i] >= 1.0)
            m_phases[i] -= 1.0;
        metallicSource += (m_phases[i] < 0.5 ? 1.0 : -1.0);
    }
    metallicSource /= 6.0;

    float source = static_cast<float>(metallicSource) * 0.7f + noise * 0.3f;

    m_filter.setSampleRate(sr);
    m_filter.setCutoff(0.4f + m_tune * 0.5f);
    m_filter.setResonance(m_resonance);
    const auto out = static_cast<float>(m_filter.process(source) * m_ampEnv * m_velocity);

    const float decayRate { 1.0f - (1.0f / (std::max(0.01f, m_decay) * 2.0f * static_cast<float>(sampleRate()))) };
    m_ampEnv *= decayRate;
    if (m_ampEnv < AmplitudeThreshold) {
        m_active = false;
        m_ampEnv = 0.0f;
    }

    return out;
}

bool RideEngine::isActive() const
{
    return m_active;
}

void RideEngine::reset()
{
    m_active = false;
    m_ampEnv = 0.0f;
}

void RideEngine::setTune(float tune)
{
    m_tune = tune;
}

void RideEngine::setDecay(float decay)
{
    m_decay = decay;
}

void RideEngine::setResonance(float resonance)
{
    m_resonance = resonance;
}

} // namespace noteahead
