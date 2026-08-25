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

#include "song_overview_renderer.hpp"

#include <QFontMetricsF>
#include <QPainter>
#include <QPainterPath>
#include <QVariantMap>

#include <algorithm>
#include <cmath>

namespace noteahead {

namespace {

// Node kinds and cell kinds, as SongOverviewService orders them. Kept as plain values because the
// graph arrives from QML as variants, and reaching into the service from a painter would tie the
// two together for nothing.
constexpr int KindDevice = 0;
constexpr int KindSubMixer = 1;
constexpr int KindMaster = 2;
constexpr int KindSendEffect = 3;

constexpr int CellSource = 0;
constexpr int CellMeter = 1;
constexpr int CellFader = 2;
constexpr int CellInsert = 3;
constexpr int CellClip = 4;

constexpr int EdgeDirectOut = 0;
constexpr int EdgeSubMixerMember = 1;
constexpr int EdgeSend = 2;

constexpr qreal Margin = 16.0;
constexpr qreal RowHeight = 92.0;
constexpr qreal RowGap = 16.0;
constexpr qreal ColumnGap = 46.0;
constexpr qreal HeaderHeight = 38.0;
constexpr qreal CellHeight = 26.0;

// Type sizes. The first pass was set from what fitted rather than from what reads, and on a real
// project it was too small to take in at a glance.
constexpr int TitlePixelSize = 14;
constexpr int ChainPixelSize = 12;
constexpr int SmallPixelSize = 11;
constexpr qreal CellGap = 4.0;
constexpr qreal MinCellWidth = 20.0;
constexpr qreal TapRadius = 3.0;
//! Inset of the header text, counted on both sides when a box is sized.
constexpr qreal HeaderPadding = 8.0;

//! How far anything off the hovered route is pushed back. Dimmed rather than hidden: the shape of
//! the rest of the song is still worth seeing behind the path being followed.
constexpr int DimAlpha = 60;

QColor dimmed(QColor color, bool dim)
{
    if (dim) {
        color.setAlpha(DimAlpha);
    }
    return color;
}

const QColor Background { "#141414" };
const QColor BoxFill { "#232323" };
const QColor BoxFillMaster { "#2c2418" };
const QColor BoxBorder { "#3d3d3d" };
const QColor CellFill { "#1a1a1a" };
const QColor TextColor { "#dddddd" };
const QColor SubtleText { "#8a8a8a" };
const QColor LineColor { "#6a6a6a" };
const QColor SuppressedColor { "#4a4a4a" };

//! Width a chain cell wants, so a long effect name is not clipped.
qreal cellWidth(const QFontMetricsF & fm, const QVariantMap & cell)
{
    switch (cell["kind"].toInt()) {
    case CellSource:
    case CellMeter:
    case CellClip:
        return MinCellWidth;
    case CellFader:
        return std::max(MinCellWidth * 2.0, fm.horizontalAdvance("FADER") + 12.0);
    default:
        return std::max(MinCellWidth * 2.0, fm.horizontalAdvance(cell["label"].toString()) + 12.0);
    }
}

} // namespace

SongOverviewRenderer::SongOverviewRenderer(QQuickItem * parent)
  : QQuickPaintedItem { parent }
{
    setAntialiasing(true);
}

QVariantList SongOverviewRenderer::nodes() const
{
    return m_nodes;
}

void SongOverviewRenderer::setNodes(const QVariantList & nodes)
{
    m_nodes = nodes;
    updateLayout();
    emit nodesChanged();
    update();
}

QVariantList SongOverviewRenderer::edges() const
{
    return m_edges;
}

void SongOverviewRenderer::setEdges(const QVariantList & edges)
{
    m_edges = edges;
    updateHighlight();
    emit edgesChanged();
    update();
}

QColor SongOverviewRenderer::accentColor() const
{
    return m_accentColor;
}

void SongOverviewRenderer::setAccentColor(const QColor & color)
{
    if (m_accentColor != color) {
        m_accentColor = color;
        emit accentColorChanged();
        update();
    }
}

int SongOverviewRenderer::hoveredNode() const
{
    return m_hoveredNode;
}

void SongOverviewRenderer::setHoveredNode(int nodeIndex)
{
    if (m_hoveredNode != nodeIndex) {
        m_hoveredNode = nodeIndex;
        updateHighlight();
        emit hoveredNodeChanged();
        update();
    }
}

void SongOverviewRenderer::updateHighlight()
{
    m_highlightedNodes.clear();
    m_highlightedEdges.clear();
    if (m_hoveredNode < 0 || m_hoveredNode >= m_nodes.size()) {
        return;
    }

    // Breadth-first downstream. The graph runs one way and DeviceService forbids cycles among Sub
    // Mixers, so the visited set is belt and braces rather than a necessity.
    std::vector<int> queue { m_hoveredNode };
    m_highlightedNodes.insert(m_hoveredNode);
    while (!queue.empty()) {
        const auto node = queue.back();
        queue.pop_back();
        for (int i = 0; i < m_edges.size(); i++) {
            const auto edge = m_edges.at(i).toMap();
            if (edge["fromNode"].toInt() != node) {
                continue;
            }
            m_highlightedEdges.insert(i);
            if (const auto to = edge["toNode"].toInt(); !m_highlightedNodes.contains(to)) {
                m_highlightedNodes.insert(to);
                queue.push_back(to);
            }
        }
    }
}

QVariantList SongOverviewRenderer::boxRects() const
{
    return m_boxRects;
}

qreal SongOverviewRenderer::contentWidth() const
{
    return m_contentWidth;
}

qreal SongOverviewRenderer::contentHeight() const
{
    return m_contentHeight;
}

QString SongOverviewRenderer::titleText(const QVariantMap & node) const
{
    const auto kind = node["kind"].toInt();
    return kind == KindDevice || kind == KindSubMixer
      ? QString { "%1  %2" }.arg(node["slot"].toInt() + 1).arg(node["name"].toString())
      : node["name"].toString();
}

QString SongOverviewRenderer::notesText(const QVariantMap & node) const
{
    QStringList notes;
    if (const auto tracks = node["trackNames"].toString(); !tracks.isEmpty()) {
        notes << tracks;
    }
    // Said only when it differs from what a device starts with, so the header stays about what is
    // unusual in this project rather than repeating the defaults on every box.
    if (node["faderPostInserts"].toBool()) {
        notes << tr("fader post-inserts");
    }
    if (node["sendPreFader"].toBool()) {
        notes << tr("sends pre-fader");
    }
    return notes.join(" · ");
}

void SongOverviewRenderer::updateLayout()
{
    m_boxRects.clear();
    m_contentWidth = 0.0;
    m_contentHeight = 0.0;

    QFont chainFont;
    chainFont.setPixelSize(ChainPixelSize);
    const QFontMetricsF fm { chainFont };

    // The header is drawn larger than the chain, and in bold, so measuring it with the chain's
    // metrics came up short and clipped the titles of boxes whose chain was shorter than their name.
    QFont titleFont;
    titleFont.setPixelSize(TitlePixelSize);
    titleFont.setBold(true);
    const QFontMetricsF titleFm { titleFont };
    QFont smallFont;
    smallFont.setPixelSize(SmallPixelSize);
    const QFontMetricsF smallFm { smallFont };

    // Each column is as wide as its widest box, so a device with eight inserts does not push the
    // ones beside it off the diagram.
    std::map<int, qreal> columnWidth;
    std::vector<qreal> nodeWidth(static_cast<size_t>(m_nodes.size()), 0.0);
    for (int i = 0; i < m_nodes.size(); i++) {
        const auto node = m_nodes.at(i).toMap();
        qreal width = 2.0 * CellGap;
        // A send node draws no chain -- its title names the effect -- so nothing but the header
        // should be deciding how wide it is.
        if (node["kind"].toInt() != KindSendEffect) {
            for (const auto & cellVariant : node["chain"].toList()) {
                width += cellWidth(fm, cellVariant.toMap()) + CellGap;
            }
        }
        width = std::max(width, titleFm.horizontalAdvance(titleText(node)) + 2.0 * HeaderPadding);
        width = std::max(width, smallFm.horizontalAdvance(notesText(node)) + 2.0 * HeaderPadding);
        width = std::max(width, 120.0);
        nodeWidth[static_cast<size_t>(i)] = width;
        const auto column = node["column"].toInt();
        columnWidth[column] = std::max(columnWidth.contains(column) ? columnWidth[column] : 0.0, width);
    }

    std::map<int, qreal> columnX;
    qreal x = Margin;
    for (const auto & [column, width] : columnWidth) {
        columnX[column] = x;
        x += width + ColumnGap;
    }
    m_contentWidth = x - ColumnGap + Margin;

    for (int i = 0; i < m_nodes.size(); i++) {
        const auto node = m_nodes.at(i).toMap();
        const auto column = node["column"].toInt();
        const auto row = node["row"].toInt();
        const qreal boxY = Margin + static_cast<qreal>(row) * (RowHeight + RowGap);
        QVariantMap rect;
        rect["x"] = columnX.contains(column) ? columnX[column] : Margin;
        rect["y"] = boxY;
        rect["width"] = nodeWidth[static_cast<size_t>(i)];
        rect["height"] = RowHeight;
        m_boxRects.append(rect);
        m_contentHeight = std::max(m_contentHeight, boxY + RowHeight + Margin);
    }

    emit boxRectsChanged();
}

void SongOverviewRenderer::paint(QPainter * painter)
{
    painter->fillRect(boundingRect(), Background);
    if (m_nodes.isEmpty() || m_boxRects.size() != m_nodes.size()) {
        return;
    }

    QFont titleFont;
    titleFont.setPixelSize(TitlePixelSize);
    titleFont.setBold(true);
    QFont chainFont;
    chainFont.setPixelSize(ChainPixelSize);
    QFont smallFont;
    smallFont.setPixelSize(SmallPixelSize);
    const QFontMetricsF chainFm { chainFont };

    const auto rectOf = [&](int index) {
        const auto map = m_boxRects.at(index).toMap();
        return QRectF { map["x"].toReal(), map["y"].toReal(), map["width"].toReal(), map["height"].toReal() };
    };

    // Where a given cell of a given node sits, so an edge can leave from the tap it belongs to
    // rather than from the edge of the box.
    const auto cellX = [&](int nodeIndex, int cellIndex) {
        const auto box = rectOf(nodeIndex);
        const auto chain = m_nodes.at(nodeIndex).toMap()["chain"].toList();
        qreal cursor = box.left() + CellGap;
        for (int i = 0; i < chain.size() && i < cellIndex; i++) {
            cursor += cellWidth(chainFm, chain.at(i).toMap()) + CellGap;
        }
        return std::min(cursor, box.right());
    };

    // --- edges first, so the boxes sit on top of them -------------------------------------------
    const bool hovering = m_hoveredNode >= 0;
    for (int edgeIndex = 0; edgeIndex < m_edges.size(); edgeIndex++) {
        const auto edge = m_edges.at(edgeIndex).toMap();
        const bool lit = !hovering || m_highlightedEdges.contains(edgeIndex);
        const auto from = edge["fromNode"].toInt();
        const auto to = edge["toNode"].toInt();
        if (from < 0 || to < 0 || from >= m_nodes.size() || to >= m_nodes.size()) {
            continue;
        }
        const auto fromBox = rectOf(from);
        const auto toBox = rectOf(to);
        const auto kind = edge["kind"].toInt();
        const bool isSend = kind == EdgeSend;

        QPointF start { fromBox.right(), fromBox.center().y() };
        if (isSend) {
            // Leaves from the tap itself. That is the whole point of drawing sends here: a
            // pre-fader send visibly comes off the chain before the fader.
            start = QPointF { cellX(from, edge["tapCellIndex"].toInt()), fromBox.bottom() };
        }
        const QPointF end { toBox.left(), toBox.center().y() };

        QPen pen { isSend ? m_accentColor : LineColor, isSend ? 1.0 : 1.6 };
        if (kind == EdgeDirectOut && m_nodes.at(from).toMap()["kind"].toInt() == KindSendEffect) {
            pen = QPen { m_accentColor, 1.0 };
        }
        // On the hovered route the line is drawn in the accent and thicker, which is what lets one
        // device be followed through the bundle the master attracts.
        if (hovering && lit) {
            pen = QPen { m_accentColor, pen.widthF() + 1.0 };
        }
        pen.setColor(dimmed(pen.color(), hovering && !lit));
        painter->setPen(pen);

        QPainterPath path;
        path.moveTo(start);
        if (isSend) {
            const qreal drop = std::max(start.y() + 18.0, end.y());
            path.lineTo(start.x(), drop);
            path.lineTo(end.x() - 10.0, drop);
            path.lineTo(end.x() - 10.0, end.y());
            path.lineTo(end);
        } else {
            const qreal mid = (start.x() + end.x()) * 0.5;
            path.cubicTo(QPointF { mid, start.y() }, QPointF { mid, end.y() }, end);
        }
        painter->drawPath(path);

        // Arrow head
        painter->setBrush(pen.color());
        const QPointF tip = end;
        painter->drawPolygon(QPolygonF { { tip, tip + QPointF { -6.0, -3.5 }, tip + QPointF { -6.0, 3.5 } } });
        painter->setBrush(Qt::NoBrush);

        if (isSend) {
            painter->setFont(smallFont);
            painter->setPen(dimmed(m_accentColor, hovering && !lit));
            const auto label = QString { "%1%2%" }
                                 .arg(edge["preFader"].toBool() ? "pre " : "")
                                 .arg(static_cast<int>(std::round(edge["amount"].toReal() * 100.0)));
            painter->drawText(QPointF { start.x() + 3.0, start.y() + 12.0 }, label);
        }
    }

    // --- nodes ----------------------------------------------------------------------------------
    for (int i = 0; i < m_nodes.size(); i++) {
        const auto node = m_nodes.at(i).toMap();
        const auto box = rectOf(i);
        const auto kind = node["kind"].toInt();
        const bool lit = !hovering || m_highlightedNodes.contains(i);
        const bool dim = hovering && !lit;

        // The hovered device itself gets the accent border, so it is clear which one the lit route
        // is leaving from rather than merely that a route is lit.
        const auto border = i == m_hoveredNode || kind == KindMaster ? m_accentColor : BoxBorder;
        painter->setPen(QPen { dimmed(border, dim), i == m_hoveredNode ? 1.6 : 1.0 });
        painter->setBrush(dimmed(kind == KindMaster ? BoxFillMaster : BoxFill, dim));
        painter->drawRoundedRect(box, 4.0, 4.0);
        painter->setBrush(Qt::NoBrush);

        painter->setFont(titleFont);
        painter->setPen(dimmed(kind == KindMaster ? m_accentColor : TextColor, dim));
        painter->drawText(QRectF { box.left() + HeaderPadding, box.top() + 5.0, box.width() - 2.0 * HeaderPadding, 18.0 },
                          Qt::AlignLeft | Qt::AlignVCenter, titleText(node));

        // Tracks, and the tap settings when they are not the ones a device starts with. Saying so
        // only when it differs keeps the diagram about what is unusual in this project.
        painter->setFont(smallFont);
        painter->setPen(dimmed(SubtleText, dim));
        if (const auto notes = notesText(node); !notes.isEmpty()) {
            painter->drawText(QRectF { box.left() + HeaderPadding, box.top() + 23.0, box.width() - 2.0 * HeaderPadding, 14.0 },
                              Qt::AlignLeft | Qt::AlignVCenter, notes);
        }

        // A send node is a destination rather than a chain: its title already names the effect, so
        // drawing the one cell as well would just say it twice.
        if (kind == KindSendEffect) {
            continue;
        }

        // The chain, left to right in the order the engine runs it.
        painter->setFont(chainFont);
        qreal cursor = box.left() + CellGap;
        const auto chain = node["chain"].toList();
        const qreal chainY = box.top() + HeaderHeight + 6.0;
        for (const auto & cellVariant : chain) {
            const auto cell = cellVariant.toMap();
            const auto cellKind = cell["kind"].toInt();
            const qreal width = cellWidth(chainFm, cell);
            const QRectF cellRect { cursor, chainY, width, CellHeight };

            if (cellKind == CellMeter || cellKind == CellClip || cellKind == CellSource) {
                // Taps are dots on the path rather than boxes: they measure, they do not process.
                painter->setPen(QPen { dimmed(cellKind == CellSource ? TextColor : SubtleText, dim), 1.0 });
                painter->setBrush(dimmed(cellKind == CellSource ? TextColor : SubtleText, dim));
                painter->drawEllipse(cellRect.center(), TapRadius, TapRadius);
                painter->setBrush(Qt::NoBrush);
            } else {
                const bool isFader = cellKind == CellFader;
                painter->setPen(QPen { dimmed(isFader ? m_accentColor : BoxBorder, dim), isFader ? 1.4 : 1.0 });
                painter->setBrush(dimmed(CellFill, dim));
                painter->drawRoundedRect(cellRect, 2.0, 2.0);
                painter->setBrush(Qt::NoBrush);
                painter->setPen(dimmed(isFader ? m_accentColor : TextColor, dim));
                painter->drawText(cellRect, Qt::AlignCenter,
                                  isFader ? tr("FADER") : cell["label"].toString());
            }

            // The line joining one step to the next.
            if (cursor > box.left() + CellGap) {
                painter->setPen(QPen { dimmed(LineColor, dim), 1.0 });
                painter->drawLine(QPointF { cursor - CellGap, chainY + CellHeight * 0.5 },
                                  QPointF { cursor, chainY + CellHeight * 0.5 });
            }
            cursor += width + CellGap;
        }
    }
}

} // namespace noteahead
