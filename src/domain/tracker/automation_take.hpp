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

#ifndef AUTOMATION_TAKE_HPP
#define AUTOMATION_TAKE_HPP

#include "automation_curve.hpp"
#include "parameter_automation.hpp"

#include <optional>
#include <vector>

namespace noteahead::AutomationTake {

//! Where a control stood at one moment of a take, and when: a line of a pattern, between lines.
struct Sample
{
    size_t pattern = 0;
    double line = 0.0;
    double value = 0.0;
};

using SampleList = std::vector<Sample>;

//! Cuts a take where it leaves a pattern, or jumps back in one as a looped pattern does, so that
//! each piece was played over one pass of one pattern.
std::vector<SampleList> split(const SampleList & samples);

//! Drops every node the curve can do without and stay within @p tolerance of each one dropped,
//! measured up and down. The ends stay.
AutomationCurve::NodeList simplify(const AutomationCurve::NodeList & nodes, double tolerance);

//! A piece of a take as a drawn curve, from the line it began on to the line after it ended, held
//! within a pattern of @p lineCount lines. Nothing when the pattern leaves no room for a curve.
std::optional<ParameterAutomation::InterpolationParameters> curve(const SampleList & piece, size_t lineCount, double tolerance);

//! What is left of @p automation outside lines @p line0 to @p line1, which a take has just been
//! recorded over: none, one or two automations, drawn so that they play as before.
std::vector<ParameterAutomation> remainderOutside(const ParameterAutomation & automation, size_t line0, size_t line1);

} // namespace noteahead::AutomationTake

#endif // AUTOMATION_TAKE_HPP
