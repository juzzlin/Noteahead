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

#ifndef METAL_909_SOURCES_HPP
#define METAL_909_SOURCES_HPP

#include <array>
#include <span>

//! What the 909's metal sounds are made of: the partials and the noise between them that analyses
//! of an RD-9 found, each sound at every knob's noon. Generated from those analyses; see
//! Metal909Engine for how they are played.
namespace noteahead::Metal909Sources {

//! A sine at @p hz, of @p amplitude, starting at @p phase and dying away as exp(-t / seconds), or
//! not at all for 0
struct Partial
{
    double hz;
    double amplitude;
    double phase;
    double seconds;
};

//! White noise through a third-octave band-pass at @p hz, of @p gain, dying away as
//! exp(-t / seconds), or not at all for 0
struct Band
{
    double hz;
    double gain;
    double seconds;
};

std::span<const Partial> closedHatPartials();
std::span<const Band> closedHatBands();
std::span<const Partial> openHatPartials();
std::span<const Band> openHatBands();
std::span<const Partial> crashPartials();
std::span<const Band> crashBands();
std::span<const Partial> ridePartials();
std::span<const Band> rideBands();

} // namespace noteahead::Metal909Sources

#endif // METAL_909_SOURCES_HPP
