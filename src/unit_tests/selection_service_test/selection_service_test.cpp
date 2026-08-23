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

#include "selection_service_test.hpp"

#include "../../application/service/selection_service.hpp"

#include <QSignalSpy>
#include <QTest>

namespace noteahead {

void SelectionServiceTest::test_selectedPositions_shouldReturnEmptyIfInvalid()
{
    SelectionService service;
    QVERIFY(service.selectedPositions().empty());
}

void SelectionServiceTest::test_selectedPositions_shouldReturnCorrectRange()
{
    SelectionService service;
    service.requestSelectionStart(1, 2, 3, 4);
    service.requestSelectionEnd(1, 2, 3, 6);

    const auto positions = service.selectedPositions();
    QCOMPARE(positions.size(), 3);
    QCOMPARE(positions.at(0).line, 4);
    QCOMPARE(positions.at(1).line, 5);
    QCOMPARE(positions.at(2).line, 6);
}

void SelectionServiceTest::test_selectedPositions_reversed_shouldReturnCorrectRange()
{
    SelectionService service;
    service.requestSelectionStart(1, 2, 3, 6);
    service.requestSelectionEnd(1, 2, 3, 4);

    const auto positions = service.selectedPositions();
    QCOMPARE(positions.size(), 3);
    QCOMPARE(positions.at(0).line, 4);
    QCOMPARE(positions.at(1).line, 5);
    QCOMPARE(positions.at(2).line, 6);
}

void SelectionServiceTest::test_selectedPositions_reorderedColumns_shouldFollowDisplayOrder()
{
    SelectionService service;

    // A track that had columns 0, 1, 2 and then got a new one inserted on the left. The new column
    // takes the next free index, 3, and sits at display position 0: an index is an identity, not a
    // place on screen, so after an insert the two stop agreeing.
    service.setColumnOrderResolver([](size_t) { return SelectionService::ColumnIndexList { 3, 0, 1, 2 }; });

    // Dragging across the first three columns on screen, which are indices 3, 0 and 1.
    service.requestSelectionStart(0, 0, 3, 0);
    service.requestSelectionEnd(0, 0, 1, 0);

    QVERIFY(service.isSelected(0, 0, 3, 0));
    QVERIFY(service.isSelected(0, 0, 0, 0));
    QVERIFY(service.isSelected(0, 0, 1, 0));

    // Index 2 is the fourth column on screen, past the end of the drag. Walking indices numerically
    // from 1 to 3 would have taken it in and left out index 0, which is what made the rectangle
    // cover the wrong columns.
    QVERIFY(!service.isSelected(0, 0, 2, 0));

    // Left to right on screen, which is what anything walking the selection has to follow.
    const SelectionService::ColumnIndexList expected { 3, 0, 1 };
    QCOMPARE(service.selectedColumns(), expected);
}

void SelectionServiceTest::test_selectedPositions_noResolver_shouldFallBackOnIndexOrder()
{
    // Without a resolver there is nothing to say what the display order is, so the indices are the
    // only order there is. Keeps a bare SelectionService behaving as it always did.
    SelectionService service;
    service.requestSelectionStart(0, 0, 1, 0);
    service.requestSelectionEnd(0, 0, 3, 0);

    QVERIFY(service.isSelected(0, 0, 1, 0));
    QVERIFY(service.isSelected(0, 0, 2, 0));
    QVERIFY(service.isSelected(0, 0, 3, 0));
}

void SelectionServiceTest::test_isValidSelection_shouldReturnFalseForIncompleteSelection()
{
    SelectionService service;
    QVERIFY(!service.isValidSelection());
    service.requestSelectionStart(1, 2, 3, 4);
    QVERIFY(!service.isValidSelection());
}

void SelectionServiceTest::test_isValidSelection_shouldReturnTrueForValidSelection()
{
    SelectionService service;
    service.requestSelectionStart(1, 2, 3, 4);
    service.requestSelectionEnd(1, 2, 3, 5);
    QVERIFY(service.isValidSelection());
}

void SelectionServiceTest::test_isSelected_shouldReturnTrueForSelectedPosition()
{
    SelectionService service;
    service.requestSelectionStart(1, 2, 3, 4);
    service.requestSelectionEnd(1, 2, 3, 5);
    QVERIFY(service.isSelected(1, 2, 3, 4));
    QVERIFY(service.isSelected(1, 2, 3, 5));
}

void SelectionServiceTest::test_isSelected_shouldReturnFalseForNonSelectedPosition()
{
    SelectionService service;
    service.requestSelectionStart(1, 2, 3, 4);
    service.requestSelectionEnd(1, 2, 3, 5);
    QVERIFY(!service.isSelected(1, 2, 3, 6));
}

void SelectionServiceTest::test_requestSelectionStart_shouldSetStartPosition()
{
    SelectionService service;
    QVERIFY(service.requestSelectionStart(1, 2, 3, 4));
}

void SelectionServiceTest::test_requestSelectionEnd_shouldSetEndPosition()
{
    SelectionService service;
    service.requestSelectionStart(1, 2, 3, 4);
    QVERIFY(service.requestSelectionEnd(1, 2, 3, 6));
}

void SelectionServiceTest::test_clear_shouldResetSelection()
{
    SelectionService service;
    service.requestSelectionStart(1, 2, 3, 4);
    service.requestSelectionEnd(1, 2, 3, 6);
    service.clear();
    QVERIFY(!service.isValidSelection());
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::SelectionServiceTest)
