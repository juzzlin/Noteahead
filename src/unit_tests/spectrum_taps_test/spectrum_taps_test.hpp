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

#ifndef SPECTRUM_TAPS_TEST_HPP
#define SPECTRUM_TAPS_TEST_HPP

#include <QObject>

namespace noteahead {

class SpectrumTapsTest : public QObject
{
    Q_OBJECT

private slots:
    void test_core_beforeEnabled_shouldBeNull();
    void test_setEnabled_preOnly_shouldCreateOnlyPreCore();
    void test_setEnabled_off_shouldKeepCoreAndStopRunning();
    void test_isSettled_eq8_whileEnabled_shouldBeFalse();
    void test_isSettled_eq32_whileEnabled_shouldBeFalse();
    void test_postMinusPre_eq8_flat_shouldBeZero();
    void test_postMinusPre_eq32_boost_shouldMatchResponse();
    void test_postMinusPre_eq32LinearPhase_boost_shouldMatchResponse();
    void test_postMinusPre_eq32LinearPhase_afterSignalStops_shouldStayAligned();
};

} // namespace noteahead

#endif // SPECTRUM_TAPS_TEST_HPP
