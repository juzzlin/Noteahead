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

#include "drawn_curve_sync.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <type_traits>

namespace noteahead {

namespace {

//! @p value as the type the start and end values are stored in: rounded for whole numbers
template<typename Value>
Value storedValue(double value)
{
    if constexpr (std::is_integral_v<Value>) {
        const auto lowest = static_cast<double>(std::numeric_limits<Value>::lowest());
        const auto highest = static_cast<double>(std::numeric_limits<Value>::max());
        return static_cast<Value>(std::clamp(std::round(value), lowest, highest));
    } else {
        return static_cast<Value>(value);
    }
}

void sync(auto & edited, const auto & previous)
{
    if (edited.curve != Interpolator::CurveType::Drawn) {
        edited.drawnCurve = {};
        return;
    }

    auto & curve = edited.drawnCurve;
    if (curve.isEmpty()) {
        curve = AutomationCurve::ramp(edited.line0, static_cast<double>(edited.value0), edited.line1, static_cast<double>(edited.value1));
    } else if (curve != previous.drawnCurve) {
        curve.setSpan(edited.line0, edited.line1);
    } else if (edited.line0 != previous.line0 || edited.line1 != previous.line1) {
        curve.setSpan(edited.line0, edited.line1);
    } else if (edited.value0 != previous.value0) {
        curve.move(0, curve.nodes().front().line, static_cast<double>(edited.value0));
    } else if (edited.value1 != previous.value1) {
        curve.move(curve.nodes().size() - 1, curve.nodes().back().line, static_cast<double>(edited.value1));
    }

    using Value = std::remove_cvref_t<decltype(edited.value0)>;
    edited.value0 = storedValue<Value>(curve.nodes().front().value);
    edited.value1 = storedValue<Value>(curve.nodes().back().value);
}

AutomationCurve shape(const auto & interpolation)
{
    if (interpolation.curve == Interpolator::CurveType::Drawn && !interpolation.drawnCurve.isEmpty()) {
        return interpolation.drawnCurve;
    }
    const auto value0 = static_cast<double>(interpolation.value0);
    const auto value1 = static_cast<double>(interpolation.value1);
    if (interpolation.line1 <= interpolation.line0) {
        return AutomationCurve::ramp(interpolation.line0, value0, interpolation.line1, value1);
    }
    // Plenty for the steepest of the curves at any width a row is drawn at
    constexpr size_t segments = 48;
    const double span = static_cast<double>(interpolation.line1 - interpolation.line0);
    AutomationCurve::NodeList nodes;
    for (size_t i = 0; i <= segments; i++) {
        const double phase = static_cast<double>(i) / segments;
        nodes.push_back({ static_cast<double>(interpolation.line0) + phase * span, value0 + Interpolator::applyCurve(interpolation.curve, phase) * (value1 - value0) });
    }
    return AutomationCurve { std::move(nodes) };
}

} // namespace

AutomationCurve curveShape(const MidiCcAutomation::InterpolationParameters & interpolation)
{
    return shape(interpolation);
}

AutomationCurve curveShape(const PitchBendAutomation::InterpolationParameters & interpolation)
{
    return shape(interpolation);
}

AutomationCurve curveShape(const ParameterAutomation::InterpolationParameters & interpolation)
{
    return shape(interpolation);
}

void syncDrawnCurve(MidiCcAutomation::InterpolationParameters & edited, const MidiCcAutomation::InterpolationParameters & previous)
{
    sync(edited, previous);
}

void syncDrawnCurve(PitchBendAutomation::InterpolationParameters & edited, const PitchBendAutomation::InterpolationParameters & previous)
{
    sync(edited, previous);
}

void syncDrawnCurve(ParameterAutomation::InterpolationParameters & edited, const ParameterAutomation::InterpolationParameters & previous)
{
    sync(edited, previous);
}

} // namespace noteahead
