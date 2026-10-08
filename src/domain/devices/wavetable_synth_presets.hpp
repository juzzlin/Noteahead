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

#ifndef WAVETABLE_SYNTH_PRESETS_HPP
#define WAVETABLE_SYNTH_PRESETS_HPP

#include "synth_presets.hpp"

#include <vector>

namespace noteahead {

//! Factory patches for the Wavetable Synth, one or two for each of its tables.
//!
//! A preset names only the parameters that differ from the init patch: loading one puts everything
//! back to its default first. The value of each is the parameter's internal one -- 0..1 for a knob,
//! the raw number for a choice -- worked out from the seconds, hertz and cents the table below is
//! written in.
class WavetableSynthPresets
{
public:
    static const std::vector<SynthPreset> & presets();
};

} // namespace noteahead

#endif // WAVETABLE_SYNTH_PRESETS_HPP
