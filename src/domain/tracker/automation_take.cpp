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

#include "automation_take.hpp"

#include "interpolator.hpp"

#include <algorithm>
#include <cmath>

namespace noteahead::AutomationTake {

std::vector<SampleList> split(const SampleList & samples)
{
    std::vector<SampleList> pieces;
    for (auto && sample : samples) {
        if (pieces.empty() || pieces.back().back().pattern != sample.pattern || sample.line < pieces.back().back().line) {
            pieces.emplace_back();
        }
        pieces.back().push_back(sample);
    }
    return pieces;
}

namespace {

void simplifyInto(const AutomationCurve::NodeList & nodes, size_t first, size_t last, double tolerance, std::vector<bool> & keep)
{
    if (last <= first + 1) {
        return;
    }
    const auto & a = nodes.at(first);
    const auto & b = nodes.at(last);
    double farthest = 0.0;
    size_t farthestIndex = first;
    for (size_t index = first + 1; index < last; index++) {
        const auto & node = nodes.at(index);
        const double t = b.line > a.line ? (node.line - a.line) / (b.line - a.line) : 0.0;
        if (const double distance = std::abs(node.value - (a.value + t * (b.value - a.value))); distance > farthest) {
            farthest = distance;
            farthestIndex = index;
        }
    }
    if (farthest > tolerance) {
        keep.at(farthestIndex) = true;
        simplifyInto(nodes, first, farthestIndex, tolerance, keep);
        simplifyInto(nodes, farthestIndex, last, tolerance, keep);
    }
}

//! The automation's own value at @p line, as it plays it before modulation
double valueAt(const ParameterAutomation::InterpolationParameters & interpolation, double line)
{
    if (interpolation.curve == Interpolator::CurveType::Drawn && !interpolation.drawnCurve.isEmpty()) {
        return interpolation.drawnCurve.valueAt(line);
    }
    const double span = static_cast<double>(interpolation.line1) - static_cast<double>(interpolation.line0);
    const double phase = span > 0.0 ? std::clamp((line - static_cast<double>(interpolation.line0)) / span, 0.0, 1.0) : 0.0;
    return interpolation.value0 + Interpolator::applyCurve(interpolation.curve, phase) * (interpolation.value1 - interpolation.value0);
}

//! The whole automation as a drawn curve: its own nodes, its ramp, or its shape on every line,
//! which is as finely as it is played
AutomationCurve drawnCurveOf(const ParameterAutomation::InterpolationParameters & interpolation)
{
    if (interpolation.curve == Interpolator::CurveType::Drawn && !interpolation.drawnCurve.isEmpty()) {
        return interpolation.drawnCurve;
    }
    if (interpolation.curve == Interpolator::CurveType::Linear || interpolation.curve == Interpolator::CurveType::Drawn) {
        return AutomationCurve::ramp(interpolation.line0, interpolation.value0, interpolation.line1, interpolation.value1);
    }
    AutomationCurve::NodeList nodes;
    for (size_t line = interpolation.line0; line <= interpolation.line1; line++) {
        nodes.push_back({ static_cast<double>(line), valueAt(interpolation, static_cast<double>(line)) });
    }
    return AutomationCurve { std::move(nodes) };
}

} // namespace

AutomationCurve::NodeList simplify(const AutomationCurve::NodeList & nodes, double tolerance)
{
    if (nodes.size() < 3) {
        return nodes;
    }
    std::vector<bool> keep(nodes.size(), false);
    keep.front() = true;
    keep.back() = true;
    simplifyInto(nodes, 0, nodes.size() - 1, tolerance, keep);
    AutomationCurve::NodeList simplified;
    for (size_t index = 0; index < nodes.size(); index++) {
        if (keep.at(index)) {
            simplified.push_back(nodes.at(index));
        }
    }
    return simplified;
}

std::optional<ParameterAutomation::InterpolationParameters> curve(const SampleList & piece, size_t lineCount, double tolerance)
{
    if (piece.empty() || lineCount < 2) {
        return std::nullopt;
    }
    const size_t lastLine = lineCount - 1;
    const auto line0 = static_cast<size_t>(std::max(0.0, std::floor(piece.front().line)));
    if (line0 >= lastLine) {
        return std::nullopt;
    }
    const size_t line1 = std::clamp(static_cast<size_t>(std::ceil(std::max(0.0, piece.back().line))), line0 + 1, lastLine);
    const double start = static_cast<double>(line0);
    const double end = static_cast<double>(line1);

    // The take holds its first value from the start of its line and its last to the end of its
    // last, so a move is never stretched to fit the lines
    AutomationCurve::NodeList nodes { { start, piece.front().value } };
    double lastValue = piece.front().value;
    for (auto && sample : piece) {
        if (sample.line > end) {
            break;
        }
        lastValue = sample.value;
        if (sample.line >= nodes.back().line + AutomationCurve::minNodeGap && sample.line <= end - AutomationCurve::minNodeGap) {
            nodes.push_back({ sample.line, sample.value });
        }
    }
    nodes.push_back({ end, lastValue });
    nodes = simplify(nodes, tolerance);
    return ParameterAutomation::InterpolationParameters { line0, line1, nodes.front().value, nodes.back().value, Interpolator::CurveType::Drawn, AutomationCurve { std::move(nodes) } };
}

std::vector<ParameterAutomation> remainderOutside(const ParameterAutomation & automation, size_t line0, size_t line1)
{
    const auto & interpolation = automation.interpolation();
    if (interpolation.line1 < line0 || interpolation.line0 > line1) {
        return { automation };
    }
    const auto whole = drawnCurveOf(interpolation);
    std::vector<ParameterAutomation> remainder;
    const auto keep = [&](size_t from, size_t to) {
        auto piece = whole;
        piece.setSpan(from, to);
        auto kept = automation;
        kept.setInterpolation({ from, to, piece.nodes().front().value, piece.nodes().back().value, Interpolator::CurveType::Drawn, piece });
        remainder.push_back(std::move(kept));
    };
    // A piece needs two lines to be an automation, and stops a line short of the take
    if (line0 >= interpolation.line0 + 2) {
        keep(interpolation.line0, line0 - 1);
    }
    if (interpolation.line1 >= line1 + 2) {
        keep(line1 + 1, interpolation.line1);
    }
    return remainder;
}

} // namespace noteahead::AutomationTake
