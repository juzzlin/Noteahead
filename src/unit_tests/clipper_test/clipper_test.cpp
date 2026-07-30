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

#include "clipper_test.hpp"

#include "../../common/constants.hpp"
#include "../../domain/dsp/audio_context.hpp"
#include "../../domain/effects/clipper.hpp"

#include <QTest>

#include <cmath>
#include <numbers>
#include <vector>

namespace noteahead {

namespace {

constexpr uint32_t sampleRate = 48000;
constexpr uint32_t frameCount = 4096;

//! The oversample factors the effect supports. The meter has to behave at every one of them: the
//! half-band filters only delay the signal when the factor is above 1, which is exactly the case
//! that used to read wrong.
const std::vector<uint8_t> oversampleFactors = { 1, 2, 4 };

void setParameter(Clipper & effect, const QString & key, float value)
{
    if (auto p = effect.parameter(key.toStdString()); p) {
        p->get().update(value);
    }
    effect.sync();
}

//! Threshold in dB down from full scale, as the dialog's slider expresses it.
void setThresholdDb(Clipper & effect, float thresholdDb)
{
    setParameter(effect, Constants::NahdXml::xmlKeyThreshold(), (thresholdDb + 24.0f) / 24.0f);
}

void setHardMode(Clipper & effect)
{
    setParameter(effect, Constants::NahdXml::xmlKeyMode(), 0.0f);
}

//! Runs a 1 kHz sine of the given amplitude through the effect and returns the peak of the output.
double processSine(Clipper & effect, double amplitude, uint8_t oversampleFactor)
{
    std::vector<double> buffer(frameCount * 2, 0.0);
    for (uint32_t i = 0; i < frameCount; i++) {
        const double phase = 2.0 * std::numbers::pi * 1000.0 * static_cast<double>(i) / static_cast<double>(sampleRate);
        buffer[i * 2] = amplitude * std::sin(phase);
        buffer[i * 2 + 1] = amplitude * std::sin(phase);
    }

    AudioContext context;
    context.buffer = buffer;
    context.frameCount = frameCount;
    context.sampleRate = sampleRate;
    context.oversampleFactor = oversampleFactor;

    effect.process(context);

    double peak = 0.0;
    for (auto && sample : buffer) {
        peak = std::max(peak, std::abs(sample));
    }
    return peak;
}

} // namespace

void ClipperTest::test_hardMode_belowThreshold_shouldNotClip()
{
    for (auto && factor : oversampleFactors) {
        Clipper effect;
        setHardMode(effect);
        setThresholdDb(effect, 0.0f);

        // Half of full scale against a full-scale threshold: nothing to clip. The band limiting of
        // the round trip costs a little, hence the tolerance rather than an exact compare.
        const auto peak = processSine(effect, 0.5, factor);
        QVERIFY2(peak > 0.49 && peak < 0.51, qPrintable(QString { "Peak %1 at %2x" }.arg(peak).arg(factor)));
    }
}

void ClipperTest::test_hardMode_aboveThreshold_shouldClipToThreshold()
{
    for (auto && factor : oversampleFactors) {
        Clipper effect;
        setHardMode(effect);
        setThresholdDb(effect, -6.0f); // ~0.501 linear

        // Oversampled clipping rings a little past the threshold after decimation, which is the
        // point of doing it at a higher rate, so allow headroom above but require real clipping.
        const auto peak = processSine(effect, 1.0, factor);
        QVERIFY2(peak < 0.65, qPrintable(QString { "Peak %1 at %2x" }.arg(peak).arg(factor)));
    }
}

void ClipperTest::test_reductionMeter_belowThreshold_shouldStayAtZero()
{
    for (auto && factor : oversampleFactors) {
        Clipper effect;
        setHardMode(effect);
        setThresholdDb(effect, 0.0f);

        processSine(effect, 0.5, factor);

        // Nothing was clipped, so the meter must read nothing. Measuring across the whole
        // upsample/clip/decimate round trip instead of around the clipping alone compares the
        // current input against an output delayed by the half-band filters, which pinned this near
        // the meter's 30 dB full scale on any periodic signal.
        QVERIFY2(std::abs(effect.reductionDb()) < 0.1f, qPrintable(QString { "Meter read %1 dB at %2x with nothing to clip" }.arg(effect.reductionDb()).arg(factor)));
    }
}

void ClipperTest::test_reductionMeter_aboveThreshold_shouldShowReduction()
{
    for (auto && factor : oversampleFactors) {
        Clipper effect;
        setHardMode(effect);
        setThresholdDb(effect, -12.0f);

        processSine(effect, 1.0, factor);

        // A full-scale sine against a -12 dB threshold is unmistakably clipped.
        QVERIFY2(effect.reductionDb() < -6.0f, qPrintable(QString { "Meter read %1 dB at %2x" }.arg(effect.reductionDb()).arg(factor)));
    }
}

void ClipperTest::test_reductionMeter_higherThreshold_shouldShowLessReduction()
{
    for (auto && factor : oversampleFactors) {
        Clipper low;
        setHardMode(low);
        setThresholdDb(low, -18.0f);
        processSine(low, 1.0, factor);

        Clipper high;
        setHardMode(high);
        setThresholdDb(high, -6.0f);
        processSine(high, 1.0, factor);

        // The meter has to track the threshold. It previously read the same pinned value whatever
        // the threshold was, which is what made it useless.
        QVERIFY2(low.reductionDb() < high.reductionDb(), qPrintable(QString { "%1 dB at -18, %2 dB at -6, %3x" }.arg(low.reductionDb()).arg(high.reductionDb()).arg(factor)));
    }
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::ClipperTest)
