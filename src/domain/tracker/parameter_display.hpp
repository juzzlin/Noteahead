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

#ifndef PARAMETER_DISPLAY_HPP
#define PARAMETER_DISPLAY_HPP

#include <string>

namespace noteahead {

//! What a parameter's stored 0..1 knob position means to the person turning it: the curve the
//! knob follows, the unit and range it reads out in, and where it rests.
//!
//! A parameter stores only the position, which is meaningless outside the dialog that knows how
//! to read it -- nobody can tell what cutoff 0.78 is. This is what lets anything else, automation
//! first, speak in hertz and milliseconds rather than in positions.
//!
//! The curves are the knob's own, named as Knob.qml names them, so a value shown here reads exactly
//! as it does on the knob, and the knob's readout and typed entry can be reused for it.
struct ParameterDisplay
{
    enum class Mapping
    {
        Linear,
        Exponential,
        CubicCentered,
        LogFrequency,
        LfoFrequency
    };

    //! Where the control does nothing, which is what a relative automation scales towards. The
    //! bottom of the travel for most controls; the middle for one that runs either way about off,
    //! so that scaling it down fades the effect out instead of turning it inside out.
    enum class Rest
    {
        Minimum,
        Centre
    };

    Mapping mapping = Mapping::Linear;
    std::string unit;
    double min = 0.0;
    double max = 1.0;
    Rest rest = Rest::Minimum;

    //! The value a knob position reads out as, in unit.
    double toUnits(double position) const;
    //! The knob position that reads out as @p units, clamped to the travel.
    double toPosition(double units) const;
    //! The knob position the control rests at.
    double restPosition() const;
    //! The name Knob.qml and KnobController use for mapping.
    std::string mappingName() const;
};

} // namespace noteahead

#endif // PARAMETER_DISPLAY_HPP
