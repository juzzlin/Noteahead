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

#include <QTest>

namespace noteahead {

void PlayOrderTest::test_initialization_shouldReturnInitialMapping()
{
    PlayOrder playOrder { 666 };

    QCOMPARE(playOrder.positionToPattern(0), 666);
}

void PlayOrderTest::test_insertPattern_shouldInsertPattern()
{
    PlayOrder playOrder;

    playOrder.setPatternAtPosition(0, 0);
    playOrder.setPatternAtPosition(1, 1);
    playOrder.setPatternAtPosition(2, 2);

    playOrder.insertPattern(0, 3);

    QCOMPARE(playOrder.positionToPattern(0), 3);
    QCOMPARE(playOrder.positionToPattern(1), 0);
    QCOMPARE(playOrder.positionToPattern(2), 1);
    QCOMPARE(playOrder.positionToPattern(3), 2);
}

void PlayOrderTest::test_getPatterns_shouldReturnCorrectPatterns()
{
    PlayOrder playOrder;
    playOrder.setPatternAtPosition(0, 10);
    playOrder.setPatternAtPosition(1, 11);

    QCOMPARE(playOrder.getPatterns(2).at(0), 10);
    QCOMPARE(playOrder.getPatterns(2).at(1), 11);
    QCOMPARE(playOrder.getPatterns(3).at(2), 0);
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

void PlayOrderTest::test_removePattern_shouldRemovePattern()
{
    PlayOrder playOrder;

    playOrder.setPatternAtPosition(0, 0);
    playOrder.setPatternAtPosition(1, 1);
    playOrder.setPatternAtPosition(2, 2);

    playOrder.removePattern(0);

    QCOMPARE(playOrder.positionToPattern(0), 1);
    QCOMPARE(playOrder.positionToPattern(1), 2);
}

void PlayOrderTest::test_setAndGetPattern_shouldReturnCorrectValues()
{
    PlayOrder playOrder;
    playOrder.setPatternAtPosition(0, 1);
    playOrder.setPatternAtPosition(1, 2);

    QCOMPARE(playOrder.positionToPattern(0), 1);
    QCOMPARE(playOrder.positionToPattern(1), 2);
    QCOMPARE(playOrder.positionToPattern(2), 0); // Default return value
}

void PlayOrderTest::test_positionToPattern_shouldReturnCorrectValues()
{
    PlayOrder playOrder;
    playOrder.setPatternAtPosition(5, 42);

    QCOMPARE(playOrder.positionToPattern(5), 42);
    QCOMPARE(playOrder.positionToPattern(6), 0); // Nonexistent position
}

void PlayOrderTest::test_skipped_shouldReturnCorrectValues()
{
    PlayOrder playOrder;
    QCOMPARE(playOrder.isSkipped(0), false);
    playOrder.setSkipped(0, true);
    QCOMPARE(playOrder.isSkipped(0), true);
    playOrder.setSkipped(0, false);
    QCOMPARE(playOrder.isSkipped(0), false);

    playOrder.setSkipped(5, true);
    QCOMPARE(playOrder.isSkipped(5), true);
    QCOMPARE(playOrder.isSkipped(4), false);
}

void PlayOrderTest::test_hasPattern_shouldReturnCorrectValues()
{
    PlayOrder playOrder;
    QVERIFY(playOrder.hasPattern(0));
    QVERIFY(!playOrder.hasPattern(1));

    playOrder.setPatternAtPosition(1, 1);
    QVERIFY(playOrder.hasPattern(1));

    playOrder.removePattern(1);
    QVERIFY(!playOrder.hasPattern(1));

    playOrder.insertPattern(0, 5);
    QVERIFY(playOrder.hasPattern(5));
}

void PlayOrderTest::test_removeMissingPatterns_shouldRemoveNonExistentPatterns()
{
    PlayOrder playOrder;
    playOrder.setPatternAtPosition(0, 0);
    playOrder.setPatternAtPosition(1, 1);
    playOrder.setPatternAtPosition(2, 2);

    PlayOrder::PatternSet validPatterns { 0, 2 };
    playOrder.removeMissingPatterns(validPatterns);

    QCOMPARE(playOrder.length(), 2);
    QCOMPARE(playOrder.positionToPattern(0), 0);
    QCOMPARE(playOrder.positionToPattern(1), 2);
}

void PlayOrderTest::test_insertPattern_atEnd_shouldIncreaseLengthByOne()
{
    PlayOrder playOrder { 0 };
    QCOMPARE(playOrder.length(), 1);

    playOrder.insertPattern(1, 1);
    QCOMPARE(playOrder.length(), 2);
    QCOMPARE(playOrder.positionToPattern(0), 0);
    QCOMPARE(playOrder.positionToPattern(1), 1);
}

void PlayOrderTest::test_insertPattern_inMiddle_shouldIncreaseLengthByOne()
{
    PlayOrder playOrder { 0 };
    playOrder.setPatternAtPosition(0, 0);
    playOrder.setPatternAtPosition(1, 1);
    QCOMPARE(playOrder.length(), 2);

    playOrder.insertPattern(1, 2); // Insert 2 at pos 1
    QCOMPARE(playOrder.length(), 3);
    QCOMPARE(playOrder.positionToPattern(0), 0);
    QCOMPARE(playOrder.positionToPattern(1), 2);
    QCOMPARE(playOrder.positionToPattern(2), 1);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::PlayOrderTest)
