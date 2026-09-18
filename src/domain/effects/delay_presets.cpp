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

#include "delay_presets.hpp"

#include "../../common/constants.hpp"
#include "delay.hpp"

#include <algorithm>
#include <string>

namespace noteahead {

namespace {

// Written in the units a delay is thought in -- note divisions, milliseconds, percent -- and
// converted to parameter values here, so that a patch below reads as what it is rather than as a
// row of knob positions.

std::string key(const QString & xmlKey)
{
    return xmlKey.toStdString();
}

//! The delay line the echoes run through. Also decides what Depth means: stereo bounce on a
//! ping-pong, tape saturation on a tape, and nothing at all on the other two.
float type(Delay::Type value)
{
    return static_cast<float>(static_cast<int>(value));
}

//! Parameter value for a free-running delay time. The parameter is the fraction it takes of the
//! line's ten seconds.
float milliseconds(double ms)
{
    return static_cast<float>(std::clamp(ms / 10000.0, 0.0, 1.0));
}

//! Parameter value for a synced delay time, named as the note it lands on.
//!
//! The division is counted in whole notes, so a quarter note is 0.25 and the dotted eighth every
//! other delay patch is built on is 0.1875. Written as a fraction of four beats rather than in
//! beats because that is the number the sync slider itself steps through.
float division(double wholeNotes)
{
    return static_cast<float>(std::clamp(wholeNotes, 0.0, 1.0));
}

//! A percentage as the parameter takes it, for the controls that are plain fractions: feedback,
//! depth, mix and the two feedback filters.
float percent(double value)
{
    return static_cast<float>(std::clamp(value / 100.0, 0.0, 1.0));
}

EffectPreset preset(const std::string & name, std::map<std::string, float> parameters)
{
    return EffectPreset { name, std::move(parameters) };
}

//! The controls every patch below sets, so that none of them is left to whatever the last patch
//! happened to use. Mix especially: the effect's own default is fully dry, and a preset that did
//! not say otherwise would load as silence.
std::map<std::string, float> patch(Delay::Type delayType, bool sync, double timeOrDivision, double feedback, double depth, double mix, double lpf, double hpf)
{
    return {
        { key(Constants::NahdXml::xmlKeyDelayType()), type(delayType) },
        { key(Constants::NahdXml::xmlKeyDelaySync()), sync ? 1.0f : 0.0f },
        { key(sync ? Constants::NahdXml::xmlKeyDelaySyncDivision() : Constants::NahdXml::xmlKeyDelayTime()), sync ? division(timeOrDivision) : milliseconds(timeOrDivision) },
        { key(Constants::NahdXml::xmlKeyDelayFeedback()), percent(feedback) },
        { key(Constants::NahdXml::xmlKeyDelayDepth()), percent(depth) },
        { key(Constants::NahdXml::xmlKeyDelayMix()), percent(mix) },
        { key(Constants::NahdXml::xmlKeyDelayFeedbackLpf()), percent(lpf) },
        { key(Constants::NahdXml::xmlKeyDelayFeedbackHpf()), percent(hpf) }
    };
}

} // namespace

const EffectPresetList & DelayPresets::presets()
{
    // Alphabetical, the way the EQ's are: a list this long is read by name rather than from the top.
    //
    // Mix is set for an insert. On a send bus the same number simply returns less of the same echo,
    // since send mode blends additively and the bus level sets the amount.
    static const EffectPresetList presets {
        // Half-note repeats fed back almost to the point of running away, with the top and the
        // bottom taken off each pass so the tail turns to fog rather than to mud.
        preset("Ambient Wash", patch(Delay::Type::Stereo, true, 0.5, 75.0, 0.0, 45.0, 40.0, 12.0)),

        // The delay that fits a straight beat without landing on it. Its whole reason for being is
        // the three-against-two it makes against eighth notes.
        preset("Dotted Eighth", patch(Delay::Type::Stereo, true, 0.1875, 40.0, 0.0, 30.0, 80.0, 0.0)),

        // One repeat close enough behind the source to be heard as the same sound, not as an echo.
        preset("Doubler", patch(Delay::Type::Stereo, false, 40.0, 0.0, 0.0, 40.0, 100.0, 0.0)),

        // Long feedback through a tape line, darkening and thinning with every pass, which is what
        // a dub echo is: the repeats leave the mix rather than pile up in it.
        preset("Dub Echo", patch(Delay::Type::Tape, true, 0.1875, 68.0, 35.0, 40.0, 45.0, 18.0)),

        preset("Eighth Note", patch(Delay::Type::Stereo, true, 0.125, 35.0, 0.0, 30.0, 80.0, 0.0)),

        // Depth is the width here rather than any kind of modulation: at 80 % the repeats cross
        // most of the way over between the speakers.
        preset("Ping-Pong Eighth", patch(Delay::Type::PingPong, true, 0.125, 45.0, 80.0, 35.0, 80.0, 0.0)),

        preset("Quarter Note", patch(Delay::Type::Stereo, true, 0.25, 35.0, 0.0, 30.0, 80.0, 0.0)),

        // Mono and barely fed back, the way a slapback is printed: one repeat, close, and in the
        // middle where the source is.
        preset("Slapback", patch(Delay::Type::Mono, false, 110.0, 8.0, 0.0, 30.0, 75.0, 0.0)),

        // Free-running rather than synced: the point is a repeat that does not sit on the grid, and
        // the tape saturation that thickens each pass.
        preset("Tape Wobble", patch(Delay::Type::Tape, false, 260.0, 45.0, 60.0, 30.0, 60.0, 8.0))
    };

    return presets;
}

} // namespace noteahead
