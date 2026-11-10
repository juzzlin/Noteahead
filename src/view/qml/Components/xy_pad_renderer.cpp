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

#include "xy_pad_renderer.hpp"

#include "../../controllers/xy_pad_controller.hpp"

#include <QPainter>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>

namespace noteahead {

namespace {
// About a display frame: smooth enough to watch the glide, and it only runs while the pad moves
const auto frameInterval = std::chrono::milliseconds { 16 };
//! How long a pad goes on moving after an automation's step: a few of its slew times
constexpr qint64 AutomationFollowMs { 100 };
//! Straight pieces per gap between two of the tail's points, so that a fast drag, sampled once a
//! frame, curves instead of showing its corners
constexpr int TrailSubdivisions { 6 };

//! How far, in pixels, each piece of the tail reaches into its neighbours
constexpr double SeamOverlap { 0.5 };

//! The Catmull-Rom curve through @p p1 to @p p2 at @p t, shaped by their neighbours @p p0 and @p p3
QPointF catmullRom(const QPointF & p0, const QPointF & p1, const QPointF & p2, const QPointF & p3, double t)
{
    const double t2 = t * t;
    const double t3 = t2 * t;
    return 0.5 * ((2.0 * p1) + (-p0 + p2) * t + (2.0 * p0 - 5.0 * p1 + 4.0 * p2 - p3) * t2 + (-p0 + 3.0 * p1 - 3.0 * p2 + p3) * t3);
}

} // namespace

XyPadRenderer::XyPadRenderer(QQuickItem * parent)
  : QQuickPaintedItem { parent }
{
    setAntialiasing(true);
    setAcceptedMouseButtons(Qt::LeftButton);
    m_frameTimer.setInterval(frameInterval);
    connect(&m_frameTimer, &QTimer::timeout, this, &XyPadRenderer::onFrame);
}

QObject * XyPadRenderer::controller() const
{
    return m_controller;
}

void XyPadRenderer::setController(QObject * controller)
{
    if (const auto xyPadController = qobject_cast<XyPadController *>(controller); xyPadController != m_controller) {
        letGo();
        if (m_controller) {
            disconnect(m_controller, nullptr, this, nullptr);
        }
        m_controller = xyPadController;
        m_trail.clear();
        if (m_controller) {
            // The same pad can be shown twice, in its dialog and on the performance panel
            connect(m_controller, &XyPadController::padPressed, this, &XyPadRenderer::onPadPressed);
            connect(m_controller, &XyPadController::padsAutomated, this, &XyPadRenderer::onPadsAutomated);
        }
        emit controllerChanged();
        update();
    }
}

int XyPadRenderer::pad() const
{
    return m_pad;
}

void XyPadRenderer::setPad(int pad)
{
    if (m_pad != pad) {
        letGo();
        m_pad = pad;
        m_trail.clear();
        emit padChanged();
        update();
    }
}

QString XyPadRenderer::xLabel() const
{
    return m_xLabel;
}

void XyPadRenderer::setXLabel(const QString & label)
{
    if (m_xLabel != label) {
        m_xLabel = label;
        emit xLabelChanged();
        update();
    }
}

QString XyPadRenderer::yLabel() const
{
    return m_yLabel;
}

void XyPadRenderer::setYLabel(const QString & label)
{
    if (m_yLabel != label) {
        m_yLabel = label;
        emit yLabelChanged();
        update();
    }
}

QColor XyPadRenderer::accentColor() const
{
    return m_accentColor;
}

void XyPadRenderer::setAccentColor(const QColor & color)
{
    if (m_accentColor != color) {
        m_accentColor = color;
        emit accentColorChanged();
        update();
    }
}

bool XyPadRenderer::trailEnabled() const
{
    return m_trailEnabled;
}

void XyPadRenderer::setTrailEnabled(bool enabled)
{
    if (m_trailEnabled != enabled) {
        m_trailEnabled = enabled;
        m_trail.clear();
        emit trailEnabledChanged();
        update();
    }
}

QPointF XyPadRenderer::toPad(const QPointF & position) const
{
    const double x = width() > 0 ? position.x() / width() * 2.0 - 1.0 : 0.0;
    const double y = height() > 0 ? 1.0 - position.y() / height() * 2.0 : 0.0;
    return { std::clamp(x, -1.0, 1.0), std::clamp(y, -1.0, 1.0) };
}

void XyPadRenderer::handOn(const QPointF & position)
{
    if (!m_controller) {
        return;
    }
    const auto point = toPad(position);
    if (m_handOn) {
        m_controller->move(m_pad, point.x(), point.y());
    } else {
        m_handOn = true;
        m_controller->press(m_pad, point.x(), point.y());
        m_frameTimer.start();
    }
}

void XyPadRenderer::letGo()
{
    if (m_handOn) {
        m_handOn = false;
        if (m_controller) {
            m_controller->release(m_pad);
        }
    }
}

void XyPadRenderer::onFrame()
{
    const auto now = XyPadTrail::Clock::now();
    if (m_trailEnabled && m_controller) {
        m_trail.add({ m_controller->positionX(m_pad), m_controller->positionY(m_pad) }, now);
    }
    update();
    // The pad is still gliding after the hand has gone, so the redraws stop only once it is home.
    // The hand can also be on it in another view of the same pad.
    // An automation moves it in steps, and it rests between them
    const bool automated = std::chrono::steady_clock::now() < m_automatedUntil;
    const bool fading = m_trailEnabled && !m_trail.isEmpty(now);
    if (!m_handOn && !automated && !fading && (!m_controller || (!m_controller->isPressed(m_pad) && !m_controller->isMoving(m_pad)))) {
        m_frameTimer.stop();
    }
}

bool XyPadRenderer::showsPad(int pad) const
{
    return pad == m_pad;
}

void XyPadRenderer::startRedrawing()
{
    // Not restarted: a drawn automation steps every tick, far more often than a frame, and
    // restarting the timer on each step would keep it from ever firing
    if (!m_frameTimer.isActive()) {
        m_frameTimer.start();
    }
}

void XyPadRenderer::onPadPressed(int pad)
{
    if (showsPad(pad) && isVisible()) {
        startRedrawing();
    }
}

void XyPadRenderer::onPadsAutomated(qint64 delayMs)
{
    if (isVisible()) {
        // Past the step itself, by the time the pad takes to follow it
        m_automatedUntil = std::max(m_automatedUntil, std::chrono::steady_clock::now() + std::chrono::milliseconds { delayMs + AutomationFollowMs });
        startRedrawing();
    }
}

void XyPadRenderer::mousePressEvent(QMouseEvent * event)
{
    if (!m_controller) {
        event->ignore();
        return;
    }
    // The pad keeps the mouse for the whole gesture: a drag past its edge pins the pad to that edge
    // instead of handing the drag to whatever is underneath, such as a ScrollView
    setKeepMouseGrab(true);
    handOn(event->position());
    event->accept();
}

void XyPadRenderer::mouseMoveEvent(QMouseEvent * event)
{
    handOn(event->position());
    event->accept();
}

void XyPadRenderer::mouseReleaseEvent(QMouseEvent * event)
{
    setKeepMouseGrab(false);
    letGo();
    event->accept();
}

void XyPadRenderer::mouseUngrabEvent()
{
    setKeepMouseGrab(false);
    letGo();
}

void XyPadRenderer::itemChange(ItemChange change, const ItemChangeData & data)
{
    if (change == ItemVisibleHasChanged) {
        if (data.boolValue) {
            // The pad may be on its way home from being played elsewhere: one frame finds out
            m_frameTimer.start();
        } else {
            letGo();
            m_trail.clear();
            m_frameTimer.stop();
        }
    }
    QQuickPaintedItem::itemChange(change, data);
}

QPointF XyPadRenderer::toItem(const QPointF & padPosition) const
{
    return { (padPosition.x() + 1.0) / 2.0 * width(), (1.0 - padPosition.y()) / 2.0 * height() };
}

void XyPadRenderer::paintTrail(QPainter * painter) const
{
    const auto points = m_trail.points(XyPadTrail::Clock::now());
    if (points.size() < 2) {
        return;
    }

    struct Vertex
    {
        QPointF position;
        double life;
    };

    // The curve through the points, in short straight pieces, each vertex as old as where it lies
    std::vector<Vertex> vertices;
    vertices.reserve(points.size() * TrailSubdivisions + 1);
    vertices.push_back({ toItem(points.front().position), 1.0 - points.front().age });
    for (size_t index = 0; index + 1 < points.size(); index++) {
        const auto p0 = toItem(points.at(index > 0 ? index - 1 : index).position);
        const auto p1 = toItem(points.at(index).position);
        const auto p2 = toItem(points.at(index + 1).position);
        const auto p3 = toItem(points.at(index + 2 < points.size() ? index + 2 : index + 1).position);
        for (int step = 1; step <= TrailSubdivisions; step++) {
            const double t = static_cast<double>(step) / TrailSubdivisions;
            const double age = points.at(index).age + (points.at(index + 1).age - points.at(index).age) * t;
            vertices.push_back({ catmullRom(p0, p1, p2, p3, t), 1.0 - std::clamp(age, 0.0, 1.0) });
        }
    }
    // Each vertex's normal, between those of the pieces on either side, so the pieces of the ribbon
    // share their edges instead of overlapping
    std::vector<QPointF> normals(vertices.size());
    for (size_t index = 0; index < vertices.size(); index++) {
        const auto & before = vertices.at(index > 0 ? index - 1 : index).position;
        const auto & after = vertices.at(index + 1 < vertices.size() ? index + 1 : index).position;
        const auto direction = after - before;
        const double length = std::hypot(direction.x(), direction.y());
        normals.at(index) = length > 0.0 ? QPointF { -direction.y() / length, direction.x() / length } : QPointF {};
    }

    painter->save();
    // Where pieces overlap the brighter one wins instead of adding up, so nothing ever gets brighter
    // than the accent colour at the head
    painter->setCompositionMode(QPainter::CompositionMode_Lighten);
    painter->setPen(Qt::NoPen);
    // A wide faint glow first, then a thin core over it, both in the accent colour fading out
    for (const bool core : { false, true }) {
        const auto halfWidth = [core](double life) { return (core ? 0.5 + 1.5 * life : 1.0 + 5.0 * life); };
        for (size_t index = 0; index + 1 < vertices.size(); index++) {
            const auto & from = vertices.at(index);
            const auto & to = vertices.at(index + 1);
            const double life = (from.life + to.life) * 0.5;
            if (life <= 0.0) {
                continue;
            }
            auto color = m_accentColor;
            color.setAlphaF(static_cast<float>(core ? std::pow(life, 1.5) : 0.18 * life));
            painter->setBrush(color);
            // Each piece reaches half a pixel into the next, so the antialiased seams never show
            const auto direction = to.position - from.position;
            const double length = std::hypot(direction.x(), direction.y());
            const auto overlap = length > 0.0 ? direction * (SeamOverlap / length) : QPointF {};
            const auto fromOffset = normals.at(index) * halfWidth(from.life);
            const auto toOffset = normals.at(index + 1) * halfWidth(to.life);
            const auto start = from.position - overlap;
            const auto end = to.position + overlap;
            const std::array quad { start + fromOffset, end + toOffset, end - toOffset, start - fromOffset };
            painter->drawPolygon(quad.data(), static_cast<int>(quad.size()));
        }
    }
    painter->restore();
}

void XyPadRenderer::paint(QPainter * painter)
{
    const QRectF area { 0, 0, width(), height() };
    painter->fillRect(area, QColor { "#1a1a1a" });

    // The centre, where the pad rests and moves nothing
    painter->setPen(QPen { QColor { "#333333" }, 1.0, Qt::DashLine });
    painter->drawLine(QPointF { area.center().x(), 0 }, QPointF { area.center().x(), area.height() });
    painter->drawLine(QPointF { 0, area.center().y() }, QPointF { area.width(), area.center().y() });

    painter->setPen(QColor { "#888888" });
    auto font = painter->font();
    font.setPointSizeF(8.0);
    painter->setFont(font);
    const double textMargin = 4.0;
    // The pad's number, as the settings below the pads head each pad
    painter->drawText(area.adjusted(textMargin, textMargin, -textMargin, -textMargin), Qt::AlignTop | Qt::AlignRight, QString::number(m_pad + 1));
    if (!m_xLabel.isEmpty()) {
        painter->drawText(area.adjusted(textMargin, textMargin, -textMargin, -textMargin), Qt::AlignBottom | Qt::AlignHCenter, m_xLabel);
    }
    if (!m_yLabel.isEmpty()) {
        // Along the left edge, reading upwards like a chart's y axis
        painter->save();
        painter->translate(0, area.height());
        painter->rotate(-90);
        painter->drawText(QRectF { textMargin, textMargin, area.height() - 2 * textMargin, area.width() - 2 * textMargin }, Qt::AlignTop | Qt::AlignHCenter, m_yLabel);
        painter->restore();
    }

    painter->setPen(QPen { QColor { "#444444" }, 1.0 });
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(area.adjusted(0.5, 0.5, -0.5, -0.5));

    if (!m_controller) {
        return;
    }
    if (m_trailEnabled) {
        paintTrail(painter);
    }
    const auto [x, y] = toItem({ m_controller->positionX(m_pad), m_controller->positionY(m_pad) });
    auto color = m_accentColor;
    // Fainter at rest, so a pad that is being played stands out from the other three
    const bool active = m_handOn || m_controller->isPressed(m_pad) || m_controller->isMoving(m_pad);
    color.setAlphaF(active ? 1.0 : 0.6);
    painter->setPen(QPen { color, 1.5 });
    painter->drawLine(QPointF { x, 0 }, QPointF { x, area.height() });
    painter->drawLine(QPointF { 0, y }, QPointF { area.width(), y });
    painter->setBrush(color);
    painter->drawEllipse(QPointF { x, y }, 5.0, 5.0);
}

} // namespace noteahead
