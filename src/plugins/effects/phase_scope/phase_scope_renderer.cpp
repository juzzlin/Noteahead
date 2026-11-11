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

#include "phase_scope_renderer.hpp"

#include <QPainter>
#include <QPainterPath>
#include <QPen>

#include <algorithm>
#include <cmath>

namespace noteahead {

PhaseScopeRenderer::PhaseScopeRenderer(QQuickItem * parent)
  : QQuickPaintedItem { parent }
{
}

QVariantList PhaseScopeRenderer::reference() const
{
    return m_reference;
}

void PhaseScopeRenderer::setReference(const QVariantList & reference)
{
    m_reference = reference;
    emit referenceChanged();
    update();
}

QVariantList PhaseScopeRenderer::trace() const
{
    return m_trace;
}

void PhaseScopeRenderer::setTrace(const QVariantList & trace)
{
    m_trace = trace;
    emit traceChanged();
    update();
}

QColor PhaseScopeRenderer::accentColor() const
{
    return m_accentColor;
}

void PhaseScopeRenderer::setAccentColor(const QColor & color)
{
    if (m_accentColor != color) {
        m_accentColor = color;
        emit accentColorChanged();
        update();
    }
}

void PhaseScopeRenderer::paint(QPainter * painter)
{
    const auto w = width();
    const auto h = height();
    painter->fillRect(QRectF { 0, 0, w, h }, QColor { "#141414" });
    painter->setRenderHint(QPainter::Antialiasing, true);

    // The zero line, which is where the two waves crossing in opposite directions shows
    painter->setPen(QPen { QColor { "#3a3a3a" }, 1.0 });
    painter->drawLine(QPointF { 0.0, h / 2.0 }, QPointF { w, h / 2.0 });

    double peak = 1.0e-6;
    for (const auto * list : { &m_reference, &m_trace }) {
        for (auto && value : *list) {
            peak = std::max(peak, std::abs(value.toDouble()));
        }
    }
    const double scale = (h / 2.0) * 0.9 / peak;

    const auto draw = [&](const QVariantList & samples, const QColor & color, double thickness) {
        if (samples.size() < 2) {
            return;
        }
        QPainterPath path;
        for (qsizetype i = 0; i < samples.size(); i++) {
            const QPointF point { w * static_cast<double>(i) / static_cast<double>(samples.size() - 1), h / 2.0 - samples.at(i).toDouble() * scale };
            if (i == 0) {
                path.moveTo(point);
            } else {
                path.lineTo(point);
            }
        }
        painter->setPen(QPen { color, thickness });
        painter->drawPath(path);
    };
    draw(m_reference, QColor { "#9a9a9a" }, 1.5);
    draw(m_trace, m_accentColor, 2.0);
}

} // namespace noteahead
