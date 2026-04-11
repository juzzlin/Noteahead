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

#include "kick_808_presets.hpp"

#include "../../../common/constants.hpp"

#include <algorithm>
#include <cmath>
#include <map>
#include <string>

namespace noteahead {

namespace {

//! A kick in panel units, the knobs 0..10 as the dialog shows them. Every field starts at the
//! device's default, so a preset names only what it changes.
struct Kick
{
    double tune { 5.0 };
    double tone { 3.5 };
    double decay { 6.0 };
    double pitchDepth { 5.0 };
    double pitchDecay { 2.5 };
    double drive { 0.0 };
    double glide { 0.0 };
    //! Off pins the kick at Tuning, as a drum; on plays it from the note column, as a bass
    bool keyTrack { true };
    double lpfHz { 20000.0 };
    double hpfHz { 20.0 };
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
        { Xml::xmlKeyTone().toStdString(), panel(kick.tone) },
        { Xml::xmlKeyDecay().toStdString(), panel(kick.decay) },
        { Xml::xmlKeyPitchDepth().toStdString(), panel(kick.pitchDepth) },
        { Xml::xmlKeyPitchDecay().toStdString(), panel(kick.pitchDecay) },
        { Xml::xmlKeyDrive().toStdString(), panel(kick.drive) },
        { Xml::xmlKeyPortamento().toStdString(), panel(kick.glide) },
        { Xml::xmlKeyKeyTrack().toStdString(), kick.keyTrack ? 1.0f : 0.0f },
        { Xml::xmlKeyLpfCutoff().toStdString(), cutoff(kick.lpfHz) },
        { Xml::xmlKeyHpfCutoff().toStdString(), cutoff(kick.hpfHz) },
        { Xml::xmlKeyGain().toStdString(), position(kick.gainDb, -30.0, 30.0) },
    };
}

} // namespace

const std::vector<SynthPreset> & Kick808Presets::presets()
{
    // Init first, as the way back; the rest alphabetical. The drums have Key Track off, so they play
    // at their Tuning from any note, as the hardware's single tuned voice does.
    static const std::vector<SynthPreset> presets {
        { .name = "Init", .parameters = {} },
        // Nothing but the start of the hit, high-passed: a tick for layering over another kick
        { .name = "Click Layer", .parameters = parameters({ .tone = 10.0, .decay = 0.5, .pitchDepth = 8.0, .pitchDecay = 0.5, .keyTrack = false, .hpfHz = 150.0, .gainDb = 11.0 }) },
        // Driven into a growl, the sweep a little longer to carry it. Under Init's peak, as the drive
        // makes it denser and so louder at the same peak
        { .name = "Distorted", .parameters = parameters({ .tone = 5.0, .decay = 6.0, .pitchDepth = 6.0, .pitchDecay = 3.0, .drive = 7.0, .keyTrack = false, .gainDb = -4.0 }) },
        // Steep, fast and short, with the sub cut: the techno kick pushed hard
        { .name = "Hard Techno", .parameters = parameters({ .tune = 5.8, .tone = 6.0, .decay = 4.5, .pitchDepth = 7.0, .pitchDecay = 2.5, .drive = 8.0, .keyTrack = false, .hpfHz = 40.0, .gainDb = -2.9 }) },
        // Low and long, the sweep slow and shallow: the boom that fills a hip-hop bar
        { .name = "Long Boom", .parameters = parameters({ .tune = 4.4, .tone = 3.0, .decay = 9.0, .pitchDepth = 4.0, .pitchDecay = 3.0, .keyTrack = false, .gainDb = 0.6 }) },
        // Short with a hard snap: a kick that gets out of the way
        { .name = "Short Punch", .parameters = parameters({ .tune = 5.4, .tone = 6.0, .decay = 2.5, .pitchDepth = 7.0, .pitchDecay = 1.5, .keyTrack = false }) },
        // Long and round, sliding between the notes of the column: the 808 as a bass line
        { .name = "Sub Glide", .parameters = parameters({ .tone = 2.0, .decay = 8.0, .pitchDepth = 2.0, .glide = 3.0, .lpfHz = 2000.0, .gainDb = 1.0 }) },
        // Tight and a little driven, the lowest lows cut: the four-to-the-floor techno kick
        { .name = "Techno", .parameters = parameters({ .tune = 5.4, .tone = 4.5, .decay = 4.0, .pitchDepth = 6.0, .pitchDecay = 2.0, .drive = 3.0, .keyTrack = false, .hpfHz = 30.0, .gainDb = 2.1 }) },
    };
    return presets;
}

} // namespace noteahead
