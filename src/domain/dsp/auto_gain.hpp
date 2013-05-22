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

#ifndef AUTO_GAIN_HPP
#define AUTO_GAIN_HPP

#include <array>
#include <cstddef>

namespace noteahead {

//! Level compensation for an equalizer, worked out from its own response rather than measured.
//!
//! The response is averaged as power over a log-spaced grid, so that every octave counts the same,
//! which is the weighting pink noise has and close to the spectrum of a full mix. The gain is
//! what brings that average back to unity. Because it depends on the settings alone it costs
//! nothing per sample, does not pump, and renders the same every time.
class AutoGain
{
public:
    static constexpr size_t GridSize { 64 };
    using Grid = std::array<double, GridSize>;

    //! Where a response is sampled for compensationDb(): 20 Hz to 20 kHz, kept below Nyquist.
    static Grid gridFrequencies(double sampleRate);

    //! The gain in dB that brings the average power of @p magnitudesDb, sampled at
    //! gridFrequencies(), back to unity.
    static double compensationDb(const Grid & magnitudesDb);

    //! Ramps to @p gainDb, or jumps there when no sample rate is known yet.
    void setTargetDb(double gainDb, double sampleRate);

    //! Jumps to the target, for a reset: nothing is playing to click.
    void reset();

    //! The gain for the next frame, as a linear factor. Exactly 1 once settled at 0 dB, so an
    //! equalizer with auto gain off stays bit for bit what it was.
    double next();

private:
    double m_gain { 1.0 };
    double m_target { 1.0 };
    double m_step { 0.0 };
    size_t m_framesLeft { 0 };
};

} // namespace noteahead

#endif // AUTO_GAIN_HPP
