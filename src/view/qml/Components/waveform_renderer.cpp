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

#include "waveform_renderer.hpp"

#include "../../../domain/dsp/drum/drum_amp_envelope.hpp"

#include <QLineF>
#include <QPainter>
#include <QPolygonF>

#include <algorithm>
#include <cmath>
#include <limits>

namespace noteahead {

namespace {

//! The share of the height the loudest peak spans, and the envelope at full level either side.
constexpr double peakHeight = 0.9;
constexpr double envelopeHalfHeight = 0.45;
//! Opacity of the shaded trims, and of the envelope: held back a little, as on a sound with no
//! envelope dialled in the shape is a plain box that at full strength reads as a second border.
constexpr double trimOpacity = 0.3;
constexpr double envelopeOpacity = 0.6;
//! Points a bent segment is drawn with. At zero curve they all fall on one straight line.
constexpr int segmentSteps = 24;

} // namespace

WaveformRenderer::WaveformRenderer(QQuickItem * parent)
  : QQuickPaintedItem { parent }
{
    m_envelope.maxCurvature = DrumAmpEnvelope::MaxCurvature;
}

QVariantList WaveformRenderer::waveformData() const
{
    return m_waveformData;
}

void WaveformRenderer::setWaveformData(const QVariantList & data)
{
    if (m_waveformData != data) {
        m_waveformData = data;
        std::vector<double> peaks;
        peaks.reserve(static_cast<size_t>(data.size()));
        for (auto && value : data) {
            peaks.push_back(value.toDouble());
        }
        m_peaks = normalisedPeaks(peaks);
        emit waveformDataChanged();
        update();
    }
}

double WaveformRenderer::startOffset() const
{
    return m_startOffset;
}

void WaveformRenderer::setStartOffset(double offset)
{
    if (m_startOffset != offset) {
        m_startOffset = offset;
        emit startOffsetChanged();
        update();
    }
}

double WaveformRenderer::endOffset() const
{
    return m_endOffset;
}

void WaveformRenderer::setEndOffset(double offset)
{
    if (m_endOffset != offset) {
        m_endOffset = offset;
        emit endOffsetChanged();
        update();
    }
}

QColor WaveformRenderer::accentColor() const
{
    return m_accentColor;
}

void WaveformRenderer::setAccentColor(const QColor & color)
{
    if (m_accentColor != color) {
        m_accentColor = color;
        emit accentColorChanged();
        update();
    }
}

bool WaveformRenderer::showEnvelope() const
{
    return m_showEnvelope;
}

void WaveformRenderer::setShowEnvelope(bool show)
{
    if (m_showEnvelope != show) {
        m_showEnvelope = show;
        emit envelopeChanged();
        update();
    }
}

double WaveformRenderer::duration() const
{
    return m_duration;
}

void WaveformRenderer::setDuration(double seconds)
{
    setEnvelopeValue(m_duration, seconds);
}

double WaveformRenderer::envelopeAttack() const
{
    return m_envelope.attack;
}

void WaveformRenderer::setEnvelopeAttack(double seconds)
{
    setEnvelopeValue(m_envelope.attack, seconds);
}

double WaveformRenderer::envelopeHold() const
{
    return m_envelope.hold;
}

void WaveformRenderer::setEnvelopeHold(double seconds)
{
    setEnvelopeValue(m_envelope.hold, seconds);
}

double WaveformRenderer::envelopeDecay() const
{
    return m_envelope.decay;
}

void WaveformRenderer::setEnvelopeDecay(double seconds)
{
    setEnvelopeValue(m_envelope.decay, seconds);
}

double WaveformRenderer::envelopeSustain() const
{
    return m_envelope.sustain;
}

void WaveformRenderer::setEnvelopeSustain(double level)
{
    setEnvelopeValue(m_envelope.sustain, level);
}

double WaveformRenderer::envelopeRelease() const
{
    return m_envelope.release;
}

void WaveformRenderer::setEnvelopeRelease(double seconds)
{
    setEnvelopeValue(m_envelope.release, seconds);
}

double WaveformRenderer::envelopeCurve() const
{
    return m_envelope.curve;
}

void WaveformRenderer::setEnvelopeCurve(double curve)
{
    setEnvelopeValue(m_envelope.curve, curve);
}

double WaveformRenderer::envelopeMaxCurvature() const
{
    return m_envelope.maxCurvature;
}

void WaveformRenderer::setEnvelopeMaxCurvature(double curvature)
{
    setEnvelopeValue(m_envelope.maxCurvature, curvature);
}

void WaveformRenderer::setEnvelopeValue(double & member, double value)
{
    if (member != value) {
        member = value;
        emit envelopeChanged();
        update();
    }
}

double WaveformRenderer::shape(double phase, double curve, double maxCurvature)
{
    const double curvature = curve * maxCurvature;
    if (curvature < 1.0e-6) {
        return phase;
    }
    return std::expm1(-curvature * phase) / std::expm1(-curvature);
}

std::vector<QPointF> WaveformRenderer::envelopePoints(const Envelope & envelope, double x0, double x1, double pixelsPerSecond)
{
    std::vector<QPointF> points { { x0, 0.0 } };
    double x = x0;
    double level = 0.0;

    // Rising segments bend towards their target and falling ones away from their start, which is
    // the asymmetry that makes one read as an attack and the other as a decay. Returns whether the
    // segment ended before the boundary.
    const auto segment = [&](double seconds, double target, bool rising, double boundary) {
        const double start = level;
        const double startX = x;
        const double endX = startX + seconds * pixelsPerSecond;
        const bool cut = endX >= boundary;
        const double lastPhase = cut ? (endX > startX ? (boundary - startX) / (endX - startX) : 1.0) : 1.0;
        for (int i = 1; i <= segmentSteps; i++) {
            const double phase = lastPhase * i / segmentSteps;
            const double shaped = shape(phase, envelope.curve, envelope.maxCurvature);
            level = rising ? start + (target - start) * shaped : target + (start - target) * (1.0 - shaped);
            points.emplace_back(startX + (endX - startX) * phase, level);
        }
        x = cut ? boundary : endX;
        return !cut;
    };

    // The hold is flat, so it is a segment whose target is where it already is: the same clipping
    // and the same boundary handling as the others, without a special case.
    if (segment(envelope.attack, 1.0, true, x1) && segment(envelope.hold, 1.0, false, x1) && segment(envelope.decay, envelope.sustain, false, x1)) {
        level = envelope.sustain;
        points.emplace_back(x1, level);
    }
    x = x1;
    segment(envelope.release, 0.0, false, std::numeric_limits<double>::infinity());
    return points;
}

std::vector<double> WaveformRenderer::normalisedPeaks(const std::vector<double> & peaks)
{
    const double loudest = peaks.empty() ? 0.0 : *std::max_element(peaks.begin(), peaks.end());
    if (loudest <= 0.0) {
        return std::vector<double>(peaks.size(), 0.0);
    }
    std::vector<double> scaled;
    scaled.reserve(peaks.size());
    for (const double peak : peaks) {
        scaled.push_back(std::max(peak, 0.0) / loudest);
    }
    return scaled;
}

void WaveformRenderer::paint(QPainter * painter)
{
    paintPeaks(*painter);
    paintTrims(*painter);
    paintEnvelope(*painter);
}

void WaveformRenderer::paintPeaks(QPainter & painter) const
{
    if (m_peaks.empty()) {
        return;
    }
    // One bar per pixel, so no antialiasing: on the pixel centre each bar is one crisp column
    painter.setRenderHint(QPainter::Antialiasing, false);
    const double middle = height() / 2.0;
    const double step = width() / static_cast<double>(m_peaks.size());
    std::vector<QLineF> bars;
    bars.reserve(m_peaks.size());
    for (size_t i = 0; i < m_peaks.size(); i++) {
        const double half = m_peaks.at(i) * height() * peakHeight / 2.0;
        const double x = static_cast<double>(i) * step + 0.5;
        bars.emplace_back(x, middle - half, x, middle + half);
    }
    painter.setPen(QPen { m_accentColor, 1.0 });
    painter.drawLines(bars.data(), static_cast<int>(bars.size()));
}

void WaveformRenderer::paintTrims(QPainter & painter) const
{
    QColor shade = m_accentColor;
    shade.setAlphaF(trimOpacity);
    const double start = std::clamp(m_startOffset * width(), 0.0, width());
    if (m_startOffset > 0.0) {
        painter.fillRect(QRectF { 0.0, 0.0, start, height() }, shade);
    }
    const double end = std::clamp(m_endOffset * width(), 0.0, width());
    if (m_endOffset < 1.0) {
        painter.fillRect(QRectF { end, 0.0, width() - end, height() }, shade);
    }
}

void WaveformRenderer::paintEnvelope(QPainter & painter) const
{
    if (!m_showEnvelope || m_duration <= 0.0) {
        return;
    }
    const double rangeStart = std::max(0.0, m_startOffset * width());
    const double rangeEnd = std::min(width(), m_endOffset * width());
    if (rangeEnd <= rangeStart) {
        return;
    }
    const auto points = envelopePoints(m_envelope, rangeStart, rangeEnd, width() / m_duration);
    const double middle = height() / 2.0;
    QColor color { Qt::white };
    color.setAlphaF(envelopeOpacity);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(QPen { color, 1.0 });
    for (const double side : { -1.0, 1.0 }) {
        QPolygonF line;
        line.reserve(static_cast<qsizetype>(points.size()));
        for (auto && point : points) {
            line.append({ point.x(), middle + side * point.y() * height() * envelopeHalfHeight });
        }
        painter.drawPolyline(line);
    }
}

} // namespace noteahead
