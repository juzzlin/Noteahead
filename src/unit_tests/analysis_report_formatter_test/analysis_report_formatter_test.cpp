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

#include "analysis_report_formatter_test.hpp"

#include "../../application/service/analysis_report_formatter.hpp"
#include "../../application/service/audio_analysis.hpp"

#include <QTest>

namespace noteahead {

namespace {

//! An analysis with the bands and summary a test asks for, so the formatting can be checked without
//! measuring anything.
AudioAnalysis analysisOf(const QString & fileName, std::vector<SpectrumAnalyzer::Band> bands, float highDb, float loudness = -10.0f)
{
    AudioAnalysis analysis;
    analysis.filePath = "/tmp/" + fileName;
    analysis.sampleRate = 44100;
    analysis.channels = 2;
    analysis.frames = 44100;
    analysis.isValid = true;
    analysis.loudness.integratedLoudness = loudness;
    analysis.loudness.truePeak = -1.0f;
    analysis.spectrum.bands = std::move(bands);
    analysis.spectrum.highDb = highDb;
    analysis.spectrum.isValid = true;
    return analysis;
}

//! The row the difference table writes for @p band, or an empty string if there is none. Only that
//! table is searched: each file's own report follows it, and lists every band it has.
QString rowFor(const QString & report, const QString & band)
{
    const auto comparison = report.left(report.indexOf("\n\nA\n=\n"));
    for (auto && line : comparison.split('\n')) {
        if (line.startsWith(band)) {
            return line.simplified();
        }
    }
    return {};
}

} // namespace

void AnalysisReportFormatterTest::test_text_shouldReportTheFileAndItsLoudness()
{
    const auto analysis = analysisOf("mix.flac", { { 1000.0, -2.0f } }, -3.0f, -9.3f);

    const auto report = AnalysisReportFormatter::text(analysis);

    QVERIFY(report.contains("mix.flac"));
    QVERIFY(report.contains("-9.3 LUFS"));
    QVERIFY(report.contains("44100 Hz"));
    QVERIFY(report.contains("1.0 kHz"));
}

void AnalysisReportFormatterTest::test_comparisonText_shouldSubtractTheLeftFromTheRight()
{
    // B - A, so a band the right file has more of reads positive: that is the cut it needs to match
    // the left, which is what an EQ match is read off.
    const auto a = analysisOf("reference.wav", { { 100.0, -1.0f }, { 1000.0, 0.0f } }, -6.0f);
    const auto b = analysisOf("mine.wav", { { 100.0, -3.0f }, { 1000.0, 2.5f } }, -2.0f);

    const auto report = AnalysisReportFormatter::comparisonText(a, b);

    QVERIFY(report.contains("reference.wav"));
    QVERIFY(report.contains("mine.wav"));
    QCOMPARE(rowFor(report, "100 Hz"), QString { "100 Hz -1.0 -3.0 -2.0" });
    QCOMPARE(rowFor(report, "1.0 kHz"), QString { "1.0 kHz 0.0 2.5 2.5" });
    QVERIFY(report.contains("Highs 2.5-8 kHz:"));
    // The bottom belongs in the comparison as much as the top: it is what a club will find.
    QVERIFY(report.contains("Sub 20-50 Hz:"));
    QVERIFY(report.contains("Bass 50-100 Hz:"));
    QVERIFY(report.contains("Air 10-20 kHz:"));
}

void AnalysisReportFormatterTest::test_comparisonText_oneSideOnly_shouldLeaveTheDifferenceOut()
{
    const auto a = analysisOf("alone.wav", { { 1000.0, 0.0f } }, -6.0f);

    const auto report = AnalysisReportFormatter::comparisonText(a, AudioAnalysis {});

    QVERIFY(report.contains("alone.wav"));
    // Nothing to subtract from, so the table of differences is not written at all rather than
    // printed as a column of zeroes against an empty side.
    QVERIFY(!report.contains("B - A"));
}

void AnalysisReportFormatterTest::test_comparisonText_differentSampleRates_shouldCompareSharedBands()
{
    // A 44.1 kHz file has bands a 22 kHz one cannot: those are skipped rather than compared against
    // whatever happens to sit at the same index.
    const auto a = analysisOf("full.wav", { { 1000.0, 0.0f }, { 16000.0, -4.0f } }, -6.0f);
    const auto b = analysisOf("narrow.wav", { { 1000.0, 1.0f } }, -6.0f);

    const auto report = AnalysisReportFormatter::comparisonText(a, b);

    QCOMPARE(rowFor(report, "1.0 kHz"), QString { "1.0 kHz 0.0 1.0 1.0" });
    QVERIFY(rowFor(report, "16 kHz").isEmpty());
}

void AnalysisReportFormatterTest::test_analysisFilePath_shouldKeepTheAudioExtension()
{
    // Rendering the same song to both WAV and FLAC must not have one report overwrite the other.
    QCOMPARE(AnalysisReportFormatter::analysisFilePath("/tmp/song.wav"), QString { "/tmp/song.wav.loudness.txt" });
    QCOMPARE(AnalysisReportFormatter::analysisFilePath("/tmp/song.flac"), QString { "/tmp/song.flac.loudness.txt" });
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::AnalysisReportFormatterTest)
