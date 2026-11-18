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
    updateCoefficients();

    const double drive = 1.0 + m_drive * 3.0;
    const double in = static_cast<double>(input) * drive * m_gainCompensation;

    // Zero-Delay Feedback solver for 4-pole TPT diode ladder:
    // Each stage has y_i = G * x_i + (1 - G) * s_i, where G = g / (1 + g).
    // Across all 4 stages, lp4 = G^4 * v + S, with S = (1 - G) * (G^3*s1 + G^2*s2 + G*s3 + s4).
    const double g = m_g;
    const double k = m_k;
    const double oneOverOnePlusG = 1.0 / (1.0 + g);
    const double G = g * oneOverOnePlusG;
    const double G2 = G * G;
    const double G3 = G2 * G;
    const double G4 = G2 * G2;

    const double S = oneOverOnePlusG * (G3 * m_s1 + G2 * m_s2 + G * m_s3 + m_s4);
    const double u = (in - k * S) / (1.0 + k * G4);

    // Soft-clipping non-linearity in the feedback loop for the "squelch" and saturation
    const double v = std::tanh(u);

    // 4-pole ladder stages (ZDF)
    const double v1 = (v - m_s1) * G;
    const double lp1 = v1 + m_s1;
    m_s1 = lp1 + v1;

    const double v2 = (lp1 - m_s2) * G;
    const double lp2 = v2 + m_s2;
    m_s2 = lp2 + v2;

    const double v3 = (lp2 - m_s3) * G;
    const double lp3 = v3 + m_s3;
    m_s3 = lp3 + v3;

    const double v4 = (lp3 - m_s4) * G;
    const double lp4 = v4 + m_s4;
    m_s4 = lp4 + v4;

    // NaN protection
    if (std::isnan(lp4)) {
        reset();
        return 0.0f;
    }

    // Wide open and undriven, the ladder is "off" and hands the signal straight through, which is
    // what a patch parked there expects. The stages above still ran: skipping them would freeze
    // s1..s4 at whatever they held when the cutoff last reached the top, so a sweep coming back
    // down would re-enter with stale state and step the output. The handover is crossfaded over a
    // narrow band for the same reason it is in CascadedSvf -- at the threshold the ladder is not
    // quite an identity, so swapping dry for filtered in one sample is its own discontinuity.
    if (m_resonance > BypassMaxResonance) {
        return static_cast<float>(lp4);
    }
    const double filteredWeight = std::clamp((BypassThresholdCutoff - m_cutoff) / BypassBlendWidth, 0.0, 1.0);
    return static_cast<float>(static_cast<double>(input) + (lp4 - static_cast<double>(input)) * filteredWeight);
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

    // S-curve resonance mapping fitted to TD-3-MO V2 resonance sweeps (knob 0, 2, 5, 8, 10 ->
    // k = 0.0, 0.33, 1.58, 2.82, 3.15), staying below the 4-pole self-oscillation ceiling (k = 4).
    const double res = m_resonance;
    m_k = MaxResonanceFeedback * res * res * (3.0 - 2.0 * res);

    // Partial passband gain compensation (~(1 + k)^0.32) matching the TD-3-MO V2 passband drop
    // (-1.0 dB at Res 2, -5.2 dB at Res 5, -8.5 dB at Res 10 instead of -12.4 dB uncompensated).
    m_gainCompensation = std::pow(1.0 + m_k, PassbandCompensationExponent);

    // Standard filter range 20Hz to 20kHz, with the TD-3-MO upward pole frequency shift under resonance.
    const double maxFreq = std::min(20000.0, m_sampleRate * 0.49);
    const double baseFreq = 20.0 * std::pow(maxFreq / 20.0, m_cutoff);
    const double freq = std::min(maxFreq, baseFreq * (1.0 + ResonanceCutoffShift * (m_k / MaxResonanceFeedback)));
    m_g = std::tan(std::numbers::pi * freq / m_sampleRate);

    m_lastCutoff = m_cutoff;
    m_lastResonance = m_resonance;
    m_lastSampleRate = m_sampleRate;
}

} // namespace noteahead
