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

#ifndef EFFECT_PRESETS_HPP
#define EFFECT_PRESETS_HPP

#include <map>
#include <string>
#include <vector>

namespace noteahead {

//! A built-in patch: a name, and the parameter values that make it.
//!
//! Exactly what a preset the user saves is, which is the point: applying either one resets every
//! parameter to its default first, so an entry below only has to name what the patch is actually
//! about and can leave the rest of the panel out.
struct EffectPreset
{
    std::string name;
    std::map<std::string, float> parameters;
};

using EffectPresetList = std::vector<EffectPreset>;

} // namespace noteahead

#endif // EFFECT_PRESETS_HPP
