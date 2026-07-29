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

#include "divide_down_generator.hpp"

#include <cmath>

namespace noteahead {

namespace {

//! Highest phase increment that still yields a usable band-limited saw. Above it the tap would be
//! at or beyond Nyquist and polyBLEP has nothing left to correct, so the tap is dropped instead.
constexpr double MaxIncrement { 0.45 };

double noteToFrequency(int note)
{
    return 440.0 * std::pow(2.0, (static_cast<double>(note) - 69.0) / 12.0);
}

//! Standard polyBLEP residual for the saw discontinuity at the phase wrap.
double polyBlep(double t, double dt)
{
    if (t < dt) {
        t /= dt;
        return t + t - t * t - 1.0;
    }
    if (t > 1.0 - dt) {
        t = (t - 1.0) / dt;
        return t * t + t + t + 1.0;
    }
    return 0.0;
}

} // namespace

DivideDownGenerator::DivideDownGenerator()
{
    // The increments have to exist before the first setSampleRate() call, which skips the update
    // when the rate already matches the default: the phasors would otherwise never advance.
    updateIncrements();
}

void DivideDownGenerator::setSampleRate(double sampleRate)
{
    if (std::abs(m_sampleRate - sampleRate) < 0.1) {
        return;
    }
    DspComponent::setSampleRate(sampleRate);
    updateIncrements();
}

void DivideDownGenerator::updateIncrements()
{
    if (m_sampleRate <= 0.0) {
        return;
    }
    for (int pitchClass = 0; pitchClass < PitchClassCount; pitchClass++) {
        m_masters.at(static_cast<size_t>(pitchClass)).increment = noteToFrequency(LowestNote + pitchClass) / m_sampleRate;
    }
}

void DivideDownGenerator::tick()
{
    for (auto && master : m_masters) {
        master.phase += master.increment;
        if (master.phase >= 1.0) {
            master.phase -= std::floor(master.phase);
        }
    }
}

double DivideDownGenerator::saw(uint8_t note, int octaveOffset) const
{
    const int octave = static_cast<int>(note) / PitchClassCount + octaveOffset;
    if (octave < 0) {
        return 0.0;
    }

    const auto & master = m_masters.at(static_cast<size_t>(note % PitchClassCount));
    const double divider = std::exp2(static_cast<double>(octave));
    const double increment = master.increment * divider;
    if (increment > MaxIncrement) {
        return 0.0;
    }

    const double scaled = master.phase * divider;
    const double phase = scaled - std::floor(scaled);

    return 2.0 * phase - 1.0 - polyBlep(phase, increment);
}

void DivideDownGenerator::reset()
{
    for (auto && master : m_masters) {
        master.phase = 0.0;
    }
}

} // namespace noteahead
