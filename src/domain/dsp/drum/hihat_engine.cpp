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

#include "hihat_engine.hpp"

#include <algorithm>
#include <cmath>

namespace noteahead {

HiHatEngine::HiHatEngine()
{
    m_rng.seed(std::random_device {}());
    m_filter.setMode(CascadedSvf::Mode::HighPass);
    m_bodyFilter.setMode(CascadedSvf::Mode::BandPass);
}

void HiHatEngine::trigger(float velocity)
{
    m_velocity = velocity;
    m_ampEnv = 1.0f;
    m_bodyEnv = 1.0f;
    m_active = true;
    m_choking = false;
    m_filter.reset();
    m_bodyFilter.reset();
    for (auto && phase : m_phases) {
        phase = 0.0;
    }
}

float HiHatEngine::nextSample()
{
    if (!m_active) {
        return 0.0f;
    }

    const double sr { sampleRate() };
    const float noise { m_dist(m_rng) };

    // Metallic part: 6 square wave oscillators with ratios (808-style)
    const double baseFreq { 450.0 + m_tune * 750.0 };
    static constexpr std::array<double, 6> ratios { 1.0, 1.47, 1.91, 2.51, 3.39, 4.21 };

    double metallicSource = 0.0;
    for (size_t i = 0; i < 6; ++i) {
        const double freq = baseFreq * ratios[i];
        m_phases[i] += freq / sr;
        if (m_phases[i] >= 1.0)
            m_phases[i] -= 1.0;
        metallicSource += (m_phases[i] < 0.5 ? 1.0 : -1.0);
    }
    metallicSource /= 6.0;

    // Blend: metallic core with noise - more noise for 909-style character
    float source = static_cast<float>(metallicSource) * 0.7f + noise * 0.6f;

    // Body component: metallic + noise-based impact for "thickness"
    // Use a band-pass filter to create a "thump"
    m_bodyFilter.setSampleRate(sr);
    m_bodyFilter.setCutoff(0.28f + m_tune * 0.15f); // Centered around mid-range
    m_bodyFilter.setResonance(0.5f);

    // Body is more prominent if decay is long (Open Hat)
    const float bodyGain = 0.85f * std::min(1.0f, m_decay * 2.0f);
    const float bodyOut = m_bodyFilter.process(source) * m_bodyEnv * bodyGain;

    m_filter.setSampleRate(sr);
    // High-pass filter to keep it thin and "metallic"
    // Starting slightly lower to let more body through
    m_filter.setCutoff(0.68f + m_tune * 0.2f);
    m_filter.setResonance(m_resonance * 0.35f);

    // Sum and saturate for "body" and warmth (909-style)
    float mixed = (m_filter.process(source) + bodyOut);
    float out = std::tanh(mixed * 1.4f) * m_ampEnv * m_velocity * 1.1f;

    // Body decay is slightly longer to provide more "meat"
    const float bodyDecayRate { 1.0f - (1.0f / (0.06f * static_cast<float>(sr))) };
    m_bodyEnv *= bodyDecayRate;

    const float decayRate = m_choking
        ? 1.0f - (1.0f / (0.015f * static_cast<float>(sr)))
        : 1.0f - (1.0f / (std::max(0.001f, m_decay) * 0.18f * static_cast<float>(sr)));
    m_ampEnv *= decayRate;

    if (m_ampEnv < AmplitudeThreshold) {
        m_active = false;
        m_choking = false;
        m_ampEnv = 0.0f;
    }

    return out;
}

bool HiHatEngine::isActive() const
{
    return m_active;
}

void HiHatEngine::reset()
{
    m_active = false;
    m_choking = false;
    m_ampEnv = 0.0f;
}

void HiHatEngine::setTune(float tune)
{
    m_tune = tune;
}

void HiHatEngine::setDecay(float decay)
{
    m_decay = decay;
}

void HiHatEngine::setResonance(float resonance)
{
    m_resonance = resonance;
}

void HiHatEngine::stop()
{
    m_choking = true;
}

} // namespace noteahead
