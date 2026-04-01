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

#ifndef ARPEGGIATOR_TEST_HPP
#define ARPEGGIATOR_TEST_HPP

#include <QtTest>

namespace noteahead {

class ArpeggiatorTest : public QObject
{
    Q_OBJECT

private slots:
    void test_generate_singleNote_shouldReturnInput();
    void test_generate_up_shouldSortNotes();
    void test_generate_down_shouldSortNotesDescending();
    void test_generate_upDown_shouldFollowPattern();
    void test_generate_downUp_shouldFollowPattern();
    void test_generate_random_shouldReturnPermutation();
};

} // namespace noteahead

#endif // ARPEGGIATOR_TEST_HPP
