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

#include "eq_curve_renderer.hpp"

#include <QPainter>
#include <QPainterPath>
#include <QPen>

#include <algorithm>
#include <cmath>

namespace noteahead {

namespace {

//! The range the curve is drawn over. The bands span 20 Hz to 20 kHz, so the drawing does too.
constexpr double MinFrequency = 20.0;
constexpr double MaxFrequency = 20000.0;

//! The frequencies that get a gridline and a label. The decades plus the two thirds between them,
//! which is as many as fit in a corner of a dialog without the labels touching.
constexpr double GridFrequencies[] { 100.0, 1000.0, 10000.0 };

constexpr qreal LabelMargin = 22.0;
constexpr qreal PlotInset = 2.0;

} // namespace

EqCurveRenderer::EqCurveRenderer(QQuickItem * parent)
  : QQuickPaintedItem { parent }
{
}

QVariantList EqCurveRenderer::response() const
{
    return m_response;
}

void EqCurveRenderer::setResponse(const QVariantList & response)
{
    m_response = response;
    update();
    emit responseChanged();
}

int EqCurveRenderer::dbRange() const
{
    return m_dbRange;
}

void EqCurveRenderer::setDbRange(int dbRange)
{
    if (m_dbRange != dbRange) {
        m_dbRange = std::max(1, dbRange);
        update();
        emit dbRangeChanged();
    }
}

QColor EqCurveRenderer::accentColor() const
{
    return m_accentColor;
}

void EqCurveRenderer::setAccentColor(const QColor & color)
{
    if (m_accentColor != color) {
        m_accentColor = color;
        update();
        emit accentColorChanged();
    }
}

double EqCurveRenderer::frequencyPosition(double hz)
{
    return std::log(hz / MinFrequency) / std::log(MaxFrequency / MinFrequency);
}

void EqCurveRenderer::paint(QPainter * painter)
{
    const qreal w = width();
    const qreal h = height();
    if (w <= 0.0 || h <= 0.0) {
        return;
    }

    painter->fillRect(QRectF(0, 0, w, h), QColor("#111111"));

    const qreal plotX = PlotInset;
    const qreal plotY = PlotInset;
    const qreal plotW = w - PlotInset * 2.0;
    const qreal plotH = h - PlotInset - LabelMargin;
    if (plotW <= 0.0 || plotH <= 0.0) {
        return;
    }

    painter->fillRect(QRectF(plotX, plotY, plotW, plotH), QColor("#1a1a1a"));

    QFont labelFont;
    labelFont.setPixelSize(9);
    labelFont.setFamily("monospace");
    painter->setFont(labelFont);

    painter->setRenderHint(QPainter::Antialiasing, true);

    // Horizontal gridlines every 6 dB, with flat drawn brighter: flat is the line a curve is read
    // against, so it has to be findable without counting the others.
    const double range = static_cast<double>(m_dbRange);
    for (int db = -m_dbRange; db <= m_dbRange; db += 6) {
        const qreal y = plotY + plotH * (0.5 - static_cast<double>(db) / (range * 2.0));
        painter->setPen(QPen(db == 0 ? QColor("#555555") : QColor("#2a2a2a"), 1));
        painter->drawLine(QPointF(plotX, y), QPointF(plotX + plotW, y));
    }

    for (auto && hz : GridFrequencies) {
        const qreal x = plotX + plotW * frequencyPosition(hz);
        painter->setPen(QPen(QColor("#2a2a2a"), 1));
        painter->drawLine(QPointF(x, plotY), QPointF(x, plotY + plotH));
        painter->setPen(QPen(QColor("#777777")));
        const QString label = hz >= 1000.0 ? QString::number(hz / 1000.0, 'g', 2) + "k" : QString::number(hz, 'g', 3);
        painter->drawText(QRectF(x - 20.0, plotY + plotH + 2.0, 40.0, LabelMargin - 4.0), Qt::AlignHCenter | Qt::AlignTop, label);
    }

    const int points = m_response.size();
    if (points < 2) {
        return;
    }

    // The curve, and the same curve closed against flat and filled. The fill is what makes a boost
    // read as a boost at a glance -- a bare line leaves the eye to work out which side of flat it is
    // on, which is the one thing this drawing exists to say.
    QPainterPath curve;
    for (int i = 0; i < points; i++) {
        const qreal x = plotX + plotW * static_cast<double>(i) / static_cast<double>(points - 1);
        const double db = std::clamp(m_response.at(i).toDouble(), -range, range);
        const qreal y = plotY + plotH * (0.5 - db / (range * 2.0));
        if (i == 0) {
            curve.moveTo(x, y);
        } else {
            curve.lineTo(x, y);
        }
    }

    QPainterPath filled = curve;
    filled.lineTo(plotX + plotW, plotY + plotH * 0.5);
    filled.lineTo(plotX, plotY + plotH * 0.5);
    filled.closeSubpath();

    QColor fill = m_accentColor;
    fill.setAlpha(60);
    painter->fillPath(filled, fill);

    painter->setPen(QPen(m_accentColor, 2));
    painter->drawPath(curve);
}

} // namespace noteahead
