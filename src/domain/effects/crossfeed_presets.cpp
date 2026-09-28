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

#include "crossfeed_presets.hpp"

#include "../../common/constants.hpp"
#include "../../common/parameter_mapper.hpp"

#include <algorithm>

namespace noteahead {

namespace {

// The same ranges the effect maps its parameters over. A preset is a row of knob positions, so the
// hertz and microseconds below have to be turned back into positions here.
constexpr double MinDelayUs = 100.0;
constexpr double MaxDelayUs = 500.0;
constexpr double MinCutoffHz = 300.0;
constexpr double MaxCutoffHz = 1500.0;

float delay(double microseconds)
{
    return static_cast<float>(std::clamp((microseconds - MinDelayUs) / (MaxDelayUs - MinDelayUs), 0.0, 1.0));
}

float cutoff(double hz)
{
    return static_cast<float>(ParameterMapper::unmapLogFrequency(hz, MinCutoffHz, MaxCutoffHz));
}

EffectPreset preset(std::string name, double amount, double microseconds, double hz)
{
    namespace C = Constants::NahdXml;
    return EffectPreset { std::move(name),
                          { { C::xmlKeyAmount().toStdString(), static_cast<float>(amount) },
                            { C::xmlKeyDelay().toStdString(), delay(microseconds) },
                            { C::xmlKeyCutoff().toStdString(), cutoff(hz) } } };
}

} // namespace

const EffectPresetList & CrossfeedPresets::presets()
{
    // The three voicings the hardware designs settled on, which differ in how much of the far
    // channel is let through and how early it is cut away. Off comes first because it is the way
    // back to hearing the mix as it is.
    static const EffectPresetList presets {
        preset("Off", 0.0, 270.0, 700.0),

        // Bauer's, the one most software implements: enough to pull a hard-panned part out of the
        // middle of the head without narrowing anything that was already centred.
        preset("Bauer", 0.6, 270.0, 700.0),

        // The resistor network Chu Moy published, which feeds more across and higher up. The image
        // is the most speaker-like of the three, and the most obviously narrowed.
        preset("Chu Moy", 0.75, 300.0, 900.0),

        // Meier's, the lightest: barely there below the voice, and out of the way above it. For
        // listening rather than for checking.
        preset("Meier", 0.45, 200.0, 1100.0)
    };
    return presets;
}

} // namespace noteahead
