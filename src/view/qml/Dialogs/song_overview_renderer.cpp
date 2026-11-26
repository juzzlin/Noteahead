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
//! A focused lane is at least this tall per semitone of its range, and scrolls if that is taller
//! than the view.
constexpr qreal FocusPixelsPerSemitone { 10.0 };
//! Semitones a focused lane's range reaches past its notes, so that none is at the very edge.
constexpr int FocusPitchMargin { 2 };
constexpr qreal FocusButtonSize { 16.0 };
//! Where a track's name starts, right of its focus button.
constexpr qreal TrackNameLeft { 32.0 };
//! The band of a track's cell its focus button and name are in.
constexpr qreal TrackHeaderHeight { 28.0 };
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
constexpr qreal HoveredCurveThickness { 3.0 };
//! How near a curve the pointer has to be to hit it, as in the editor.
constexpr qreal CurveReachPixels { 6.0 };

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

double distanceToSegment(const QPointF & point, const QPointF & a, const QPointF & b)
{
    const QPointF ab = b - a;
    const double lengthSquared = QPointF::dotProduct(ab, ab);
    const double t = lengthSquared > 0.0 ? std::clamp(QPointF::dotProduct(point - a, ab) / lengthSquared, 0.0, 1.0) : 0.0;
    const QPointF nearest = a + t * ab;
    return std::hypot(point.x() - nearest.x(), point.y() - nearest.y());
}

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

namespace {

std::set<size_t> trackSet(const QVariantList & tracks)
{
    std::set<size_t> set;
    for (auto && track : tracks) {
        set.insert(static_cast<size_t>(track.toULongLong()));
    }
    return set;
}

QVariantList trackList(const std::set<size_t> & tracks)
{
    QVariantList list;
    for (auto && track : tracks) {
        list.append(static_cast<quint64>(track));
    }
    return list;
}

} // namespace

QVariantList SongOverviewRenderer::focusedTracks() const
{
    return trackList(m_focusedTracks);
}

void SongOverviewRenderer::setFocusedTracks(const QVariantList & tracks)
{
    if (auto set = trackSet(tracks); m_focusedTracks != set) {
        m_focusedTracks = std::move(set);
        emit layoutChanged();
        update();
    }
}

QVariantList SongOverviewRenderer::markedTracks() const
{
    return trackList(m_markedTracks);
}

void SongOverviewRenderer::setMarkedTracks(const QVariantList & tracks)
{
    if (auto set = trackSet(tracks); m_markedTracks != set) {
        m_markedTracks = std::move(set);
        emit markedTracksChanged();
        update();
    }
}

void SongOverviewRenderer::geometryChange(const QRectF & newGeometry, const QRectF & oldGeometry)
{
    QQuickPaintedItem::geometryChange(newGeometry, oldGeometry);
    // Focused lanes fill the view's height
    if (!m_focusedTracks.empty() && newGeometry.height() != oldGeometry.height()) {
        emit layoutChanged();
    }
}

bool SongOverviewRenderer::isFocused(size_t lane) const
{
    return m_controller && lane < m_controller->overview().tracks.size() && m_focusedTracks.contains(m_controller->overview().tracks.at(lane).index);
}

SongOverviewRenderer::LaneLayout SongOverviewRenderer::laneLayout() const
{
    if (!m_controller) {
        return {};
    }
    const auto & tracks = m_controller->overview().tracks;
    LaneLayout layout(tracks.size());
    std::vector<size_t> focused;
    for (size_t lane = 0; lane < tracks.size(); lane++) {
        if (isFocused(lane)) {
            focused.push_back(lane);
        }
    }
    // Every lane, the same height, unless a track in focus is still there to show
    if (focused.empty()) {
        for (size_t lane = 0; lane < tracks.size(); lane++) {
            layout.at(lane) = LaneGeometry { .top = StripHeight + static_cast<double>(lane) * (m_laneHeight + LaneSpacing), .height = m_laneHeight };
        }
        return layout;
    }
    const double fitHeight = (height() - StripHeight) / static_cast<double>(focused.size()) - LaneSpacing;
    double top = StripHeight;
    for (auto && lane : focused) {
        const auto & track = tracks.at(lane);
        const int semitones = std::min(track.highestPitch + FocusPitchMargin, 127) - std::max(track.lowestPitch - FocusPitchMargin, 0) + 1;
        const double readableHeight = static_cast<double>(semitones) * FocusPixelsPerSemitone + 2.0 * NotePadding;
        const double laneHeight = std::floor(std::max({ fitHeight, readableHeight, MinLaneHeight }));
        layout.at(lane) = LaneGeometry { .top = top, .height = laneHeight };
        top += laneHeight + LaneSpacing;
    }
    return layout;
}

std::optional<size_t> SongOverviewRenderer::laneAt(const LaneLayout & layout, double contentY) const
{
    for (size_t lane = 0; lane < layout.size(); lane++) {
        if (const auto & geometry = layout.at(lane); geometry && contentY >= geometry->top && contentY <= geometry->top + geometry->height) {
            return lane;
        }
    }
    return {};
}

double SongOverviewRenderer::headerTop(const LaneGeometry & geometry) const
{
    if (geometry.height <= 2.0 * TrackHeaderHeight) {
        return geometry.top + (geometry.height - TrackHeaderHeight) / 2.0;
    }
    // A tall lane keeps its name in sight while it scrolls past
    return std::clamp(m_scrollY + StripHeight, geometry.top, geometry.top + geometry.height - TrackHeaderHeight);
}

QRectF SongOverviewRenderer::focusButtonRect(const LaneGeometry & geometry) const
{
    return { 10.0, headerTop(geometry) + (TrackHeaderHeight - FocusButtonSize) / 2.0, FocusButtonSize, FocusButtonSize };
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
    qreal bottom = StripHeight;
    for (auto && geometry : laneLayout()) {
        if (geometry) {
            bottom = std::max(bottom, geometry->top + geometry->height + LaneSpacing);
        }
    }
    return bottom;
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
    const auto layout = laneLayout();
    for (size_t next = static_cast<size_t>(std::max(lane, 0)); next < layout.size(); next++) {
        if (layout.at(next)) {
            return layout.at(next)->top;
        }
    }
    return contentHeight();
}

qreal SongOverviewRenderer::pitchY(int lane, int pitch) const
{
    const auto layout = laneLayout();
    if (lane < 0 || static_cast<size_t>(lane) >= layout.size() || !layout.at(static_cast<size_t>(lane))) {
        return laneY(std::max(lane, 0));
    }
    const auto scale = pitchScale(static_cast<size_t>(lane), *layout.at(static_cast<size_t>(lane)));
    return scale.top + (scale.highest - pitch + 0.5) * scale.pixelsPerSemitone;
}

SongOverviewRenderer::PitchScale SongOverviewRenderer::pitchScale(size_t lane, const LaneGeometry & geometry) const
{
    const auto & track = m_controller->overview().tracks.at(lane);
    const int margin = isFocused(lane) ? FocusPitchMargin : 0;
    const int lowest = std::max(track.lowestPitch - margin, 0);
    const int highest = std::min(track.highestPitch + margin, 127);
    const double semitones = static_cast<double>(highest - lowest) + 1.0;
    return {
        .top = geometry.top + NotePadding,
        .pixelsPerSemitone = (geometry.height - 2.0 * NotePadding) / semitones,
        .lowest = lowest,
        .highest = highest
    };
}

QColor SongOverviewRenderer::trackColor(int trackPosition) const
{
    // Golden-ratio steps round the hue circle: neighbours never look alike, however many tracks
    const double hue = std::fmod(0.08 + static_cast<double>(std::max(trackPosition, 0)) * 0.6180339887, 1.0);
    return QColor::fromHsvF(static_cast<float>(hue), 0.55f, 0.95f);
}

void SongOverviewRenderer::setHoveredCurve(int kind, qint64 automationId)
{
    std::optional<std::pair<int, uint64_t>> curve;
    if (kind >= 0 && automationId >= 0) {
        curve = std::pair { kind, static_cast<uint64_t>(automationId) };
    }
    if (m_hoveredCurve != curve) {
        m_hoveredCurve = curve;
        update();
    }
}

const SongOverview::Curve * SongOverviewRenderer::curveAt(size_t row, size_t lane, const LaneGeometry & geometry, QPointF point) const
{
    const auto & overview = m_controller->overview();
    const auto & overviewRow = overview.rows.at(row);
    const auto pattern = overview.patterns.find(overviewRow.pattern);
    if (pattern == overview.patterns.end()) {
        return nullptr;
    }
    const auto track = overview.tracks.at(lane).index;
    const double rowStart = static_cast<double>(overviewRow.startLine);
    const double bottom = geometry.top + geometry.height - CurvePadding;
    const double span = std::max(geometry.height - 2.0 * CurvePadding, 1.0);
    // The segments that can be within reach: those of the lines about the point
    const double line = (point.x() - TrackColumnWidth) / m_pixelsPerLine - rowStart;
    const double reachLines = CurveReachPixels / m_pixelsPerLine;
    const auto first = static_cast<size_t>(std::max(std::floor(line - reachLines) - 1.0, 0.0));
    const auto last = static_cast<size_t>(std::max(std::ceil(line + reachLines) + 1.0, 0.0));

    const SongOverview::Curve * nearest = nullptr;
    double nearestDistance = CurveReachPixels;
    const auto pointAt = [&](size_t index, double value) {
        return QPointF { songLineX(rowStart + static_cast<double>(index)), bottom - std::clamp(value, 0.0, 1.0) * span };
    };
    // The last one drawn is the one on top
    for (auto && curve : pattern->second.curves) {
        if (curve.track != track) {
            continue;
        }
        const size_t end = std::min(curve.values.size(), last + 1);
        for (size_t index = first; index + 1 < end; index++) {
            const auto & a = curve.values.at(index);
            const auto & b = curve.values.at(index + 1);
            if (a.has_value() && b.has_value()) {
                if (const double distance = distanceToSegment(point, pointAt(index, *a), pointAt(index + 1, *b)); distance <= nearestDistance) {
                    nearest = &curve;
                    nearestDistance = distance;
                }
            }
        }
    }
    return nearest;
}

QVariantMap SongOverviewRenderer::hitTest(qreal x, qreal y, bool includeCurves) const
{
    const QVariantMap invalid { { "valid", false } };
    if (!m_controller) {
        return invalid;
    }
    const auto & overview = m_controller->overview();
    const auto layout = laneLayout();

    // On a track's name
    if (x < TrackColumnWidth) {
        const double contentY = y + m_scrollY;
        const auto lane = laneAt(layout, contentY);
        if (y < StripHeight || !lane) {
            return invalid;
        }
        return {
            { "valid", true },
            { "isTrack", true },
            { "isFocusButton", focusButtonRect(*layout.at(*lane)).adjusted(-2.0, -2.0, 2.0, 2.0).contains(QPointF { x, contentY }) },
            { "lane", static_cast<int>(*lane) },
            { "track", static_cast<quint64>(overview.tracks.at(*lane).index) }
        };
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
    const auto shownLane = laneAt(layout, contentY);
    if (!shownLane) {
        return invalid;
    }
    const int lane = static_cast<int>(*shownLane);
    const auto & geometry = *layout.at(*shownLane);
    if (includeCurves && m_automationsVisible) {
        if (const auto curve = curveAt(*row, *shownLane, geometry, QPointF { x + m_scrollX, contentY }); curve) {
            return {
                { "valid", true },
                { "isCurve", true },
                { "curveKind", curve->kind },
                { "automationId", static_cast<quint64>(curve->automationId) },
                { "comment", QString::fromStdString(curve->comment) },
                { "controller", curve->controller.has_value() ? static_cast<int>(*curve->controller) : -1 },
                { "parameter", QString::fromStdString(curve->parameter) },
                { "trackName", QString::fromStdString(overview.tracks.at(static_cast<size_t>(lane)).name) },
                { "row", static_cast<int>(*row) },
                { "lane", lane }
            };
        }
    }
    const auto scale = pitchScale(*shownLane, geometry);
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
    const auto layout = laneLayout();
    const double rightEdge = songLineX(lastLine) + 1.0;

    // Grid steps that are far enough apart to read
    const double beatPixels = static_cast<double>(overview.linesPerBeat) * m_pixelsPerLine;
    const bool drawBeats = beatPixels >= MinGridSpacingPixels;
    const bool drawBars = beatPixels * BeatsPerBar >= MinGridSpacingPixels;

    for (size_t lane = 0; lane < layout.size(); lane++) {
        if (!layout.at(lane)) {
            continue;
        }
        const auto & geometry = *layout.at(lane);
        if (geometry.top + geometry.height < m_scrollY + StripHeight || geometry.top > m_scrollY + height()) {
            continue;
        }
        const auto & track = overview.tracks.at(lane);
        const double top = geometry.top;
        const double laneHeight = geometry.height;
        const auto scale = pitchScale(lane, geometry);
        const double barHeight = std::max(scale.pixelsPerSemitone, MinBarHeight);
        const QColor trackColour = trackColor(static_cast<int>(lane));

        painter->fillRect(QRectF { TrackColumnWidth, top, rightEdge - TrackColumnWidth, laneHeight }, PastEndFill);

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
            painter->fillRect(QRectF { left, top, rowWidth, laneHeight }, RollFill);

            if (scale.pixelsPerSemitone >= MinKeyShadingPixels) {
                for (int pitch = scale.lowest; pitch <= scale.highest; pitch++) {
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
                    painter->drawLine(QPointF { x, top }, QPointF { x, top + laneHeight });
                }
            }
            // Where the position starts
            painter->setPen(QPen { PositionLine, 1.0 });
            const double x = std::round(left) + 0.5;
            painter->drawLine(QPointF { x, top }, QPointF { x, top + laneHeight });

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
                    paintCurves(painter, pattern->second.curves, track.index, rowStart, geometry, firstLine, lastLine);
                }
            }

            if (row.skipped) {
                painter->fillRect(QRectF { left, top, rowWidth, laneHeight }, SkippedShade);
            }
        }
    }
    painter->restore();
}

void SongOverviewRenderer::paintCurves(QPainter * painter, const std::vector<SongOverview::Curve> & curves, size_t track, double rowStart, const LaneGeometry & geometry, double firstLine, double lastLine) const
{
    if (m_automationCurveColors.isEmpty()) {
        return;
    }
    const double bottom = geometry.top + geometry.height - CurvePadding;
    const double span = std::max(geometry.height - 2.0 * CurvePadding, 1.0);
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
        const bool hovered = m_hoveredCurve == std::pair { curve.kind, curve.automationId };
        const double alpha = m_controller->isColumnAudible(curve.track, curve.column) ? CurveAlpha : CurveAlpha * 0.3;
        color.setAlphaF(static_cast<float>(hovered ? 1.0 : alpha));
        painter->setPen(QPen { color, hovered ? HoveredCurveThickness : CurveThickness, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin });
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

    const auto layout = laneLayout();
    for (size_t lane = 0; lane < layout.size(); lane++) {
        if (!layout.at(lane)) {
            continue;
        }
        const auto & geometry = *layout.at(lane);
        const auto & track = overview.tracks.at(lane);
        const double top = geometry.top;
        if (top + geometry.height < m_scrollY + StripHeight || top > m_scrollY + height()) {
            continue;
        }
        painter->fillRect(QRectF { 0.0, top, TrackColumnWidth - LaneSpacing, geometry.height }, CellFill);
        painter->fillRect(QRectF { 0.0, top, 4.0, geometry.height }, trackColor(static_cast<int>(lane)));
        paintFocusButton(painter, focusButtonRect(geometry), m_markedTracks.contains(track.index));
        painter->setFont(font);
        painter->setPen(TextColor);
        const double nameWidth = TrackColumnWidth - TrackNameLeft - 40.0;
        painter->drawText(QRectF { TrackNameLeft, headerTop(geometry), nameWidth, TrackHeaderHeight }, Qt::AlignLeft | Qt::AlignVCenter,
                          metrics.elidedText(QString::fromStdString(track.name), Qt::ElideRight, nameWidth));

        // A C on each octave of the lane's range, where there is room to read one, as on a keyboard
        const auto scale = pitchScale(lane, geometry);
        if (scale.pixelsPerSemitone * 12.0 >= 11.0) {
            painter->setFont(smallFont);
            painter->setPen(FaintText);
            for (int pitch = scale.lowest; pitch <= scale.highest; pitch++) {
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

void SongOverviewRenderer::paintFocusButton(QPainter * painter, const QRectF & rect, bool marked) const
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    QColor fill = m_accentColor;
    fill.setAlphaF(marked ? 0.85f : 0.0f);
    painter->setBrush(fill);
    painter->setPen(QPen { marked ? m_accentColor : SubtleText, 1.0 });
    painter->drawRoundedRect(rect.adjusted(0.5, 0.5, -0.5, -0.5), 3.0, 3.0);
    // A vertical double arrow: the lane opened out to its full height
    painter->setPen(QPen { marked ? QColor { Qt::black } : SubtleText, 1.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin });
    const double x = rect.center().x();
    const double top = rect.top() + 3.5;
    const double bottom = rect.bottom() - 3.5;
    painter->drawLine(QPointF { x, top }, QPointF { x, bottom });
    painter->drawPolyline(QPolygonF { { QPointF { x - 3.0, top + 3.0 }, QPointF { x, top }, QPointF { x + 3.0, top + 3.0 } } });
    painter->drawPolyline(QPolygonF { { QPointF { x - 3.0, bottom - 3.0 }, QPointF { x, bottom }, QPointF { x + 3.0, bottom - 3.0 } } });
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
