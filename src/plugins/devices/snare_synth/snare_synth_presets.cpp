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

#include "snare_synth_presets.hpp"

#include "../../../common/constants.hpp"

#include <algorithm>
#include <cmath>

namespace noteahead {

namespace {

//! A kit in the units the knobs show. Every field starts at the device's own default, so a preset
//! names only what it changes.
struct Kit
{
    double tuneHz { 200.0 };
    float tone { 0.65f };
    double decaySeconds { 0.25 };
    float headTension { 0.35f };
    double pitchDepthSemitones { 6.0 };
    double pitchDecaySeconds { 0.025 };
    float snappy { 0.6f };
    double snareDecaySeconds { 0.28 };
    float snareTension { 0.5f };
    float snareBuzz { 0.5f };
    float click { 0.4f };
    float hardness { 0.5f };
    float position { 0.2f };
    float body { 0.3f };
    float drive { 0.0f };
    double crack { 0.0 };
    float punch { 0.20f };
    float wireWidth { 0.35f };
    double lpfHz { 20000.0 };
    double hpfHz { 20.0 };
    //! Trims the preset to about Init's level, which the drum and the drive otherwise move
    double gainDb { 0.0 };
};

float position(double value, double min, double max)
{
    return static_cast<float>(std::clamp((value - min) / (max - min), 0.0, 1.0));
}

//! Where the filters' log sweep from 20 Hz to 20 kHz puts @p hz.
float cutoff(double hz)
{
    return static_cast<float>(std::clamp(std::log2(hz / 20.0) / std::log2(1000.0), 0.0, 1.0));
}

//! The knob positions SnareSynthDevice::syncParameters() maps back to @p kit.
std::map<std::string, float> parameters(const Kit & kit)
{
    namespace Xml = Constants::NahdXml;
    return {
        { Xml::xmlKeyTune().toStdString(), position(kit.tuneHz, 100.0, 450.0) },
        { Xml::xmlKeyTone().toStdString(), kit.tone },
        { Xml::xmlKeyDecay().toStdString(), position(kit.decaySeconds, 0.02, 1.5) },
        { Xml::xmlKeyHeadTension().toStdString(), kit.headTension },
        { Xml::xmlKeyPitchDepth().toStdString(), position(kit.pitchDepthSemitones, 0.0, 24.0) },
        { Xml::xmlKeyPitchDecay().toStdString(), position(kit.pitchDecaySeconds, 0.005, 0.1) },
        { Xml::xmlKeySnappy().toStdString(), kit.snappy },
        { Xml::xmlKeySnareDecay().toStdString(), position(kit.snareDecaySeconds, 0.03, 1.2) },
        { Xml::xmlKeySnareTension().toStdString(), kit.snareTension },
        { Xml::xmlKeySnareBuzz().toStdString(), kit.snareBuzz },
        { Xml::xmlKeyClick().toStdString(), kit.click },
        { Xml::xmlKeyHardness().toStdString(), kit.hardness },
        { Xml::xmlKeyPosition().toStdString(), kit.position },
        { Xml::xmlKeyBody().toStdString(), kit.body },
        { Xml::xmlKeyDrive().toStdString(), kit.drive },
        { Xml::xmlKeyCrack().toStdString(), position(kit.crack, 0.0, 1.5) },
        { Xml::xmlKeyPunch().toStdString(), kit.punch },
        { Xml::xmlKeyWireWidth().toStdString(), kit.wireWidth },
        { Xml::xmlKeyLpfCutoff().toStdString(), cutoff(kit.lpfHz) },
        { Xml::xmlKeyHpfCutoff().toStdString(), cutoff(kit.hpfHz) },
        { Xml::xmlKeyGain().toStdString(), position(kit.gainDb, -30.0, 30.0) },
    };
}

} // namespace

const std::vector<SynthPreset> & SnareSynthPresets::presets()
{
    // Init first, as the way back; the rest alphabetical
    static const std::vector<SynthPreset> presets {
        { .name = "Init", .parameters = {} },
        // The drum machine's: high, short and thin, mostly noise
        { .name = "808 Snare", .parameters = parameters({ .tuneHz = 240.0, .tone = 0.35f, .decaySeconds = 0.18, .headTension = 0.2f, .pitchDepthSemitones = 4.0, .pitchDecaySeconds = 0.015, .snappy = 0.7f, .snareDecaySeconds = 0.2, .snareTension = 0.3f, .snareBuzz = 0.2f, .click = 0.2f, .hardness = 0.4f, .position = 0.3f, .body = 0.2f, .punch = 0.0f, .wireWidth = 0.0f }) },
        // Low and loose, hit hard, a little driven
        { .name = "Deep Rock", .parameters = parameters({ .tuneHz = 150.0, .tone = 0.45f, .decaySeconds = 0.5, .headTension = 0.25f, .snareDecaySeconds = 0.45, .snareBuzz = 0.55f, .click = 0.5f, .hardness = 0.65f, .body = 0.7f, .drive = 0.3f, .gainDb = -2.0 }) },
        // The everyday backbeat, with more body and a longer ring
        { .name = "Fat Backbeat", .parameters = parameters({ .tuneHz = 170.0, .tone = 0.4f, .decaySeconds = 0.35, .headTension = 0.3f, .pitchDepthSemitones = 5.0, .snappy = 0.65f, .snareDecaySeconds = 0.4, .snareBuzz = 0.6f, .click = 0.45f, .hardness = 0.55f, .body = 0.6f, .drive = 0.2f, .gainDb = -1.5 }) },
        // Big and boomy with the tail cut short, as if through a gated room
        { .name = "Gated 80s", .parameters = parameters({ .tuneHz = 190.0, .tone = 0.5f, .decaySeconds = 0.3, .pitchDepthSemitones = 3.0, .snappy = 0.7f, .snareDecaySeconds = 0.3, .snareBuzz = 0.6f, .click = 0.5f, .hardness = 0.6f, .body = 0.8f, .drive = 0.25f, .gainDb = -2.5 }) },
        // Dull, crunchy and band-limited, like a sampled break
        { .name = "Lo-Fi", .parameters = parameters({ .tuneHz = 220.0, .tone = 0.3f, .decaySeconds = 0.2, .snappy = 0.55f, .snareDecaySeconds = 0.25, .snareBuzz = 0.7f, .drive = 0.5f, .punch = 0.0f, .wireWidth = 0.0f, .lpfHz = 4500.0, .hpfHz = 200.0, .gainDb = 6.0 }) },
        // Small, high and cracked tight
        { .name = "Piccolo", .parameters = parameters({ .tuneHz = 400.0, .tone = 0.65f, .decaySeconds = 0.15, .headTension = 0.8f, .pitchDepthSemitones = 2.0, .snappy = 0.8f, .snareDecaySeconds = 0.2, .snareTension = 0.85f, .snareBuzz = 0.45f, .click = 0.6f, .hardness = 0.75f, .body = 0.1f, .punch = 0.0f, .wireWidth = 0.0f, .hpfHz = 150.0 }) },
        // Struck at the edge, hard: all crack and little wire
        { .name = "Rimshot", .parameters = parameters({ .tuneHz = 330.0, .tone = 0.5f, .decaySeconds = 0.12, .headTension = 0.7f, .snappy = 0.35f, .snareDecaySeconds = 0.15, .click = 0.9f, .hardness = 0.9f, .position = 0.9f, .body = 0.15f, .gainDb = -9.0 }) },
        // The rock presets are fitted to the loudest hits of real snares, close and overhead mics
        // mixed, from Karoryfer's Frankensnare (CC0)
        //
        // A deep 14x8 aluminium snare, hit hard
        { .name = "Rock Aluminium", .parameters = parameters({ .tuneHz = 122.0, .tone = 0.32f, .decaySeconds = 0.497, .headTension = 0.67f, .pitchDepthSemitones = 4.7, .pitchDecaySeconds = 0.07, .snappy = 0.45f, .snareDecaySeconds = 0.603, .snareTension = 0.2f, .snareBuzz = 0.71f, .click = 0.0f, .hardness = 0.9f, .position = 0.51f, .body = 0.0f, .drive = 0.6f, .crack = 1.14, .punch = 0.09f, .wireWidth = 0.53f, .gainDb = -4.5 }) },
        // A 14x6.5 maple snare: higher and woodier
        { .name = "Rock Maple", .parameters = parameters({ .tuneHz = 244.0, .tone = 0.65f, .decaySeconds = 0.682, .headTension = 0.36f, .pitchDepthSemitones = 7.5, .pitchDecaySeconds = 0.058, .snappy = 0.51f, .snareDecaySeconds = 0.43, .snareTension = 0.2f, .snareBuzz = 0.81f, .click = 0.0f, .hardness = 0.2f, .position = 0.14f, .body = 0.13f, .drive = 0.32f, .crack = 1.5, .punch = 0.64f, .wireWidth = 0.54f, .gainDb = -3.0 }) },
        // The 14x8 aluminium struck rim and head at once
        { .name = "Rock Rimshot", .parameters = parameters({ .tuneHz = 191.5, .tone = 0.51f, .decaySeconds = 0.333, .headTension = 0.56f, .pitchDepthSemitones = 3.6, .pitchDecaySeconds = 0.014, .snappy = 0.64f, .snareDecaySeconds = 0.388, .snareTension = 0.25f, .snareBuzz = 0.81f, .click = 0.33f, .hardness = 0.33f, .position = 0.74f, .body = 0.34f, .drive = 0.42f, .crack = 1.5, .punch = 0.47f, .wireWidth = 0.43f, .gainDb = -6.0 }) },
        // A big 16x7 sapele snare: low, loose and long
        { .name = "Rock Sapele", .parameters = parameters({ .tuneHz = 143.0, .tone = 0.3f, .decaySeconds = 0.656, .headTension = 0.56f, .pitchDepthSemitones = 1.5, .pitchDecaySeconds = 0.049, .snappy = 0.72f, .snareDecaySeconds = 0.557, .snareTension = 0.2f, .snareBuzz = 0.32f, .click = 0.6f, .hardness = 0.9f, .position = 0.13f, .body = 0.0f, .drive = 0.56f, .crack = 1.5, .punch = 0.2f, .wireWidth = 0.75f, .gainDb = -3.0 }) },
        // The wires thrown off: the head alone, close to a tom
        { .name = "Snares Off", .parameters = parameters({ .tuneHz = 180.0, .tone = 0.5f, .decaySeconds = 0.4, .pitchDepthSemitones = 8.0, .pitchDecaySeconds = 0.04, .snappy = 0.05f, .body = 0.6f, .punch = 0.0f, .wireWidth = 0.0f, .gainDb = -1.0 }) },
        // Tuned up and damped, the wires tight and quick
        { .name = "Tight Funk", .parameters = parameters({ .tuneHz = 290.0, .tone = 0.5f, .decaySeconds = 0.14, .headTension = 0.6f, .pitchDepthSemitones = 3.0, .snappy = 0.75f, .snareDecaySeconds = 0.16, .snareTension = 0.75f, .snareBuzz = 0.4f, .click = 0.55f, .hardness = 0.7f, .position = 0.3f, .body = 0.25f, .gainDb = -6.0 }) },
        // Bright and snappy with a pitch drop, the lows cleaned out
        { .name = "Trap Snare", .parameters = parameters({ .tuneHz = 260.0, .tone = 0.55f, .decaySeconds = 0.22, .pitchDepthSemitones = 7.0, .pitchDecaySeconds = 0.02, .snappy = 0.8f, .snareDecaySeconds = 0.3, .click = 0.6f, .hardness = 0.7f, .drive = 0.15f, .hpfHz = 120.0, .gainDb = -3.5 }) },
    };
    return presets;
}

} // namespace noteahead
