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

#ifndef DEVICE_PARAMETER_DISPLAY_HPP
#define DEVICE_PARAMETER_DISPLAY_HPP

#include <QString>

namespace noteahead {

class ParameterContainer;

//! The readouts devices share, so that each one describes what a parameter automation may drive in
//! a line a control instead of spelling the units out every time. A key the device does not have
//! is skipped, which lets the helpers describe a family of devices that differ in what they carry.
namespace DeviceParameterDisplay {

//! LPF Cutoff, LPF Resonance and HPF Cutoff, the cutoffs in hertz over 20 Hz to 20 kHz the way
//! FilterKnob shows them.
void filters(ParameterContainer & container);
//! Rate and Intensity of both LFOs: the rate in hertz, the intensity either way about none.
void lfos(ParameterContainer & container);
//! A plain 0-100 % control, the way a Knob without a mapping reads out.
void percent(ParameterContainer & container, const QString & key, const char * label);
//! A control that runs either way about a centre, -100 to +100 %, resting in the middle.
void bipolar(ParameterContainer & container, const QString & key, const char * label);
//! A tuning in cents, @p range either way, on the cubic curve the tuning knobs use.
void cents(ParameterContainer & container, const QString & key, const char * label, double range);

} // namespace DeviceParameterDisplay

} // namespace noteahead

#endif // DEVICE_PARAMETER_DISPLAY_HPP
