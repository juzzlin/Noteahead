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

#ifndef PARAMETER_MODULATION_HPP
#define PARAMETER_MODULATION_HPP

namespace noteahead {

class Parameter;

//! How a modulation source -- an LFO -- moves a parameter around where it is set.
//!
//! A modulation is computed from the parameter's unmodulated value, whatever the user or an
//! automation left it at, and written on top of it: see Parameter::setModulatedValue(). Everything
//! is in knob space, so a log-mapped cutoff is swept musically, as relative automation does.
namespace ParameterModulation {

//! Persisted as an ordinal by whatever owns a modulation, so append-only.
enum class Mode
{
    //! Either way of the setting: depth 100 % swings half the knob's travel up and down.
    Bipolar = 0,
    //! From the setting towards where the control rests, the way relative automation scales.
    Unipolar,
    //! A fader's gain scaled, as Expression does; anything that is not a fader takes Unipolar.
    Tremolo
};

static constexpr int ModeCount { 3 };

//! The value a parameter takes under a modulation. @p source runs from -1 to 1, as an LFO's output
//! does, and @p depth from 0 to 1.
float modulatedValue(Mode mode, double source, double depth, const Parameter & parameter);

} // namespace ParameterModulation

} // namespace noteahead

#endif // PARAMETER_MODULATION_HPP
