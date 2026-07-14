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

#include "kick_synth_presets.hpp"

#include "../../../common/constants.hpp"
#include "kick_synth_device.hpp"

#include <algorithm>
#include <cmath>

namespace noteahead {

namespace {

//! A kick in the units the engine takes. Every field starts at the device's own default, so a
//! preset names only what it changes.
struct Kick
{
    double tuneHz { 55.0 };
    double decaySeconds { 1.5 };
    float tone { 0.4f };
    float pitchDrop { 0.6f };
    float damping { 0.6f };
    double ring { 0.306 };
    double ringDecaySeconds { 0.4 };
    float beater { 0.3f };
    float click { 0.3f };
    float position { 0.3f };
    float frontHead { 0.6f };
    double frontTune { 1.6 };
    float shell { 0.5f };
    float mic { 0.5f };
    float drive { 0.3f };
    double lpfHz { 20000.0 };
    double hpfHz { 20.0 };
    //! Trims the preset to about Init's peak, which the drum and the drive otherwise move
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

//! Where the filters' log sweep from 20 Hz to 20 kHz puts @p hz.
float cutoff(double hz)
{
    return static_cast<float>(std::clamp(std::log2(hz / 20.0) / std::log2(1000.0), 0.0, 1.0));
}

//! The knob positions KickSynthDevice::syncParameters() maps back to @p kick.
std::map<std::string, float> parameters(const Kick & kick)
{
    namespace Xml = Constants::NahdXml;
    using Device = KickSynthDevice;
    return {
        { Xml::xmlKeyTune().toStdString(), exponential(kick.tuneHz, Device::MinTuneHz, Device::MaxTuneHz) },
        { Xml::xmlKeyDecay().toStdString(), exponential(kick.decaySeconds, Device::MinDecaySeconds, Device::MaxDecaySeconds) },
        { Xml::xmlKeyTone().toStdString(), kick.tone },
        { Xml::xmlKeyPitchDrop().toStdString(), kick.pitchDrop },
        { Xml::xmlKeyDamping().toStdString(), kick.damping },
        { Xml::xmlKeyRing().toStdString(), static_cast<float>(std::sqrt(std::clamp(kick.ring / Device::MaxRing, 0.0, 1.0))) },
        { Xml::xmlKeyRingDecay().toStdString(), exponential(kick.ringDecaySeconds, Device::MinRingDecaySeconds, Device::MaxRingDecaySeconds) },
        { Xml::xmlKeyHardness().toStdString(), kick.beater },
        { Xml::xmlKeyClick().toStdString(), kick.click },
        { Xml::xmlKeyPosition().toStdString(), kick.position },
        { Xml::xmlKeyFrontHead().toStdString(), kick.frontHead },
        { Xml::xmlKeyFrontTune().toStdString(), exponential(kick.frontTune, Device::MinFrontTune, Device::MaxFrontTune) },
        { Xml::xmlKeyShell().toStdString(), kick.shell },
        { Xml::xmlKeyMicPosition().toStdString(), kick.mic },
        { Xml::xmlKeyDrive().toStdString(), kick.drive },
        { Xml::xmlKeyLpfCutoff().toStdString(), cutoff(kick.lpfHz) },
        { Xml::xmlKeyHpfCutoff().toStdString(), cutoff(kick.hpfHz) },
        { Xml::xmlKeyGain().toStdString(), position(kick.gainDb, -30.0, 30.0) },
    };
}

} // namespace

const std::vector<SynthPreset> & KickSynthPresets::presets()
{
    // Init first, as the way back; the rest alphabetical, so the techno kicks and the toms group
    // together. The presets named after a drum are fitted to the loudest hits of real drums from
    // Karoryfer's Big Rusty Drums and Unruly Drums and from Virtuosity Drums (all CC0)
    static const std::vector<SynthPreset> presets {
        { .name = "Init", .parameters = {} },
        // A tight kick through a dusty sampler: rounded off, a little crunchy
        { .name = "Boom Bap", .parameters = parameters({ .tuneHz = 58.0, .decaySeconds = 1.2, .tone = 0.35f, .pitchDrop = 0.5f, .damping = 0.85f, .ring = 0.1, .ringDecaySeconds = 0.2, .beater = 0.4f, .click = 0.4f, .position = 0.3f, .frontHead = 0.4f, .frontTune = 1.3, .shell = 0.6f, .mic = 0.6f, .drive = 0.65f, .lpfHz = 4500.0, .hpfHz = 30.0, .gainDb = -0.4 }) },
        // A 24" kick with a pillow in it, close mic: short, deep and punchy
        { .name = "Damped 24", .parameters = parameters({ .tuneHz = 37.4, .decaySeconds = 3.38, .tone = 0.95f, .pitchDrop = 0.96f, .damping = 0.97f, .ring = 0.3, .ringDecaySeconds = 1.2, .beater = 0.07f, .click = 0.24f, .position = 0.0f, .frontHead = 0.56f, .frontTune = 1.83, .shell = 0.82f, .mic = 0.1f, .drive = 0.12f, .gainDb = -2.8 }) },
        // A felt beater on a small kick tuned up, both heads open: the jazz kick
        { .name = "Felt Jazz", .parameters = parameters({ .tuneHz = 90.0, .decaySeconds = 2.5, .tone = 0.5f, .pitchDrop = 0.3f, .damping = 0.2f, .ring = 0.3, .ringDecaySeconds = 0.5, .beater = 0.1f, .click = 0.1f, .position = 0.2f, .frontHead = 0.9f, .frontTune = 1.2, .shell = 0.6f, .mic = 0.8f, .drive = 0.1f, .gainDb = -0.2 }) },
        // A small kick packed with a blanket: dry and quick, for busy patterns
        { .name = "Funk Tight", .parameters = parameters({ .tuneHz = 75.0, .decaySeconds = 0.8, .tone = 0.5f, .pitchDrop = 0.4f, .damping = 0.97f, .ring = 0.1, .ringDecaySeconds = 0.1, .beater = 0.5f, .click = 0.5f, .position = 0.35f, .frontHead = 0.2f, .frontTune = 1.5, .shell = 0.5f, .mic = 0.3f, .drive = 0.3f, .gainDb = -1.3 }) },
        // An open drum with the shell and the ring pushed and driven: metal and air
        { .name = "Industrial", .parameters = parameters({ .tuneHz = 40.0, .decaySeconds = 3.0, .tone = 0.8f, .pitchDrop = 0.9f, .damping = 0.5f, .ring = 1.5, .ringDecaySeconds = 0.6, .beater = 1.0f, .click = 0.9f, .position = 0.7f, .frontHead = 0.9f, .frontTune = 2.2, .shell = 1.0f, .mic = 0.2f, .drive = 1.0f, .gainDb = 1.3 }) },
        // A 20" kick from inside, by the beater: attack and the head's ring
        { .name = "Inside 20", .parameters = parameters({ .tuneHz = 40.7, .decaySeconds = 4.13, .tone = 0.86f, .pitchDrop = 0.0f, .damping = 0.94f, .ring = 1.33, .ringDecaySeconds = 0.92, .beater = 0.58f, .click = 0.46f, .position = 0.95f, .frontHead = 0.79f, .frontTune = 2.18, .shell = 0.77f, .mic = 0.0f, .drive = 0.06f, .gainDb = -13.1 }) },
        // The pillow kick with a hard beater: the click of a metal-era kick
        { .name = "Metal Beater", .parameters = parameters({ .tuneHz = 45.0, .decaySeconds = 3.0, .tone = 0.5f, .pitchDrop = 0.8f, .damping = 0.9f, .ring = 0.3, .ringDecaySeconds = 0.3, .beater = 0.95f, .click = 0.95f, .position = 0.4f, .frontHead = 0.7f, .frontTune = 2.0, .shell = 0.3f, .mic = 0.2f, .drive = 0.4f, .hpfHz = 30.0, .gainDb = -4.1 }) },
        // A 24" with nothing in it: the long, booming tail of an open drum
        { .name = "Open 24", .parameters = parameters({ .tuneHz = 34.5, .decaySeconds = 1.86, .tone = 0.95f, .pitchDrop = 1.0f, .damping = 0.65f, .ring = 0.66, .ringDecaySeconds = 1.2, .beater = 0.57f, .click = 0.08f, .position = 0.0f, .frontHead = 0.84f, .frontTune = 1.84, .shell = 0.86f, .mic = 0.32f, .drive = 0.07f, .gainDb = -5.2 }) },
        // The same 20" from outside the front head: rounder, with the boom in front
        { .name = "Outside 20", .parameters = parameters({ .tuneHz = 48.9, .decaySeconds = 2.59, .tone = 0.94f, .pitchDrop = 1.0f, .damping = 0.77f, .ring = 0.29, .ringDecaySeconds = 1.14, .beater = 0.1f, .click = 0.4f, .position = 0.04f, .frontHead = 0.51f, .frontTune = 2.28, .shell = 0.87f, .mic = 0.67f, .drive = 0.0f, .gainDb = -4.3 }) },
        // A 22" with a pillow and a plastic beater: the all-round rock kick
        { .name = "Rock 22", .parameters = parameters({ .tuneHz = 50.0, .decaySeconds = 2.5, .tone = 0.5f, .pitchDrop = 0.7f, .damping = 0.8f, .ring = 0.3, .ringDecaySeconds = 0.3, .beater = 0.6f, .click = 0.6f, .position = 0.35f, .frontHead = 0.6f, .frontTune = 1.8, .shell = 0.5f, .mic = 0.35f, .drive = 0.45f, .gainDb = -2.3 }) },
        // Deep and soft under a low-pass: the kick that sits under chords and delays
        { .name = "Techno Dub", .parameters = parameters({ .tuneHz = 42.0, .decaySeconds = 2.5, .tone = 0.2f, .pitchDrop = 0.7f, .damping = 0.7f, .ring = 0.05, .ringDecaySeconds = 0.2, .beater = 0.05f, .click = 0.05f, .position = 0.0f, .frontHead = 0.8f, .frontTune = 1.0, .shell = 0.2f, .mic = 0.8f, .drive = 0.5f, .lpfHz = 900.0, .hpfHz = 25.0, .gainDb = 2.3 }) },
        // Driven into the ground: a hard beater, a deep drop and the drive at the top
        { .name = "Techno Hard", .parameters = parameters({ .tuneHz = 52.0, .decaySeconds = 2.0, .tone = 0.7f, .pitchDrop = 1.0f, .damping = 0.75f, .ring = 0.3, .ringDecaySeconds = 0.15, .beater = 0.9f, .click = 0.8f, .position = 0.2f, .frontHead = 0.4f, .frontTune = 1.0, .shell = 0.3f, .mic = 0.3f, .drive = 1.0f, .hpfHz = 30.0, .gainDb = -3.5 }) },
        // Open and long under heavy drive: the low rumble left between the hits
        { .name = "Techno Rumble", .parameters = parameters({ .tuneHz = 45.0, .decaySeconds = 4.0, .tone = 0.3f, .pitchDrop = 0.8f, .damping = 0.35f, .ring = 0.1, .ringDecaySeconds = 0.3, .beater = 0.5f, .click = 0.5f, .position = 0.1f, .frontHead = 0.9f, .frontTune = 1.0, .shell = 0.2f, .mic = 0.7f, .drive = 0.95f, .lpfHz = 2500.0, .hpfHz = 28.0, .gainDb = -3.0 }) },
        // Short, deep and driven, with a hard click on top: the four-to-the-floor kick
        { .name = "Techno Thump", .parameters = parameters({ .tuneHz = 50.0, .decaySeconds = 1.5, .tone = 0.5f, .pitchDrop = 0.9f, .damping = 0.78f, .ring = 0.1, .ringDecaySeconds = 0.1, .beater = 0.75f, .click = 0.6f, .position = 0.1f, .frontHead = 0.3f, .frontTune = 1.0, .shell = 0.3f, .mic = 0.4f, .drive = 0.8f, .hpfHz = 30.0, .gainDb = -1.8 }) },
        // An 18" floor tom with sticks, close mic: fitted to Big Rusty Drums
        { .name = "Tom Floor 18", .parameters = parameters({ .tuneHz = 62.6, .decaySeconds = 2.84, .tone = 0.49f, .pitchDrop = 0.28f, .damping = 0.6f, .ring = 1.18, .ringDecaySeconds = 0.72, .beater = 0.4f, .click = 0.2f, .position = 0.9f, .frontHead = 0.34f, .frontTune = 1.7, .shell = 0.01f, .mic = 0.73f, .drive = 0.13f, .gainDb = -1.2 }) },
        // A small rack tom, tuned up and left open
        { .name = "Tom High", .parameters = parameters({ .tuneHz = 190.0, .decaySeconds = 1.0, .tone = 0.3f, .pitchDrop = 0.35f, .damping = 0.15f, .ring = 0.3, .ringDecaySeconds = 0.6, .beater = 0.3f, .click = 0.6f, .position = 0.45f, .frontHead = 0.9f, .frontTune = 2.0, .shell = 0.8f, .mic = 0.1f, .drive = 0.6f, .gainDb = 1.7 }) },
        // The 14" tom with soft mallets: fitted to Big Rusty Drums
        { .name = "Tom Mallet 14", .parameters = parameters({ .tuneHz = 91.5, .decaySeconds = 0.94, .tone = 0.34f, .pitchDrop = 0.26f, .damping = 0.23f, .ring = 0.76, .ringDecaySeconds = 1.2, .beater = 0.41f, .click = 0.13f, .position = 0.67f, .frontHead = 0.81f, .frontTune = 2.2, .shell = 0.02f, .mic = 0.51f, .drive = 0.0f }) },
        // A mid rack tom between the high one and the 14"
        { .name = "Tom Mid", .parameters = parameters({ .tuneHz = 130.0, .decaySeconds = 0.9, .tone = 0.15f, .pitchDrop = 0.35f, .damping = 0.15f, .ring = 0.3, .ringDecaySeconds = 0.9, .beater = 0.25f, .click = 0.7f, .position = 0.45f, .frontHead = 0.95f, .frontTune = 2.2, .shell = 0.9f, .mic = 0.1f, .drive = 0.65f, .gainDb = 1.4 }) },
        // A 14" tom with sticks, close mic: fitted to Big Rusty Drums
        { .name = "Tom Rack 14", .parameters = parameters({ .tuneHz = 91.3, .decaySeconds = 0.89, .tone = 0.71f, .pitchDrop = 0.31f, .damping = 0.2f, .ring = 0.88, .ringDecaySeconds = 0.92, .beater = 0.53f, .click = 0.2f, .position = 0.84f, .frontHead = 0.88f, .frontTune = 2.02, .shell = 0.0f, .mic = 0.27f, .drive = 0.02f, .gainDb = -2.4 }) },
        // A kick tuned high, struck dead centre, from its own mic
        { .name = "Tuned Up", .parameters = parameters({ .tuneHz = 81.5, .decaySeconds = 1.3, .tone = 0.03f, .pitchDrop = 0.46f, .damping = 0.2f, .ring = 1.0, .ringDecaySeconds = 0.75, .beater = 0.0f, .click = 0.04f, .position = 0.18f, .frontHead = 0.38f, .frontTune = 2.09, .shell = 0.7f, .mic = 0.34f, .drive = 0.13f, .gainDb = 2.9 }) },
    };
    return presets;
}

} // namespace noteahead
