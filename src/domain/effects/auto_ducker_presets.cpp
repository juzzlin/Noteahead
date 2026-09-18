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

#include "auto_ducker_presets.hpp"

#include "../../common/constants.hpp"
#include "../../common/parameter_mapper.hpp"

#include <algorithm>
#include <string>

namespace noteahead {

namespace {

// Written in decibels and milliseconds and converted here, for the same reason the EQ's presets are
// written in hertz: attack and release are exponential, so a plausible-looking knob position lands
// nowhere near the time it was meant to be.

std::string key(const QString & xmlKey)
{
    return xmlKey.toStdString();
}

//! Parameter value for the threshold, which runs linearly from -60 dB to 0 dB.
float thresholdDb(double db)
{
    return static_cast<float>(std::clamp((db + 60.0) / 60.0, 0.0, 1.0));
}

//! Parameter value for the amount, which runs from -24 dB to +24 dB about a transparent centre.
//! Ducking is the negative half; every patch here is in it.
float amountDb(double db)
{
    return static_cast<float>(std::clamp((db + 24.0) / 48.0, 0.0, 1.0));
}

//! Parameter value for the knee width, 0 dB to 24 dB. A wide knee is what makes the duck a slope
//! rather than a switch.
float kneeDb(double db)
{
    return static_cast<float>(std::clamp(db / 24.0, 0.0, 1.0));
}

//! Parameter value for the attack, mapped exponentially over 0.1 ms to 500 ms.
float attackMs(double ms)
{
    return static_cast<float>(std::clamp(ParameterMapper::unmapExponential(ms, 0.1, 500.0), 0.0, 1.0));
}

//! Parameter value for the release, mapped exponentially over 1 ms to 2000 ms. The control that
//! decides whether a duck is heard as a pump or as a fade.
float releaseMs(double ms)
{
    return static_cast<float>(std::clamp(ParameterMapper::unmapExponential(ms, 1.0, 2000.0), 0.0, 1.0));
}

//! Parameter value for the hold, which runs linearly from 0 ms to 500 ms.
float holdMs(double ms)
{
    return static_cast<float>(std::clamp(ms / 500.0, 0.0, 1.0));
}

//! Parameter value for the side-chain listening filter, as a fraction of its travel. Closing it
//! leaves only the low end of the source triggering the duck.
float sideChainLpf(double fraction)
{
    return static_cast<float>(std::clamp(fraction, 0.0, 1.0));
}

//! The controls every patch below sets. The side-chain source is deliberately not among them: it
//! names a device in this project, and a preset has no business knowing about one.
std::map<std::string, float> patch(double threshold, double amount, double knee, double attack, double release, double hold, double lpf = 1.0)
{
    return {
        { key(Constants::NahdXml::xmlKeyThreshold()), thresholdDb(threshold) },
        { key(Constants::NahdXml::xmlKeyAmount()), amountDb(amount) },
        { key(Constants::NahdXml::xmlKeyKnee()), kneeDb(knee) },
        { key(Constants::NahdXml::xmlKeyAttack()), attackMs(attack) },
        { key(Constants::NahdXml::xmlKeyRelease()), releaseMs(release) },
        { key(Constants::NahdXml::xmlKeyHold()), holdMs(hold) },
        { key(Constants::NahdXml::xmlKeySideChainLpf()), sideChainLpf(lpf) }
    };
}

EffectPreset preset(const std::string & name, std::map<std::string, float> parameters)
{
    return EffectPreset { name, std::move(parameters) };
}

} // namespace

const EffectPresetList & AutoDuckerPresets::presets()
{
    // Alphabetical, like the other preset lists. Every patch still needs a side-chain source chosen
    // for it: the duck is by definition something another device does to this one, and picking that
    // device is the one part of it no preset can carry.
    static const EffectPresetList presets {
        // Only the kick's own low end is allowed to trigger the duck, so the bass gets out of the
        // way of the thump without flinching at every hi-hat that shares the bus.
        preset("Bass Under Kick", patch(-20.0, -5.0, 6.0, 2.0, 120.0, 0.0, 0.35)),

        // The sound the control is named for: a fast, deep duck that lets go over a quarter note,
        // so the track breathes in time rather than merely getting quieter.
        preset("Classic Pump", patch(-20.0, -9.0, 6.0, 1.0, 180.0, 0.0)),

        // As far as the amount goes, with a hard knee and a slow release: the source disappears and
        // comes back rather than dipping.
        preset("Deep Duck", patch(-24.0, -24.0, 2.0, 0.3, 400.0, 0.0)),

        // Long release and a wide knee, a few dB at most: the kind of duck that is felt as space
        // rather than heard as movement.
        preset("Gentle Glue", patch(-18.0, -3.0, 9.0, 10.0, 300.0, 0.0)),

        // Far enough down and long enough to carry a whole pattern, which is what an eight-bar
        // build wants from a sidechain.
        preset("Hard Sidechain", patch(-28.0, -18.0, 3.0, 0.5, 250.0, 20.0)),

        // Out of the way of the transient and back before the next one: the duck is over long
        // before the following kick, so the bass keeps its level between hits.
        preset("Kick Tight", patch(-16.0, -6.0, 4.0, 0.5, 90.0, 10.0)),

        // Slow either side and held through the gaps between words, so music under a voice settles
        // once rather than pumping on every syllable.
        preset("Under Voice", patch(-30.0, -12.0, 6.0, 20.0, 600.0, 200.0))
    };

    return presets;
}

} // namespace noteahead
