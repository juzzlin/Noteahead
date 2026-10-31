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

#ifndef DRAWN_CURVE_SYNC_HPP
#define DRAWN_CURVE_SYNC_HPP

#include "../midi/midi_cc_automation.hpp"
#include "../midi/pitch_bend_automation.hpp"
#include "parameter_automation.hpp"

namespace noteahead {

//! Keeps a drawn curve in step with one edit of the interpolation it belongs to.
//!
//! @p edited is @p previous with one thing changed. Switched to Drawn, the curve starts as the ramp
//! it replaces; switched away, its nodes go. New lines refit the nodes to them, a new start or end
//! value moves that end node, and new nodes take their own ends. Whatever was edited, a drawn
//! curve's ends are then the start and end values, which is what versions without drawn curves play
//! a ramp between.
void syncDrawnCurve(MidiCcAutomation::InterpolationParameters & edited, const MidiCcAutomation::InterpolationParameters & previous);
void syncDrawnCurve(PitchBendAutomation::InterpolationParameters & edited, const PitchBendAutomation::InterpolationParameters & previous);
void syncDrawnCurve(ParameterAutomation::InterpolationParameters & edited, const ParameterAutomation::InterpolationParameters & previous);

//! The shape an interpolation draws, before modulation: its nodes when it is drawn, otherwise its
//! curve sampled finely enough to look smooth, so one renderer can show either.
AutomationCurve curveShape(const MidiCcAutomation::InterpolationParameters & interpolation);
AutomationCurve curveShape(const PitchBendAutomation::InterpolationParameters & interpolation);
AutomationCurve curveShape(const ParameterAutomation::InterpolationParameters & interpolation);

} // namespace noteahead

#endif // DRAWN_CURVE_SYNC_HPP
