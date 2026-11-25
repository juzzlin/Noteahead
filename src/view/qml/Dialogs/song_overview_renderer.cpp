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

#include "../../../application/note_converter.hpp"
#include "../../controllers/song_overview_controller.hpp"

#include <QFontMetricsF>
#include <QPainter>

#include <algorithm>
#include <cmath>

namespace noteahead {

namespace {

constexpr qreal MinLaneHeight { 40.0 };
constexpr qreal MaxLaneHeight { 160.0 };
constexpr qreal LaneSpacing { 2.0 };
constexpr qreal MinPixelsPerLine { 0.5 };
constexpr qreal MaxPixelsPerLine { 32.0 };
constexpr qreal TrackColumnWidth { 150.0 };
constexpr qreal StripHeight { 40.0 };
constexpr qreal RightPadding { 8.0 };
//! Keeps the highest and the lowest note off the lane's edges.
constexpr qreal NotePadding { 3.0 };
//! A bar is never thinner than this, however wide the lane's range.
constexpr qreal MinBarHeight { 2.0 };
//! Black keys are shaded only when a semitone has room to show.
constexpr qreal MinKeyShadingPixels { 2.0 };
//! Grid lines closer than this are left out: at a low zoom, beats would only be a grey wash.
constexpr qreal MinGridSpacingPixels { 6.0 };
constexpr int BeatsPerBar { 4 };
//! A click this close to a bar, in pixels, is on it.
constexpr qreal HitSlopPixels { 3.0 };
//! Keeps a curve's top and bottom off the lane's edges.
constexpr qreal CurvePadding { 4.0 };
constexpr qreal CurveThickness { 1.5 };
//! Over the notes, but not hiding them.
constexpr double CurveAlpha { 0.85 };

const QColor CellFill { "#232323" };
const QColor CornerFill { "#1e1e1e" };
const QColor RollFill { "#191919" };
const QColor BlackKeyFill { "#151515" };
const QColor PastEndFill { "#111111" };
const QColor BeatLine { "#262626" };
const QColor BarLine { "#363636" };
const QColor PositionLine { "#505050" };
const QColor TextColor { "#dddddd" };
const QColor SubtleText { "#8a8a8a" };
const QColor FaintText { "#5a5a5a" };
const QColor SkippedShade { 0, 0, 0, 150 };

bool isBlackKey(int pitch)
{
    switch (pitch % 12) {
    case 1:
    case 3:
    case 6:
    case 8:
    case 10:
        return true;
    default:
        return false;
    }
}

} // namespace

SongOverviewRenderer::SongOverviewRenderer(QQuickItem * parent)
  : QQuickPaintedItem { parent }
{
    setAntialiasing(true);
}

QObject * SongOverviewRenderer::controller() const
{
    return m_controller;
}

void SongOverviewRenderer::setController(QObject * controller)
{
    const auto songOverviewController = qobject_cast<SongOverviewController *>(controller);
    if (m_controller == songOverviewController) {
        return;
    }
    if (m_controller) {
        disconnect(m_controller, nullptr, this, nullptr);
    }
    m_controller = songOverviewController;
    if (m_controller) {
        connect(m_controller, &SongOverviewController::overviewChanged, this, [this] {
            emit totalLinesChanged();
            emit layoutChanged();
            update();
        });
        connect(m_controller, &SongOverviewController::mixChanged, this, [this] { update(); });
    }
    emit controllerChanged();
    emit totalLinesChanged();
    emit layoutChanged();
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

bool SongOverviewRenderer::automationsVisible() const
{
    return m_automationsVisible;
}

void SongOverviewRenderer::setAutomationsVisible(bool visible)
{
    if (m_automationsVisible != visible) {
        m_automationsVisible = visible;
        emit automationsVisibleChanged();
        update();
    }
}

QVariantList SongOverviewRenderer::automationCurveColors() const
{
    QVariantList colors;
    for (auto && color : m_automationCurveColors) {
        colors.append(color);
    }
    return colors;
}

void SongOverviewRenderer::setAutomationCurveColors(const QVariantList & colors)
{
    QList<QColor> list;
    for (auto && color : colors) {
        list.append(qvariant_cast<QColor>(color));
    }
    if (m_automationCurveColors != list) {
        m_automationCurveColors = list;
        emit automationCurveColorsChanged();
        update();
    }
}

qreal SongOverviewRenderer::scrollX() const
{
    return m_scrollX;
}

void SongOverviewRenderer::setScrollX(qreal scrollX)
{
    if (m_scrollX != scrollX) {
        m_scrollX = scrollX;
        emit scrollChanged();
        update();
    }
}

qreal SongOverviewRenderer::scrollY() const
{
    return m_scrollY;
}

void SongOverviewRenderer::setScrollY(qreal scrollY)
{
    if (m_scrollY != scrollY) {
        m_scrollY = scrollY;
        emit scrollChanged();
        update();
    }
}

int SongOverviewRenderer::activeRow() const
{
    return m_activeRow;
}

void SongOverviewRenderer::setActiveRow(int activeRow)
{
    if (m_activeRow != activeRow) {
        m_activeRow = activeRow;
        emit activeRowChanged();
        update();
    }
}

qreal SongOverviewRenderer::laneHeight() const
{
    return m_laneHeight;
}

void SongOverviewRenderer::setLaneHeight(qreal laneHeight)
{
    laneHeight = std::clamp(std::floor(laneHeight), MinLaneHeight, MaxLaneHeight);
    if (m_laneHeight != laneHeight) {
        m_laneHeight = laneHeight;
        emit layoutChanged();
        update();
    }
}

qreal SongOverviewRenderer::minLaneHeight() const
{
    return MinLaneHeight;
}

qreal SongOverviewRenderer::maxLaneHeight() const
{
    return MaxLaneHeight;
}

qreal SongOverviewRenderer::laneSpacing() const
{
    return LaneSpacing;
}

qreal SongOverviewRenderer::pixelsPerLine() const
{
    return m_pixelsPerLine;
}

void SongOverviewRenderer::setPixelsPerLine(qreal pixelsPerLine)
{
    pixelsPerLine = std::clamp(pixelsPerLine, MinPixelsPerLine, MaxPixelsPerLine);
    if (m_pixelsPerLine != pixelsPerLine) {
        m_pixelsPerLine = pixelsPerLine;
        emit layoutChanged();
        update();
    }
}

qreal SongOverviewRenderer::minPixelsPerLine() const
{
    return MinPixelsPerLine;
}

qreal SongOverviewRenderer::maxPixelsPerLine() const
{
    return MaxPixelsPerLine;
}

qreal SongOverviewRenderer::trackColumnWidth() const
{
    return TrackColumnWidth;
}

qreal SongOverviewRenderer::stripHeight() const
{
    return StripHeight;
}

qreal SongOverviewRenderer::contentWidth() const
{
    return TrackColumnWidth + totalLines() * m_pixelsPerLine + RightPadding;
}

qreal SongOverviewRenderer::contentHeight() const
{
    const auto lanes = m_controller ? m_controller->overview().tracks.size() : 0;
    return StripHeight + (lanes ? static_cast<qreal>(lanes) * (m_laneHeight + LaneSpacing) : 0.0);
}

qreal SongOverviewRenderer::totalLines() const
{
    return m_controller ? static_cast<qreal>(m_controller->overview().totalLines) : 0.0;
}

qreal SongOverviewRenderer::songLineX(double songLine) const
{
    return TrackColumnWidth + songLine * m_pixelsPerLine;
}

qreal SongOverviewRenderer::laneY(int lane) const
{
    return StripHeight + static_cast<qreal>(lane) * (m_laneHeight + LaneSpacing);
}

qreal SongOverviewRenderer::pitchY(int lane, int pitch) const
{
    if (!m_controller || lane < 0 || static_cast<size_t>(lane) >= m_controller->overview().tracks.size()) {
        return laneY(std::max(lane, 0)) + m_laneHeight / 2.0;
    }
    const auto scale = pitchScale(static_cast<size_t>(lane));
    return scale.top + (scale.highest - pitch + 0.5) * scale.pixelsPerSemitone;
}

SongOverviewRenderer::PitchScale SongOverviewRenderer::pitchScale(size_t lane) const
{
    const auto & track = m_controller->overview().tracks.at(lane);
    const double semitones = static_cast<double>(track.highestPitch - track.lowestPitch) + 1.0;
    return {
        .top = laneY(static_cast<int>(lane)) + NotePadding,
        .pixelsPerSemitone = (m_laneHeight - 2.0 * NotePadding) / semitones,
        .highest = static_cast<double>(track.highestPitch)
    };
}

QColor SongOverviewRenderer::trackColor(int trackPosition) const
{
    // Golden-ratio steps round the hue circle: neighbours never look alike, however many tracks
    const double hue = std::fmod(0.08 + static_cast<double>(std::max(trackPosition, 0)) * 0.6180339887, 1.0);
    return QColor::fromHsvF(static_cast<float>(hue), 0.55f, 0.95f);
}

QVariantMap SongOverviewRenderer::hitTest(qreal x, qreal y) const
{
    const QVariantMap invalid { { "valid", false } };
    if (!m_controller) {
        return invalid;
    }
    const auto & overview = m_controller->overview();

    // On a track's name
    if (x < TrackColumnWidth) {
        const double contentY = y + m_scrollY;
        const int lane = static_cast<int>((contentY - StripHeight) / (m_laneHeight + LaneSpacing));
        if (y < StripHeight || lane < 0 || static_cast<size_t>(lane) >= overview.tracks.size() || contentY - laneY(lane) > m_laneHeight) {
            return invalid;
        }
        return { { "valid", true }, { "isTrack", true }, { "lane", lane }, { "track", static_cast<quint64>(overview.tracks.at(static_cast<size_t>(lane)).index) } };
    }
    const double songLine = (x + m_scrollX - TrackColumnWidth) / m_pixelsPerLine;
    const auto row = overview.rowAt(songLine);
    if (!row.has_value()) {
        return invalid;
    }
    const double line = songLine - static_cast<double>(overview.rows.at(*row).startLine);

    // On the play order strip: the position's first line
    if (y < StripHeight) {
        auto target = m_controller->targetAt(static_cast<int>(*row), -1, 0.0, 0.0, 0.0);
        target["row"] = static_cast<int>(*row);
        target["lane"] = -1;
        return target;
    }

    const double contentY = y + m_scrollY;
    const int lane = static_cast<int>((contentY - StripHeight) / (m_laneHeight + LaneSpacing));
    if (lane < 0 || static_cast<size_t>(lane) >= overview.tracks.size() || contentY - laneY(lane) > m_laneHeight) {
        return invalid;
    }
    const auto scale = pitchScale(static_cast<size_t>(lane));
    const double pitch = scale.highest - (contentY - scale.top) / scale.pixelsPerSemitone + 0.5;
    const double tolerance = std::max(0.5, HitSlopPixels / scale.pixelsPerSemitone);
    auto target = m_controller->targetAt(static_cast<int>(*row), lane, line, pitch, tolerance);
    target["row"] = static_cast<int>(*row);
    target["lane"] = lane;
    if (target.value("isNote").toBool()) {
        const auto trackIndex = overview.tracks.at(static_cast<size_t>(lane)).index;
        if (const auto note = overview.noteAt(*row, trackIndex, line, pitch, tolerance); note.has_value()) {
            const double barHeight = std::max(scale.pixelsPerSemitone, MinBarHeight);
            target["barX"] = songLineX(static_cast<double>(overview.rows.at(*row).startLine) + note->startLine);
            target["barY"] = scale.top + (scale.highest - note->pitch) * scale.pixelsPerSemitone + (scale.pixelsPerSemitone - barHeight) / 2.0;
            target["barWidth"] = std::max(note->lengthLines * m_pixelsPerLine, 1.0);
            target["barHeight"] = barHeight;
        }
    }
    return target;
}

void SongOverviewRenderer::paintLanes(QPainter * painter) const
{
    const auto & overview = m_controller->overview();
    painter->save();
    painter->setClipRect(QRectF { TrackColumnWidth, StripHeight, width() - TrackColumnWidth, height() - StripHeight });
    painter->translate(-m_scrollX, -m_scrollY);

    const double firstLine = m_scrollX / m_pixelsPerLine;
    const double lastLine = (m_scrollX + width() - TrackColumnWidth) / m_pixelsPerLine;
    const double stride = m_laneHeight + LaneSpacing;
    const int firstLane = std::max(static_cast<int>(m_scrollY / stride), 0);
    const int lastLane = std::min(static_cast<int>((m_scrollY + height() - StripHeight) / stride), static_cast<int>(overview.tracks.size()) - 1);
    const double rightEdge = songLineX(lastLine) + 1.0;

    // Grid steps that are far enough apart to read
    const double beatPixels = static_cast<double>(overview.linesPerBeat) * m_pixelsPerLine;
    const bool drawBeats = beatPixels >= MinGridSpacingPixels;
    const bool drawBars = beatPixels * BeatsPerBar >= MinGridSpacingPixels;

    for (int lane = firstLane; lane <= lastLane; lane++) {
        const auto & track = overview.tracks.at(static_cast<size_t>(lane));
        const double top = laneY(lane);
        const auto scale = pitchScale(static_cast<size_t>(lane));
        const double barHeight = std::max(scale.pixelsPerSemitone, MinBarHeight);
        const QColor trackColour = trackColor(lane);

        painter->fillRect(QRectF { TrackColumnWidth, top, rightEdge - TrackColumnWidth, m_laneHeight }, PastEndFill);

        for (auto && row : overview.rows) {
            const double rowStart = static_cast<double>(row.startLine);
            const double rowEnd = rowStart + static_cast<double>(row.lineCount);
            if (rowEnd < firstLine) {
                continue;
            }
            if (rowStart > lastLine) {
                break;
            }
            const double left = songLineX(rowStart);
            const double rowWidth = static_cast<double>(row.lineCount) * m_pixelsPerLine;
            painter->fillRect(QRectF { left, top, rowWidth, m_laneHeight }, RollFill);

            if (scale.pixelsPerSemitone >= MinKeyShadingPixels) {
                for (int pitch = track.lowestPitch; pitch <= track.highestPitch; pitch++) {
                    if (isBlackKey(pitch)) {
                        painter->fillRect(QRectF { left, scale.top + (scale.highest - pitch) * scale.pixelsPerSemitone, rowWidth, scale.pixelsPerSemitone }, BlackKeyFill);
                    }
                }
            }

            if (drawBars) {
                for (size_t line = overview.linesPerBeat; line < row.lineCount; line += overview.linesPerBeat) {
                    const bool isBar = (line / overview.linesPerBeat) % BeatsPerBar == 0;
                    if (!isBar && !drawBeats) {
                        continue;
                    }
                    painter->setPen(QPen { isBar ? BarLine : BeatLine, 1.0 });
                    const double x = std::round(songLineX(rowStart + static_cast<double>(line))) + 0.5;
                    painter->drawLine(QPointF { x, top }, QPointF { x, top + m_laneHeight });
                }
            }
            // Where the position starts
            painter->setPen(QPen { PositionLine, 1.0 });
            const double x = std::round(left) + 0.5;
            painter->drawLine(QPointF { x, top }, QPointF { x, top + m_laneHeight });

            if (const auto pattern = overview.patterns.find(row.pattern); pattern != overview.patterns.end()) {
                for (auto && note : pattern->second.notes) {
                    if (note.track != track.index) {
                        continue;
                    }
                    QColor color = trackColour;
                    double alpha = 0.45 + 0.55 * static_cast<double>(note.velocity) / 127.0;
                    if (!m_controller->isColumnAudible(note.track, note.column)) {
                        alpha *= 0.25;
                    }
                    color.setAlphaF(static_cast<float>(alpha));
                    const QRectF bar {
                        songLineX(rowStart + note.startLine),
                        scale.top + (scale.highest - note.pitch) * scale.pixelsPerSemitone + (scale.pixelsPerSemitone - barHeight) / 2.0,
                        std::max(note.lengthLines * m_pixelsPerLine, 1.0),
                        barHeight
                    };
                    painter->fillRect(bar, color);
                    // A darker edge keeps notes side by side apart
                    if (bar.width() >= 4.0 && bar.height() >= 3.0) {
                        painter->setPen(QPen { color.darker(180), 1.0 });
                        painter->drawRect(bar.adjusted(0.5, 0.5, -0.5, -0.5));
                    }
                }
            }

            if (m_automationsVisible) {
                if (const auto pattern = overview.patterns.find(row.pattern); pattern != overview.patterns.end()) {
                    paintCurves(painter, pattern->second.curves, track.index, rowStart, top, firstLine, lastLine);
                }
            }

            if (row.skipped) {
                painter->fillRect(QRectF { left, top, rowWidth, m_laneHeight }, SkippedShade);
            }
        }
    }
    painter->restore();
}

void SongOverviewRenderer::paintCurves(QPainter * painter, const std::vector<SongOverview::Curve> & curves, size_t track, double rowStart, double top, double firstLine, double lastLine) const
{
    if (m_automationCurveColors.isEmpty()) {
        return;
    }
    const double bottom = top + m_laneHeight - CurvePadding;
    const double span = std::max(m_laneHeight - 2.0 * CurvePadding, 1.0);
    // Only the lines in view, and one past each end so that the line runs to the edge
    const auto first = static_cast<size_t>(std::max(std::floor(firstLine - rowStart) - 1.0, 0.0));
    const double last = std::ceil(lastLine - rowStart) + 1.0;

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    for (auto && curve : curves) {
        if (curve.track != track) {
            continue;
        }
        QColor color = m_automationCurveColors.at(static_cast<int>(curve.colorIndex % static_cast<size_t>(m_automationCurveColors.size())));
        color.setAlphaF(static_cast<float>(m_controller->isColumnAudible(curve.track, curve.column) ? CurveAlpha : CurveAlpha * 0.3));
        painter->setPen(QPen { color, CurveThickness, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin });
        // A line it does not cover breaks it into runs
        QPolygonF run;
        const auto flush = [&] {
            if (run.size() > 1) {
                painter->drawPolyline(run);
            }
            run.clear();
        };
        const size_t end = std::min(curve.values.size(), static_cast<size_t>(std::max(last, 0.0)));
        for (size_t line = first; line < end; line++) {
            if (const auto value = curve.values.at(line); value.has_value()) {
                run.append(QPointF { songLineX(rowStart + static_cast<double>(line)), bottom - std::clamp(*value, 0.0, 1.0) * span });
            } else {
                flush();
            }
        }
        flush();
    }
    painter->restore();
}

void SongOverviewRenderer::paintStrip(QPainter * painter) const
{
    const auto & overview = m_controller->overview();
    painter->save();
    painter->setClipRect(QRectF { TrackColumnWidth, 0.0, width() - TrackColumnWidth, StripHeight });
    painter->fillRect(QRectF { TrackColumnWidth, 0.0, width() - TrackColumnWidth, StripHeight }, CornerFill);
    painter->translate(-m_scrollX, 0.0);

    QFont boldFont = painter->font();
    boldFont.setPixelSize(13);
    boldFont.setBold(true);
    QFont font = boldFont;
    font.setBold(false);
    font.setPixelSize(11);
    QFont smallFont = font;
    smallFont.setPixelSize(9);
    const QFontMetricsF metrics { font };
    const QFontMetricsF boldMetrics { boldFont };

    const double firstLine = m_scrollX / m_pixelsPerLine;
    const double lastLine = (m_scrollX + width() - TrackColumnWidth) / m_pixelsPerLine;
    // The active position's block takes on the accent
    QColor activeFill = m_accentColor;
    activeFill.setAlphaF(0.35f);
    for (size_t rowIndex = 0; rowIndex < overview.rows.size(); rowIndex++) {
        const auto & row = overview.rows.at(rowIndex);
        const double rowStart = static_cast<double>(row.startLine);
        const double rowEnd = rowStart + static_cast<double>(row.lineCount);
        if (rowEnd < firstLine) {
            continue;
        }
        if (rowStart > lastLine) {
            break;
        }
        const QRectF block { songLineX(rowStart) + 1.0, 2.0, std::max(static_cast<double>(row.lineCount) * m_pixelsPerLine - 2.0, 1.0), StripHeight - 4.0 };
        painter->fillRect(block, CellFill);
        if (static_cast<int>(rowIndex) == m_activeRow) {
            painter->fillRect(block, activeFill);
        }
        if (block.width() >= 16.0) {
            // The pattern, with the position in small print above it
            const auto inner = block.adjusted(4.0, 0.0, -4.0, 0.0);
            painter->setFont(smallFont);
            painter->setPen(FaintText);
            painter->drawText(QRectF { inner.left(), inner.top() + 1.0, inner.width(), 11.0 }, Qt::AlignLeft | Qt::AlignVCenter, QString::number(row.position));
            const auto number = QString::number(row.pattern);
            const QRectF bottom { inner.left(), inner.top() + 12.0, inner.width(), inner.height() - 13.0 };
            painter->setFont(boldFont);
            painter->setPen(TextColor);
            painter->drawText(bottom, Qt::AlignLeft | Qt::AlignVCenter, number);
            const auto pattern = overview.patterns.find(row.pattern);
            const auto name = pattern != overview.patterns.end() ? QString::fromStdString(pattern->second.name) : QString {};
            const double nameLeft = bottom.left() + boldMetrics.horizontalAdvance(number) + 6.0;
            if (!name.isEmpty() && bottom.right() - nameLeft > 12.0) {
                painter->setFont(font);
                painter->setPen(SubtleText);
                painter->drawText(QRectF { nameLeft, bottom.top(), bottom.right() - nameLeft, bottom.height() }, Qt::AlignLeft | Qt::AlignVCenter,
                                  metrics.elidedText(name, Qt::ElideRight, bottom.right() - nameLeft));
            }
        }
        if (row.skipped) {
            painter->fillRect(block, SkippedShade);
        }
    }
    painter->restore();
}

void SongOverviewRenderer::paintTrackColumn(QPainter * painter) const
{
    const auto & overview = m_controller->overview();
    painter->save();
    painter->setClipRect(QRectF { 0.0, StripHeight, TrackColumnWidth, height() - StripHeight });
    painter->translate(0.0, -m_scrollY);

    QFont font = painter->font();
    font.setPixelSize(13);
    const QFontMetricsF metrics { font };
    QFont smallFont = font;
    smallFont.setPixelSize(9);

    for (size_t lane = 0; lane < overview.tracks.size(); lane++) {
        const auto & track = overview.tracks.at(lane);
        const double top = laneY(static_cast<int>(lane));
        if (top + m_laneHeight < m_scrollY + StripHeight || top > m_scrollY + height()) {
            continue;
        }
        painter->fillRect(QRectF { 0.0, top, TrackColumnWidth - LaneSpacing, m_laneHeight }, CellFill);
        painter->fillRect(QRectF { 0.0, top, 4.0, m_laneHeight }, trackColor(static_cast<int>(lane)));
        painter->setFont(font);
        painter->setPen(TextColor);
        painter->drawText(QRectF { 12.0, top, TrackColumnWidth - 52.0, m_laneHeight }, Qt::AlignLeft | Qt::AlignVCenter,
                          metrics.elidedText(QString::fromStdString(track.name), Qt::ElideRight, TrackColumnWidth - 52.0));

        // A C on each octave of the lane's range, where there is room to read one, as on a keyboard
        const auto scale = pitchScale(lane);
        if (scale.pixelsPerSemitone * 12.0 >= 11.0) {
            painter->setFont(smallFont);
            painter->setPen(FaintText);
            for (int pitch = track.lowestPitch; pitch <= track.highestPitch; pitch++) {
                if (pitch % 12 == 0) {
                    const double y = scale.top + (scale.highest - pitch) * scale.pixelsPerSemitone;
                    painter->drawText(QRectF { TrackColumnWidth - 40.0, y + scale.pixelsPerSemitone / 2.0 - 5.0, 34.0, 10.0 }, Qt::AlignRight | Qt::AlignVCenter,
                                      QString::fromStdString(NoteConverter::midiToString(static_cast<uint8_t>(pitch))));
                }
            }
        }
    }
    painter->restore();
}

void SongOverviewRenderer::paint(QPainter * painter)
{
    if (!m_controller || m_controller->overview().rows.empty()) {
        return;
    }
    paintLanes(painter);
    paintStrip(painter);
    paintTrackColumn(painter);
    painter->fillRect(QRectF { 0.0, 0.0, TrackColumnWidth - LaneSpacing, StripHeight - LaneSpacing }, CornerFill);
}

} // namespace noteahead
