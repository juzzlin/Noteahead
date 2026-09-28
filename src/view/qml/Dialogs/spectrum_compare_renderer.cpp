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

#include "spectrum_compare_renderer.hpp"

#include <QFontMetrics>
#include <QPainter>
#include <QPainterPath>
#include <QPen>

#include <algorithm>
#include <cmath>

namespace noteahead {

namespace {

//! The range drawn, which is the range the analyzer reports bands over.
constexpr double MinHz = 25.0;
constexpr double MaxHz = 16000.0;

//! Room for the dB scale on the left and the frequency labels below.
constexpr double LeftMargin = 34.0;
constexpr double BottomMargin = 18.0;
constexpr double TopMargin = 18.0;
constexpr double RightMargin = 8.0;

QString frequencyLabel(double frequency)
{
    return frequency >= 1000.0
      ? QString { "%1k" }.arg(frequency / 1000.0, 0, 'f', frequency >= 10000.0 ? 0 : 1)
      : QString { "%1" }.arg(frequency, 0, 'f', 0);
}

} // namespace

SpectrumCompareRenderer::SpectrumCompareRenderer(QQuickItem * parent)
  : QQuickPaintedItem { parent }
{
}

void SpectrumCompareRenderer::setBands(const Bands & left, const Bands & right)
{
    m_left = left;
    m_right = right;
    update();
}

void SpectrumCompareRenderer::setNames(const QString & left, const QString & right)
{
    m_leftName = left;
    m_rightName = right;
    update();
}

int SpectrumCompareRenderer::dbRange() const
{
    return m_dbRange;
}

void SpectrumCompareRenderer::setDbRange(int dbRange)
{
    if (m_dbRange != dbRange) {
        m_dbRange = dbRange;
        emit dbRangeChanged();
        update();
    }
}

QColor SpectrumCompareRenderer::leftColor() const
{
    return m_leftColor;
}

void SpectrumCompareRenderer::setLeftColor(const QColor & color)
{
    if (m_leftColor != color) {
        m_leftColor = color;
        emit leftColorChanged();
        update();
    }
}

QColor SpectrumCompareRenderer::rightColor() const
{
    return m_rightColor;
}

void SpectrumCompareRenderer::setRightColor(const QColor & color)
{
    if (m_rightColor != color) {
        m_rightColor = color;
        emit rightColorChanged();
        update();
    }
}

double SpectrumCompareRenderer::position(double frequency)
{
    const double clamped = std::clamp(frequency, MinHz, MaxHz);
    return std::log(clamped / MinHz) / std::log(MaxHz / MinHz);
}

void SpectrumCompareRenderer::paintGrid(QPainter * painter)
{
    const double plotWidth = width() - LeftMargin - RightMargin;
    const double plotHeight = height() - TopMargin - BottomMargin;
    if (plotWidth <= 0.0 || plotHeight <= 0.0) {
        return;
    }

    painter->setPen(QPen { QColor { "#3a3a3a" }, 1 });
    painter->setFont(QFont { painter->font().family(), 7 });

    // A line every 6 dB, and a brighter one at the centre: the difference is read off how far a bar
    // reaches, so the scale has to be there to read it against.
    for (int db = -m_dbRange; db <= m_dbRange; db += 6) {
        const double y = TopMargin + plotHeight * (0.5 - static_cast<double>(db) / (2.0 * m_dbRange));
        painter->setPen(QPen { db == 0 ? QColor { "#666666" } : QColor { "#303030" }, 1 });
        painter->drawLine(QPointF { LeftMargin, y }, QPointF { LeftMargin + plotWidth, y });
        painter->setPen(QColor { "#888888" });
        painter->drawText(QRectF { 0.0, y - 7.0, LeftMargin - 4.0, 14.0 }, Qt::AlignRight | Qt::AlignVCenter, QString::number(db));
    }

    for (const double frequency : { 31.5, 63.0, 125.0, 250.0, 500.0, 1000.0, 2000.0, 4000.0, 8000.0, 16000.0 }) {
        const double x = LeftMargin + plotWidth * position(frequency);
        painter->setPen(QPen { QColor { "#303030" }, 1 });
        painter->drawLine(QPointF { x, TopMargin }, QPointF { x, TopMargin + plotHeight });
        painter->setPen(QColor { "#888888" });
        painter->drawText(QRectF { x - 18.0, TopMargin + plotHeight + 2.0, 36.0, BottomMargin - 2.0 },
                          Qt::AlignHCenter | Qt::AlignTop, frequencyLabel(frequency));
    }
}

void SpectrumCompareRenderer::paintCurve(QPainter * painter, const Bands & bands, const QColor & color)
{
    if (bands.size() < 2) {
        return;
    }

    const double plotWidth = width() - LeftMargin - RightMargin;
    const double plotHeight = height() - TopMargin - BottomMargin;

    QPainterPath path;
    for (size_t i = 0; i < bands.size(); i++) {
        const double x = LeftMargin + plotWidth * position(bands[i].first);
        const double y = TopMargin + plotHeight * (0.5 - std::clamp(static_cast<double>(bands[i].second), -static_cast<double>(m_dbRange), static_cast<double>(m_dbRange)) / (2.0 * m_dbRange));
        if (!i) {
            path.moveTo(x, y);
        } else {
            path.lineTo(x, y);
        }
    }

    painter->setPen(QPen { color, 2 });
    painter->setBrush(Qt::NoBrush);
    painter->drawPath(path);
}

void SpectrumCompareRenderer::paintDifference(QPainter * painter)
{
    if (m_left.empty() || m_right.empty()) {
        return;
    }

    const double plotWidth = width() - LeftMargin - RightMargin;
    const double plotHeight = height() - TopMargin - BottomMargin;
    const double centreY = TopMargin + plotHeight * 0.5;

    // One bar per band the two files share. A band only one of them reaches has nothing to compare
    // against, so it is left out rather than drawn against zero.
    const double barWidth = std::max(2.0, plotWidth / static_cast<double>(m_left.size()) - 2.0);
    for (auto && band : m_left) {
        const auto it = std::find_if(m_right.begin(), m_right.end(), [&](auto && candidate) {
            return std::abs(candidate.first - band.first) < 0.1;
        });
        if (it == m_right.end()) {
            continue;
        }
        const double difference = std::clamp(static_cast<double>(it->second - band.second), -static_cast<double>(m_dbRange), static_cast<double>(m_dbRange));
        const double x = LeftMargin + plotWidth * position(band.first);
        const double y = centreY - plotHeight * difference / (2.0 * m_dbRange);
        auto color = difference >= 0.0 ? m_rightColor : m_leftColor;
        color.setAlpha(110);
        painter->fillRect(QRectF { x - barWidth * 0.5, std::min(y, centreY), barWidth, std::abs(y - centreY) }, color);
    }
}

void SpectrumCompareRenderer::paintLegend(QPainter * painter)
{
    painter->setFont(QFont { painter->font().family(), 8 });
    const QFontMetrics metrics { painter->font() };

    double x = LeftMargin;
    const auto entry = [&](const QString & text, const QColor & color) {
        if (text.isEmpty()) {
            return;
        }
        painter->fillRect(QRectF { x, 4.0, 10.0, 3.0 }, color);
        painter->setPen(QColor { "#cccccc" });
        painter->drawText(QRectF { x + 14.0, 0.0, static_cast<double>(metrics.horizontalAdvance(text)) + 4.0, TopMargin }, Qt::AlignLeft | Qt::AlignVCenter, text);
        x += 14.0 + metrics.horizontalAdvance(text) + 16.0;
    };

    entry(m_leftName, m_leftColor);
    entry(m_rightName, m_rightColor);
}

void SpectrumCompareRenderer::paint(QPainter * painter)
{
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->fillRect(boundingRect(), QColor { "#1b1b1b" });

    paintGrid(painter);
    paintDifference(painter);
    paintCurve(painter, m_left, m_leftColor);
    paintCurve(painter, m_right, m_rightColor);
    paintLegend(painter);
}

} // namespace noteahead
