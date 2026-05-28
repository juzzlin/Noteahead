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
    updateRates();
    m_velocity = velocity;
    m_ampEnv = 1.0f;
    m_attackEnv = 0.0f;
    m_bodyEnv = 1.0f;
    m_active = true;
    m_choking = false;
    m_filter.reset();
    m_bodyFilter.reset();
    for (auto && phase : m_phases) {
        phase = 0.0;
    }

    const double sr { sampleRate() };
    m_bodyFilter.setSampleRate(sr);
    m_bodyFilter.setCutoff(0.28f + m_tune * 0.15f);
    m_bodyFilter.setResonance(0.5f);

    m_filter.setSampleRate(sr);
    m_filter.setCutoff(0.68f + m_tune * 0.2f);
    m_filter.setResonance(m_resonance * 0.35f);
}

float HiHatEngine::nextSample()
{
    if (!m_active) {
        return 0.0f;
    }

    const double sr { sampleRate() };
    if (sr != m_lastSampleRate) {
        updateRates();
        m_bodyFilter.setSampleRate(sr);
        m_filter.setSampleRate(sr);
    }

    const float noise { m_dist(m_rng) };

    // Metallic part: 6 square wave oscillators with ratios (808-style)
    const double baseFreq { 450.0 + m_tune * 750.0 };
    static constexpr std::array<double, 6> ratios { 1.0, 1.47, 1.91, 2.51, 3.39, 4.21 };

    double metallicSource = 0.0;
    const double invSr = 1.0 / sr;
    for (size_t i = 0; i < 6; ++i) {
        const double freq = baseFreq * ratios[i];
        m_phases[i] += freq * invSr;
        if (m_phases[i] >= 1.0)
            m_phases[i] -= 1.0;
        metallicSource += (m_phases[i] < 0.5 ? 1.0 : -1.0);
    }
    metallicSource /= 6.0;

    // Blend: metallic core with noise - more noise for 909-style character
    float source = static_cast<float>(metallicSource) * 0.7f + noise * 0.6f;

    // Body component: metallic + noise-based impact for "thickness"
    // Body is more prominent if decay is long (Open Hat)
    const float bodyGain = 0.85f * std::min(1.0f, m_decay * 2.0f);
    const float bodyOut = m_bodyFilter.process(source) * m_bodyEnv * bodyGain;

    // Sum and saturate for "body" and warmth (909-style)
    float mixed = (m_filter.process(source) + bodyOut);
    float out = std::tanh(mixed * 1.4f) * m_ampEnv * m_attackEnv * m_velocity * 1.1f;

    const float attackRate { 1.0f / (0.0005f * static_cast<float>(sampleRate())) };
    m_attackEnv = std::min(1.0f, m_attackEnv + attackRate);

    // Body decay is slightly longer to provide more "meat"
    m_bodyEnv *= m_bodyDecayRate;
    m_ampEnv *= m_choking ? m_chokeDecayRate : m_decayRate;

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
    m_bodyFilter.setCutoff(0.28f + m_tune * 0.15f);
    m_filter.setCutoff(0.68f + m_tune * 0.2f);
}

void HiHatEngine::setDecay(float decay)
{
    m_decay = decay;
    updateRates();
}

void HiHatEngine::setResonance(float resonance)
{
    m_resonance = resonance;
    m_filter.setResonance(m_resonance * 0.35f);
}

void HiHatEngine::stop()
{
    m_choking = true;
}

void HiHatEngine::updateRates()
{
    if (const double sr = sampleRate(); sr > 0) {
        m_lastSampleRate = sr;
        m_bodyDecayRate = 1.0f - (1.0f / (0.06f * static_cast<float>(sr)));
        m_decayRate = 1.0f - (1.0f / (std::max(0.001f, m_decay) * 0.18f * static_cast<float>(sr)));
        m_chokeDecayRate = 1.0f - (1.0f / (0.015f * static_cast<float>(sr)));
    }
}

} // namespace noteahead
