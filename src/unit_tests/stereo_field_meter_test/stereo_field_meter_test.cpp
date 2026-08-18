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

#include "stereo_field_meter_test.hpp"

#include "../../domain/dsp/audio_context.hpp"
#include "../../domain/utility/stereo_field_meter.hpp"

#include <QTest>

#include <cmath>
#include <functional>
#include <numbers>
#include <random>
#include <vector>

namespace noteahead {

namespace {

constexpr uint32_t sampleRate = 48000;

//! Two seconds, which is several times the slowest time constant the meter is run at here.
constexpr int frameCount = 96000;

using Generator = std::function<void(int, double &, double &)>;

//! Drives the meter the way the mixer does, a block at a time: the readings are published per
//! block, so nothing is measurable until whole blocks have gone through.
void run(StereoFieldMeter & meter, const Generator & generator, int frames = frameCount)
{
    constexpr uint32_t blockFrames = 512;
    std::vector<double> buffer(blockFrames * 2, 0.0);

    for (int i = 0; i < frames;) {
        const uint32_t count = std::min<uint32_t>(blockFrames, static_cast<uint32_t>(frames - i));
        for (uint32_t k = 0; k < count; k++) {
            double left = 0.0;
            double right = 0.0;
            generator(i + static_cast<int>(k), left, right);
            buffer[k * 2] = left;
            buffer[k * 2 + 1] = right;
        }

        AudioContext context {};
        context.buffer = std::span<double> { buffer.data(), count * 2 };
        context.frameCount = count;
        context.sampleRate = sampleRate;
        meter.process(context);

        i += static_cast<int>(count);
    }
}

std::unique_ptr<StereoFieldMeter> makeMeter()
{
    auto meter = std::make_unique<StereoFieldMeter>();
    meter->setSampleRate(sampleRate);
    meter->setAnalysisEnabled(true);
    return meter;
}

double sine(int index, double frequency)
{
    return std::sin(2.0 * std::numbers::pi * frequency * index / sampleRate);
}

Generator noise(bool independentChannels, double rightScale = 1.0)
{
    auto engine = std::make_shared<std::mt19937>(20260818);
    auto distribution = std::make_shared<std::uniform_real_distribution<double>>(-0.5, 0.5);
    return [engine, distribution, independentChannels, rightScale](int, double & left, double & right) {
        left = (*distribution)(*engine);
        right = independentChannels ? (*distribution)(*engine) : left * rightScale;
    };
}

} // namespace

void StereoFieldMeterTest::test_process_meter_shouldPassAudioThrough()
{
    const auto meter = makeMeter();

    std::vector<double> buffer { 0.5, -0.25, 0.125, 0.75 };
    const auto expected = buffer;

    AudioContext context {};
    context.buffer = std::span<double> { buffer.data(), buffer.size() };
    context.frameCount = 2;
    context.sampleRate = sampleRate;
    meter->process(context);

    // A meter that changed what it measures would be worse than no meter at all.
    QCOMPARE(buffer, expected);
}

void StereoFieldMeterTest::test_correlation_monoInput_shouldReadOne()
{
    const auto meter = makeMeter();
    run(*meter, noise(false));

    QVERIFY(meter->reading().correlation > 0.99f);
}

void StereoFieldMeterTest::test_correlation_invertedInput_shouldReadMinusOne()
{
    const auto meter = makeMeter();
    run(*meter, noise(false, -1.0));

    // The pair that vanishes entirely when the mix is summed to mono.
    QVERIFY(meter->reading().correlation < -0.99f);
}

void StereoFieldMeterTest::test_correlation_decorrelatedInput_shouldReadNearZero()
{
    const auto meter = makeMeter();
    run(*meter, noise(true));

    QVERIFY(std::abs(meter->reading().correlation) < 0.3f);
}

void StereoFieldMeterTest::test_bandCorrelation_wideHighsOverMonoLows_shouldSeparateTheBands()
{
    const auto meter = makeMeter();

    // A centred bass under an out-of-phase top, which is the arrangement the per-band reading
    // exists to tell apart from a mix that is simply wide.
    run(*meter, [](int index, double & left, double & right) {
        const double low = sine(index, 50.0) * 0.5;
        const double high = sine(index, 8000.0) * 0.5;
        left = low + high;
        right = low - high;
    });

    const auto reading = meter->reading();
    QVERIFY(reading.bandCorrelation[0] > 0.95f);
    QVERIFY(reading.bandCorrelation[2] < -0.95f);

    // Broadband, the two cancel out into something in between, which is exactly why the bands are
    // worth measuring separately.
    QVERIFY(std::abs(reading.correlation) < 0.9f);
}

void StereoFieldMeterTest::test_levels_monoInput_shouldReportSilentSide()
{
    const auto meter = makeMeter();
    run(*meter, [](int index, double & left, double & right) {
        left = sine(index, 440.0) * 0.5;
        right = left;
    });

    const auto reading = meter->reading();
    QVERIFY(reading.midDb > -20.0f);
    QVERIFY(reading.sideDb < -90.0f);
}

void StereoFieldMeterTest::test_levels_sideOnlyInput_shouldReportSilentMid()
{
    const auto meter = makeMeter();
    run(*meter, [](int index, double & left, double & right) {
        left = sine(index, 440.0) * 0.5;
        right = -left;
    });

    const auto reading = meter->reading();
    QVERIFY(reading.sideDb > -20.0f);
    QVERIFY(reading.midDb < -90.0f);
}

void StereoFieldMeterTest::test_balance_leftOnlyInput_shouldReadLeft()
{
    const auto meter = makeMeter();
    run(*meter, [](int index, double & left, double & right) {
        left = sine(index, 440.0) * 0.5;
        right = 0.0;
    });

    QVERIFY(meter->reading().balance < -0.99f);
}

void StereoFieldMeterTest::test_balance_centredInput_shouldReadCentre()
{
    const auto meter = makeMeter();
    run(*meter, noise(false));

    QVERIFY(std::abs(meter->reading().balance) < 0.01f);
}

void StereoFieldMeterTest::test_analysis_disabled_shouldNotUpdateTheReading()
{
    StereoFieldMeter meter;
    meter.setSampleRate(sampleRate);

    // Left as it is found in a rack with no dialog open on it.
    run(meter, noise(false, -1.0));

    const auto reading = meter.reading();
    QCOMPARE(reading.correlation, 1.0f);
    QCOMPARE(reading.balance, 0.0f);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::StereoFieldMeterTest)
