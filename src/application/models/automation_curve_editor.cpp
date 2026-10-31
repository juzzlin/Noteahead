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

#include "automation_curve_editor.hpp"

#include <QVariantMap>

#include <algorithm>
#include <cmath>

namespace noteahead {

AutomationCurveEditor::AutomationCurveEditor(QObject * parent)
  : QObject { parent }
{
}

void AutomationCurveEditor::edit(const QVariantList & nodes, quint64 line0, quint64 line1, double value0, double value1, double minValue, double maxValue, double valueStep)
{
    m_line0 = line0;
    m_line1 = std::max(line0, line1);
    m_minValue = std::min(minValue, maxValue);
    m_maxValue = std::max(minValue, maxValue);
    m_valueStep = std::max(valueStep, 0.0);
    m_curve = fromVariantList(nodes);
    if (m_curve.isEmpty()) {
        m_curve = AutomationCurve::ramp(m_line0, constrainedValue(value0), m_line1, constrainedValue(value1));
    }
    m_curve.setSpan(m_line0, m_line1);
    m_history.clear();
    emit rangeChanged();
    emit nodesChanged();
    emit historyChanged();
}

QVariantList AutomationCurveEditor::nodes() const
{
    return toVariantList(m_curve);
}

quint64 AutomationCurveEditor::line0() const
{
    return m_line0;
}

quint64 AutomationCurveEditor::line1() const
{
    return m_line1;
}

double AutomationCurveEditor::minValue() const
{
    return m_minValue;
}

double AutomationCurveEditor::maxValue() const
{
    return m_maxValue;
}

int AutomationCurveEditor::insertNode(double line, double value, bool snapToLine)
{
    if (const auto index = m_curve.insert(AutomationCurve::snapLine(line, snapToLine), constrainedValue(value)); index) {
        emit nodesChanged();
        return static_cast<int>(*index);
    }
    return -1;
}

void AutomationCurveEditor::moveNode(int index, double line, double value, bool snapToLine)
{
    const auto & nodes = m_curve.nodes();
    if (index < 0 || static_cast<size_t>(index) >= nodes.size()) {
        return;
    }
    const auto at = static_cast<size_t>(index);
    double newLine = line;
    if (snapToLine && at > 0 && at + 1 < nodes.size()) {
        // The nearest line its neighbours leave room on, or where it is when they leave none: pushed
        // against a neighbour, a snapped node would otherwise land off the grid
        const double first = std::ceil(nodes.at(at - 1).line + AutomationCurve::minNodeGap);
        const double last = std::floor(nodes.at(at + 1).line - AutomationCurve::minNodeGap);
        newLine = first <= last ? std::clamp(std::round(line), first, last) : nodes.at(at).line;
    }
    const auto before = nodes.at(at);
    m_curve.move(at, newLine, constrainedValue(value));
    if (m_curve.nodes().at(at) != before) {
        emit nodesChanged();
    }
}

bool AutomationCurveEditor::removeNode(int index)
{
    if (index >= 0 && m_curve.remove(static_cast<size_t>(index))) {
        emit nodesChanged();
        return true;
    }
    return false;
}

void AutomationCurveEditor::beginGesture()
{
    m_gestureStart = m_curve;
}

void AutomationCurveEditor::endGesture()
{
    if (m_gestureStart != m_curve && !m_gestureStart.isEmpty()) {
        m_history.push_back(m_gestureStart);
        emit historyChanged();
    }
    m_gestureStart = {};
}

bool AutomationCurveEditor::canUndo() const
{
    return !m_history.empty();
}

void AutomationCurveEditor::undo()
{
    if (m_history.empty()) {
        return;
    }
    m_curve = m_history.back();
    m_history.pop_back();
    emit nodesChanged();
    emit historyChanged();
}

const AutomationCurve & AutomationCurveEditor::curve() const
{
    return m_curve;
}

QString AutomationCurveEditor::lineKey()
{
    return "line";
}

QString AutomationCurveEditor::valueKey()
{
    return "value";
}

QVariantList AutomationCurveEditor::toVariantList(const AutomationCurve & curve)
{
    QVariantList list;
    for (auto && node : curve.nodes()) {
        list.append(QVariantMap { { lineKey(), node.line }, { valueKey(), node.value } });
    }
    return list;
}

AutomationCurve AutomationCurveEditor::fromVariantList(const QVariantList & nodes)
{
    AutomationCurve::NodeList list;
    for (auto && node : nodes) {
        const auto map = node.toMap();
        list.push_back({ map.value(lineKey()).toDouble(), map.value(valueKey()).toDouble() });
    }
    return AutomationCurve { std::move(list) };
}

double AutomationCurveEditor::constrainedValue(double value) const
{
    if (m_valueStep > 0.0) {
        value = m_minValue + std::round((value - m_minValue) / m_valueStep) * m_valueStep;
    }
    return std::clamp(value, m_minValue, m_maxValue);
}

} // namespace noteahead
