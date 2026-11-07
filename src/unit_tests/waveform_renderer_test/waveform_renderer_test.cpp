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

#include "waveform_renderer_test.hpp"

#include "../../domain/dsp/adsr_envelope.hpp"
#include "../../domain/dsp/drum/drum_amp_envelope.hpp"
#include "../../view/qml/Components/waveform_renderer.hpp"

#include <QImage>
#include <QPainter>
#include <QTest>

#include <algorithm>
#include <cmath>

namespace noteahead {

namespace {

constexpr int imageWidth = 100;
constexpr int imageHeight = 60;
const QColor accent { 0, 200, 0 };

//! What @p renderer draws, on black.
QImage paintToImage(WaveformRenderer & renderer)
{
    renderer.setSize({ imageWidth, imageHeight });
    QImage image { imageWidth, imageHeight, QImage::Format_ARGB32 };
    image.fill(Qt::black);
    QPainter painter { &image };
    renderer.paint(&painter);
    painter.end();
    return image;
}

//! Pixels in column @p x with any green in them, which is where the accent was painted.
int litPixelsInColumn(const QImage & image, int x)
{
    int lit = 0;
    for (int y = 0; y < image.height(); y++) {
        if (qGreen(image.pixel(x, y)) > 0) {
            lit++;
        }
    }
    return lit;
}

//! Whether anything but the accent was painted, which is the white of the envelope.
bool hasRedAnywhere(const QImage & image)
{
    for (int y = 0; y < image.height(); y++) {
        for (int x = 0; x < image.width(); x++) {
            if (qRed(image.pixel(x, y)) > 0) {
                return true;
            }
        }
    }
    return false;
}

} // namespace

void WaveformRendererTest::test_shape_zeroCurve_shouldBeLinear()
{
    for (const double phase : { 0.0, 0.25, 0.5, 1.0 }) {
        QCOMPARE(WaveformRenderer::shape(phase, 0.0, DrumAmpEnvelope::MaxCurvature), phase);
        QCOMPARE(WaveformRenderer::shape(phase, 1.0, 0.0), phase);
    }
}

void WaveformRendererTest::test_shape_fullCurve_shouldMatchTheEnvelopes()
{
    // Both ends stay put, and the bend is the envelopes' own: expm1(-k p) / expm1(-k)
    for (const double curvature : { DrumAmpEnvelope::MaxCurvature, AdsrEnvelope::MaxCurvature }) {
        QCOMPARE(WaveformRenderer::shape(0.0, 1.0, curvature), 0.0);
        QCOMPARE(WaveformRenderer::shape(1.0, 1.0, curvature), 1.0);
        const double expected = std::expm1(-curvature * 0.5) / std::expm1(-curvature);
        QCOMPARE(WaveformRenderer::shape(0.5, 1.0, curvature), expected);
        QVERIFY(expected > 0.9);
    }
    // The Sampler's envelope bends further than the drum pads'
    QVERIFY(WaveformRenderer::shape(0.1, 1.0, AdsrEnvelope::MaxCurvature) > WaveformRenderer::shape(0.1, 1.0, DrumAmpEnvelope::MaxCurvature));
}

void WaveformRendererTest::test_envelopePoints_defaultEnvelope_shouldDrawABox()
{
    const auto points = WaveformRenderer::envelopePoints({}, 10.0, 90.0, 100.0);
    QCOMPARE(points.front(), QPointF(10.0, 0.0));
    QCOMPARE(points.back(), QPointF(90.0, 0.0));
    for (auto && point : points) {
        QVERIFY(point.x() == 10.0 || point.x() == 90.0);
    }
    // Up at the start, along the top, and down at the end
    QVERIFY(std::any_of(points.begin(), points.end(), [](auto && point) { return point == QPointF(10.0, 1.0); }));
    QVERIFY(std::any_of(points.begin(), points.end(), [](auto && point) { return point == QPointF(90.0, 1.0); }));
}

void WaveformRendererTest::test_envelopePoints_longAttack_shouldBeCutAtTheRangeEnd()
{
    // A two-second attack over one second of sound only ever gets half way up
    WaveformRenderer::Envelope envelope;
    envelope.attack = 2.0;
    const auto points = WaveformRenderer::envelopePoints(envelope, 0.0, 100.0, 100.0);

    double highest = 0.0;
    for (auto && point : points) {
        QVERIFY(point.x() <= 100.0);
        highest = std::max(highest, point.y());
    }
    QVERIFY(std::abs(highest - 0.5) < 1.0e-9);
    QCOMPARE(points.back(), QPointF(100.0, 0.0));
}

void WaveformRendererTest::test_envelopePoints_decay_shouldHoldTheSustainToTheRangeEnd()
{
    WaveformRenderer::Envelope envelope;
    envelope.hold = 0.1;
    envelope.decay = 0.2;
    envelope.sustain = 0.25;
    const auto points = WaveformRenderer::envelopePoints(envelope, 0.0, 100.0, 100.0);

    // Held at the top to 10 px, decayed to the sustain by 30 px, and level from there to the end
    QVERIFY(std::any_of(points.begin(), points.end(), [](auto && point) { return point == QPointF(10.0, 1.0); }));
    QVERIFY(std::any_of(points.begin(), points.end(), [](auto && point) { return std::abs(point.x() - 30.0) < 1.0e-9 && std::abs(point.y() - 0.25) < 1.0e-9; }));
    QVERIFY(std::any_of(points.begin(), points.end(), [](auto && point) { return point == QPointF(100.0, 0.25); }));
}

void WaveformRendererTest::test_envelopePoints_release_shouldFallPastTheRangeEnd()
{
    WaveformRenderer::Envelope envelope;
    envelope.release = 0.5;
    const auto points = WaveformRenderer::envelopePoints(envelope, 0.0, 100.0, 100.0);
    QCOMPARE(points.back(), QPointF(150.0, 0.0));
}

void WaveformRendererTest::test_normalisedPeaks_loudest_shouldBeOne()
{
    const auto peaks = WaveformRenderer::normalisedPeaks({ 0.1, 0.4, 0.2 });
    QCOMPARE(peaks.size(), size_t { 3 });
    QCOMPARE(peaks.at(1), 1.0);
    QCOMPARE(peaks.at(0), 0.25);
    QCOMPARE(peaks.at(2), 0.5);
}

void WaveformRendererTest::test_normalisedPeaks_silence_shouldStayZero()
{
    QCOMPARE(WaveformRenderer::normalisedPeaks({ 0.0, 0.0 }), (std::vector<double> { 0.0, 0.0 }));
    QVERIFY(WaveformRenderer::normalisedPeaks({}).empty());
}

void WaveformRendererTest::test_paint_peaks_shouldDrawOneBarPerPeak()
{
    WaveformRenderer renderer;
    renderer.setAccentColor(accent);
    QVariantList peaks;
    for (int i = 0; i < imageWidth; i++) {
        peaks.append(i < imageWidth / 2 ? 1.0 : 0.5);
    }
    renderer.setWaveformData(peaks);
    const auto image = paintToImage(renderer);

    // The loudest spans nine tenths of the height, around the middle, and half as loud half as much
    const int loud = litPixelsInColumn(image, 10);
    const int quiet = litPixelsInColumn(image, 80);
    QVERIFY(std::abs(loud - 54) <= 1);
    QVERIFY(std::abs(quiet - 27) <= 1);
    QVERIFY(qGreen(image.pixel(10, imageHeight / 2)) > 0);
    QVERIFY(qGreen(image.pixel(10, 0)) == 0);
}

void WaveformRendererTest::test_paint_trims_shouldShadeOutsideTheRange()
{
    WaveformRenderer renderer;
    renderer.setAccentColor(accent);
    renderer.setStartOffset(0.2);
    renderer.setEndOffset(0.7);
    const auto image = paintToImage(renderer);

    QCOMPARE(litPixelsInColumn(image, 10), imageHeight);
    QCOMPARE(litPixelsInColumn(image, 50), 0);
    QCOMPARE(litPixelsInColumn(image, 90), imageHeight);
}

void WaveformRendererTest::test_paint_envelope_shouldDrawOnlyWhenShown()
{
    WaveformRenderer renderer;
    renderer.setAccentColor(accent);
    renderer.setDuration(1.0);
    QVERIFY(!hasRedAnywhere(paintToImage(renderer)));

    renderer.setShowEnvelope(true);
    QVERIFY(hasRedAnywhere(paintToImage(renderer)));

    // Nothing to put the envelope's seconds against
    renderer.setDuration(0.0);
    QVERIFY(!hasRedAnywhere(paintToImage(renderer)));
}

} // namespace noteahead

QTEST_MAIN(noteahead::WaveformRendererTest)
