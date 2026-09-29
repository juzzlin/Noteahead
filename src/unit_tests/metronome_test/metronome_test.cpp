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

#include "metronome_test.hpp"

#include "../../domain/dsp/metronome.hpp"

#include <QTest>

#include <cmath>
#include <vector>

namespace noteahead {

namespace {

constexpr uint32_t SampleRate { 48000 };

//! Renders \p frames in blocks of \p blockSize and returns the whole interleaved stereo result, so a
//! test can say where a click landed relative to the start of the run rather than to a buffer.
std::vector<double> render(Metronome & metronome, uint32_t frames, uint32_t blockSize = 512)
{
    std::vector<double> out(static_cast<size_t>(frames) * 2, 0.0);
    for (uint32_t done = 0; done < frames; done += blockSize) {
        const auto count = std::min(blockSize, frames - done);
        metronome.render(std::span<double> { out.data() + static_cast<size_t>(done) * 2, static_cast<size_t>(count) * 2 },
                         count, SampleRate);
    }
    return out;
}

//! The first frame of each click.
//!
//! A click is a decaying sine, so it is silent for a frame or two at every zero crossing: a gap only
//! counts as the end of a click once it has lasted longer than a cycle. ReArmFrames sits well above
//! one cycle of the lowest click and well below the shortest gap between beats.
std::vector<uint32_t> clickOnsets(const std::vector<double> & interleaved)
{
    constexpr uint32_t ReArmFrames { 200 };
    std::vector<uint32_t> onsets;
    uint32_t quietFor = ReArmFrames;
    for (size_t frame = 0; frame < interleaved.size() / 2; frame++) {
        if (std::abs(interleaved[frame * 2]) > 0.01) {
            if (quietFor >= ReArmFrames) {
                onsets.push_back(static_cast<uint32_t>(frame));
            }
            quietFor = 0;
        } else {
            quietFor++;
        }
    }
    return onsets;
}

//! Peak of one click, measured from its onset over a window shorter than a beat.
double clickPeak(const std::vector<double> & interleaved, uint32_t onset, uint32_t window = 2000)
{
    double peak = 0.0;
    for (uint32_t i = 0; i < window && (onset + i) * 2 < interleaved.size(); i++) {
        peak = std::max(peak, std::abs(interleaved[(onset + i) * 2]));
    }
    return peak;
}

} // namespace

void MetronomeTest::test_render_stopped_shouldAddNothing()
{
    // A metronome nobody started must cost nothing and add nothing, since it is rendered on every
    // callback for the whole life of the application.
    Metronome metronome;
    metronome.setSampleRate(SampleRate);

    const auto out = render(metronome, SampleRate);

    QVERIFY(!metronome.running());
    for (const auto sample : out) {
        QCOMPARE(sample, 0.0);
    }
}

void MetronomeTest::test_render_shouldClickOnEveryBeat()
{
    Metronome metronome;
    metronome.setSampleRate(SampleRate);
    metronome.setBpm(120.0); // two beats a second, so 24000 frames apart
    metronome.start(0);

    const auto out = render(metronome, SampleRate * 2); // two seconds, so four beats
    const auto onsets = clickOnsets(out);

    QCOMPARE(onsets.size(), static_cast<size_t>(4));
    for (size_t i = 0; i < onsets.size(); i++) {
        // Never early, and at most a frame or two late: the click is a sine starting at phase zero,
        // so its first frame is silent and the first frame above the floor is the one after the beat.
        const auto expected = static_cast<uint32_t>(i * 24000);
        QVERIFY2(onsets.at(i) >= expected && onsets.at(i) <= expected + 2,
                 qPrintable(QString { "beat %1 at %2, expected %3" }.arg(i).arg(onsets.at(i)).arg(expected)));
    }
}

void MetronomeTest::test_render_acrossBufferBoundaries_shouldKeepTheBeatsEvenlySpaced()
{
    // A beat interval that is not a whole number of buffers, rendered in an awkward block size: the
    // remainder has to carry across callbacks or the clicks drift.
    Metronome metronome;
    metronome.setSampleRate(SampleRate);
    metronome.setBpm(137.0);
    metronome.start(0);

    const auto out = render(metronome, SampleRate * 4, 333);
    const auto onsets = clickOnsets(out);

    QVERIFY2(onsets.size() >= 8, qPrintable(QString::number(onsets.size())));
    const double framesPerBeat = SampleRate * 60.0 / 137.0;
    // Measured against the start of the run rather than against the previous beat: an error that
    // accumulated would show here however small each step was, which is the whole point of the test.
    for (size_t i = 0; i < onsets.size(); i++) {
        const double expected = static_cast<double>(i) * framesPerBeat;
        const double late = static_cast<double>(onsets.at(i)) - expected;
        QVERIFY2(late >= 0.0 && late <= 2.0,
                 qPrintable(QString { "beat %1 is %2 frames off (%3 vs %4)" }.arg(i).arg(late).arg(onsets.at(i)).arg(expected)));
    }
}

void MetronomeTest::test_render_firstBeatOfTheBar_shouldBeAccented()
{
    Metronome metronome;
    metronome.setSampleRate(SampleRate);
    metronome.setBpm(120.0);
    metronome.setBeatsPerBar(4);
    metronome.start(0);

    const auto out = render(metronome, SampleRate * 3); // six beats, so two bars and a half
    const auto onsets = clickOnsets(out);
    QVERIFY(onsets.size() >= 5);

    const auto downbeat = clickPeak(out, onsets.at(0));
    const auto offbeat = clickPeak(out, onsets.at(1));
    QVERIFY2(downbeat > offbeat, qPrintable(QString { "%1 vs %2" }.arg(downbeat).arg(offbeat)));
    // And the accent comes round again with the bar, not on every beat.
    QVERIFY(clickPeak(out, onsets.at(4)) > offbeat);
    QVERIFY(qFuzzyCompare(clickPeak(out, onsets.at(2)), offbeat));
}

void MetronomeTest::test_countIn_shouldFinishOnTheLastCountedBeat()
{
    Metronome metronome;
    metronome.setSampleRate(SampleRate);
    metronome.setBpm(120.0);
    metronome.start(4);

    QVERIFY(!metronome.countInFinished());
    QCOMPARE(metronome.countInBeatsRemaining(), 4);

    // Three beats in: still counting.
    render(metronome, 24000 * 3 - 10);
    QCOMPARE(metronome.countInBeatsRemaining(), 1);
    QVERIFY(!metronome.countInFinished());

    // The fourth click lands, and the take begins on the beat after it.
    render(metronome, 100);
    QCOMPARE(metronome.countInBeatsRemaining(), 0);
    QVERIFY(metronome.countInFinished());
}

void MetronomeTest::test_countIn_zeroBeats_shouldStartFinished()
{
    // Clicking with no pre-count at all: nothing to wait for.
    Metronome metronome;
    metronome.setSampleRate(SampleRate);
    metronome.start(0);

    QVERIFY(metronome.countInFinished());
    QCOMPARE(metronome.countInBeatsRemaining(), 0);
}

void MetronomeTest::test_start_afterStopping_shouldClickImmediately()
{
    // Starting a count-in has to put a click on the first frame, not finish the interval the previous
    // run was part way through.
    Metronome metronome;
    metronome.setSampleRate(SampleRate);
    metronome.setBpm(120.0);
    metronome.start(0);
    render(metronome, 10000);
    metronome.stop();

    metronome.start(4);
    const auto out = render(metronome, 1000);
    const auto onsets = clickOnsets(out);

    QCOMPARE(onsets.size(), static_cast<size_t>(1));
    QVERIFY2(onsets.at(0) <= 2, qPrintable(QString::number(onsets.at(0))));
}

void MetronomeTest::test_setBpm_shouldChangeTheBeatSpacing()
{
    Metronome metronome;
    metronome.setSampleRate(SampleRate);
    metronome.setBpm(60.0); // one a second
    metronome.start(0);

    const auto slow = clickOnsets(render(metronome, SampleRate * 2));
    QCOMPARE(slow.size(), static_cast<size_t>(2));
    QVERIFY2(slow.at(1) - slow.at(0) >= 48000 && slow.at(1) - slow.at(0) <= 48002, qPrintable(QString::number(slow.at(1))));

    metronome.stop();
    metronome.setBpm(240.0); // four a second
    metronome.start(0);
    const auto fast = clickOnsets(render(metronome, SampleRate));
    QCOMPARE(fast.size(), static_cast<size_t>(4));
    QVERIFY2(fast.at(1) - fast.at(0) >= 12000 && fast.at(1) - fast.at(0) <= 12002, qPrintable(QString::number(fast.at(1))));
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::MetronomeTest)
