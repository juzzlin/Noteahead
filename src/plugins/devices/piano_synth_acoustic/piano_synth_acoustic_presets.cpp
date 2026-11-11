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

#include "piano_synth_acoustic_presets.hpp"

#include "../../../common/constants.hpp"

#include <algorithm>
#include <map>
#include <string>

namespace noteahead {

namespace {

//! A piano as its knobs, 0..1. Every field starts at the device's own default, so a preset names
//! only what it changes.
struct Piano
{
    float hardness { 0.5f };
    float knock { 0.5f };
    float decay { 0.5f };
    float brightness { 0.5f };
    float inharmonicity { 0.5f };
    float detune { 0.3f };
    float release { 0.3f };
    float resonance { 0.5f };
    float panSpread { 0.6f };
    float stretch { 1.0f };
    //! Trims the preset to about Init's level, which the hammer otherwise moves
    double gainDb { 0.0 };
};

std::map<std::string, float> parameters(const Piano & piano)
{
    namespace Xml = Constants::NahdXml;
    return {
        { Xml::xmlKeyHardness().toStdString(), piano.hardness },
        { Xml::xmlKeyBody().toStdString(), piano.knock },
        { Xml::xmlKeyDecay().toStdString(), piano.decay },
        { Xml::xmlKeyBrightness().toStdString(), piano.brightness },
        { Xml::xmlKeyInharmonicity().toStdString(), piano.inharmonicity },
        { Xml::xmlKeyStringDetune().toStdString(), piano.detune },
        { Xml::xmlKeyReleaseTime().toStdString(), piano.release },
        { Xml::xmlKeyResonance().toStdString(), piano.resonance },
        { Xml::xmlKeyPanSpread().toStdString(), piano.panSpread },
        { Xml::xmlKeyStretch().toStdString(), piano.stretch },
        { Xml::xmlKeyGain().toStdString(), static_cast<float>(std::clamp((piano.gainDb + 30.0) / 60.0, 0.0, 1.0)) },
    };
}

} // namespace

const std::vector<SynthPreset> & PianoSynthAcousticPresets::presets()
{
    // Init first, as the way back; the rest alphabetical
    static const std::vector<SynthPreset> presets {
        { .name = "Init", .parameters = {} },
        // Harder hammers and livelier strings: a piano voiced to cut through a band
        { .name = "Bright", .parameters = parameters({ .hardness = 0.75f, .brightness = 0.65f, .gainDb = -1.2 }) },
        // The microphones in under the lid: more knock, the keyboard spread wider, a little more hammer
        { .name = "Close Mic", .parameters = parameters({ .hardness = 0.6f, .knock = 0.85f, .panSpread = 0.9f, .gainDb = -1.9 }) },
        // Tuned to equal temperament, to sit with the synthesizers
        { .name = "Equal Tempered", .parameters = parameters({ .stretch = 0.0f }) },
        // A strip of felt between the hammers and the strings, as an upright's practice rail lays
        // down: soft, short and intimate
        { .name = "Felt", .parameters = parameters({ .hardness = 0.08f, .knock = 0.25f, .decay = 0.3f, .brightness = 0.3f, .release = 0.45f, .resonance = 0.3f, .panSpread = 0.4f, .gainDb = 4.9 }) },
        // The unisons let go and the dampers worn: a bar-room upright
        { .name = "Honky Tonk", .parameters = parameters({ .hardness = 0.7f, .knock = 0.7f, .decay = 0.4f, .inharmonicity = 0.8f, .detune = 1.0f, .release = 0.55f, .gainDb = -1.6 }) },
        // Older, softer hammers: round, for ballads
        { .name = "Mellow", .parameters = parameters({ .hardness = 0.28f, .knock = 0.4f, .brightness = 0.4f, .resonance = 0.6f, .gainDb = 2.0 }) },
        // The keyboard spread wide across the stereo field, with the pedal's resonance up
        { .name = "Wide", .parameters = parameters({ .resonance = 0.75f, .panSpread = 1.0f }) },
    };
    return presets;
}

} // namespace noteahead
