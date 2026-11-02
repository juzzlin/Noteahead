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

#include "wave_shape_renderer.hpp"

#include <QPainter>
#include <QPainterPath>

#include <algorithm>

namespace noteahead {

WaveShapeRenderer::WaveShapeRenderer(QQuickItem * parent)
  : QQuickPaintedItem { parent }
{
    setAntialiasing(true);
}

QList<qreal> WaveShapeRenderer::samples() const
{
    return m_samples;
}

void WaveShapeRenderer::setSamples(const QList<qreal> & samples)
{
    if (m_samples != samples) {
        m_samples = samples;
        emit samplesChanged();
        update();
    }
}

QColor WaveShapeRenderer::accentColor() const
{
    return m_accentColor;
}

void WaveShapeRenderer::setAccentColor(const QColor & color)
{
    if (m_accentColor != color) {
        m_accentColor = color;
        emit accentColorChanged();
        update();
    }
}

bool WaveShapeRenderer::dimmed() const
{
    return m_dimmed;
}

void WaveShapeRenderer::setDimmed(bool dimmed)
{
    if (m_dimmed != dimmed) {
        m_dimmed = dimmed;
        emit dimmedChanged();
        update();
    }
}

void WaveShapeRenderer::paint(QPainter * painter)
{
    // The same backdrop and centre line as the automation previews, so the two read as one family
    painter->fillRect(QRectF { 0, 0, width(), height() }, QColor { "#1a1a1a" });
    const double pen = 1.5;
    const double centre = height() / 2.0;
    // Room for half the pen at the top and bottom, where a wave at full swing runs
    const double swing = std::max(centre - pen, 1.0);
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setPen(QPen { QColor { "#333333" }, 1 });
    painter->drawLine(QPointF { 0.0, centre }, QPointF { width(), centre });

    if (m_samples.size() < 2) {
        return;
    }
    QPainterPath path;
    const double step = width() / static_cast<double>(m_samples.size() - 1);
    for (qsizetype i = 0; i < m_samples.size(); i++) {
        const QPointF point { static_cast<double>(i) * step, centre - std::clamp(m_samples.at(i), -1.0, 1.0) * swing };
        if (i == 0) {
            path.moveTo(point);
        } else {
            path.lineTo(point);
        }
    }
    QColor color = m_accentColor;
    color.setAlphaF(m_dimmed ? 0.35 : 1.0);
    painter->setPen(QPen { color, pen });
    painter->drawPath(path);
}

} // namespace noteahead
