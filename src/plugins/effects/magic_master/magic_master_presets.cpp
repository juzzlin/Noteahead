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

#include "magic_master_presets.hpp"

#include "../../../common/constants.hpp"
#include "magic_master.hpp"

#include <utility>

namespace noteahead {

namespace {

struct PresetValues
{
    MagicMaster::EqProfile eqProfile;
    bool subClean;
    bool eq;
    bool multiband;
    bool stereo;
    bool clipper;
    bool limiter;
};

EffectPreset preset(std::string name, const PresetValues & values)
{
    namespace C = Constants::NahdXml;
    return EffectPreset {
        std::move(name),
        {
          { C::xmlKeyEqProfile().toStdString(), static_cast<float>(values.eqProfile) },
          { C::xmlKeySubCleanEnabled().toStdString(), values.subClean ? 1.0f : 0.0f },
          { C::xmlKeyEqEnabled().toStdString(), values.eq ? 1.0f : 0.0f },
          { C::xmlKeyMultibandEnabled().toStdString(), values.multiband ? 1.0f : 0.0f },
          { C::xmlKeyStereoEnabled().toStdString(), values.stereo ? 1.0f : 0.0f },
          { C::xmlKeyClipperEnabled().toStdString(), values.clipper ? 1.0f : 0.0f },
          { C::xmlKeyLimiterEnabled().toStdString(), values.limiter ? 1.0f : 0.0f },
        }
    };
}

} // namespace

const EffectPresetList & MagicMasterPresets::presets()
{
    using Profile = MagicMaster::EqProfile;

    // Init first, as the way back; the rest alphabetical. Threshold & Ceiling are exempt from presets
    // so switching mastering style preserves the project's gain staging.
    static const EffectPresetList presets {
        { "Init", {} },
        preset("Clean Peak Safety", { Profile::FlatPink, true, false, false, false, false, true }),
        preset("Clip & Limit Only", { Profile::FlatPink, false, false, false, false, true, true }),
        preset("Club Bass", { Profile::ClubBass, true, true, true, true, true, true }),
        preset("Club Warm", { Profile::HouseWarm, true, true, true, true, true, true }),
        preset("EDM Standard", { Profile::Edm, true, true, true, true, true, true }),
        preset("Festival Scoop", { Profile::Festival, true, true, true, true, true, true }),
        preset("Keep Mix Tone", { Profile::FlatPink, true, false, true, true, true, true }),
        preset("Mix Bus Polish", { Profile::Edm, true, true, true, true, false, false }),
        preset("Punchy Open", { Profile::TechnoPunch, true, true, false, true, true, true }),
        preset("Synthwave Retro", { Profile::Synthwave, true, true, true, true, true, true }),
        preset("Techno Raw", { Profile::TechnoPunch, true, true, true, false, true, true }),
        preset("Trance Bright", { Profile::TranceAir, true, true, true, true, true, true }),
        preset("Transparent", { Profile::FlatPink, true, true, true, true, false, true }),
        preset("Vinyl & Dynamic", { Profile::HouseWarm, true, true, false, true, false, true }),
    };
    return presets;
}

} // namespace noteahead
