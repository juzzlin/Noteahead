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

#ifndef DC_BLOCKER_TEST_HPP
#define DC_BLOCKER_TEST_HPP

#include <QObject>

namespace noteahead {

class DcBlockerTest : public QObject
{
    Q_OBJECT

private slots:
    void test_dcBlocker_constantInput_shouldConvergeToZero();
    void test_dcBlocker_dcPlusSine_shouldRemoveDcFromOutput();
    void test_dcBlocker_highFrequencySine_shouldPassThrough();
    void test_dcBlocker_reset_shouldClearState();
};

} // namespace noteahead

#endif // DC_BLOCKER_TEST_HPP
