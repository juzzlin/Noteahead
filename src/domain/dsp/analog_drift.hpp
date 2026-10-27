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

#ifndef ANALOG_DRIFT_HPP
#define ANALOG_DRIFT_HPP

#include <array>
#include <cstddef>

namespace noteahead {

//! The slow wander of an analogue oscillator's tuning, a few tenths of a hertz. A single sine made
//! two oscillators at one pitch close in and part again on a regular cycle, which is a flanger; three
//! at rates no ratio of small numbers relates never repeat, which is drift. Every index has a rate
//! and starting phases of its own, taken from the fractional parts of multiples of two irrationals,
//! so no two indices ever move together and the same index always moves the same way: like the
//! Synth's drift, a render is repeatable.
class AnalogDrift
{
public:
    //! Puts the wander back to where the index starts.
    void initialize(size_t index);

    //! Advances by one sample and returns where the wander is, -1 to 1.
    double next(double sampleRate);

    //! A fixed offset, -1 to 1, of one component of one voice: its part-to-part tolerance. The
    //! component tells apart the parts of one voice, so that a voice tuned sharp is not also the
    //! one with the brightest filter.
    static double tolerance(size_t index, size_t component);

private:
    static constexpr size_t PartialCount { 3 };
    std::array<double, PartialCount> m_phases {};
    double m_rateHz { 0.1 };
};

} // namespace noteahead

#endif // ANALOG_DRIFT_HPP
