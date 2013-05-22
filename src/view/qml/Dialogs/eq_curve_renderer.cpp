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
#include <utility>

namespace noteahead {

namespace {

//! The range the curve is drawn over. The bands span 20 Hz to 20 kHz, so the drawing does too.
constexpr double MinFrequency = 20.0;
constexpr double MaxFrequency = 20000.0;

//! The frequencies that get a gridline and a label. The decades plus the two thirds between them,
//! which is as many as fit in a corner of a dialog without the labels touching.
constexpr double GridFrequencies[] { 100.0, 1000.0, 10000.0 };

//! The top of the spectrum's own scale, which reaches spectrumRange below it. Independent of the
//! curve's: a spectrum spans tens of dB where a curve spans a few either side of flat.
constexpr double SpectrumTopDb = 0.0;

//! Below this a band is too quiet for a difference to mean anything: both taps are reading the
//! analyzer's floor, and what is left between them is noise.
constexpr float DifferenceGateDb = -70.0f;

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

QVariantList EqCurveRenderer::secondaryResponse() const
{
    return m_secondaryResponse;
}

void EqCurveRenderer::setSecondaryResponse(const QVariantList & response)
{
    m_secondaryResponse = response;
    update();
    emit secondaryResponseChanged();
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

int EqCurveRenderer::spectrumRange() const
{
    return m_spectrumRange;
}

void EqCurveRenderer::setSpectrumRange(int range)
{
    range = std::max(range, 1);
    if (m_spectrumRange != range) {
        m_spectrumRange = range;
        update();
        emit spectrumRangeChanged();
    }
}

QColor EqCurveRenderer::spectrumPostColor() const
{
    return m_spectrumPostColor;
}

void EqCurveRenderer::setSpectrumPostColor(const QColor & color)
{
    if (m_spectrumPostColor != color) {
        m_spectrumPostColor = color;
        update();
        emit spectrumColorsChanged();
    }
}

QColor EqCurveRenderer::spectrumPreColor() const
{
    return m_spectrumPreColor;
}

void EqCurveRenderer::setSpectrumPreColor(const QColor & color)
{
    if (m_spectrumPreColor != color) {
        m_spectrumPreColor = color;
        update();
        emit spectrumColorsChanged();
    }
}

int EqCurveRenderer::spectrumMode() const
{
    return m_spectrumMode;
}

void EqCurveRenderer::setSpectrumMode(int mode)
{
    if (m_spectrumMode != mode) {
        m_spectrumMode = std::clamp(mode, static_cast<int>(SpectrumOff), static_cast<int>(SpectrumPostMinusPre));
        m_preLevels.clear();
        m_postLevels.clear();
        update();
        emit spectrumModeChanged();
    }
}

bool EqCurveRenderer::spectrumUsesPre() const
{
    return m_spectrumMode == SpectrumPre || m_spectrumMode == SpectrumBoth || m_spectrumMode == SpectrumPreMinusPost || m_spectrumMode == SpectrumPostMinusPre;
}

bool EqCurveRenderer::spectrumUsesPost() const
{
    return m_spectrumMode == SpectrumPost || m_spectrumMode == SpectrumBoth || m_spectrumMode == SpectrumPreMinusPost || m_spectrumMode == SpectrumPostMinusPre;
}

void EqCurveRenderer::setSpectrumLevels(std::vector<float> pre, std::vector<float> post)
{
    m_preLevels = std::move(pre);
    m_postLevels = std::move(post);
    update();
}

void EqCurveRenderer::setSpectrumLayout(const std::vector<std::pair<float, float>> & positions, uint32_t generation)
{
    m_spectrumPositions = positions;
    m_spectrumLayoutGeneration = generation;
    update();
}

uint32_t EqCurveRenderer::spectrumLayoutGeneration() const
{
    return m_spectrumLayoutGeneration;
}

qreal EqCurveRenderer::bandCentreX(size_t band, const QRectF & plot) const
{
    const auto & [lo, hi] = m_spectrumPositions.at(band);
    return plot.x() + plot.width() * 0.5 * static_cast<double>(lo + hi);
}

QPainterPath EqCurveRenderer::spectrumPath(const std::vector<float> & levels, const QRectF & plot) const
{
    const double floorDb = SpectrumTopDb - static_cast<double>(m_spectrumRange);
    QPainterPath path;
    for (size_t i = 0; i < levels.size(); i++) {
        const double db = std::clamp(static_cast<double>(levels.at(i)), floorDb, SpectrumTopDb);
        const qreal y = plot.y() + plot.height() * (SpectrumTopDb - db) / (SpectrumTopDb - floorDb);
        if (i == 0) {
            path.moveTo(bandCentreX(i, plot), y);
        } else {
            path.lineTo(bandCentreX(i, plot), y);
        }
    }
    return path;
}

void EqCurveRenderer::paintSpectrum(QPainter & painter, const QRectF & plot) const
{
    const auto fits = [this](const std::vector<float> & levels) {
        return levels.size() >= 2 && levels.size() == m_spectrumPositions.size();
    };

    // The output is the one filled whenever it is shown: it is what is heard, and the input is
    // there to be compared against it
    const bool fillPre = m_spectrumMode == SpectrumPre;
    if (const auto & filled = fillPre ? m_preLevels : m_postLevels; fits(filled) && m_spectrumMode != SpectrumOff) {
        const QPainterPath line = spectrumPath(filled, plot);
        QPainterPath area = line;
        area.lineTo(bandCentreX(filled.size() - 1, plot), plot.bottom());
        area.lineTo(bandCentreX(0, plot), plot.bottom());
        area.closeSubpath();
        // The color of whichever tap it is, so that Pre alone reads as the input it is
        const QColor color = fillPre ? m_spectrumPreColor : m_spectrumPostColor;
        QColor fill = color;
        fill.setAlpha(40);
        painter.fillPath(area, fill);
        QColor outline = color;
        outline.setAlpha(150);
        painter.setPen(QPen { outline, 1 });
        painter.drawPath(line);
    }

    if (m_spectrumMode == SpectrumBoth && fits(m_preLevels)) {
        QColor color = m_spectrumPreColor;
        color.setAlpha(200);
        QPen pen { color, 1 };
        pen.setStyle(Qt::DotLine);
        painter.setPen(pen);
        painter.drawPath(spectrumPath(m_preLevels, plot));
    }
}

void EqCurveRenderer::paintSpectrumDifference(QPainter & painter, const QRectF & plot, bool inverted) const
{
    const size_t bands = m_spectrumPositions.size();
    if (bands < 2 || m_preLevels.size() != bands || m_postLevels.size() != bands) {
        return;
    }

    const double range = static_cast<double>(m_dbRange);
    const auto & reference = inverted ? m_postLevels : m_preLevels;
    QPainterPath path;
    bool drawing = false;
    for (size_t i = 0; i < bands; i++) {
        if (reference.at(i) < DifferenceGateDb) {
            drawing = false;
            continue;
        }
        const double difference = static_cast<double>(m_postLevels.at(i) - m_preLevels.at(i)) * (inverted ? -1.0 : 1.0);
        const qreal y = plot.y() + plot.height() * (0.5 - std::clamp(difference, -range, range) / (range * 2.0));
        if (drawing) {
            path.lineTo(bandCentreX(i, plot), y);
        } else {
            path.moveTo(bandCentreX(i, plot), y);
            drawing = true;
        }
    }
    QColor color = m_spectrumPreColor;
    color.setAlpha(230);
    painter.setPen(QPen { color, 1.5 });
    painter.drawPath(path);
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

    const QRectF plot { plotX, plotY, plotW, plotH };

    // Behind everything the equalizer draws: the curve is what this plot is for, and the spectrum is
    // there to read it against
    if (m_spectrumMode == SpectrumPre || m_spectrumMode == SpectrumPost || m_spectrumMode == SpectrumBoth) {
        painter->save();
        painter->setClipRect(plot);
        paintSpectrum(*painter, plot);
        painter->restore();
    }

    // Drawn first and left unfilled, so that where the two coincide the filled one wins and the
    // secondary reads as the lesser of the two without needing a legend to say so.
    if (m_secondaryResponse.size() >= 2) {
        QPen pen { m_accentColor.lighter(120), 1 };
        pen.setStyle(Qt::DashLine);
        painter->setPen(pen);
        painter->setOpacity(0.55);
        painter->drawPath(curvePath(m_secondaryResponse, plot));
        painter->setOpacity(1.0);
    }

    if (m_response.size() < 2) {
        return;
    }

    // The curve, and the same curve closed against flat and filled. The fill is what makes a boost
    // read as a boost at a glance -- a bare line leaves the eye to work out which side of flat it is
    // on, which is the one thing this drawing exists to say.
    const QPainterPath curve = curvePath(m_response, plot);

    QPainterPath filled = curve;
    filled.lineTo(plotX + plotW, plotY + plotH * 0.5);
    filled.lineTo(plotX, plotY + plotH * 0.5);
    filled.closeSubpath();

    QColor fill = m_accentColor;
    fill.setAlpha(60);
    painter->fillPath(filled, fill);

    painter->setPen(QPen(m_accentColor, 2));
    painter->drawPath(curve);

    // Over the curve rather than under it: where the two coincide is the expected case, and a line
    // hidden exactly there would look like no line at all
    if (m_spectrumMode == SpectrumPreMinusPost || m_spectrumMode == SpectrumPostMinusPre) {
        paintSpectrumDifference(*painter, plot, m_spectrumMode == SpectrumPreMinusPost);
    }
}

QPainterPath EqCurveRenderer::curvePath(const QVariantList & response, const QRectF & plot) const
{
    const double range = static_cast<double>(m_dbRange);
    const int points = response.size();

    QPainterPath path;
    for (int i = 0; i < points; i++) {
        const qreal x = plot.x() + plot.width() * static_cast<double>(i) / static_cast<double>(points - 1);
        const double db = std::clamp(response.at(i).toDouble(), -range, range);
        const qreal y = plot.y() + plot.height() * (0.5 - db / (range * 2.0));
        if (i == 0) {
            path.moveTo(x, y);
        } else {
            path.lineTo(x, y);
        }
    }
    return path;
}

} // namespace noteahead
