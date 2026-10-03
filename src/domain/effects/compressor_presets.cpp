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

#include "compressor_presets.hpp"

#include "../../common/constants.hpp"
#include "../../common/parameter_mapper.hpp"

#include <algorithm>

namespace noteahead {

namespace {

//! A preset in the units it is reasoned about, rather than in the normalized 0-1 the parameters
//! store. preset() does the conversion, so the table below stays readable and reviewable.
struct PresetValues
{
    float thresholdDb;
    float ratio;
    float attackMs;
    float releaseMs;
    float kneeDb;
    float makeupDb;
    float lookaheadMs;
    bool rms;
};

//! The inverses of the mappings in Compressor::syncParameters(). The side chain is left out: its
//! source is the user's patching, and its filter is not part of the sound a preset describes.
EffectPreset preset(std::string name, const PresetValues & values)
{
    namespace C = Constants::NahdXml;
    return EffectPreset { std::move(name),
                          { { C::xmlKeyThreshold().toStdString(), (values.thresholdDb + 60.0f) / 60.0f },
                            { C::xmlKeyRatio().toStdString(), (values.ratio - 1.0f) / 19.0f },
                            { C::xmlKeyAttack().toStdString(), static_cast<float>(ParameterMapper::unmapExponential(values.attackMs, 0.1, 500.0)) },
                            { C::xmlKeyRelease().toStdString(), static_cast<float>(ParameterMapper::unmapExponential(values.releaseMs, 1.0, 2000.0)) },
                            { C::xmlKeyKnee().toStdString(), values.kneeDb / 24.0f },
                            { C::xmlKeyMakeup().toStdString(), (values.makeupDb + 12.0f) / 24.0f },
                            { C::xmlKeyLookahead().toStdString(), values.lookaheadMs / 10.0f },
                            { C::xmlKeyMode().toStdString(), values.rms ? 1.0f : 0.0f } } };
}

} // namespace

const EffectPresetList & CompressorPresets::presets()
{
    // Starting points calibrated against Noteahead's own signal levels rather than ported from
    // elsewhere: its devices run quiet and span some 20 dB between them, so thresholds taken from
    // general practice either do nothing or clamp down hard. Each is set against the material it
    // is named for.
    static const EffectPresetList presets {
        // Mix-bus glue. The release is short for a bus on purpose: at 140 BPM a four-on-the-floor
        // kick lands every 428 ms, and a release of a few hundred milliseconds is still recovering
        // when the next one arrives, which takes the front off it.
        preset("Glue", { -22.0f, 2.0f, 30.0f, 120.0f, 6.0f, 2.0f, 0.0f, true }),
        preset("Drum Bus", { -20.0f, 4.0f, 10.0f, 100.0f, 3.0f, 3.0f, 0.0f, false }),
        // The slow attack is the point: it lets the stick through before the body is caught.
        preset("Punchy Drums", { -22.0f, 4.0f, 30.0f, 80.0f, 0.0f, 4.0f, 0.0f, false }),
        preset("Piano", { -22.0f, 3.0f, 20.0f, 150.0f, 6.0f, 3.0f, 0.0f, true }),
        preset("Vocal", { -20.0f, 3.0f, 5.0f, 150.0f, 6.0f, 4.0f, 0.0f, true }),
        // Threshold well above the others: the bass devices run some 10 dB hotter than the rest.
        preset("Bass", { -16.0f, 4.0f, 15.0f, 100.0f, 3.0f, 4.0f, 0.0f, false }),
        preset("Pump", { -24.0f, 8.0f, 0.5f, 180.0f, 0.0f, 5.0f, 0.0f, false }),
        // Lookahead so the ceiling holds on transients the attack alone would miss.
        preset("Brickwall", { -6.0f, 20.0f, 0.1f, 50.0f, 0.0f, 0.0f, 5.0f, false })
    };
    return presets;
}

} // namespace noteahead
