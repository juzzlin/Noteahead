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

#ifndef ADSR_ENVELOPE_TEST_HPP
#define ADSR_ENVELOPE_TEST_HPP

#include <QObject>

namespace noteahead {

class AdsrEnvelopeTest : public QObject
{
    Q_OBJECT

private slots:
    void test_isSilent_beforeTrigger_shouldBeTrue();
    void test_isSilent_duringAttack_shouldBeFalse();
    void test_isSilent_zeroSustain_shouldEndWithoutRelease();
    void test_isSilent_nonZeroSustain_shouldHoldUntilReleased();
    void test_isSilent_afterRelease_shouldBeTrue();

    void test_curve_zeroDecay_shouldStayLinear();
    void test_curve_zeroRelease_shouldStayLinear();
    void test_curve_halfDecay_shouldMatchPluckShape();
    void test_curve_fullDecay_shouldFallFasterThanLinear();
    void test_curve_fullDecay_shouldReachSustainAtSameTime();
    void test_curve_fullAttack_shouldRiseFasterThanLinear();
};

} // namespace noteahead

#endif // ADSR_ENVELOPE_TEST_HPP
