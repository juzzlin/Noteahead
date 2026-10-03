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

#include "reverb_presets.hpp"

#include "../../common/constants.hpp"

#include <algorithm>

namespace noteahead {

namespace {

// Written in the units a room is described in -- milliseconds, a width factor, percent -- and
// converted to parameter values here, so that a patch below reads as what it is rather than as a
// row of knob positions. The conversions are the inverses of the ranges the effect maps over.

float clamped(double value)
{
    return static_cast<float>(std::clamp(value, 0.0, 1.0));
}

//! The controls every patch below sets. Mix especially: the effect's own default is fully dry, and
//! a preset resets everything it does not name, so one that left it out would load as silence.
EffectPreset preset(std::string name, double size, double decayMs, double damping, double preDelayMs, double width, double lpf, double hpf, double mixPercent)
{
    namespace C = Constants::NahdXml;
    return EffectPreset { std::move(name),
                          { { C::xmlKeySize().toStdString(), clamped(size) },
                            { C::xmlKeyDecay().toStdString(), clamped(decayMs / 10000.0) },
                            { C::xmlKeyDamping().toStdString(), clamped(damping) },
                            { C::xmlKeyPreDelay().toStdString(), clamped(preDelayMs / 500.0) },
                            { C::xmlKeyWidth().toStdString(), clamped(width / 2.0) },
                            { C::xmlKeyLpfCutoff().toStdString(), clamped(lpf) },
                            { C::xmlKeyHpfCutoff().toStdString(), clamped(hpf) },
                            { C::xmlKeyMix().toStdString(), clamped(mixPercent / 100.0) } } };
}

} // namespace

const EffectPresetList & ReverbPresets::presets()
{
    // In the order the dropdown has always offered them, from the big rooms down to the oddities.
    //
    // Mix is set for an insert. On a send bus the same number simply returns less of the same tail,
    // since send mode blends additively and the bus level sets the amount.
    static const EffectPresetList presets {
        preset("Hall", 0.8, 3000.0, 0.3, 30.0, 1.2, 0.86, 0.18, 30.0),
        preset("Large Room", 0.6, 1500.0, 0.4, 15.0, 1.0, 0.82, 0.2, 25.0),
        preset("Small Room", 0.3, 600.0, 0.5, 5.0, 0.8, 0.78, 0.24, 20.0),
        preset("Plate", 0.5, 2000.0, 0.2, 10.0, 1.5, 0.9, 0.2, 25.0),
        preset("Cathedral", 1.0, 8000.0, 0.2, 50.0, 2.0, 0.84, 0.16, 35.0),
        preset("Basement", 0.2, 400.0, 0.7, 2.0, 0.5, 0.68, 0.3, 20.0),
        preset("Tunnel", 0.9, 5000.0, 0.1, 40.0, 0.6, 0.88, 0.18, 30.0),
        preset("Spring", 0.4, 1200.0, 0.4, 5.0, 1.0, 0.72, 0.26, 25.0)
    };
    return presets;
}

} // namespace noteahead
