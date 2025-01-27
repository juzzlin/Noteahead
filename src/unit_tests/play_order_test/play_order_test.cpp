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

#include "play_order_test.hpp"

#include "../../domain/play_order.hpp"

namespace noteahead {

void PlayOrderTest::test_initialization_shouldReturnInitialMapping()
{
    PlayOrder playOrder;

    QCOMPARE(playOrder.positionToPattern(0), uint32_t(0));
}

void PlayOrderTest::test_length_shouldReturnCorrectLength()
{
    PlayOrder playOrder;
    playOrder.setPatternAtPosition(0, 0);
    playOrder.setPatternAtPosition(1, 1);

    QCOMPARE(playOrder.length(), 2);

    playOrder.setPatternAtPosition(10, 2);

    QCOMPARE(playOrder.length(), 11);
}

void PlayOrderTest::test_setAndGetPattern_shouldReturnCorrectValues()
{
    PlayOrder playOrder;
    playOrder.setPatternAtPosition(0, 1);
    playOrder.setPatternAtPosition(1, 2);

    QCOMPARE(playOrder.positionToPattern(0), uint32_t(1));
    QCOMPARE(playOrder.positionToPattern(1), uint32_t(2));
    QCOMPARE(playOrder.positionToPattern(2), uint32_t(0)); // Default return value
}

void PlayOrderTest::test_positionToPattern_shouldReturnCorrectValues()
{
    PlayOrder playOrder;
    playOrder.setPatternAtPosition(5, 42);

    QCOMPARE(playOrder.positionToPattern(5), uint32_t(42));
    QCOMPARE(playOrder.positionToPattern(6), uint32_t(0)); // Nonexistent position
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::PlayOrderTest)
