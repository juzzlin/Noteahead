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

#include "spectrum_analyzer_test.hpp"

#include "../../domain/dsp/svf_filter.hpp"
#include "../../domain/utility/spectrum_analyzer.hpp"

#include <QTest>

#include <algorithm>
#include <cmath>
#include <numbers>
#include <random>
#include <vector>

namespace noteahead {

namespace {

constexpr double SampleRate = 44100.0;

//! Long enough that the overlapped frames average rather than measure one window.
constexpr size_t Seconds = 6;

void feed(SpectrumAnalyzer & analyzer, const std::vector<float> & interleaved)
{
    // Handed over in chunks, the way the render worker streams a file, so that frame assembly is
    // exercised rather than bypassed by one big call.
    constexpr size_t Chunk = 16384;
    for (size_t offset = 0; offset < interleaved.size(); offset += Chunk) {
        const auto count = std::min(Chunk, interleaved.size() - offset);
        analyzer.process(interleaved.data() + offset, count);
    }
}

std::vector<float> tone(double hz, double amplitude, int channels = 2)
{
    std::vector<float> out(static_cast<size_t>(SampleRate) * Seconds * static_cast<size_t>(channels));
    for (size_t frame = 0; frame < static_cast<size_t>(SampleRate) * Seconds; frame++) {
        const auto value = static_cast<float>(amplitude * std::sin(2.0 * std::numbers::pi * hz * static_cast<double>(frame) / SampleRate));
        for (int channel = 0; channel < channels; channel++) {
            out[frame * static_cast<size_t>(channels) + static_cast<size_t>(channel)] = value;
        }
    }
    return out;
}

std::vector<float> noise(double amplitude)
{
    std::mt19937 rng { 1234 };
    std::uniform_real_distribution<double> dist { -1.0, 1.0 };
    std::vector<float> out(static_cast<size_t>(SampleRate) * Seconds * 2);
    for (size_t frame = 0; frame < out.size() / 2; frame++) {
        const auto value = static_cast<float>(amplitude * dist(rng));
        out[frame * 2] = value;
        out[frame * 2 + 1] = value;
    }
    return out;
}

const SpectrumAnalyzer::Band & bandAt(const SpectrumAnalyzer::Result & result, double hz)
{
    return *std::ranges::min_element(result.bands, {}, [hz](const auto & band) { return std::abs(band.centerHz - hz); });
}

} // namespace

void SpectrumAnalyzerTest::test_bands_silence_shouldNotBeValid()
{
    SpectrumAnalyzer analyzer { SampleRate };
    feed(analyzer, std::vector<float>(static_cast<size_t>(SampleRate) * 2, 0.0f));

    QVERIFY(!analyzer.calculate().isValid);
}

void SpectrumAnalyzerTest::test_bands_belowTheGate_shouldNotBeValid()
{
    // A render carries its lead-in, fade and tail by design. Averaging those in would drag the whole
    // curve down without saying anything about the mix, so the gate has to actually drop them.
    SpectrumAnalyzer analyzer { SampleRate };
    feed(analyzer, tone(1000.0, 0.0005)); // about -66 dBFS

    QVERIFY(!analyzer.calculate().isValid);
}

void SpectrumAnalyzerTest::test_bands_tone_shouldPeakInItsOwnBand()
{
    SpectrumAnalyzer analyzer { SampleRate };
    feed(analyzer, tone(1000.0, 0.5));

    const auto result = analyzer.calculate();
    QVERIFY(result.isValid);

    const auto loudest = std::ranges::max_element(result.bands, {}, [](const auto & band) { return band.levelDb; });
    QVERIFY2(std::abs(loudest->centerHz - 1000.0) < 1.0,
             qPrintable(QString { "loudest band was %1 Hz" }.arg(loudest->centerHz)));
}

void SpectrumAnalyzerTest::test_bands_whiteNoise_shouldRiseThreeDbPerThird()
{
    // White noise has constant power per hertz, and a third-octave band is 23 % wider than the one
    // below it, so the bands must climb by about 1 dB each. Getting this right is what makes the
    // band levels comparable across the spectrum at all.
    SpectrumAnalyzer analyzer { SampleRate };
    feed(analyzer, noise(0.3));

    const auto result = analyzer.calculate();
    QVERIFY(result.isValid);

    const auto oneHundred = bandAt(result, 100.0).levelDb;
    const auto oneThousand = bandAt(result, 1000.0).levelDb;
    const auto expected = 10.0f * std::log10(10.0f); // one decade of bandwidth, so ten times it
    QVERIFY2(std::abs((oneThousand - oneHundred) - expected) < 1.0f,
             qPrintable(QString { "100 Hz %1 dB, 1 kHz %2 dB, difference %3" }.arg(oneHundred).arg(oneThousand).arg(oneThousand - oneHundred)));
}

void SpectrumAnalyzerTest::test_bands_level_shouldNotDependOnMasteredLoudness()
{
    // The whole point of reporting against the mix's own midrange: two masters of the same balance
    // at different levels have to read the same, or no comparison between tracks means anything.
    SpectrumAnalyzer loud { SampleRate };
    feed(loud, noise(0.5));
    SpectrumAnalyzer quiet { SampleRate };
    feed(quiet, noise(0.05)); // 20 dB down

    const auto a = loud.calculate();
    const auto b = quiet.calculate();
    QVERIFY(a.isValid && b.isValid);
    QCOMPARE(a.bands.size(), b.bands.size());

    for (size_t i = 0; i < a.bands.size(); i++) {
        QVERIFY2(std::abs(a.bands[i].levelDb - b.bands[i].levelDb) < 0.5f,
                 qPrintable(QString { "%1 Hz: %2 dB loud, %3 dB quiet" }.arg(a.bands[i].centerHz).arg(a.bands[i].levelDb).arg(b.bands[i].levelDb)));
    }
    QVERIFY(std::abs(a.upperMidToHighDb - b.upperMidToHighDb) < 0.5f);
}

void SpectrumAnalyzerTest::test_summary_scoopedPresence_shouldLowerPresenceAgainstHighs()
{
    // Noise, and the same noise with its presence band removed: the summary number a hollow mix
    // gives itself away by has to move, and in the direction that says presence is missing.
    SpectrumAnalyzer flat { SampleRate };
    feed(flat, noise(0.3));

    auto scooped = noise(0.3);
    // The equalizer the application ships, set the way the band that reads as hollow gets scooped.
    SvfFilter bell;
    bell.calculateBell(1100.0, SampleRate, 1.0, -12.0);
    for (size_t frame = 0; frame < scooped.size() / 2; frame++) {
        const auto out = static_cast<float>(bell.process(scooped[frame * 2]));
        scooped[frame * 2] = out;
        scooped[frame * 2 + 1] = out;
    }
    SpectrumAnalyzer dipped { SampleRate };
    feed(dipped, scooped);

    const auto a = flat.calculate();
    const auto b = dipped.calculate();
    QVERIFY(a.isValid && b.isValid);
    QVERIFY2(b.upperMidToHighDb < a.upperMidToHighDb - 1.0f,
             qPrintable(QString { "flat %1 dB, scooped %2 dB" }.arg(a.upperMidToHighDb).arg(b.upperMidToHighDb)));
}

void SpectrumAnalyzerTest::test_bands_mono_shouldMatchStereo()
{
    // Renders are not always two channels, and the stride is the analyzer's business rather than the
    // caller's.
    SpectrumAnalyzer stereo { SampleRate, 2 };
    feed(stereo, tone(1000.0, 0.5, 2));
    SpectrumAnalyzer mono { SampleRate, 1 };
    feed(mono, tone(1000.0, 0.5, 1));

    const auto a = stereo.calculate();
    const auto b = mono.calculate();
    QVERIFY(a.isValid && b.isValid);
    QCOMPARE(a.bands.size(), b.bands.size());
    for (size_t i = 0; i < a.bands.size(); i++) {
        QVERIFY2(std::abs(a.bands[i].levelDb - b.bands[i].levelDb) < 0.5f,
                 qPrintable(QString { "%1 Hz: %2 vs %3" }.arg(a.bands[i].centerHz).arg(a.bands[i].levelDb).arg(b.bands[i].levelDb)));
    }
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::SpectrumAnalyzerTest)
