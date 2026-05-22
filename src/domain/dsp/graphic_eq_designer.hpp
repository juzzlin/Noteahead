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

#ifndef GRAPHIC_EQ_DESIGNER_HPP
#define GRAPHIC_EQ_DESIGNER_HPP

#include <array>
#include <cstddef>

namespace noteahead {

//! Works out what a cascade of third-octave bells has to be set to for its response to pass through
//! the slider positions of a graphic equalizer.
//!
//! Setting each bell to its slider does not do it: neighbouring bells overlap, so five sliders up
//! by 3 dB come out nearer 6, and a lone slider comes out wider than its neighbours suggest. The
//! overlap is measured instead -- the response of every band at every centre and every point half
//! way between two -- and the gains are solved for by least squares, after Valimaki and Liski's
//! accurate cascade graphic equalizer. The solve is repeated twice with the bells at the gains the
//! previous pass found, because a bell's shape changes with its gain and the first pass assumes
//! they all have the same one.
//!
//! The points between the centres are weighted down: they keep the curve from rippling between the
//! bands without pulling the centres, which are what the sliders promise, off their marks. Measured
//! on the bells actually used, so whatever the top octave does at a given sample rate is accounted
//! for too. Matched to within 0.1 dB at every centre across the whole +-12 dB range.
class GraphicEqDesigner
{
public:
    //! The ISO third-octave bands from 16 Hz to 20 kHz.
    static constexpr size_t BandCount { 32 };
    using Gains = std::array<double, BandCount>;

    //! The exact centre of @p band, 1 kHz times a whole number of thirds of an octave. The panel
    //! labels them with the nominal ISO values instead.
    static double centreFrequency(size_t band);
    //! One and a half times the band spacing wide, which is what lets neighbours add up to a smooth
    //! curve rather than a row of bumps.
    static double bandQ();
    //! Whether the sample rate has room for @p band. One whose centre sits too close to Nyquist is
    //! left out of the design and passed through.
    static bool isBandUsable(size_t band, double sampleRate);

    //! The gains, in dB, to set the bells to for the response to pass through @p sliderGainsDb.
    //! All zero when every slider is, so that a flat panel is a straight wire.
    static Gains filterGains(const Gains & sliderGainsDb, double sampleRate);
};

} // namespace noteahead

#endif // GRAPHIC_EQ_DESIGNER_HPP
