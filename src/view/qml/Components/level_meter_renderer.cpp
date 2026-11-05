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
#include "level_meter_renderer.hpp"

#include <QFontMetricsF>
#include <QPainter>

#include <algorithm>

namespace noteahead {

namespace {
// The meter dialogs' colours, so that a bar reads the same on the panel
const QColor cautionColor { "#ffaa00" };
const QColor dangerColor { "#ff4444" };
const QColor troughColor { "#1a1a1a" };
const QColor tickColor { "#444444" };
const QColor textColor { "#888888" };
const double scaleWidth = 30.0;
const double barGap = 6.0;
} // namespace

LevelMeterRenderer::LevelMeterRenderer(QQuickItem * parent)
  : QQuickPaintedItem { parent }
{
    setAntialiasing(true);
}

QColor LevelMeterRenderer::accentColor() const
{
    return m_accentColor;
}

void LevelMeterRenderer::setAccentColor(const QColor & color)
{
    if (m_accentColor != color) {
        m_accentColor = color;
        emit accentColorChanged();
        update();
    }
}

const LevelMeterRenderer::Scale & LevelMeterRenderer::scale() const
{
    return m_scale;
}

void LevelMeterRenderer::setScale(const Scale & scale)
{
    if (m_scale != scale) {
        m_scale = scale;
        update();
    }
}

const std::vector<LevelMeterRenderer::Bar> & LevelMeterRenderer::bars() const
{
    return m_bars;
}

void LevelMeterRenderer::setBars(const std::vector<Bar> & bars)
{
    if (m_bars != bars) {
        m_bars = bars;
        update();
    }
}

double LevelMeterRenderer::fraction(double value) const
{
    return m_scale.max > m_scale.min ? std::clamp((value - m_scale.min) / (m_scale.max - m_scale.min), 0.0, 1.0) : 0.0;
}

QColor LevelMeterRenderer::colorOf(double value) const
{
    if (value >= m_scale.danger) {
        return dangerColor;
    }
    return value >= m_scale.caution ? cautionColor : m_accentColor;
}

QString LevelMeterRenderer::textOf(double value) const
{
    return value <= m_scale.min ? QString { "−∞" } : QString::number(value, 'f', 1);
}

void LevelMeterRenderer::paint(QPainter * painter)
{
    auto font = painter->font();
    font.setPointSizeF(8.0);
    painter->setFont(font);
    const QFontMetricsF metrics { font };
    const double lineHeight = metrics.height();

    // A caption over each bar and its number under it, the scale's unit over the scale
    const QRectF meterArea { scaleWidth, lineHeight + 2, width() - scaleWidth, height() - 2 * lineHeight - 4 };
    if (meterArea.width() <= 0 || meterArea.height() <= 0) {
        return;
    }
    const auto yOf = [&](double value) { return meterArea.bottom() - fraction(value) * meterArea.height(); };

    painter->setPen(textColor);
    for (const double tick : m_scale.ticks) {
        const QRectF labelArea { 0, yOf(tick) - lineHeight / 2, scaleWidth - 4, lineHeight };
        painter->drawText(labelArea, Qt::AlignRight | Qt::AlignVCenter, QString::number(tick));
    }
    painter->drawText(QRectF { 0, 0, scaleWidth - 4, lineHeight }, Qt::AlignRight | Qt::AlignTop, m_scale.unit);

    if (m_bars.empty()) {
        return;
    }
    const double barWidth = (meterArea.width() - barGap * static_cast<double>(m_bars.size() - 1)) / static_cast<double>(m_bars.size());
    for (size_t i = 0; i < m_bars.size(); i++) {
        const auto & bar = m_bars.at(i);
        const QRectF trough { meterArea.left() + static_cast<double>(i) * (barWidth + barGap), meterArea.top(), barWidth, meterArea.height() };
        painter->fillRect(trough, troughColor);
        painter->setPen(QPen { tickColor, 1.0 });
        for (const double tick : m_scale.ticks) {
            painter->drawLine(QPointF { trough.left(), yOf(tick) }, QPointF { trough.right(), yOf(tick) });
        }
        const double top = yOf(bar.value);
        painter->fillRect(QRectF { trough.left() + 2, top, trough.width() - 4, trough.bottom() - top }, colorOf(bar.value));
        if (bar.hold && *bar.hold > m_scale.min) {
            painter->fillRect(QRectF { trough.left(), yOf(*bar.hold) - 1, trough.width(), 2 }, colorOf(*bar.hold));
        }

        painter->setPen(textColor);
        painter->drawText(QRectF { trough.left(), 0, trough.width(), lineHeight }, Qt::AlignHCenter | Qt::AlignTop, bar.label);
        const double shown = bar.hold.value_or(bar.value);
        painter->setPen(colorOf(shown));
        painter->drawText(QRectF { trough.left(), trough.bottom() + 2, trough.width(), lineHeight }, Qt::AlignHCenter | Qt::AlignTop, textOf(shown));
    }
}

} // namespace noteahead
