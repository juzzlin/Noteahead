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

#include "automation_curve_renderer.hpp"

#include "../../../application/models/automation_curve_editor.hpp"

#include <QFont>
#include <QPainter>
#include <QPainterPath>
#include <QVariantMap>

#include <algorithm>
#include <cmath>

namespace noteahead {

namespace {
constexpr qreal sideMargin = 10.0;
constexpr qreal topMargin = 10.0;
//! Room under the plot for the beat numbers
constexpr qreal labelMargin = 18.0;
constexpr qreal nodeRadius = 4.5;
constexpr qreal hoveredNodeRadius = 6.5;
//! How far from a node a press still takes hold of it, in pixels: wider than the node, so a
//! node can be caught without aiming at its centre
constexpr qreal grabDistance = 9.0;
//! Below this many pixels apart, the lines between the beats are not drawn
constexpr qreal minLineSpacing = 6.0;
//! Below this many pixels apart, beat numbers skip beats so as not to run into each other
constexpr qreal minLabelSpacing = 28.0;

double nodeLine(const QVariant & node)
{
    return node.toMap().value(AutomationCurveEditor::lineKey()).toDouble();
}

double nodeValue(const QVariant & node)
{
    return node.toMap().value(AutomationCurveEditor::valueKey()).toDouble();
}
} // namespace

AutomationCurveRenderer::AutomationCurveRenderer(QQuickItem * parent)
  : QQuickPaintedItem { parent }
{
    setAntialiasing(true);
}

QVariantList AutomationCurveRenderer::nodes() const
{
    return m_nodes;
}

void AutomationCurveRenderer::setNodes(const QVariantList & nodes)
{
    if (m_nodes != nodes) {
        m_nodes = nodes;
        emit nodesChanged();
        update();
    }
}

double AutomationCurveRenderer::line0() const
{
    return m_line0;
}

void AutomationCurveRenderer::setLine0(double line0)
{
    if (m_line0 != line0) {
        m_line0 = line0;
        emit rangeChanged();
        update();
    }
}

double AutomationCurveRenderer::line1() const
{
    return m_line1;
}

void AutomationCurveRenderer::setLine1(double line1)
{
    if (m_line1 != line1) {
        m_line1 = line1;
        emit rangeChanged();
        update();
    }
}

double AutomationCurveRenderer::minValue() const
{
    return m_minValue;
}

void AutomationCurveRenderer::setMinValue(double minValue)
{
    if (m_minValue != minValue) {
        m_minValue = minValue;
        emit rangeChanged();
        update();
    }
}

double AutomationCurveRenderer::maxValue() const
{
    return m_maxValue;
}

void AutomationCurveRenderer::setMaxValue(double maxValue)
{
    if (m_maxValue != maxValue) {
        m_maxValue = maxValue;
        emit rangeChanged();
        update();
    }
}

int AutomationCurveRenderer::linesPerBeat() const
{
    return m_linesPerBeat;
}

void AutomationCurveRenderer::setLinesPerBeat(int linesPerBeat)
{
    if (m_linesPerBeat != linesPerBeat) {
        m_linesPerBeat = linesPerBeat;
        emit rangeChanged();
        update();
    }
}

int AutomationCurveRenderer::hoveredNode() const
{
    return m_hoveredNode;
}

void AutomationCurveRenderer::setHoveredNode(int hoveredNode)
{
    if (m_hoveredNode != hoveredNode) {
        m_hoveredNode = hoveredNode;
        emit hoveredNodeChanged();
        update();
    }
}

QColor AutomationCurveRenderer::accentColor() const
{
    return m_accentColor;
}

void AutomationCurveRenderer::setAccentColor(const QColor & color)
{
    if (m_accentColor != color) {
        m_accentColor = color;
        emit accentColorChanged();
        update();
    }
}

bool AutomationCurveRenderer::compact() const
{
    return m_compact;
}

void AutomationCurveRenderer::setCompact(bool compact)
{
    if (m_compact != compact) {
        m_compact = compact;
        emit compactChanged();
        update();
    }
}

QRectF AutomationCurveRenderer::plotRect() const
{
    if (m_compact) {
        // Room for half the pen at the top and bottom, where a curve at full range runs
        return { 0.0, 1.5, std::max(width(), 1.0), std::max(height() - 3.0, 1.0) };
    }
    return { sideMargin, topMargin, std::max(width() - 2.0 * sideMargin, 1.0), std::max(height() - topMargin - labelMargin, 1.0) };
}

double AutomationCurveRenderer::lineAt(qreal x) const
{
    const auto plot = plotRect();
    const double fraction = std::clamp((x - plot.left()) / plot.width(), 0.0, 1.0);
    return m_line0 + fraction * (m_line1 - m_line0);
}

double AutomationCurveRenderer::valueAt(qreal y) const
{
    const auto plot = plotRect();
    const double fraction = std::clamp((plot.bottom() - y) / plot.height(), 0.0, 1.0);
    return m_minValue + fraction * (m_maxValue - m_minValue);
}

qreal AutomationCurveRenderer::xForLine(double line) const
{
    const auto plot = plotRect();
    const double span = m_line1 - m_line0;
    return plot.left() + (span > 0.0 ? (line - m_line0) / span : 0.0) * plot.width();
}

qreal AutomationCurveRenderer::yForValue(double value) const
{
    const auto plot = plotRect();
    const double span = m_maxValue - m_minValue;
    return plot.bottom() - (span > 0.0 ? (value - m_minValue) / span : 0.0) * plot.height();
}

int AutomationCurveRenderer::nodeAt(qreal x, qreal y) const
{
    int nearest = -1;
    qreal nearestDistance = grabDistance;
    for (int i = 0; i < m_nodes.size(); i++) {
        const qreal distance = std::hypot(xForLine(nodeLine(m_nodes.at(i))) - x, yForValue(nodeValue(m_nodes.at(i))) - y);
        if (distance <= nearestDistance) {
            nearest = i;
            nearestDistance = distance;
        }
    }
    return nearest;
}

void AutomationCurveRenderer::paintGrid(QPainter & painter, const QRectF & plot) const
{
    const double span = m_line1 - m_line0;
    if (span <= 0.0) {
        return;
    }
    const qreal lineSpacing = plot.width() / span;
    const int linesPerBeat = std::max(m_linesPerBeat, 1);
    const qreal beatSpacing = lineSpacing * linesPerBeat;
    const int labelEvery = std::max(1, static_cast<int>(std::ceil(minLabelSpacing / beatSpacing)));

    QFont labelFont;
    labelFont.setPixelSize(10);
    painter.setFont(labelFont);

    const auto first = static_cast<long long>(std::ceil(m_line0));
    const auto last = static_cast<long long>(std::floor(m_line1));
    for (auto line = first; line <= last; line++) {
        const qreal x = xForLine(static_cast<double>(line));
        const bool onBeat = line % linesPerBeat == 0;
        if (onBeat) {
            painter.setPen(QPen { QColor { "#3a3a3a" }, 1 });
            painter.drawLine(QPointF { x, plot.top() }, QPointF { x, plot.bottom() });
            if ((line / linesPerBeat) % labelEvery == 0) {
                painter.setPen(QColor { "#777777" });
                painter.drawText(QRectF { x - 20.0, plot.bottom() + 2.0, 40.0, labelMargin - 4.0 }, Qt::AlignHCenter | Qt::AlignTop, QString::number(line));
            }
        } else if (lineSpacing >= minLineSpacing) {
            painter.setPen(QPen { QColor { "#262626" }, 1 });
            painter.drawLine(QPointF { x, plot.top() }, QPointF { x, plot.bottom() });
        }
    }

    // Quarters of the range, and the middle stronger: the centre of a pitch bend or a pan
    for (int i = 1; i < 4; i++) {
        const qreal y = plot.top() + plot.height() * i / 4.0;
        painter.setPen(QPen { i == 2 ? QColor { "#444444" } : QColor { "#262626" }, 1 });
        painter.drawLine(QPointF { plot.left(), y }, QPointF { plot.right(), y });
    }
}

void AutomationCurveRenderer::paint(QPainter * painter)
{
    painter->fillRect(QRectF { 0, 0, width(), height() }, m_compact ? QColor { "#1a1a1a" } : QColor { "#111111" });
    const auto plot = plotRect();
    painter->setRenderHint(QPainter::Antialiasing, true);
    if (m_compact) {
        // Only the middle, where a pitch bend or a pan is centred
        painter->setPen(QPen { QColor { "#333333" }, 1 });
        painter->drawLine(QPointF { plot.left(), plot.center().y() }, QPointF { plot.right(), plot.center().y() });
    } else {
        painter->fillRect(plot, QColor { "#1a1a1a" });
        paintGrid(*painter, plot);
    }

    if (m_nodes.isEmpty()) {
        return;
    }

    QPainterPath curve;
    for (int i = 0; i < m_nodes.size(); i++) {
        const QPointF point { xForLine(nodeLine(m_nodes.at(i))), yForValue(nodeValue(m_nodes.at(i))) };
        if (i == 0) {
            curve.moveTo(point);
        } else {
            curve.lineTo(point);
        }
    }

    QPainterPath filled = curve;
    filled.lineTo(curve.currentPosition().x(), plot.bottom());
    filled.lineTo(xForLine(nodeLine(m_nodes.first())), plot.bottom());
    filled.closeSubpath();
    QColor fill = m_accentColor;
    fill.setAlpha(40);
    painter->fillPath(filled, fill);

    painter->setPen(QPen { m_accentColor, m_compact ? 1.5 : 2.0 });
    painter->drawPath(curve);
    if (m_compact) {
        return;
    }

    for (int i = 0; i < m_nodes.size(); i++) {
        const QPointF point { xForLine(nodeLine(m_nodes.at(i))), yForValue(nodeValue(m_nodes.at(i))) };
        const bool hovered = i == m_hoveredNode;
        // The ends hollow: they move only up and down, and cannot be removed
        const bool isEnd = i == 0 || i == m_nodes.size() - 1;
        painter->setPen(QPen { hovered ? Qt::white : m_accentColor, 1.5 });
        painter->setBrush(isEnd ? QColor { "#1a1a1a" } : (hovered ? QColor { Qt::white } : m_accentColor));
        const qreal radius = hovered ? hoveredNodeRadius : nodeRadius;
        painter->drawEllipse(point, radius, radius);
    }
    painter->setBrush(Qt::NoBrush);
}

} // namespace noteahead
