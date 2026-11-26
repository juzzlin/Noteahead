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

#include "true_peak_limiter_presets.hpp"

#include "../../../common/constants.hpp"
#include "true_peak_limiter.hpp"

#include <utility>

namespace noteahead {

namespace {

struct PresetValues
{
    double thresholdDb;
    double ceilingDb;
    double releaseMs;
    double lookaheadMs;
    bool boost;
    bool autoRelease;
};

EffectPreset preset(std::string name, const PresetValues & values)
{
    namespace C = Constants::NahdXml;
    return EffectPreset {
        std::move(name),
        {
          { C::xmlKeyThreshold().toStdString(), TruePeakLimiter::thresholdPosition(values.thresholdDb) },
          { C::xmlKeyCeiling().toStdString(), TruePeakLimiter::ceilingPosition(values.ceilingDb) },
          { C::xmlKeyRelease().toStdString(), TruePeakLimiter::releasePosition(values.releaseMs) },
          { C::xmlKeyLookahead().toStdString(), TruePeakLimiter::lookaheadPosition(values.lookaheadMs) },
          { C::xmlKeyBoost().toStdString(), values.boost ? 1.0f : 0.0f },
          { C::xmlKeyAutoRelease().toStdString(), values.autoRelease ? 1.0f : 0.0f },
        }
    };
}

} // namespace

const EffectPresetList & TruePeakLimiterPresets::presets()
{
    static const EffectPresetList presets {
        { "Init", {} },
        preset("Brickwall Safety", { 0.0, -1.0, 300.0, 10.0, false, true }),
        preset("EDM Master Loud", { -12.0, -1.2, 80.0, 4.0, true, true }),
        preset("Master Gentle", { -6.0, -1.0, 200.0, 5.0, true, true }),
        preset("Master Moderate", { -10.0, -1.0, 150.0, 5.0, true, true }),
        preset("Streaming Safe", { -6.0, -1.2, 120.0, 5.0, true, true }),
        preset("Transparent Peak Catch", { -1.0, -0.3, 500.0, 10.0, true, false }),
    };
    return presets;
}

} // namespace noteahead
