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

#include "auto_gain.hpp"

#include <algorithm>
#include <cmath>

namespace noteahead {

namespace {

constexpr double LowestFrequency = 20.0;
constexpr double HighestFrequency = 20000.0;

// The furthest an equalizer can be compensated either way, which is as far as EQ 8 can boost or
// cut a band. Only a panel set to extremes gets near it.
constexpr double MaxCompensationDb = 24.0;

// Long enough that a gain change under a sustained tone does not click, short enough that a knob
// being turned is followed without an audible lag.
constexpr double RampSeconds = 0.02;

} // namespace

AutoGain::Grid AutoGain::gridFrequencies(double sampleRate)
{
    const double highest = sampleRate > 0.0 ? std::min(HighestFrequency, sampleRate * 0.45) : HighestFrequency;
    const double ratio = highest / LowestFrequency;
    Grid frequencies {};
    for (size_t i = 0; i < GridSize; i++) {
        const double t = static_cast<double>(i) / static_cast<double>(GridSize - 1);
        frequencies.at(i) = LowestFrequency * std::pow(ratio, t);
    }
    return frequencies;
}

double AutoGain::compensationDb(const Grid & magnitudesDb)
{
    double power = 0.0;
    for (const double db : magnitudesDb) {
        power += std::pow(10.0, db / 10.0);
    }
    power /= static_cast<double>(GridSize);
    return std::clamp(-10.0 * std::log10(power), -MaxCompensationDb, MaxCompensationDb);
}

void AutoGain::setTargetDb(double gainDb, double sampleRate)
{
    // Exactly 1 at 0 dB rather than whatever pow() makes of it
    m_target = gainDb == 0.0 ? 1.0 : std::pow(10.0, gainDb / 20.0);
    if (sampleRate <= 0.0) {
        reset();
        return;
    }
    m_framesLeft = std::max(size_t { 1 }, static_cast<size_t>(std::lround(RampSeconds * sampleRate)));
    m_step = (m_target - m_gain) / static_cast<double>(m_framesLeft);
}

void AutoGain::reset()
{
    m_gain = m_target;
    m_step = 0.0;
    m_framesLeft = 0;
}

double AutoGain::next()
{
    if (m_framesLeft > 0) {
        m_framesLeft--;
        // Lands on the target itself, not on the sum of the steps
        m_gain = m_framesLeft == 0 ? m_target : m_gain + m_step;
    }
    return m_gain;
}

} // namespace noteahead
