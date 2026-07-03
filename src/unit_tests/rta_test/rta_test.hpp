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

#ifndef RTA_TEST_HPP
#define RTA_TEST_HPP

#include <QObject>

namespace noteahead {

class RtaTest : public QObject
{
    Q_OBJECT

private slots:
    void test_typeId_shouldReturnExpectedString();
    void test_type_shouldReturnExpectedString();
    void test_process_simple_shouldBePassthrough();
    void test_bandMagnitudesDb_afterConstruction_shouldBeNonEmpty();
    void test_bandLogPositions_sizeMatchesBandCount();
    void test_process_audioContext_analysisDisabled_shouldNotUpdateBands();
    void test_process_audioContext_analysisEnabled_silence_shouldGoLowAfterAnalysis();
    void test_process_audioContext_analysisEnabled_sine1kHz_shouldDetectEnergy();
    void test_reset_shouldClearBandMagnitudesToFloor();
    void test_bandCount_mode64_shouldIncreaseBandCount();
};

} // namespace noteahead

#endif // RTA_TEST_HPP
