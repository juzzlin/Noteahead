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

#ifndef BASS_SYNTH_PRESETS_HPP
#define BASS_SYNTH_PRESETS_HPP

#include "synth_presets.hpp"

#include <vector>

namespace noteahead {

//! Factory patches for the bass synth.
//!
//! A preset names only the parameters that differ from the init patch: loading one puts everything
//! back to its default first, so nothing left unsaid can carry over from whatever was loaded before.
//!
//! SynthPreset is reused rather than duplicated, as the FM synth's list reuses it: a name and a map
//! of parameter values is as true here as it is there.
class BassSynthPresets
{
public:
    static const std::vector<SynthPreset> & presets();
};

} // namespace noteahead

#endif // BASS_SYNTH_PRESETS_HPP
