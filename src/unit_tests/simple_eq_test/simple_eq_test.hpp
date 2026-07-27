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

#ifndef SIMPLE_EQ_TEST_HPP
#define SIMPLE_EQ_TEST_HPP

#include <QObject>

namespace noteahead {

class SimpleEqTest : public QObject
{
    Q_OBJECT

private slots:
    void test_amount_zero_shouldPassThrough();
    void test_amount_engaged_shouldBoostBodyAndAir();
    void test_amount_engaged_shouldScoopLowMid();
    void test_amount_higher_shouldApplyStrongerCurve();
};

} // namespace noteahead

#endif // SIMPLE_EQ_TEST_HPP
