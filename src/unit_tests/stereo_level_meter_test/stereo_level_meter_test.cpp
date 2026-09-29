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

#include "stereo_level_meter_test.hpp"

#include "../../domain/utility/stereo_level_meter.hpp"

#include <QTest>

#include <cmath>
#include <vector>

namespace noteahead {

namespace {

constexpr uint32_t SampleRate { 48000 };
constexpr uint32_t FrameCount { 512 };

//! A stereo buffer holding a different constant in each channel, which is what tells a meter that
//! keeps its channels apart from one that folds them together.
std::vector<double> stereoBuffer(double left, double right)
{
    std::vector<double> samples(static_cast<size_t>(FrameCount) * 2, 0.0);
    for (uint32_t frame = 0; frame < FrameCount; frame++) {
        samples[frame * 2] = left;
        samples[frame * 2 + 1] = right;
    }
    return samples;
}

//! Feeds the same buffer repeatedly so the smoothed RMS reading settles.
void feed(StereoLevelMeter & meter, const std::vector<double> & samples, int buffers, uint32_t channelCount = 2)
{
    for (int i = 0; i < buffers; i++) {
        meter.write(samples.data(), FrameCount, SampleRate, channelCount);
    }
}

} // namespace

void StereoLevelMeterTest::test_write_inactive_shouldStaySilent()
{
    // A meter nobody is looking at must cost nothing and report nothing.
    StereoLevelMeter meter;
    feed(meter, stereoBuffer(1.0, 1.0), 10);

    QCOMPARE(meter.leftPeakDb(), StereoLevelMeter::MinimumDb);
    QCOMPARE(meter.leftRmsDb(), StereoLevelMeter::MinimumDb);
    QCOMPARE(meter.rightPeakDb(), StereoLevelMeter::MinimumDb);
    QCOMPARE(meter.rightRmsDb(), StereoLevelMeter::MinimumDb);
}

void StereoLevelMeterTest::test_write_fullScale_shouldReadZeroDbfsOnBothChannels()
{
    StereoLevelMeter meter;
    meter.setActive(true);
    feed(meter, stereoBuffer(1.0, 1.0), 200);

    QVERIFY(std::abs(meter.leftPeakDb()) < 0.01f);
    QVERIFY(std::abs(meter.rightPeakDb()) < 0.01f);
    // A constant at full scale is also full scale in RMS.
    QVERIFY2(std::abs(meter.leftRmsDb()) < 0.1f, qPrintable(QString::number(meter.leftRmsDb())));
    QVERIFY2(std::abs(meter.rightRmsDb()) < 0.1f, qPrintable(QString::number(meter.rightRmsDb())));
}

void StereoLevelMeterTest::test_write_oneSidedSignal_shouldLeaveTheOtherChannelSilent()
{
    // The whole reason this meter exists: LevelMeter sums the pair, so a signal on one side alone
    // reads as a level on both. Here the silent side has to stay silent.
    StereoLevelMeter meter;
    meter.setActive(true);
    feed(meter, stereoBuffer(1.0, 0.0), 200);

    QVERIFY(std::abs(meter.leftPeakDb()) < 0.01f);
    QCOMPARE(meter.rightPeakDb(), StereoLevelMeter::MinimumDb);
    QCOMPARE(meter.rightRmsDb(), StereoLevelMeter::MinimumDb);
}

void StereoLevelMeterTest::test_write_differentLevels_shouldKeepTheChannelsApart()
{
    // Half amplitude is about -6 dBFS, so the two channels must read about 6 dB apart.
    StereoLevelMeter meter;
    meter.setActive(true);
    feed(meter, stereoBuffer(1.0, 0.5), 200);

    QVERIFY(std::abs(meter.leftPeakDb()) < 0.01f);
    QVERIFY(std::abs(meter.rightPeakDb() - -6.02f) < 0.05f);
    QVERIFY(std::abs(meter.leftRmsDb() - meter.rightRmsDb() - 6.02f) < 0.1f);
}

void StereoLevelMeterTest::test_write_monoBuffer_shouldReadTheSameOnBothChannels()
{
    // A mono input is one channel, not a dead right channel: a meter that showed nothing on the right
    // would read as a broken cable.
    StereoLevelMeter meter;
    meter.setActive(true);
    const std::vector<double> mono(static_cast<size_t>(FrameCount), 0.5);
    feed(meter, mono, 200, 1);

    QCOMPARE(meter.leftPeakDb(), meter.rightPeakDb());
    QCOMPARE(meter.leftRmsDb(), meter.rightRmsDb());
    QVERIFY(std::abs(meter.leftPeakDb() - -6.02f) < 0.05f);
}

void StereoLevelMeterTest::test_write_afterSilence_shouldFallBackThePeak()
{
    // The peak falls at a fixed rate rather than being reset by whoever reads it, so a reading does
    // not depend on how often the UI happens to poll.
    StereoLevelMeter meter;
    meter.setActive(true);
    feed(meter, stereoBuffer(1.0, 1.0), 10);
    const auto atFullScale = meter.leftPeakDb();

    // A quarter second of silence, which at 20 dB/s is about 5 dB of fallback.
    const auto buffers = static_cast<int>(0.25 * SampleRate / FrameCount);
    feed(meter, stereoBuffer(0.0, 0.0), buffers);

    QVERIFY2(meter.leftPeakDb() < atFullScale - 3.0f, qPrintable(QString::number(meter.leftPeakDb())));
    QVERIFY(meter.rightPeakDb() < atFullScale - 3.0f);
    // Falling back, not snapping to the floor.
    QVERIFY(meter.leftPeakDb() > StereoLevelMeter::MinimumDb);
}

void StereoLevelMeterTest::test_setActive_off_shouldResetTheReadings()
{
    StereoLevelMeter meter;
    meter.setActive(true);
    feed(meter, stereoBuffer(1.0, 1.0), 10);
    QVERIFY(meter.leftPeakDb() > StereoLevelMeter::MinimumDb);

    meter.setActive(false);

    // Closing the dialog leaves nothing behind for the next one to show.
    QCOMPARE(meter.leftPeakDb(), StereoLevelMeter::MinimumDb);
    QCOMPARE(meter.rightPeakDb(), StereoLevelMeter::MinimumDb);
    QVERIFY(!meter.active());
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::StereoLevelMeterTest)
