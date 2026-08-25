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

#ifndef SONG_OVERVIEW_RENDERER_HPP
#define SONG_OVERVIEW_RENDERER_HPP

#include <QColor>
#include <QQuickPaintedItem>
#include <QVariantList>
#include <QVariantMap>

#include <set>

namespace noteahead {

//! Paints the song's signal flow: a box per device, its chain drawn left to right in the order the
//! engine runs it, and lines for what feeds what.
//!
//! Takes the graph already laid out in grid coordinates and only turns it into pixels. The rects it
//! works out are handed back through boxRects() so that QML can put its own mouse areas over them:
//! hit testing stays declarative, where the rest of this application keeps it.
class SongOverviewRenderer : public QQuickPaintedItem
{
    Q_OBJECT

    Q_PROPERTY(QVariantList nodes READ nodes WRITE setNodes NOTIFY nodesChanged)
    Q_PROPERTY(QVariantList edges READ edges WRITE setEdges NOTIFY edgesChanged)
    Q_PROPERTY(QColor accentColor READ accentColor WRITE setAccentColor NOTIFY accentColorChanged)
    //! Node the pointer is over, or -1. Everything downstream of it is lit and the rest is dimmed,
    //! which is the only way to follow one device through the bundle of lines converging on the
    //! master once a project has a dozen of them.
    Q_PROPERTY(int hoveredNode READ hoveredNode WRITE setHoveredNode NOTIFY hoveredNodeChanged)
    //! One entry per node, in node order: { x, y, width, height }. Read-only.
    Q_PROPERTY(QVariantList boxRects READ boxRects NOTIFY boxRectsChanged)
    //! What the graph needs to be drawn in full, so the view can scroll rather than crop it.
    Q_PROPERTY(qreal contentWidth READ contentWidth NOTIFY boxRectsChanged)
    Q_PROPERTY(qreal contentHeight READ contentHeight NOTIFY boxRectsChanged)

public:
    explicit SongOverviewRenderer(QQuickItem * parent = nullptr);

    QVariantList nodes() const;
    void setNodes(const QVariantList & nodes);

    QVariantList edges() const;
    void setEdges(const QVariantList & edges);

    QColor accentColor() const;
    void setAccentColor(const QColor & color);

    int hoveredNode() const;
    void setHoveredNode(int nodeIndex);

    QVariantList boxRects() const;
    qreal contentWidth() const;
    qreal contentHeight() const;

    void paint(QPainter * painter) override;

signals:
    void nodesChanged();
    void edgesChanged();
    void accentColorChanged();
    void boxRectsChanged();
    void hoveredNodeChanged();

private:
    //! Works out every node's rect from its grid position and the width its own chain needs, so a
    //! device with a long insert chain is not cut off by a column sized for the shortest one.
    void updateLayout();

    //! Everything reachable downstream of the hovered node: its own outgoing edges, whatever they
    //! reach, and onward. A device's sends and its path through a Sub Mixer both end at the master,
    //! so following the graph rather than the one hop is what makes the whole route visible.
    void updateHighlight();

    //! The two lines of a node's header, built in one place so that what is measured when the box
    //! is sized is exactly what is drawn into it.
    QString titleText(const QVariantMap & node) const;
    QString notesText(const QVariantMap & node) const;

    QVariantList m_nodes;
    QVariantList m_edges;
    QVariantList m_boxRects;
    QColor m_accentColor { "#ffaa00" };
    int m_hoveredNode { -1 };
    std::set<int> m_highlightedNodes;
    std::set<int> m_highlightedEdges;
    qreal m_contentWidth { 0.0 };
    qreal m_contentHeight { 0.0 };
};

} // namespace noteahead

#endif // SONG_OVERVIEW_RENDERER_HPP
