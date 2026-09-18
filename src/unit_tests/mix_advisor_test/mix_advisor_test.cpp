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

#include "mix_advisor_test.hpp"

#include "../../application/service/mix_advice_formatter.hpp"
#include "../../domain/utility/mix_advisor.hpp"

#include <QTest>

#include <algorithm>
#include <cmath>

namespace noteahead {

namespace {

//! A mix whose bands lie exactly on a line of @p tiltDbPerOctave through 1 kHz.
//!
//! Built rather than measured, because what is being tested is the reading of a curve and not the
//! making of one: spectrum_analyzer_test already covers where the curve comes from.
SpectrumAnalyzer::Result makeSpectrum(float tiltDbPerOctave = 0.0f)
{
    SpectrumAnalyzer::Result result;
    for (auto && center : SpectrumAnalyzer::bandCenters()) {
        result.bands.push_back(SpectrumAnalyzer::Band {
          center,
          static_cast<float>(tiltDbPerOctave * std::log2(center / 1000.0)) });
    }
    result.isValid = true;
    return result;
}

//! Adds @p gainDb to every band whose centre falls in [lowHz, highHz).
void bump(SpectrumAnalyzer::Result & spectrum, double lowHz, double highHz, float gainDb)
{
    for (auto & band : spectrum.bands) {
        if (band.centerHz >= lowHz && band.centerHz < highHz) {
            band.levelDb += gainDb;
        }
    }
}

LoudnessAnalyzer::Result makeLoudness(float truePeakDbTp = -6.0f)
{
    LoudnessAnalyzer::Result result;
    result.integratedLoudness = -14.0f;
    result.truePeak = truePeakDbTp;
    result.loudnessRange = 7.0f;
    return result;
}

bool hasTopic(const MixAdvisor::Summary & summary, MixAdvisor::Topic topic)
{
    return std::any_of(summary.findings.begin(), summary.findings.end(), [topic](auto && finding) {
        return finding.topic == topic;
    });
}

//! Findings that are about the balance rather than about the frame it is read in.
std::vector<MixAdvisor::Finding> balanceFindings(const MixAdvisor::Summary & summary)
{
    std::vector<MixAdvisor::Finding> findings;
    for (auto && finding : summary.findings) {
        if (finding.topic != MixAdvisor::Topic::Tilt && finding.topic != MixAdvisor::Topic::Balanced && finding.topic != MixAdvisor::Topic::TruePeak) {
            findings.push_back(finding);
        }
    }
    return findings;
}

QString topicsOf(const MixAdvisor::Summary & summary)
{
    QStringList topics;
    for (auto && finding : summary.findings) {
        topics << QString::number(static_cast<int>(finding.topic)) + QString { "@%1" }.arg(finding.valueDb, 0, 'f', 1);
    }
    return topics.join(", ");
}

} // namespace

void MixAdvisorTest::test_advise_flatCurve_shouldFindNothingToSay()
{
    const auto summary = MixAdvisor::advise(makeSpectrum(), makeLoudness());

    QVERIFY(summary.hasBalance);
    QVERIFY2(balanceFindings(summary).empty(), qPrintable(topicsOf(summary)));
    // Said out loud: an empty section in the report would read as the analysis having failed.
    QVERIFY(hasTopic(summary, MixAdvisor::Topic::Balanced));
}

void MixAdvisorTest::test_advise_darkButSmooth_shouldReportTheTiltAndNothingElse()
{
    // The regression that matters most. A mix that leans steeply but evenly is a dark mix, not five
    // separate faults, and an analyzer that calls it five faults is one nobody reads twice.
    const auto summary = MixAdvisor::advise(makeSpectrum(-3.0f), makeLoudness());

    QVERIFY2(balanceFindings(summary).empty(), qPrintable(topicsOf(summary)));
    QVERIFY(hasTopic(summary, MixAdvisor::Topic::Tilt));
    QVERIFY2(std::abs(summary.tiltDbPerOctave + 3.0f) < 0.1f, qPrintable(QString::number(summary.tiltDbPerOctave)));
}

void MixAdvisorTest::test_advise_lowMidBump_shouldFlagOnlyTheLowMids()
{
    auto spectrum = makeSpectrum();
    bump(spectrum, 125.0, 400.0, 5.0f);

    const auto summary = MixAdvisor::advise(spectrum, makeLoudness());
    const auto findings = balanceFindings(summary);

    QCOMPARE(findings.size(), size_t { 1 });
    QCOMPARE(findings.front().topic, MixAdvisor::Topic::LowMid);
    QCOMPARE(findings.front().direction, MixAdvisor::Direction::Above);
    QCOMPARE(findings.front().severity, MixAdvisor::Severity::Caution);
}

void MixAdvisorTest::test_advise_scoopedPresence_shouldFlagPresenceAndTheHollowNumber()
{
    auto spectrum = makeSpectrum();
    bump(spectrum, 1250.0, 4000.0, -4.0f);
    // The report's own presence-against-highs number, which the analyzer reads rather than derives.
    spectrum.upperMidDb = -4.0f;
    spectrum.highDb = 0.0f;
    spectrum.upperMidToHighDb = spectrum.upperMidDb - spectrum.highDb;

    const auto summary = MixAdvisor::advise(spectrum, makeLoudness());

    QVERIFY2(hasTopic(summary, MixAdvisor::Topic::Presence), qPrintable(topicsOf(summary)));
    QVERIFY(hasTopic(summary, MixAdvisor::Topic::PresenceAgainstHighs));
    for (auto && finding : summary.findings) {
        if (finding.topic == MixAdvisor::Topic::Presence) {
            QCOMPARE(finding.direction, MixAdvisor::Direction::Below);
        }
    }
}

void MixAdvisorTest::test_advise_rolledOffEnds_shouldNotFlagWhatEveryMixDoes()
{
    auto spectrum = makeSpectrum();
    // What music does at both ends: nothing under the bottom octave, and a top that runs out.
    bump(spectrum, 0.0, 50.0, -12.0f);
    bump(spectrum, 10000.0, 30000.0, -8.0f);

    const auto quiet = MixAdvisor::advise(spectrum, makeLoudness());
    QVERIFY2(!hasTopic(quiet, MixAdvisor::Topic::Sub), qPrintable(topicsOf(quiet)));
    QVERIFY2(!hasTopic(quiet, MixAdvisor::Topic::Air), qPrintable(topicsOf(quiet)));

    // Standing above an extrapolated line still means something, and still gets said.
    auto loaded = makeSpectrum();
    bump(loaded, 0.0, 50.0, 6.0f);
    bump(loaded, 10000.0, 30000.0, 6.0f);

    const auto loud = MixAdvisor::advise(loaded, makeLoudness());
    QVERIFY2(hasTopic(loud, MixAdvisor::Topic::Sub), qPrintable(topicsOf(loud)));
    QVERIFY2(hasTopic(loud, MixAdvisor::Topic::Air), qPrintable(topicsOf(loud)));
}

void MixAdvisorTest::test_advise_singleBandPeak_shouldNameItsFrequency()
{
    auto spectrum = makeSpectrum();
    bump(spectrum, 790.0, 810.0, 6.0f); // The 800 Hz third-octave band alone.

    const auto summary = MixAdvisor::advise(spectrum, makeLoudness());

    const auto resonance = std::find_if(summary.findings.begin(), summary.findings.end(), [](auto && finding) {
        return finding.topic == MixAdvisor::Topic::Resonance;
    });
    QVERIFY2(resonance != summary.findings.end(), qPrintable(topicsOf(summary)));
    QVERIFY2(std::abs(resonance->frequencyHz - 800.0) < 10.0, qPrintable(QString::number(resonance->frequencyHz)));
}

void MixAdvisorTest::test_advise_findings_shouldComeWorstFirst()
{
    auto spectrum = makeSpectrum();
    bump(spectrum, 125.0, 400.0, 3.0f);
    bump(spectrum, 4000.0, 10000.0, 6.0f);

    const auto findings = balanceFindings(MixAdvisor::advise(spectrum, makeLoudness()));

    QVERIFY(findings.size() >= 2);
    // A reader who stops after one line has to have read the one that mattered.
    QVERIFY2(std::abs(findings[0].valueDb) > std::abs(findings[1].valueDb),
             qPrintable(QString { "%1 then %2" }.arg(findings[0].valueDb).arg(findings[1].valueDb)));
    QCOMPARE(findings.front().topic, MixAdvisor::Topic::Top);
}

void MixAdvisorTest::test_advise_truePeak_shouldFlagOnlyWhatIsAboveTheCeiling()
{
    QVERIFY(!hasTopic(MixAdvisor::advise(makeSpectrum(), makeLoudness(-1.5f)), MixAdvisor::Topic::TruePeak));

    const auto tight = MixAdvisor::advise(makeSpectrum(), makeLoudness(-0.2f));
    QVERIFY(hasTopic(tight, MixAdvisor::Topic::TruePeak));

    const auto over = MixAdvisor::advise(makeSpectrum(), makeLoudness(0.3f));
    for (auto && finding : over.findings) {
        if (finding.topic == MixAdvisor::Topic::TruePeak) {
            // At or above full scale it is not a matter of headroom any more.
            QCOMPARE(finding.severity, MixAdvisor::Severity::Caution);
        }
    }
}

void MixAdvisorTest::test_advise_silentSpectrum_shouldStillJudgeTruePeak()
{
    // A file too quiet to average a spectrum over still has a peak, and the report still prints one.
    const auto summary = MixAdvisor::advise(SpectrumAnalyzer::Result {}, makeLoudness(-0.5f));

    QVERIFY(!summary.hasBalance);
    QVERIFY(!hasTopic(summary, MixAdvisor::Topic::Tilt));
    QVERIFY(hasTopic(summary, MixAdvisor::Topic::TruePeak));
}

void MixAdvisorTest::test_fitTiltDbPerOctave_knownSlope_shouldRecoverIt()
{
    for (const float slope : { -4.5f, -1.5f, 0.0f, 2.0f }) {
        const auto measured = MixAdvisor::fitTiltDbPerOctave(makeSpectrum(slope).bands);
        QVERIFY2(std::abs(measured - slope) < 0.05f,
                 qPrintable(QString { "built %1, measured %2" }.arg(slope).arg(measured)));
    }
}

void MixAdvisorTest::test_sentence_everyFinding_shouldHaveWords()
{
    using Topic = MixAdvisor::Topic;
    const std::vector<Topic> topics { Topic::Tilt, Topic::Balanced, Topic::Sub, Topic::LowEnd, Topic::LowMid,
                                      Topic::Mid, Topic::Presence, Topic::Top, Topic::Air,
                                      Topic::PresenceAgainstHighs, Topic::Resonance, Topic::TruePeak };

    // A finding with no sentence would be a blank line in the report, which reads as a failure
    // rather than as a finding. Every combination the advisor can produce has to say something,
    // including the values a switch is most likely to fall through on.
    for (const auto topic : topics) {
        for (const auto severity : { MixAdvisor::Severity::Note, MixAdvisor::Severity::Caution }) {
            for (const auto direction : { MixAdvisor::Direction::Neutral, MixAdvisor::Direction::Above, MixAdvisor::Direction::Below }) {
                for (const float value : { -5.0f, 0.0f, 5.0f }) {
                    const MixAdvisor::Finding finding { topic, severity, direction, value, 800.0 };
                    QVERIFY2(!MixAdviceFormatter::sentence(finding).isEmpty(),
                             qPrintable(QString { "topic %1, value %2" }.arg(static_cast<int>(topic)).arg(value)));
                }
            }
        }
    }
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::MixAdvisorTest)
