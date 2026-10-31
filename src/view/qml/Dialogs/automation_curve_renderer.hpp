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

#ifndef AUTOMATION_CURVE_RENDERER_HPP
#define AUTOMATION_CURVE_RENDERER_HPP

#include <QColor>
#include <QQuickPaintedItem>
#include <QRectF>
#include <QVariantList>

namespace noteahead {

//! Draws a drawn automation: its nodes, the straight lines between them, and the pattern's lines
//! and beats behind them.
//!
//! The mapping between pixels and lines and values lives here, with the drawing, so the place a
//! click lands on and the place a node is drawn at cannot disagree. The mouse itself is QML's: the
//! view forwards it to the curve editor through the mapping below.
class AutomationCurveRenderer : public QQuickPaintedItem
{
    Q_OBJECT

    //! [{ line, value }, ...], sorted by line.
    Q_PROPERTY(QVariantList nodes READ nodes WRITE setNodes NOTIFY nodesChanged)
    Q_PROPERTY(double line0 READ line0 WRITE setLine0 NOTIFY rangeChanged)
    Q_PROPERTY(double line1 READ line1 WRITE setLine1 NOTIFY rangeChanged)
    Q_PROPERTY(double minValue READ minValue WRITE setMinValue NOTIFY rangeChanged)
    Q_PROPERTY(double maxValue READ maxValue WRITE setMaxValue NOTIFY rangeChanged)
    //! Every this many lines a beat line is drawn stronger and numbered.
    Q_PROPERTY(int linesPerBeat READ linesPerBeat WRITE setLinesPerBeat NOTIFY rangeChanged)
    //! The node under the mouse, or -1.
    Q_PROPERTY(int hoveredNode READ hoveredNode WRITE setHoveredNode NOTIFY hoveredNodeChanged)
    Q_PROPERTY(QColor accentColor READ accentColor WRITE setAccentColor NOTIFY accentColorChanged)
    //! Just the curve, edge to edge: no grid, numbers or nodes. A preview one line of text tall.
    Q_PROPERTY(bool compact READ compact WRITE setCompact NOTIFY compactChanged)

public:
    explicit AutomationCurveRenderer(QQuickItem * parent = nullptr);

    QVariantList nodes() const;
    void setNodes(const QVariantList & nodes);

    double line0() const;
    void setLine0(double line0);
    double line1() const;
    void setLine1(double line1);

    double minValue() const;
    void setMinValue(double minValue);
    double maxValue() const;
    void setMaxValue(double maxValue);

    int linesPerBeat() const;
    void setLinesPerBeat(int linesPerBeat);

    int hoveredNode() const;
    void setHoveredNode(int hoveredNode);

    QColor accentColor() const;
    void setAccentColor(const QColor & color);

    bool compact() const;
    void setCompact(bool compact);

    //! The area the curve is drawn in, inside the margins.
    QRectF plotRect() const;

    Q_INVOKABLE double lineAt(qreal x) const;
    Q_INVOKABLE double valueAt(qreal y) const;
    Q_INVOKABLE qreal xForLine(double line) const;
    Q_INVOKABLE qreal yForValue(double value) const;

    //! The node within grabbing distance of a point, the nearest when several are, or -1.
    Q_INVOKABLE int nodeAt(qreal x, qreal y) const;

    void paint(QPainter * painter) override;

signals:
    void nodesChanged();
    void rangeChanged();
    void hoveredNodeChanged();
    void accentColorChanged();
    void compactChanged();

private:
    void paintGrid(QPainter & painter, const QRectF & plot) const;

    QVariantList m_nodes;
    double m_line0 = 0.0;
    double m_line1 = 1.0;
    double m_minValue = 0.0;
    double m_maxValue = 1.0;
    int m_linesPerBeat = 4;
    int m_hoveredNode = -1;
    QColor m_accentColor { 0, 180, 255 };
    bool m_compact = false;
};

} // namespace noteahead

#endif // AUTOMATION_CURVE_RENDERER_HPP
