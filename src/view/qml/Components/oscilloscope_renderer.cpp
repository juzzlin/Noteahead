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

#include "oscilloscope_renderer.hpp"

#include <QPainter>
#include <QPainterPath>
#include <QPen>

#include <algorithm>

namespace noteahead {

OscilloscopeRenderer::OscilloscopeRenderer(QQuickItem * parent)
  : QQuickPaintedItem { parent }
{
}

QVariantList OscilloscopeRenderer::samples() const
{
    return m_samples;
}

void OscilloscopeRenderer::setSamples(const QVariantList & samples)
{
    m_samples = samples;
    emit samplesChanged();
    update();
}

QColor OscilloscopeRenderer::accentColor() const
{
    return m_accentColor;
}

void OscilloscopeRenderer::setAccentColor(const QColor & color)
{
    if (m_accentColor != color) {
        m_accentColor = color;
        emit accentColorChanged();
        update();
    }
}

qreal OscilloscopeRenderer::gain() const
{
    return m_gain;
}

void OscilloscopeRenderer::setGain(qreal gain)
{
    if (!qFuzzyCompare(m_gain, gain)) {
        m_gain = gain;
        emit gainChanged();
        update();
    }
}

bool OscilloscopeRenderer::showGrid() const
{
    return m_showGrid;
}

void OscilloscopeRenderer::setShowGrid(bool show)
{
    if (m_showGrid != show) {
        m_showGrid = show;
        emit showGridChanged();
        update();
    }
}

void OscilloscopeRenderer::paint(QPainter * painter)
{
    const qreal w = width();
    const qreal h = height();

    // Background
    painter->fillRect(QRectF(0, 0, w, h), QColor("#111111"));

    painter->setRenderHint(QPainter::Antialiasing, true);

    // Grid: center line plus quarter-height reference lines.
    if (m_showGrid) {
        painter->setPen(QPen(QColor("#2a2a2a"), 1));
        painter->drawLine(QPointF(0, h * 0.25), QPointF(w, h * 0.25));
        painter->drawLine(QPointF(0, h * 0.75), QPointF(w, h * 0.75));
        painter->setPen(QPen(QColor("#444444"), 1));
        painter->drawLine(QPointF(0, h * 0.5), QPointF(w, h * 0.5));
    }

    const int n = m_samples.size();
    if (n < 2) {
        // Flat idle line
        painter->setPen(QPen(m_accentColor, 1.5));
        painter->drawLine(QPointF(0, h * 0.5), QPointF(w, h * 0.5));
        return;
    }

    // Map samples to a path: x across the width, y centered with vertical gain, clamped to bounds.
    QPainterPath path;
    for (int i = 0; i < n; i++) {
        const qreal x = w * static_cast<qreal>(i) / static_cast<qreal>(n - 1);
        const qreal value = std::clamp(m_samples[i].toReal() * m_gain, -1.0, 1.0);
        const qreal y = h * 0.5 - value * (h * 0.5 - 2.0);
        if (i == 0) {
            path.moveTo(x, y);
        } else {
            path.lineTo(x, y);
        }
    }

    painter->setPen(QPen(m_accentColor, 1.5));
    painter->setBrush(Qt::NoBrush);
    painter->drawPath(path);

    // Border
    painter->setPen(QPen(QColor("#444444"), 1));
    painter->drawRect(QRectF(0.5, 0.5, w - 1.0, h - 1.0));
}

} // namespace noteahead
