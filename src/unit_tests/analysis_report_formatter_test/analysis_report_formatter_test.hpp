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

#ifndef ANALYSIS_REPORT_FORMATTER_TEST_HPP
#define ANALYSIS_REPORT_FORMATTER_TEST_HPP

#include <QObject>

namespace noteahead {

class AnalysisReportFormatterTest : public QObject
{
    Q_OBJECT

private slots:
    void test_text_shouldReportTheFileAndItsLoudness();
    void test_comparisonText_shouldSubtractTheLeftFromTheRight();
    void test_comparisonText_oneSideOnly_shouldLeaveTheDifferenceOut();
    void test_comparisonText_differentSampleRates_shouldCompareSharedBands();
    void test_analysisFilePath_shouldKeepTheAudioExtension();
};

} // namespace noteahead

#endif // ANALYSIS_REPORT_FORMATTER_TEST_HPP
