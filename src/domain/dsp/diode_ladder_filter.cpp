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

#include "diode_ladder_filter.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace noteahead {

void DiodeLadderFilter::setCutoff(double cutoff)
{
    m_cutoff = std::clamp(cutoff, 0.0, 1.0);
}

void DiodeLadderFilter::setResonance(double resonance)
{
    m_resonance = std::clamp(resonance, 0.0, 1.0);
}

void DiodeLadderFilter::setDrive(double drive)
{
    m_drive = std::clamp(drive, 0.0, 1.0);
}

float DiodeLadderFilter::process(float input)
{
    if (m_cutoff >= 0.999 && m_resonance <= 0.001) {
        return input;
    }

    updateCoefficients();

    const double drive = 1.0 + m_drive * 3.0;
    const double in = static_cast<double>(input) * drive;

    // Zero-Delay Feedback solver for Diode Ladder
    // Using a simplified one-iteration approximation for the non-linear loop
    
    const double g = m_g;
    const double k = m_k;

    // Feedback compensation and non-linearity
    const double feedback = k * m_s4;
    const double u = (in - feedback);
    
    // Tanh non-linearity for the "squelch" and saturation
    const double v = std::tanh(u);

    // 4-pole ladder stages (ZDF)
    const double v1 = (v - m_s1) * g / (1.0 + g);
    const double lp1 = v1 + m_s1;
    m_s1 = lp1 + v1;

    const double v2 = (lp1 - m_s2) * g / (1.0 + g);
    const double lp2 = v2 + m_s2;
    m_s2 = lp2 + v2;

    const double v3 = (lp2 - m_s3) * g / (1.0 + g);
    const double lp3 = v3 + m_s3;
    m_s3 = lp3 + v3;

    const double v4 = (lp3 - m_s4) * g / (1.0 + g);
    const double lp4 = v4 + m_s4;
    m_s4 = lp4 + v4;

    // NaN protection
    if (std::isnan(lp4)) {
        reset();
        return 0.0f;
    }

    return static_cast<float>(lp4);
}

void DiodeLadderFilter::reset()
{
    m_s1 = m_s2 = m_s3 = m_s4 = 0.0;
}

void DiodeLadderFilter::updateCoefficients()
{
    if (std::abs(m_cutoff - m_lastCutoff) < 0.000001 && std::abs(m_resonance - m_lastResonance) < 0.000001 && std::abs(m_sampleRate - m_lastSampleRate) < 0.1) {
        return;
    }

    // 303 filter range is roughly 300Hz to 10kHz
    const double freq = 300.0 * std::pow(10000.0 / 300.0, m_cutoff);
    m_g = std::tan(std::numbers::pi * freq / m_sampleRate);
    
    // Resonance k: 0 to 17 is a common range for diode ladder models
    m_k = m_resonance * 17.0;

    m_lastCutoff = m_cutoff;
    m_lastResonance = m_resonance;
    m_lastSampleRate = m_sampleRate;
}

} // namespace noteahead
