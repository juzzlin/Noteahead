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

#include "room_view_renderer.hpp"

#include "room_acoustics.hpp"

#include <QCursor>
#include <QFontMetricsF>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPen>

#include <algorithm>
#include <cmath>
#include <numbers>

namespace noteahead {

namespace {

constexpr double margin { 18.0 };
//! How near the pointer has to be to pick something up.
constexpr double grabRadius { 14.0 };
//! The size of a polar pattern at full sensitivity.
constexpr double patternRadius { 12.0 };
constexpr double speakerSize { 14.0 };

QPointF pointOf(const QVariant & value)
{
    const auto map = value.toMap();
    return { map["x"].toDouble(), map["y"].toDouble() };
}

} // namespace

RoomViewRenderer::RoomViewRenderer(QQuickItem * parent)
  : QQuickPaintedItem { parent }
{
    setAcceptedMouseButtons(Qt::LeftButton);
    setAcceptHoverEvents(true);
    setAntialiasing(true);
}

QVariantMap RoomViewRenderer::layout() const
{
    return m_layout;
}

void RoomViewRenderer::setLayout(const QVariantMap & layout)
{
    m_layout = layout;
    emit layoutChanged();
    update();
}

QColor RoomViewRenderer::accentColor() const
{
    return m_accentColor;
}

void RoomViewRenderer::setAccentColor(const QColor & color)
{
    if (m_accentColor != color) {
        m_accentColor = color;
        emit accentColorChanged();
        update();
    }
}

QString RoomViewRenderer::sourceName() const
{
    return m_sourceName;
}

void RoomViewRenderer::setSourceName(const QString & name)
{
    if (m_sourceName != name) {
        m_sourceName = name;
        emit sourceNameChanged();
        update();
    }
}

QStringList RoomViewRenderer::groupNames() const
{
    return m_groupNames;
}

void RoomViewRenderer::setGroupNames(const QStringList & names)
{
    if (m_groupNames != names) {
        m_groupNames = names;
        emit groupNamesChanged();
        update();
    }
}

double RoomViewRenderer::pixelsPerMetre() const
{
    const double width = m_layout.value("width", 1.0).toDouble();
    const double depth = m_layout.value("depth", 1.0).toDouble();
    const double availableWidth = std::max(1.0, this->width() - 2.0 * margin);
    const double availableHeight = std::max(1.0, this->height() - 2.0 * margin);
    return std::min(availableWidth / std::max(width, 0.1), availableHeight / std::max(depth, 0.1));
}

QRectF RoomViewRenderer::roomRect() const
{
    const double scale = pixelsPerMetre();
    const double w = m_layout.value("width", 1.0).toDouble() * scale;
    const double h = m_layout.value("depth", 1.0).toDouble() * scale;
    return { (width() - w) * 0.5, (height() - h) * 0.5, w, h };
}

QPointF RoomViewRenderer::toPixels(const QPointF & metres) const
{
    const auto rect = roomRect();
    return rect.topLeft() + metres * pixelsPerMetre();
}

QPointF RoomViewRenderer::toMetres(const QPointF & pixels) const
{
    return (pixels - roomRect().topLeft()) / pixelsPerMetre();
}

QColor RoomViewRenderer::groupColor(int group) const
{
    switch (group) {
    case 0:
        return m_accentColor;
    case 1:
        return QColor { 0x4f, 0xc3, 0xf7 };
    default:
        return QColor { 0xff, 0xb7, 0x4d };
    }
}

std::optional<RoomViewRenderer::Target> RoomViewRenderer::hitTest(const QPointF & pixels, bool preferCloseMics) const
{
    if (m_layout.isEmpty()) {
        return std::nullopt;
    }
    std::optional<Target> best;
    double bestDistance = grabRadius;
    // Nearest wins; between two in the same place, the source, or the Close mics with Shift
    const auto consider = [&](int item, const QPointF & handle, const QPointF & centre, bool preferred) {
        const double distance = QLineF { pixels, toPixels(handle) }.length() - (preferred ? 1.0 : 0.0);
        if (distance < bestDistance) {
            bestDistance = distance;
            best = Target { item, centre };
        }
    };
    const auto sourceCentre = pointOf(m_layout["sourceCentre"]);
    consider(-1, sourceCentre, sourceCentre, !preferCloseMics);
    for (auto && source : m_layout["sources"].toList()) {
        consider(-1, pointOf(source), sourceCentre, !preferCloseMics);
    }
    const auto groups = m_layout["groups"].toList();
    for (int g = 0; g < groups.size(); g++) {
        const auto group = groups[g].toMap();
        const auto centre = pointOf(group["centre"]);
        const bool preferred = g != 0 || preferCloseMics;
        consider(g, centre, centre, preferred);
        for (auto && mic : group["mics"].toList()) {
            consider(g, pointOf(mic), centre, preferred);
        }
    }
    return best;
}

void RoomViewRenderer::mousePressEvent(QMouseEvent * event)
{
    m_drag = hitTest(event->position(), event->modifiers().testFlag(Qt::ShiftModifier));
    if (!m_drag) {
        event->ignore();
        return;
    }
    m_grabOffset = toMetres(event->position()) - m_drag->centre;
    // The whole gesture, rather than handing the drag to the ScrollView underneath
    setKeepMouseGrab(true);
    setCursor(Qt::ClosedHandCursor);
    event->accept();
}

void RoomViewRenderer::mouseMoveEvent(QMouseEvent * event)
{
    if (!m_drag) {
        return;
    }
    const auto metres = toMetres(event->position()) - m_grabOffset;
    const double x = std::clamp(metres.x(), 0.0, m_layout.value("width").toDouble());
    const double y = std::clamp(metres.y(), 0.0, m_layout.value("depth").toDouble());
    if (m_drag->item < 0) {
        emit sourceDragged(x, y);
    } else {
        emit micsDragged(m_drag->item, x, y);
    }
}

void RoomViewRenderer::mouseReleaseEvent(QMouseEvent *)
{
    setKeepMouseGrab(false);
    m_drag.reset();
    setCursor(Qt::OpenHandCursor);
}

void RoomViewRenderer::mouseUngrabEvent()
{
    setKeepMouseGrab(false);
    m_drag.reset();
}

void RoomViewRenderer::hoverMoveEvent(QHoverEvent * event)
{
    if (hitTest(event->position(), event->modifiers().testFlag(Qt::ShiftModifier))) {
        setCursor(Qt::OpenHandCursor);
    } else {
        unsetCursor();
    }
}

void RoomViewRenderer::paint(QPainter * painter)
{
    painter->fillRect(QRectF { 0, 0, width(), height() }, QColor { 0x11, 0x11, 0x11 });
    if (m_layout.isEmpty()) {
        return;
    }
    painter->setRenderHint(QPainter::Antialiasing, true);

    const auto rect = roomRect();
    const double scale = pixelsPerMetre();
    painter->fillRect(rect, QColor { 0x1a, 0x1a, 0x1a });

    // A metre grid, or five where metres would crowd
    const double step = scale >= 12.0 ? 1.0 : 5.0;
    painter->setPen(QPen { QColor { 0x26, 0x26, 0x26 }, 1.0 });
    for (double x = step; x < m_layout["width"].toDouble(); x += step) {
        painter->drawLine(QPointF { rect.left() + x * scale, rect.top() }, QPointF { rect.left() + x * scale, rect.bottom() });
    }
    for (double y = step; y < m_layout["depth"].toDouble(); y += step) {
        painter->drawLine(QPointF { rect.left(), rect.top() + y * scale }, QPointF { rect.right(), rect.top() + y * scale });
    }
    painter->setPen(QPen { QColor { 0x88, 0x88, 0x88 }, 2.0 });
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(rect);

    const auto sources = m_layout["sources"].toList();
    const auto groups = m_layout["groups"].toList();

    // The direct paths, faint, from each speaker to each mic that is up
    for (int g = 0; g < groups.size(); g++) {
        const auto group = groups[g].toMap();
        if (group["level"].toDouble() <= 0.0) {
            continue;
        }
        auto color = groupColor(g);
        color.setAlpha(45);
        painter->setPen(QPen { color, 1.0, Qt::DotLine });
        for (auto && mic : group["mics"].toList()) {
            for (auto && source : sources) {
                painter->drawLine(toPixels(pointOf(source)), toPixels(pointOf(mic)));
            }
        }
    }

    // The mics: each a polar pattern turned the way it faces, the pair joined, its name beside it
    for (int g = 0; g < groups.size(); g++) {
        const auto group = groups[g].toMap();
        const auto pattern = static_cast<RoomAcoustics::Pattern>(group["pattern"].toInt());
        auto color = groupColor(g);
        if (group["level"].toDouble() <= 0.0) {
            color.setAlpha(90);
        }
        const auto mics = group["mics"].toList();
        if (mics.size() == 2) {
            painter->setPen(QPen { color, 1.0, Qt::DashLine });
            painter->drawLine(toPixels(pointOf(mics[0])), toPixels(pointOf(mics[1])));
        }
        for (auto && entry : mics) {
            const auto mic = entry.toMap();
            const auto centre = toPixels(pointOf(mic));
            const double aimX = mic["aimX"].toDouble();
            const double aimY = mic["aimY"].toDouble();
            QPainterPath shape;
            constexpr int steps = 48;
            for (int i = 0; i <= steps; i++) {
                const double angle = 2.0 * std::numbers::pi * i / steps;
                const double radius = pattern == RoomAcoustics::Pattern::Omni ? 0.55 : std::abs(RoomAcoustics::patternGain(pattern, std::cos(angle)));
                // Turned from the mic's axis towards its right
                const double dx = std::cos(angle) * aimX - std::sin(angle) * aimY;
                const double dy = std::cos(angle) * aimY + std::sin(angle) * aimX;
                const QPointF point { centre.x() + dx * radius * patternRadius, centre.y() + dy * radius * patternRadius };
                if (i == 0) {
                    shape.moveTo(point);
                } else {
                    shape.lineTo(point);
                }
            }
            auto fill = color;
            fill.setAlpha(color.alpha() / 4);
            painter->setPen(QPen { color, 1.2 });
            painter->setBrush(fill);
            painter->drawPath(shape);
            painter->setBrush(color);
            painter->drawEllipse(centre, 2.5, 2.5);
        }
        if (g < m_groupNames.size()) {
            const auto centre = toPixels(pointOf(group["centre"]));
            painter->setPen(color);
            painter->drawText(QPointF { centre.x() + patternRadius + 2.0, centre.y() + patternRadius + 10.0 }, m_groupNames[g]);
        }
    }

    // The speakers last, over the Close mics standing on them, and the device's name over them
    for (auto && source : sources) {
        const auto centre = toPixels(pointOf(source));
        painter->setPen(QPen { QColor { 0xdd, 0xdd, 0xdd }, 1.0 });
        painter->setBrush(QColor { 0x33, 0x33, 0x33 });
        painter->drawRoundedRect(QRectF { centre.x() - speakerSize * 0.5, centre.y() - speakerSize * 0.5, speakerSize, speakerSize }, 2.0, 2.0);
        painter->setBrush(Qt::NoBrush);
        painter->drawEllipse(centre, speakerSize * 0.25, speakerSize * 0.25);
    }
    const auto sourceCentre = toPixels(pointOf(m_layout["sourceCentre"]));
    if (!m_sourceName.isEmpty()) {
        painter->setPen(QColor { 0xdd, 0xdd, 0xdd });
        const QFontMetricsF metrics { painter->font() };
        const double textWidth = metrics.horizontalAdvance(m_sourceName);
        const double textX = std::clamp(sourceCentre.x() - textWidth * 0.5, 2.0, std::max(2.0, width() - textWidth - 2.0));
        painter->drawText(QPointF { textX, std::max(metrics.ascent() + 1.0, sourceCentre.y() - speakerSize) }, m_sourceName);
    }
}

} // namespace noteahead
