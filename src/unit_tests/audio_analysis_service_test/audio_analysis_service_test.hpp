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

#ifndef AUDIO_ANALYSIS_SERVICE_TEST_HPP
#define AUDIO_ANALYSIS_SERVICE_TEST_HPP

#include <QObject>

namespace noteahead {

class AudioAnalysisServiceTest : public QObject
{
    Q_OBJECT

private slots:
    void test_analyze_toneFile_shouldReportItsLoudness();
    void test_analyze_unreadableFile_shouldFail();
    void test_analyze_brighterFile_shouldShowPositiveDifference();
    void test_swap_shouldInvertTheDifference();
    void test_recentFiles_shouldListWhatWasMeasured();
    void test_reportText_noFiles_shouldBeEmpty();
    void test_saveReport_shouldWriteTheComparison();
};

} // namespace noteahead

#endif // AUDIO_ANALYSIS_SERVICE_TEST_HPP
