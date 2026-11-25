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

#include "../../../application/service/song_overview_builder.hpp"

#include <QColor>
#include <QList>
#include <QPointer>
#include <QQuickPaintedItem>
#include <QVariantMap>

namespace noteahead {

class SongOverviewController;

//! Paints the Song Overview: a lane per track, the song's timeline running across them in play
//! order, each lane's notes as a piano roll, and the play order as a strip of pattern blocks on top.
//!
//! Content coordinates put the track names in a column at the left and the strip in a band at the
//! top; both stay in place while the rest scrolls, the way a spreadsheet freezes its headers. Only
//! what is in view is painted: the item is as large as its view and is told how far the view has
//! scrolled, so a long song does not become a texture of tens of thousands of pixels. Everything
//! that moves during playback (the playhead, the active position) is QML laid over it, placed
//! through songLineX(), so that playing never repaints the notes.
class SongOverviewRenderer : public QQuickPaintedItem
{
    Q_OBJECT

    Q_PROPERTY(QObject * controller READ controller WRITE setController NOTIFY controllerChanged)
    Q_PROPERTY(QColor accentColor READ accentColor WRITE setAccentColor NOTIFY accentColorChanged)
    //! Whether the automations are drawn over the notes.
    Q_PROPERTY(bool automationsVisible READ automationsVisible WRITE setAutomationsVisible NOTIFY automationsVisibleChanged)
    //! The palette the editor draws its automation curves in, picked from by a curve's colorIndex.
    Q_PROPERTY(QVariantList automationCurveColors READ automationCurveColors WRITE setAutomationCurveColors NOTIFY automationCurveColorsChanged)
    //! How far the view has scrolled: the content point at the item's top left.
    Q_PROPERTY(qreal scrollX READ scrollX WRITE setScrollX NOTIFY scrollChanged)
    Q_PROPERTY(qreal scrollY READ scrollY WRITE setScrollY NOTIFY scrollChanged)
    //! The active position, whose block in the strip is tinted. Changes once per position, not per
    //! line, so it is painted here rather than laid over the text.
    Q_PROPERTY(int activeRow READ activeRow WRITE setActiveRow NOTIFY activeRowChanged)

    //! Set by the view: lanes grow to fill it when there are few tracks, within the limits below.
    Q_PROPERTY(qreal laneHeight READ laneHeight WRITE setLaneHeight NOTIFY layoutChanged)
    Q_PROPERTY(qreal minLaneHeight READ minLaneHeight CONSTANT)
    Q_PROPERTY(qreal maxLaneHeight READ maxLaneHeight CONSTANT)
    Q_PROPERTY(qreal laneSpacing READ laneSpacing CONSTANT)
    //! Set by the view: the zoom. The view fits the song to its width when it can.
    Q_PROPERTY(qreal pixelsPerLine READ pixelsPerLine WRITE setPixelsPerLine NOTIFY layoutChanged)
    Q_PROPERTY(qreal minPixelsPerLine READ minPixelsPerLine CONSTANT)
    Q_PROPERTY(qreal maxPixelsPerLine READ maxPixelsPerLine CONSTANT)

    //! The frozen track column and play order strip.
    Q_PROPERTY(qreal trackColumnWidth READ trackColumnWidth CONSTANT)
    Q_PROPERTY(qreal stripHeight READ stripHeight CONSTANT)

    Q_PROPERTY(qreal contentWidth READ contentWidth NOTIFY layoutChanged)
    Q_PROPERTY(qreal contentHeight READ contentHeight NOTIFY layoutChanged)
    //! The song's length in lines, so the view can work out what fits.
    Q_PROPERTY(qreal totalLines READ totalLines NOTIFY totalLinesChanged)

public:
    explicit SongOverviewRenderer(QQuickItem * parent = nullptr);

    QObject * controller() const;
    void setController(QObject * controller);

    QColor accentColor() const;
    void setAccentColor(const QColor & color);

    bool automationsVisible() const;
    void setAutomationsVisible(bool visible);

    QVariantList automationCurveColors() const;
    void setAutomationCurveColors(const QVariantList & colors);

    qreal scrollX() const;
    void setScrollX(qreal scrollX);
    qreal scrollY() const;
    void setScrollY(qreal scrollY);

    int activeRow() const;
    void setActiveRow(int activeRow);

    qreal laneHeight() const;
    void setLaneHeight(qreal laneHeight);
    qreal minLaneHeight() const;
    qreal maxLaneHeight() const;
    qreal laneSpacing() const;

    qreal pixelsPerLine() const;
    void setPixelsPerLine(qreal pixelsPerLine);
    qreal minPixelsPerLine() const;
    qreal maxPixelsPerLine() const;

    qreal trackColumnWidth() const;
    qreal stripHeight() const;
    qreal contentWidth() const;
    qreal contentHeight() const;
    qreal totalLines() const;

    //! The content x of @p songLine on the timeline, and the content y of @p lane's top.
    Q_INVOKABLE qreal songLineX(double songLine) const;
    Q_INVOKABLE qreal laneY(int lane) const;
    //! The content y of the middle of @p pitch's bar in @p lane.
    Q_INVOKABLE qreal pitchY(int lane, int pitch) const;

    //! What is under view point (@p x, @p y): SongOverviewController::targetAt()'s map, plus row
    //! and lane, and for a note its bar as barX, barY, barWidth and barHeight in content coordinates.
    //! On a track's name: valid, isTrack and track.
    Q_INVOKABLE QVariantMap hitTest(qreal x, qreal y) const;

    //! The colour of the track at @p trackPosition.
    Q_INVOKABLE QColor trackColor(int trackPosition) const;

    void paint(QPainter * painter) override;

signals:
    void controllerChanged();
    void accentColorChanged();
    void automationsVisibleChanged();
    void automationCurveColorsChanged();
    void scrollChanged();
    void activeRowChanged();
    void layoutChanged();
    //! Apart from layoutChanged(), which setting the zoom emits: the zoom is bound to this.
    void totalLinesChanged();

private:
    struct PitchScale
    {
        double top { 0.0 };
        double pixelsPerSemitone { 1.0 };
        double highest { 0.0 };
    };

    //! Where pitches land in the lane at @p lane.
    PitchScale pitchScale(size_t lane) const;

    void paintStrip(QPainter * painter) const;
    void paintTrackColumn(QPainter * painter) const;
    void paintLanes(QPainter * painter) const;
    //! The curves of @p track in the row starting at @p rowStart, over the lane at @p top.
    void paintCurves(QPainter * painter, const std::vector<SongOverview::Curve> & curves, size_t track, double rowStart, double top, double firstLine, double lastLine) const;

    QPointer<SongOverviewController> m_controller;
    QColor m_accentColor { "#ffaa00" };
    bool m_automationsVisible { true };
    QList<QColor> m_automationCurveColors;
    qreal m_scrollX { 0.0 };
    qreal m_scrollY { 0.0 };
    int m_activeRow { -1 };
    qreal m_laneHeight { 60.0 };
    qreal m_pixelsPerLine { 4.0 };
};

} // namespace noteahead

#endif // SONG_OVERVIEW_RENDERER_HPP
