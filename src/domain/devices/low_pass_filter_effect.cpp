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

#include "low_pass_filter_effect.hpp"
#include "../../common/utils.hpp"
#include "../dsp/audio_context.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace noteahead {

void LowPassFilterEffect::setCutoff(double cutoff)
{
    m_cutoff = cutoff;
}

void LowPassFilterEffect::process(double & left, double & right)
{
    if (m_cutoff >= 0.999) {
        return;
    }

    // Zero-Delay Feedback State Variable Filter (2nd order)
    const double freq = Utils::Dsp::cutoffToHz(static_cast<float>(m_cutoff), static_cast<float>(m_sampleRate));
    const double g = std::tan(std::numbers::pi * freq / m_sampleRate);
    const double k = 1.0; // Q = 1.0 / k. Using k = 1.0 for Q = 1.0 (slight resonance)
    const double damping = 1.0 / (1.0 + g * (g + k));

    processSample(left, right, g, damping, k);
}

void LowPassFilterEffect::process(AudioContext & context)
{
    if (m_cutoff >= 0.999) {
        return;
    }

    const double freq = Utils::Dsp::cutoffToHz(static_cast<float>(m_cutoff), static_cast<float>(m_sampleRate));
    const double g = std::tan(std::numbers::pi * freq / m_sampleRate);
    const double k = 1.0;
    const double damping = 1.0 / (1.0 + g * (g + k));

    for (uint32_t i = 0; i < context.frameCount; i++) {
        processSample(context.buffer[i * 2], context.buffer[i * 2 + 1], g, damping, k);
    }
}

void LowPassFilterEffect::processSample(double & left, double & right, double g, double damping, double k)
{
    // Left channel
    {
        const double hp = (left - (g + k) * m_s1L - m_s2L) * damping;
        const double v1 = g * hp;
        const double v = v1 + m_s1L;
        m_s1L = v1 + v;
        const double v2 = g * v;
        const double lp = v2 + m_s2L;
        m_s2L = v2 + lp;
        left = lp;
    }

    // Right channel
    {
        const double hp = (right - (g + k) * m_s1R - m_s2R) * damping;
        const double v1 = g * hp;
        const double v = v1 + m_s1R;
        m_s1R = v1 + v;
        const double v2 = g * v;
        const double lp = v2 + m_s2R;
        m_s2R = v2 + lp;
        right = lp;
    }

    // Denormal protection
    if (std::abs(m_s1L) < 1.0e-15)
        m_s1L = 0.0;
    if (std::abs(m_s2L) < 1.0e-15)
        m_s2L = 0.0;
    if (std::abs(m_s1R) < 1.0e-15)
        m_s1R = 0.0;
    if (std::abs(m_s2R) < 1.0e-15)
        m_s2R = 0.0;

    // NaN protection
    if (std::isnan(left) || std::isnan(right)) {
        reset();
        left = 0.0;
        right = 0.0;
    }
}

void LowPassFilterEffect::reset()
{
    m_s1L = m_s2L = 0.0;
    m_s1R = m_s2R = 0.0;
}

} // namespace noteahead
