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

#include "kick_909_presets.hpp"

#include "../../../common/constants.hpp"
#include "kick_909_device.hpp"

#include <algorithm>
#include <cmath>

namespace noteahead {

namespace {

//! A kick in panel units, the knobs 0..10 as on the RD-9. Every field starts at the device's
//! default, so a preset names only what it changes.
struct Kick
{
    double tune { 5.0 };
    double attack { 5.0 };
    double decay { 5.0 };
    double pitch { 5.0 };
    double pitchDepth { 5.0 };
    double drive { 0.0 };
    double lpfHz { 20000.0 };
    double hpfHz { 20.0 };
    bool chromatic { false };
    //! Trims the preset to about Init's peak, which the drive and the decay otherwise move
    double gainDb { 0.0 };
};

float panel(double value)
{
    return static_cast<float>(std::clamp(value / 10.0, 0.0, 1.0));
}

float position(double value, double min, double max)
{
    return static_cast<float>(std::clamp((value - min) / (max - min), 0.0, 1.0));
}

//! Where the filters' log sweep from 20 Hz to 20 kHz puts @p hz
float cutoff(double hz)
{
    return static_cast<float>(std::clamp(std::log2(hz / 20.0) / std::log2(1000.0), 0.0, 1.0));
}

std::map<std::string, float> parameters(const Kick & kick)
{
    namespace Xml = Constants::NahdXml;
    return {
        { Xml::xmlKeyTune().toStdString(), panel(kick.tune) },
        { Xml::xmlKeyAttack().toStdString(), panel(kick.attack) },
        { Xml::xmlKeyDecay().toStdString(), panel(kick.decay) },
        { Xml::xmlKeyPitch().toStdString(), panel(kick.pitch) },
        { Xml::xmlKeyPitchDepth().toStdString(), panel(kick.pitchDepth) },
        { Xml::xmlKeyDrive().toStdString(), panel(kick.drive) },
        { Xml::xmlKeyLpfCutoff().toStdString(), cutoff(kick.lpfHz) },
        { Xml::xmlKeyHpfCutoff().toStdString(), cutoff(kick.hpfHz) },
        { Xml::xmlKeyChromaticMode().toStdString(), kick.chromatic ? 1.0f : 0.0f },
        { Xml::xmlKeyGain().toStdString(), position(kick.gainDb, -30.0, 30.0) },
    };
}

} // namespace

const std::vector<SynthPreset> & Kick909Presets::presets()
{
    // Init first, as the way back, with every knob at noon as an RD-9; the rest alphabetical. The two
    // named for the 909 sit at its own pitch, a little under the RD-9's
    static const std::vector<SynthPreset> presets {
        { .name = "Init", .parameters = {} },
        // Short and clicky, the sweep over in a flash: the tightest the 909 goes
        { .name = "Bare 909", .parameters = parameters({ .tune = 0.0, .attack = 10.0, .decay = 0.0, .pitch = 3.5 }) },
        // The long one: Decay up, Tune a little, Attack most of the way
        { .name = "Classic Long", .parameters = parameters({ .tune = 3.0, .attack = 7.0, .decay = 10.0, .pitch = 3.5 }) },
        // Everything but the click taken away: a tick for layering over another kick
        { .name = "Click Layer", .parameters = parameters({ .tune = 0.0, .attack = 10.0, .decay = 0.0, .hpfHz = 150.0, .gainDb = 3.9 }) },
        // Low, round and long under a low-pass: the house kick
        { .name = "Deep House", .parameters = parameters({ .tune = 2.0, .attack = 3.0, .decay = 8.0, .pitch = 3.4, .lpfHz = 3000.0, .gainDb = 0.5 }) },
        // A long sweep from high up to far below the drum, driven flat: the zap of the electro kick
        { .name = "Electro Zap", .parameters = parameters({ .tune = 10.0, .attack = 5.0, .decay = 4.0, .pitch = 6.0, .pitchDepth = 3.0, .drive = 3.0, .gainDb = 1.2 }) },
        // Tuned up, swept hard and clipped into a square: the gabber kick
        { .name = "Gabber", .parameters = parameters({ .tune = 10.0, .attack = 10.0, .decay = 8.0, .pitch = 6.5, .pitchDepth = 6.0, .drive = 10.0, .hpfHz = 35.0, .gainDb = -3.3 }) },
        // Driven, the sweep longer, the sub cut a little: the four-to-the-floor techno kick
        { .name = "Hard Techno", .parameters = parameters({ .tune = 6.0, .attack = 10.0, .decay = 6.0, .pitch = 4.0, .drive = 7.0, .hpfHz = 30.0, .gainDb = -2.8 }) },
        // Low and long, its end raised close to its start: plays the bass line in Chromatic mode
        { .name = "Tuned Sub", .parameters = parameters({ .tune = 2.0, .attack = 2.0, .decay = 10.0, .pitch = 1.0, .pitchDepth = 10.0, .chromatic = true, .gainDb = 0.5 }) },
    };
    return presets;
}

} // namespace noteahead
