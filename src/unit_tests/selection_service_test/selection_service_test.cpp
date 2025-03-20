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
