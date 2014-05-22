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

#include "air_band_eq_presets.hpp"

#include "../../common/constants.hpp"
#include "air_band_eq.hpp"

#include <array>

namespace noteahead {

namespace {

namespace C = Constants::NahdXml;

//! The AIR BAND selector's positions, as the panel names them.
enum class Air
{
    Off,
    At2500Hz,
    At5kHz,
    At10kHz,
    At20kHz,
    At40kHz
};

//! A patch written in panel units, as the knobs read: the band passes -5..+5 from Sub up to
//! 2.5 kHz, and AIR GAIN 0..5. The output trim stays at 0 dB, and auto gain is the user's to choose.
EffectPreset preset(std::string name, const std::array<double, AirBandEq::BandCount> & bands, Air air, double airGain)
{
    EffectPreset result { std::move(name), {} };
    for (size_t i = 0; i < bands.size(); i++) {
        result.parameters[C::xmlKeyBandGain(i).toStdString()] = static_cast<float>((bands.at(i) + 5.0) / 10.0);
    }
    result.parameters[C::xmlKeyAirFreq().toStdString()] = static_cast<float>(air);
    result.parameters[C::xmlKeyAirGain().toStdString()] = static_cast<float>(airGain / 5.0);
    return result;
}

} // namespace

const EffectPresetList & AirBandEqPresets::presets()
{
    // The moves the unit is known for, all of them a couple of panel steps at most: the band passes
    // add three decibels a step, and the overlap between neighbours makes a move broader than its
    // knob suggests. Flat comes first because it is the way back; the rest are alphabetical.
    static const EffectPresetList presets {
        preset("Flat", { 0.0, 0.0, 0.0, 0.0, 0.0 }, Air::Off, 0.0),

        // The top opened from far above the audible range, which brightens without a hard edge.
        preset("Air Sheen", { 0.0, 0.0, 0.0, 0.0, 0.0 }, Air::At40kHz, 3.0),

        // The upper mids for diction and the air band for breath.
        preset("Bright Vocal", { 0.0, 0.0, -0.5, 0.0, 1.5 }, Air::At10kHz, 2.0),

        // The thump of a kick at 40 Hz and its knock at 2.5 kHz, the boxiness between pulled back.
        preset("Kick Punch", { 0.0, 2.0, 0.0, -1.0, 1.0 }, Air::Off, 0.0),

        // Weight under a mix, kept off the low mids where it would turn to mud.
        preset("Low End Weight", { 1.0, 1.5, 0.0, 0.0, 0.0 }, Air::Off, 0.0),

        // The gentle all-round lift the unit is put on a master bus for.
        preset("Master Open", { 0.5, 0.5, 0.0, 0.0, 0.5 }, Air::At20kHz, 1.5),

        // The low mids taken down where a dense mix clouds over.
        preset("Mud Cut", { 0.0, 0.0, -2.0, -1.0, 0.0 }, Air::Off, 0.0),

        // Body for a thin source: the low mids filled out and a touch of top kept.
        preset("Thicken", { 0.0, 0.5, 1.5, 0.5, 0.0 }, Air::At20kHz, 1.0),
    };
    return presets;
}

} // namespace noteahead
