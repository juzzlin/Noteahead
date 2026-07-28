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

#ifndef ONE_POLE_FILTER_HPP
#define ONE_POLE_FILTER_HPP

namespace noteahead {

//! First-order topology-preserving transform filter exposing both of its taps.
//!
//! SvfFilter covers the second-order cases; this covers the first-order ones, where the 6 dB/octave
//! roll-off is the point rather than a limitation. Summing a first-order tap into a dry path yields
//! a far gentler transition than a second-order one, which is what a wide, slow shelf needs.
//!
//! A single process() call advances the state and updates both taps, so reading lowPass() and
//! highPass() after it costs nothing extra and cannot double-advance the filter.
class OnePoleFilter
{
public:
    //! Highest corner representable at the given sample rate.
    //!
    //! calculate() clamps silently, so callers that legitimately ask for corners above the audio
    //! band need this to find out whether, and by how much, their request was clamped.
    static double maxCorner(double sampleRate);

    void calculate(double frequency, double sampleRate);

    void process(double input);

    double lowPass() const;
    double highPass() const;

    void reset();

private:
    double m_g { 0.0 };
    double m_s { 0.0 };
    double m_lowPass { 0.0 };
    double m_highPass { 0.0 };
};

} // namespace noteahead

#endif // ONE_POLE_FILTER_HPP
