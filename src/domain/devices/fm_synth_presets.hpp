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

#ifndef FM_SYNTH_PRESETS_HPP
#define FM_SYNTH_PRESETS_HPP

#include "synth_presets.hpp"

#include <cstdint>
#include <vector>

namespace noteahead {

//! Factory patches for the FM synth.
//!
//! A preset names only the parameters that differ from the init patch: loading one puts everything
//! back to its default first, so nothing left unsaid can carry over from whatever was loaded
//! before. The value of each is the parameter's internal one -- 0..1 for a knob, the raw ordinal
//! for a choice.
//!
//! SynthPreset is reused rather than duplicated: it is a name and a map of parameter values, which
//! is as true here as it is for the subtractive synth.
class FmSynthPresets
{
public:
    static const std::vector<SynthPreset> & presets();

    //! A patch assembled at random, within the bounds that keep it musical.
    //!
    //! Randomising all sixty-seven parameters independently gives noise essentially every time: the
    //! interesting region of an FM synth is narrow, and most of it is about how the parameters
    //! agree with each other. So this picks a shape first -- an envelope archetype, an algorithm,
    //! whole-number ratios drawn from a weighted table -- and only then fills in the detail. It is
    //! the fastest way to find FM sounds that nobody would have thought to dial in.
    //!
    //! @p seed selects the patch, so the same seed always gives the same one.
    static SynthPreset randomPatch(uint32_t seed);
};

} // namespace noteahead

#endif // FM_SYNTH_PRESETS_HPP
