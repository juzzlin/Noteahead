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
    double targetLufs;
    double ceilingDb;
    bool autoLoudness;
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
          { C::xmlKeyTargetLufs().toStdString(), MagicMaster::targetLufsPosition(values.targetLufs) },
          { C::xmlKeyCeiling().toStdString(), MagicMaster::ceilingPosition(values.ceilingDb) },
          { C::xmlKeyAutoLoudnessEnabled().toStdString(), values.autoLoudness ? 1.0f : 0.0f },
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
    static const EffectPresetList presets {
        { "Init", {} },
        preset("Club Warm", { -10.5, -1.2, true, true, true, true, true, true, true }),
        preset("EDM Standard", { -9.0, -1.2, true, true, true, true, true, true, true }),
        preset("Festival Loud", { -7.0, -1.0, true, true, true, true, true, true, true }),
        preset("Streaming", { -14.0, -1.2, true, true, true, true, true, true, true }),
        preset("Transparent", { -12.0, -1.2, true, true, true, true, true, false, true }),
    };
    return presets;
}

} // namespace noteahead
