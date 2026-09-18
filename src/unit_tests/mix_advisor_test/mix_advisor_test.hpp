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

#ifndef MIX_ADVISOR_TEST_HPP
#define MIX_ADVISOR_TEST_HPP

#include <QObject>

namespace noteahead {

class MixAdvisorTest : public QObject
{
    Q_OBJECT

private slots:
    void test_advise_flatCurve_shouldFindNothingToSay();
    void test_advise_darkButSmooth_shouldReportTheTiltAndNothingElse();
    void test_advise_lowMidBump_shouldFlagOnlyTheLowMids();
    void test_advise_scoopedPresence_shouldFlagPresenceAndTheHollowNumber();
    void test_advise_rolledOffEnds_shouldNotFlagWhatEveryMixDoes();
    void test_advise_singleBandPeak_shouldNameItsFrequency();
    void test_advise_findings_shouldComeWorstFirst();
    void test_advise_truePeak_shouldFlagOnlyWhatIsAboveTheCeiling();
    void test_advise_silentSpectrum_shouldStillJudgeTruePeak();
    void test_fitTiltDbPerOctave_knownSlope_shouldRecoverIt();
    void test_sentence_everyFinding_shouldHaveWords();
};

} // namespace noteahead

#endif // MIX_ADVISOR_TEST_HPP
