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

#include "cascaded_svf.hpp"

#include <cmath>
#include <algorithm>
#include <numbers>

namespace noteahead {

void CascadedSVF::setSampleRate(double sampleRate)
{
    m_sampleRate = sampleRate;
}

void CascadedSVF::setCutoff(double cutoff)
{
    m_cutoff = std::clamp(cutoff, 0.0, 1.0);
}

void CascadedSVF::setResonance(double resonance)
{
    m_resonance = std::clamp(resonance, 0.0, 1.0);
}

void CascadedSVF::setMode(Mode mode)
{
    m_mode = mode;
}

float CascadedSVF::process(float input)
{
    if (m_mode == Mode::LowPass && m_cutoff >= 0.999) {
        return input;
    }
    if (m_mode == Mode::HighPass && m_cutoff <= 0.001) {
        return input;
    }

    // Zero-Delay Feedback State Variable Filter
    // Stable for all frequencies up to Nyquist
    const double freq = 20.0 * std::pow(std::min(20000.0, m_sampleRate * 0.49) / 20.0, m_cutoff);
    const double g = std::tan(std::numbers::pi * freq / m_sampleRate);
    const double k = 2.0 * (1.0 - m_resonance);
    const double damping = 1.0 / (1.0 + g * (g + k));

    float out1 = m_unit1.process(input, g, damping, k, m_mode);
    float out2 = m_unit2.process(out1, g, damping, k, m_mode);

    // NaN protection
    if (std::isnan(out2)) {
        reset();
        return 0.0f;
    }

    return out2;
}

void CascadedSVF::reset()
{
    m_unit1.reset();
    m_unit2.reset();
}

float CascadedSVF::SVFUnit::process(float input, double g, double damping, double k, Mode mode)
{
    const double hp = (input - (g + k) * s1 - s2) * damping;
    const double v1 = g * hp;
    const double bp = v1 + s1;
    s1 = v1 + bp;
    const double v2 = g * bp;
    const double lp = v2 + s2;
    s2 = v2 + lp;
    
    return static_cast<float>(mode == Mode::LowPass ? lp : hp);
}

} // namespace noteahead
