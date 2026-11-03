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
#include <chrono>

namespace noteahead {

namespace {
// About a display frame: smooth enough to watch the glide, and it only runs while the pad moves
const auto frameInterval = std::chrono::milliseconds { 16 };
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
        if (m_controller) {
            // The same pad can be shown twice, in its dialog and on the performance panel
            connect(m_controller, &XyPadController::padPressed, this, &XyPadRenderer::onPadPressed);
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
    update();
    // The pad is still gliding after the hand has gone, so the redraws stop only once it is home.
    // The hand can also be on it in another view of the same pad.
    if (!m_handOn && (!m_controller || (!m_controller->isPressed(m_pad) && !m_controller->isMoving(m_pad)))) {
        m_frameTimer.stop();
    }
}

void XyPadRenderer::onPadPressed(int pad)
{
    if (pad == m_pad && isVisible()) {
        m_frameTimer.start();
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
            m_frameTimer.stop();
        }
    }
    QQuickPaintedItem::itemChange(change, data);
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
    const double x = (m_controller->positionX(m_pad) + 1.0) / 2.0 * area.width();
    const double y = (1.0 - m_controller->positionY(m_pad)) / 2.0 * area.height();
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
