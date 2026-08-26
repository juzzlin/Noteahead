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

#include <algorithm>
#include <cmath>
#include <numbers>

namespace noteahead {

void CascadedSvf::setCutoff(double cutoff)
{
    m_cutoff = std::clamp(cutoff, 0.0, 1.0);
}

void CascadedSvf::setResonance(double resonance)
{
    m_resonance = std::clamp(resonance, 0.0, 1.0);
}

void CascadedSvf::setMode(Mode mode)
{
    m_mode = mode;
}

void CascadedSvf::setOrder(int order)
{
    m_order = (order == 2) ? 2 : 4;
}

double CascadedSvf::process(double input)
{
    // Zero-Delay Feedback State Variable Filter
    // Stable for all frequencies up to Nyquist
    if (std::abs(m_cutoff - m_lastCutoff) > 0.000001 || std::abs(m_resonance - m_lastResonance) > 0.000001 || std::abs(m_sampleRate - m_lastSampleRate) > 0.1) {
        const double maxFreq = std::min(20000.0, m_sampleRate * 0.49);
        const double freq = 20.0 * std::exp2(m_cutoff * std::log2(maxFreq / 20.0));
        m_g = std::tan(std::numbers::pi * freq / m_sampleRate);
        m_k = 2.0 * (1.0 - m_resonance);
        m_damping = 1.0 / (1.0 + m_g * (m_g + m_k));

        m_lastCutoff = m_cutoff;
        m_lastResonance = m_resonance;
        m_lastSampleRate = m_sampleRate;
    }

    double out = m_unit1.process(input, m_g, m_damping, m_k, m_mode);
    if (m_order == 4) {
        out = m_unit2.process(out, m_g, m_damping, m_k, m_mode);
    }

    // NaN protection
    if (std::isnan(out)) {
        reset();
        return 0.0;
    }

    // At the far end of its range the filter is "off" and hands the signal straight through, which
    // is what a patch parked there expects -- a default high pass must not quietly become a 20 Hz
    // four-pole and thin out the bass. Two things are needed to make that safe under a sweep.
    //
    // The filter above still ran, even while the output is dry. Skipping it would leave s1 and s2
    // holding whatever they held when the cutoff last reached the end, so a sweep coming back would
    // re-enter with state from tens of milliseconds ago and step the output.
    //
    // And the handover itself is blended rather than switched: at the threshold the filter is not
    // quite an identity -- a four-pole high pass at 20 Hz still turns a 220 Hz tone by about 20
    // degrees -- so swapping dry for filtered in one sample is its own discontinuity. Crossfading
    // over a narrow band either side keeps the endpoints exact and the sweep continuous.
    const double filteredWeight = [this] {
        if (m_mode == Mode::LowPass) {
            return std::clamp((BypassThresholdLowPass - m_cutoff) / BypassBlendWidth, 0.0, 1.0);
        }
        if (m_mode == Mode::HighPass) {
            return std::clamp((m_cutoff - BypassThresholdHighPass) / BypassBlendWidth, 0.0, 1.0);
        }
        return 1.0;
    }();

    return input + (out - input) * filteredWeight;
}

void CascadedSvf::reset()
{
    m_unit1.reset();
    m_unit2.reset();
}

double CascadedSvf::SvfUnit::process(double input, double g, double damping, double k, Mode mode)
{
    const double hp = (input - (g + k) * s1 - s2) * damping;
    const double v1 = g * hp;
    const double bp = v1 + s1;
    s1 = v1 + bp;
    const double v2 = g * bp;
    const double lp = v2 + s2;
    s2 = v2 + lp;

    if (mode == Mode::LowPass)
        return lp;
    if (mode == Mode::HighPass)
        return hp;
    if (mode == Mode::BandPass)
        return bp;
    if (mode == Mode::Notch)
        return lp + hp;
    return 0.0;
}

} // namespace noteahead
