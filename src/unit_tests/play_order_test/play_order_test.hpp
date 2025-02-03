// This file is part of Noteahead.
// Copyright (C) 2025 Jussi Lind <jussi.lind@iki.fi>
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

#ifndef PLAY_ORDER_TEST_HPP
#define PLAY_ORDER_TEST_HPP

#include <QTest>

namespace noteahead {

class PlayOrderTest : public QObject
{
    Q_OBJECT

private slots:

    void test_initialization_shouldReturnInitialMapping();

    void test_insertPattern_shouldInsertPattern();

    void test_flatten_shouldReturnCorrectMapping();

    void test_length_shouldReturnCorrectLength();

    void test_removePattern_shouldRemovePattern();

    void test_setAndGetPattern_shouldReturnCorrectValues();

    void test_positionToPattern_shouldReturnCorrectValues();
};

} // namespace noteahead

#endif // PLAY_ORDER_TEST_HPP
