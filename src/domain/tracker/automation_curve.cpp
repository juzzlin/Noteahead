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

#include "automation_curve.hpp"

#include "../../common/constants.hpp"
#include "../../common/xml/project_reader.hpp"
#include "../../common/xml/project_writer.hpp"

#include <QVariant>

#include <algorithm>
#include <cmath>

namespace noteahead {

namespace {
//! Enough digits to read back the same double, which a knob position needs
QString numberToXml(double value)
{
    return QString::number(value, 'g', 12);
}
} // namespace

AutomationCurve::AutomationCurve() = default;

AutomationCurve::AutomationCurve(NodeList nodes)
  : m_nodes { std::move(nodes) }
{
    std::ranges::sort(m_nodes, {}, &Node::line);
}

AutomationCurve AutomationCurve::ramp(size_t line0, double value0, size_t line1, double value1)
{
    if (line1 <= line0) {
        return AutomationCurve { { { static_cast<double>(line0), value0 } } };
    }
    return AutomationCurve { { { static_cast<double>(line0), value0 }, { static_cast<double>(line1), value1 } } };
}

const AutomationCurve::NodeList & AutomationCurve::nodes() const
{
    return m_nodes;
}

bool AutomationCurve::isEmpty() const
{
    return m_nodes.empty();
}

double AutomationCurve::valueAt(double line) const
{
    if (m_nodes.empty()) {
        return 0.0;
    }
    if (line <= m_nodes.front().line) {
        return m_nodes.front().value;
    }
    if (line >= m_nodes.back().line) {
        return m_nodes.back().value;
    }
    // The first node past the line, which has one before it as the line is past the first
    const auto next = std::ranges::upper_bound(m_nodes, line, {}, &Node::line);
    const auto & right = *next;
    const auto & left = *std::prev(next);
    const double span = right.line - left.line;
    return span > 0.0 ? left.value + (line - left.line) / span * (right.value - left.value) : right.value;
}

std::optional<size_t> AutomationCurve::insert(double line, double value)
{
    if (m_nodes.size() < 2) {
        return {};
    }
    const auto next = std::ranges::upper_bound(m_nodes, line, {}, &Node::line);
    if (next == m_nodes.begin() || next == m_nodes.end()) {
        return {};
    }
    if (line - std::prev(next)->line < minNodeGap || next->line - line < minNodeGap) {
        return {};
    }
    const auto inserted = m_nodes.insert(next, { line, value });
    return static_cast<size_t>(std::distance(m_nodes.begin(), inserted));
}

bool AutomationCurve::remove(size_t index)
{
    if (index == 0 || index + 1 >= m_nodes.size()) {
        return false;
    }
    m_nodes.erase(m_nodes.begin() + static_cast<std::ptrdiff_t>(index));
    return true;
}

void AutomationCurve::move(size_t index, double line, double value)
{
    if (index >= m_nodes.size()) {
        return;
    }
    auto & node = m_nodes.at(index);
    node.value = value;
    if (index > 0 && index + 1 < m_nodes.size()) {
        node.line = std::clamp(line, m_nodes.at(index - 1).line + minNodeGap, m_nodes.at(index + 1).line - minNodeGap);
    }
}

void AutomationCurve::setSpan(size_t line0, size_t line1)
{
    if (m_nodes.empty()) {
        return;
    }
    const double start = static_cast<double>(line0);
    const double end = static_cast<double>(std::max(line0, line1));
    const Node first { start, valueAt(start) };
    const Node last { end, valueAt(end) };
    NodeList nodes { first };
    std::ranges::copy_if(m_nodes, std::back_inserter(nodes), [&](auto && node) {
        return node.line >= start + minNodeGap && node.line <= end - minNodeGap;
    });
    if (end > start) {
        nodes.push_back(last);
    }
    m_nodes = std::move(nodes);
}

double AutomationCurve::snapLine(double line, bool snap)
{
    return snap ? std::round(line) : line;
}

void AutomationCurve::serializeToXml(ProjectWriter & writer) const
{
    for (auto && node : m_nodes) {
        writer.writeStartElement(Constants::NahdXml::xmlKeyNode());
        writer.writeAttribute(Constants::NahdXml::xmlKeyNodeLine(), numberToXml(node.line));
        writer.writeAttribute(Constants::NahdXml::xmlKeyValue(), numberToXml(node.value));
        writer.writeEndElement(); // Node
    }
}

void AutomationCurve::deserializeNodeFromXml(ProjectReader & reader)
{
    const Node node { reader.attribute(Constants::NahdXml::xmlKeyNodeLine()).toDouble(), reader.attribute(Constants::NahdXml::xmlKeyValue()).toDouble() };
    m_nodes.insert(std::ranges::upper_bound(m_nodes, node.line, {}, &Node::line), node);
}

} // namespace noteahead
