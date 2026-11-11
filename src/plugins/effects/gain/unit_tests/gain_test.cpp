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

#include "gain_test.hpp"
#include "../../common/constants.hpp"
#include "../../common/utils.hpp"
#include "../../domain/dsp/audio_context.hpp"
#include "../../domain/effects/gain.hpp"

#include <QTest>

#include <cmath>
#include <vector>

namespace noteahead {

namespace {

//! Sets the trim in dB, through the same 0..1 parameter the dialog writes.
void setGainDb(Gain & gain, float db)
{
    const auto parameter = gain.parameter(Constants::NahdXml::xmlKeyGain().toStdString());
    QVERIFY(parameter.has_value());
    parameter->get().setValue(db / 48.0f + 0.5f);
    gain.sync();
}

std::pair<double, double> processFrame(Gain & gain, double left, double right)
{
    std::vector<double> buffer { left, right };
    AudioContext context { std::span<double>(buffer.data(), buffer.size()), 1, 48000 };
    gain.process(context);
    return { buffer.at(0), buffer.at(1) };
}

} // namespace

void GainTest::test_default_shouldBeUnity()
{
    Gain gain;
    gain.sync();

    // A Gain dropped into a rack has to be inaudible until it is moved.
    QCOMPARE(gain.gainDb(), 0.0f);
    const auto [left, right] = processFrame(gain, 0.4, -0.6);
    QCOMPARE(left, 0.4);
    QCOMPARE(right, -0.6);
}

void GainTest::test_gain_shouldScaleBothChannels()
{
    Gain gain;
    setGainDb(gain, 6.0f);

    const auto [left, right] = processFrame(gain, 0.1, -0.2);
    const auto expected = static_cast<double>(Utils::Dsp::dbToLinear(6.0f));
    QVERIFY(std::abs(left - 0.1 * expected) < 1.0e-9);
    QVERIFY(std::abs(right + 0.2 * expected) < 1.0e-9);
}

void GainTest::test_gain_extremes_shouldReachTheStatedRange()
{
    Gain gain;

    // The knob is labelled +/-24 dB, so the ends of the parameter have to be exactly that.
    setGainDb(gain, 24.0f);
    QVERIFY(std::abs(gain.gainDb() - 24.0f) < 0.01f);
    setGainDb(gain, -24.0f);
    QVERIFY(std::abs(gain.gainDb() + 24.0f) < 0.01f);
}

void GainTest::test_clipDetector_belowFullScale_shouldNotLatch()
{
    Gain gain;
    setGainDb(gain, 6.0f);

    processFrame(gain, 0.4, -0.4);
    QVERIFY(!gain.clipDetector().clipped());
}

void GainTest::test_clipDetector_boostOverFullScale_shouldLatch()
{
    Gain gain;
    setGainDb(gain, 12.0f);

    processFrame(gain, 0.5, -0.5);
    QVERIFY(gain.clipDetector().clipped());
}

void GainTest::test_clipDetector_shouldMeasureAfterTheTrim()
{
    // The indicator answers "did this control push it over", so an input already at full scale that
    // the trim brings back down must leave it dark.
    Gain gain;
    setGainDb(gain, -12.0f);

    processFrame(gain, 1.0, -1.0);
    QVERIFY(!gain.clipDetector().clipped());
}

void GainTest::test_clipDetector_shouldLatchUntilCleared()
{
    Gain gain;
    setGainDb(gain, 12.0f);
    processFrame(gain, 0.9, 0.9);
    QVERIFY(gain.clipDetector().clipped());

    // Falling back on its own would hide exactly the short overshoot the indicator exists to report.
    setGainDb(gain, 0.0f);
    for (int i = 0; i < 100; i++) {
        processFrame(gain, 0.01, 0.01);
    }
    QVERIFY(gain.clipDetector().clipped());

    gain.clipDetector().clear();
    QVERIFY(!gain.clipDetector().clipped());
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::GainTest)
