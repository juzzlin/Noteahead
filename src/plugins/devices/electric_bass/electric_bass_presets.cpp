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

#include "electric_bass_presets.hpp"

#include "../../../common/constants.hpp"
#include "electric_bass_device.hpp"

#include <algorithm>
#include <cmath>

namespace noteahead {

namespace {

//! A bass in the units the strings take. Every field starts at the device's own default, so a
//! preset names only what it changes.
struct Bass
{
    double pluckMm { 120.0 };
    double fingerMm { 10.8 };
    double sustainSeconds { 23.0 };
    double lossPerHz { 0.0021 };
    double releaseSeconds { 0.12 };
    float neck { 1.0f };
    float bridge { 1.0f };
    float tone { 0.9f };
    //! Trims the preset to about Init's level, which the pickups and the tone otherwise move
    double gainDb { 0.0 };
};

float position(double value, double min, double max)
{
    return static_cast<float>(std::clamp((value - min) / (max - min), 0.0, 1.0));
}

float exponential(double value, double min, double max)
{
    return static_cast<float>(std::clamp(std::log(value / min) / std::log(max / min), 0.0, 1.0));
}

//! The knob positions ElectricBassDevice::syncParameters() maps back to @p bass.
std::map<std::string, float> parameters(const Bass & bass)
{
    namespace Xml = Constants::NahdXml;
    using Device = ElectricBassDevice;
    return {
        { Xml::xmlKeyPosition().toStdString(), exponential(bass.pluckMm, Device::MinPluckMm, Device::MaxPluckMm) },
        { Xml::xmlKeyHardness().toStdString(), static_cast<float>(std::clamp(std::log(bass.fingerMm / Device::SoftFingerMm) / std::log(Device::HardFingerMm / Device::SoftFingerMm), 0.0, 1.0)) },
        { Xml::xmlKeySustain().toStdString(), exponential(bass.sustainSeconds, Device::MinSustainSeconds, Device::MaxSustainSeconds) },
        { Xml::xmlKeyBrightness().toStdString(), static_cast<float>(std::clamp(std::log(bass.lossPerHz / Device::DullLossPerHz) / std::log(Device::BrightLossPerHz / Device::DullLossPerHz), 0.0, 1.0)) },
        { Xml::xmlKeyRelease().toStdString(), exponential(bass.releaseSeconds, Device::MinReleaseSeconds, Device::MaxReleaseSeconds) },
        { Xml::xmlKeyNeckPickup().toStdString(), bass.neck },
        { Xml::xmlKeyBridgePickup().toStdString(), bass.bridge },
        { Xml::xmlKeyTone().toStdString(), bass.tone },
        { Xml::xmlKeyGain().toStdString(), position(bass.gainDb, -30.0, 30.0) },
    };
}

} // namespace

const std::vector<SynthPreset> & ElectricBassPresets::presets()
{
    // Init first, as the way back; the rest alphabetical. Dark Neck and Jazz Open are the two
    // recordings the model is fitted to, Karoryfer's black-and-blue-basses and growlybass (CC0)
    static const std::vector<SynthPreset> presets {
        { .name = "Init", .parameters = {} },
        // A 5-string on its neck pickup with the tone rolled back, plucked between the pickups
        { .name = "Dark Neck", .parameters = parameters({ .pluckMm = 141.0, .neck = 1.0f, .bridge = 0.08f, .tone = 0.26f, .gainDb = 3.5 }) },
        // Neck pickup, tone off, plucked far up the neck with the flesh of the finger: all low end
        { .name = "Dub", .parameters = parameters({ .pluckMm = 250.0, .fingerMm = 22.0, .sustainSeconds = 15.0, .lossPerHz = 0.005, .bridge = 0.0f, .tone = 0.1f, .gainDb = 6.0 }) },
        // Old flatwounds, plucked over the neck with the flesh of the finger: the thump of the sixties
        { .name = "Flatwound", .parameters = parameters({ .pluckMm = 220.0, .fingerMm = 20.0, .sustainSeconds = 8.0, .lossPerHz = 0.007, .releaseSeconds = 0.06, .bridge = 0.0f, .tone = 0.4f, .gainDb = 5.2 }) },
        // The bridge pickup up, plucked hard right by it: the growl of a fretless-era solo
        { .name = "Jazz Bridge", .parameters = parameters({ .pluckMm = 70.0, .fingerMm = 6.0, .neck = 0.25f, .bridge = 1.0f, .tone = 1.0f, .gainDb = -1.3 }) },
        // The neck pickup alone, plucked over it: round and deep
        { .name = "Jazz Neck", .parameters = parameters({ .pluckMm = 160.0, .neck = 1.0f, .bridge = 0.0f, .tone = 0.8f, .gainDb = 2.2 }) },
        // A Jazz Bass with everything on 10
        { .name = "Jazz Open", .parameters = parameters({ .pluckMm = 91.0, .neck = 0.75f, .tone = 1.0f, .gainDb = 1.3 }) },
        // The edge of the hand on the strings by the bridge: short and dull, for staccato lines
        { .name = "Muted", .parameters = parameters({ .pluckMm = 100.0, .fingerMm = 15.0, .sustainSeconds = 2.0, .lossPerHz = 0.008, .releaseSeconds = 0.04, .tone = 0.6f, .gainDb = 1.8 }) },
        // A P-Bass: one split pickup between the Jazz's two, its resonance lower, plucked over it
        { .name = "P-Bass", .parameters = parameters({ .pluckMm = 125.0, .neck = 1.0f, .bridge = 0.15f, .tone = 0.75f, .gainDb = 2.4 }) },
        // A P-Bass with old flatwounds and the tone half down: the round, short note of soul records
        { .name = "P-Bass Flats", .parameters = parameters({ .pluckMm = 130.0, .fingerMm = 14.0, .sustainSeconds = 10.0, .lossPerHz = 0.006, .releaseSeconds = 0.08, .neck = 1.0f, .bridge = 0.15f, .tone = 0.55f, .gainDb = 3.9 }) },
        // A P-Bass played hard near the bridge, the hardest finger standing in for a pick
        { .name = "P-Bass Pick", .parameters = parameters({ .pluckMm = 90.0, .fingerMm = 3.0, .lossPerHz = 0.0018, .releaseSeconds = 0.08, .neck = 1.0f, .bridge = 0.15f, .tone = 0.85f, .gainDb = -2.7 }) },
        // New roundwounds, a hard fingertip and the tone open: bright and long
        { .name = "Roundwound", .parameters = parameters({ .pluckMm = 100.0, .fingerMm = 5.0, .sustainSeconds = 30.0, .lossPerHz = 0.0012, .tone = 1.0f, .gainDb = -4.3 }) },
        // The thumb over the end of the neck: soft and deep, with both pickups
        { .name = "Thumb", .parameters = parameters({ .pluckMm = 200.0, .fingerMm = 25.0, .neck = 1.0f, .bridge = 0.5f, .tone = 0.7f, .gainDb = 4.8 }) },
    };
    return presets;
}

} // namespace noteahead
