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

#include "eq_32_band_graphical_presets.hpp"

#include "../../common/constants.hpp"
#include "eq_32_band_graphical.hpp"

#include <cmath>
#include <map>

namespace noteahead {

namespace {

//! The band labelled @p hz on the panel. Exact for the nominal values, which are what the
//! presets below are written in.
size_t band(double hz)
{
    return static_cast<size_t>(std::lround(3.0 * std::log2(hz / 1000.0))) + 18;
}

//! A patch written as the slider moves it makes, in decibels by nominal band frequency. Every slider
//! it does not name is left flat, which applying a preset does anyway.
EffectPreset preset(std::string name, const std::map<double, double> & gainsByHz)
{
    EffectPreset result { std::move(name), {} };
    for (const auto & [hz, db] : gainsByHz) {
        result.parameters[Constants::NahdXml::xmlKeyBandGain(band(hz)).toStdString()] = Eq32BandGraphical::sliderParameterValue(db);
    }
    return result;
}

} // namespace

const EffectPresetList & Eq32BandGraphicalPresets::presets()
{
    // Mastering moves, so all of them broad and gentle: a few decibels spread over several bands,
    // which is the scale at which a change to a whole mix stays a matter of taste rather than a
    // repair. Flat comes first because it is the way back; the rest are alphabetical.
    static const EffectPresetList presets {
        preset("Flat", {}),

        // A shelf-like lift of the very top, where a mix gets its sense of space.
        preset("Air Lift", { { 8000.0, 0.5 }, { 10000.0, 1.0 }, { 12500.0, 2.0 }, { 16000.0, 3.0 }, { 20000.0, 3.0 } }),

        // Sub rumble out, the punch of the kick and bass up, and the boom above it trimmed back.
        preset("Bass Tighten", { { 16.0, -3.0 }, { 20.0, -2.0 }, { 25.0, -1.0 }, { 63.0, 1.0 }, { 80.0, 1.5 }, { 100.0, 1.0 }, { 200.0, -1.0 }, { 250.0, -1.5 }, { 315.0, -1.0 } }),

        // The low mids where a dense mix turns cloudy.
        preset("De-Mud", { { 200.0, -1.0 }, { 250.0, -2.0 }, { 315.0, -3.0 }, { 400.0, -2.0 }, { 500.0, -1.0 } }),

        // The loudness-curve smile: lows and highs up, the middle left where it is.
        preset("Gentle Smile", { { 31.5, 1.0 }, { 40.0, 2.0 }, { 50.0, 2.0 }, { 63.0, 1.5 }, { 80.0, 1.0 }, { 6300.0, 1.0 }, { 8000.0, 1.5 }, { 10000.0, 2.0 }, { 12500.0, 2.0 }, { 16000.0, 1.5 } }),

        // The upper mids the ear is most sensitive to, where a loud master starts to hurt.
        preset("Harshness Tame", { { 2000.0, -1.0 }, { 2500.0, -2.0 }, { 3150.0, -2.5 }, { 4000.0, -2.0 }, { 5000.0, -1.0 } }),

        // Below anything a speaker plays, where energy only costs headroom.
        preset("Rumble Cut", { { 16.0, -12.0 }, { 20.0, -9.0 }, { 25.0, -6.0 }, { 31.5, -3.0 } }),

        // A vocal brought forward without reaching for its level, and the boxiness under it cleared.
        preset("Vocal Presence", { { 250.0, -1.0 }, { 1600.0, 1.0 }, { 2000.0, 2.0 }, { 2500.0, 2.5 }, { 3150.0, 2.0 }, { 4000.0, 1.0 } }),

        // The low mids filled out and the top rolled off, for a mix that came out bright and thin.
        preset("Warm Master", { { 80.0, 1.0 }, { 100.0, 1.5 }, { 125.0, 1.5 }, { 160.0, 1.0 }, { 200.0, 0.5 }, { 8000.0, -0.5 }, { 10000.0, -1.0 }, { 12500.0, -1.5 }, { 16000.0, -2.0 }, { 20000.0, -2.0 } }),
    };
    return presets;
}

} // namespace noteahead
