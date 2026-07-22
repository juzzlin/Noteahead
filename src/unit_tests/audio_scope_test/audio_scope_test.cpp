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

#include "audio_scope_test.hpp"

#include "../../domain/utility/audio_scope.hpp"

#include <QTest>

#include <atomic>
#include <chrono>
#include <cmath>
#include <numbers>
#include <thread>
#include <vector>

namespace noteahead {

namespace {

// Fill the scope's rings with interleaved stereo sines. period is in samples.
void writeSines(AudioScope & scope, float ampL, float ampR, double period, uint32_t frames, uint32_t sampleRate)
{
    std::vector<double> buffer(frames * 2, 0.0);
    for (uint32_t n = 0; n < frames; n++) {
        const double phase = 2.0 * std::numbers::pi * static_cast<double>(n) / period;
        buffer[n * 2] = ampL * std::sin(phase);
        buffer[n * 2 + 1] = ampR * std::sin(phase);
    }
    scope.write(buffer.data(), frames, sampleRate);
}

} // namespace

void AudioScopeTest::test_write_inactive_shouldNotCapture()
{
    AudioScope scope;

    // Inactive by default: writes must be dropped, so the snapshot stays silent.
    writeSines(scope, 0.8f, 0.8f, 128.0, 8192, 48000);

    const auto snapshot = scope.snapshot(512);
    QCOMPARE(static_cast<int>(snapshot.left.size()), 512);
    for (const auto sample : snapshot.left) {
        QCOMPARE(sample, 0.0f);
    }
    for (const auto sample : snapshot.right) {
        QCOMPARE(sample, 0.0f);
    }
}

void AudioScopeTest::test_snapshot_activeSine_shouldAlignToZeroCrossing()
{
    AudioScope scope;
    scope.setActive(true);

    writeSines(scope, 0.8f, 0.8f, 128.0, 8192, 48000);

    const size_t points = 512;
    const auto snapshot = scope.snapshot(points);
    QCOMPARE(static_cast<int>(snapshot.left.size()), static_cast<int>(points));

    // Aligned to a rising zero-crossing: the first sample is near zero and the trace rises.
    QVERIFY(snapshot.left.front() <= 0.0f);
    QVERIFY(std::abs(snapshot.left.front()) < 0.1f);
    QVERIFY(snapshot.left[1] > snapshot.left.front());

    // The captured sine reaches close to its amplitude somewhere in the window.
    float peak = 0.0f;
    for (const auto sample : snapshot.left) {
        peak = std::max(peak, std::abs(sample));
    }
    QVERIFY(peak > 0.7f);
    QVERIFY(peak <= 0.85f);

    QCOMPARE(scope.sampleRate(), 48000u);
}

void AudioScopeTest::test_snapshot_stereo_shouldReturnBothChannels()
{
    AudioScope scope;
    scope.setActive(true);

    // Distinct amplitudes so the two channels are clearly independent.
    writeSines(scope, 0.8f, 0.4f, 128.0, 8192, 48000);

    const auto snapshot = scope.snapshot(512);
    QCOMPARE(snapshot.left.size(), snapshot.right.size());

    float peakL = 0.0f;
    float peakR = 0.0f;
    for (size_t i = 0; i < snapshot.left.size(); i++) {
        peakL = std::max(peakL, std::abs(snapshot.left[i]));
        peakR = std::max(peakR, std::abs(snapshot.right[i]));
    }
    QVERIFY(peakL > 0.7f);
    QVERIFY(peakR > 0.35f && peakR < 0.45f);
}

void AudioScopeTest::test_concurrentWriteAndSnapshot_shouldStayResponsive()
{
    AudioScope scope;
    scope.setActive(true);

    // A producer standing in for the audio thread and a consumer standing in for the UI thread
    // hammer the scope simultaneously. write() must never block (it try-locks and skips on
    // contention), so this must complete promptly without deadlocking, and capture must still work.
    std::atomic<bool> stop { false };
    std::atomic<int> writeCount { 0 };

    std::thread producer { [&] {
        while (!stop.load()) {
            writeSines(scope, 0.8f, 0.8f, 128.0, 256, 48000);
            writeCount.fetch_add(1);
        }
    } };

    std::thread consumer { [&] {
        while (!stop.load()) {
            const auto snapshot = scope.snapshot(512);
            Q_UNUSED(snapshot);
        }
    } };

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    stop.store(true);
    producer.join();
    consumer.join();

    // The producer kept running throughout (no deadlock), and the scope still captures.
    QVERIFY(writeCount.load() > 0);

    float peak = 0.0f;
    const auto snapshot = scope.snapshot(512);
    for (const auto sample : snapshot.left) {
        peak = std::max(peak, std::abs(sample));
    }
    QVERIFY(peak > 0.7f);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::AudioScopeTest)
