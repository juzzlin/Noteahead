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

#ifndef CLIPPER_TEST_HPP
#define CLIPPER_TEST_HPP

#include <QObject>

namespace noteahead {

class ClipperTest : public QObject
{
    Q_OBJECT

private slots:
    void test_hardMode_belowThreshold_shouldNotClip();
    void test_hardMode_aboveThreshold_shouldClipToThreshold();
    void test_reductionMeter_belowThreshold_shouldStayAtZero();
    void test_reductionMeter_aboveThreshold_shouldShowReduction();
    void test_reductionMeter_higherThreshold_shouldShowLessReduction();
};

} // namespace noteahead

#endif // CLIPPER_TEST_HPP
