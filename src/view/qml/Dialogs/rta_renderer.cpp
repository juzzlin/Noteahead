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

#include "rta_renderer.hpp"

#include <QPainter>
#include <QPen>
#include <QVector>

#include <algorithm>
#include <cmath>

namespace noteahead {

RtaRenderer::RtaRenderer(QQuickItem * parent)
  : QQuickPaintedItem { parent }
{
}

QVariantList RtaRenderer::bands() const
{
    return m_bands;
}

void RtaRenderer::setBands(const QVariantList & bands)
{
    m_bands = bands;
    update();
}

QVariantList RtaRenderer::bandPositions() const
{
    return m_bandPositions;
}

void RtaRenderer::setBandPositions(const QVariantList & positions)
{
    m_bandPositions = positions;
    update();
}

int RtaRenderer::dbRange() const
{
    return m_dbRange;
}

void RtaRenderer::setDbRange(int dbRange)
{
    if (m_dbRange != dbRange) {
        m_dbRange = dbRange;
        emit dbRangeChanged();
        update();
    }
}

bool RtaRenderer::showPinkNoise() const
{
    return m_showPinkNoise;
}

void RtaRenderer::setShowPinkNoise(bool show)
{
    if (m_showPinkNoise != show) {
        m_showPinkNoise = show;
        emit showPinkNoiseChanged();
        update();
    }
}

float RtaRenderer::pinkNoiseLevel() const
{
    return m_pinkNoiseLevel;
}

void RtaRenderer::setPinkNoiseLevel(float level)
{
    if (!qFuzzyCompare(m_pinkNoiseLevel, level)) {
        m_pinkNoiseLevel = level;
        emit pinkNoiseLevelChanged();
        update();
    }
}

QColor RtaRenderer::accentColor() const
{
    return m_accentColor;
}

void RtaRenderer::setAccentColor(const QColor & color)
{
    if (m_accentColor != color) {
        m_accentColor = color;
        emit accentColorChanged();
        update();
    }
}

// Dark accent at low levels → full accent at medium → near-white at clipping.
QColor RtaRenderer::barColor(float levelDb, float floorDb, const QColor & accent)
{
    const float t = std::clamp((levelDb - floorDb) / (0.0f - floorDb), 0.0f, 1.0f);
    const float bright = 0.15f + 0.85f * t;
    if (t < 0.90f) {
        return QColor(
          static_cast<int>(accent.red() * bright),
          static_cast<int>(accent.green() * bright),
          static_cast<int>(accent.blue() * bright));
    } else {
        // Near 0 dBFS: blend to bright white-red warning
        const float s = (t - 0.90f) / 0.10f;
        return QColor(
          static_cast<int>(accent.red() * bright + s * (255 - accent.red() * bright)),
          static_cast<int>(accent.green() * bright * (1.0f - s * 0.8f)),
          static_cast<int>(accent.blue() * bright * (1.0f - s)));
    }
}

// Normalized log position of freq within [20, 20000].
double RtaRenderer::logFreqNorm(double freq)
{
    static constexpr double logLo = std::log10(20.0);
    static constexpr double logHi = std::log10(20000.0);
    return (std::log10(std::max(freq, 1.0)) - logLo) / (logHi - logLo);
}

void RtaRenderer::paint(QPainter * painter)
{
    const int B = m_bands.size();
    if (B == 0) {
        painter->fillRect(boundingRect(), QColor("#111111"));
        return;
    }

    const float floorDb = -static_cast<float>(m_dbRange);

    // Layout margins
    static constexpr qreal leftMargin = 42.0;
    static constexpr qreal rightMargin = 8.0;
    static constexpr qreal topMargin = 6.0;
    static constexpr qreal bottomMargin = 22.0;

    const qreal W = width();
    const qreal H = height();
    const qreal plotX = leftMargin;
    const qreal plotY = topMargin;
    const qreal plotW = W - leftMargin - rightMargin;
    const qreal plotH = H - topMargin - bottomMargin;

    // Background
    painter->fillRect(QRectF(0, 0, W, H), QColor("#111111"));
    painter->fillRect(QRectF(plotX, plotY, plotW, plotH), QColor("#1a1a1a"));

    QFont labelFont;
    labelFont.setPixelSize(10);
    labelFont.setFamily("monospace");
    painter->setFont(labelFont);
    const QFontMetricsF fm { labelFont };

    // dB grid lines: every 6 dB for -30/-45, every 10 dB for -60/-80
    const int gridStep = (m_dbRange <= 45) ? 6 : 10;
    for (int db = 0; db >= -m_dbRange; db -= gridStep) {
        const qreal t = static_cast<qreal>(db - floorDb) / (0.0f - floorDb);
        const qreal y = plotY + plotH * (1.0 - t);
        const QColor gridColor = (db == 0) ? QColor("#555555") : QColor("#303030");
        painter->setPen(QPen(gridColor, 1));
        painter->drawLine(QPointF(plotX, y), QPointF(plotX + plotW, y));
        // dB label
        const QString label = QString::number(db);
        painter->setPen(QColor("#888888"));
        painter->drawText(QRectF(0, y - fm.height() * 0.5, leftMargin - 4, fm.height()),
                          Qt::AlignRight | Qt::AlignVCenter, label);
    }

    // Frequency grid lines
    static const QVector<double> gridFreqs = { 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000 };
    static const QVector<QString> gridLabels = { "20", "50", "100", "200", "500", "1k", "2k", "5k", "10k", "20k" };
    painter->setPen(QPen(QColor("#2a2a2a"), 1));
    for (int i = 0; i < gridFreqs.size(); i++) {
        const qreal x = plotX + logFreqNorm(gridFreqs[i]) * plotW;
        painter->setPen(QPen(QColor("#2a2a2a"), 1));
        painter->drawLine(QPointF(x, plotY), QPointF(x, plotY + plotH));
        // Frequency label below
        const QString & lbl = gridLabels[i];
        const qreal lw = fm.horizontalAdvance(lbl);
        painter->setPen(QColor("#666666"));
        painter->drawText(QRectF(x - lw * 0.5, plotY + plotH + 3, lw + 2, fm.height()), lbl);
    }

    // Bars — use per-band log-frequency positions when available, else equal spacing.
    const bool hasPositions = (m_bandPositions.size() == B * 2);
    painter->setClipRect(QRectF(plotX, plotY, plotW, plotH));
    for (int b = 0; b < B; b++) {
        const float levelDb = std::clamp(m_bands[b].toFloat(), floorDb, 0.0f);
        const float t = (levelDb - floorDb) / (0.0f - floorDb);
        const qreal barH = t * plotH;

        const qreal xLo = hasPositions
          ? plotX + m_bandPositions[b * 2].toDouble() * plotW
          : plotX + static_cast<qreal>(b) / B * plotW;
        const qreal xHi = hasPositions
          ? plotX + m_bandPositions[b * 2 + 1].toDouble() * plotW
          : plotX + static_cast<qreal>(b + 1) / B * plotW;
        const qreal barW = std::max(1.0, xHi - xLo - 1.0);
        const qreal barY = plotY + plotH - barH;

        painter->fillRect(QRectF(xLo, barY, barW, barH), barColor(levelDb, floorDb, m_accentColor));
    }
    painter->setClipping(false);

    // Pink noise reference lines
    if (m_showPinkNoise) {
        const auto dbToY = [&](float db) {
            const float t = std::clamp((db - floorDb) / (0.0f - floorDb), 0.0f, 1.0f);
            return plotY + plotH * (1.0f - t);
        };

        // Flat line: what pink noise actually looks like on an octave-band analyzer
        {
            const qreal y = dbToY(m_pinkNoiseLevel);
            QPen flatPen(QColor("#cc55cc"), 1, Qt::DashLine);
            flatPen.setDashPattern({ 6, 4 });
            painter->setPen(flatPen);
            painter->drawLine(QPointF(plotX, y), QPointF(plotX + plotW, y));
            const QString label = QString("pink %1 dB").arg(static_cast<int>(m_pinkNoiseLevel));
            painter->setPen(QColor("#cc55cc"));
            painter->drawText(QPointF(plotX + 4, y - 3), label);
        }

        // Sloped line: -3 dB/octave mixing reference anchored at m_pinkNoiseLevel @ 1 kHz
        {
            // level(f) = m_pinkNoiseLevel - 3 * log2(f / 1000)
            const float dbLeft = m_pinkNoiseLevel - 3.0f * std::log2(20.0f / 1000.0f);
            const float dbRight = m_pinkNoiseLevel - 3.0f * std::log2(20000.0f / 1000.0f);
            const qreal yLeft = dbToY(dbLeft);
            const qreal yRight = dbToY(dbRight);

            painter->setClipRect(QRectF(plotX, plotY, plotW, plotH));
            QPen slopePen(QColor("#cc55cc"), 1, Qt::DotLine);
            painter->setPen(slopePen);
            painter->drawLine(QPointF(plotX, yLeft), QPointF(plotX + plotW, yRight));
            painter->setClipping(false);

            // Label near the 1 kHz anchor point
            const qreal xAt1k = plotX + logFreqNorm(1000.0) * plotW;
            const qreal yAt1k = dbToY(m_pinkNoiseLevel);
            painter->setPen(QColor("#cc55cc"));
            painter->drawText(QPointF(xAt1k + 4, yAt1k - 3), "-3 dB/oct");
        }
    }

    // Plot border
    painter->setPen(QPen(QColor("#444444"), 1));
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(QRectF(plotX, plotY, plotW, plotH));
}

} // namespace noteahead
