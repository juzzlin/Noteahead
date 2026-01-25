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

#ifndef SELECTION_SERVICE_TEST_HPP
#define SELECTION_SERVICE_TEST_HPP

#include <QObject>

namespace noteahead {

class SelectionServiceTest : public QObject
{
    Q_OBJECT

private slots:
    void test_selectedPositions_shouldReturnEmptyIfInvalid();
    void test_selectedPositions_shouldReturnCorrectRange();
    void test_selectedPositions_reversed_shouldReturnCorrectRange();

    void test_isValidSelection_shouldReturnFalseForIncompleteSelection();
    void test_isValidSelection_shouldReturnTrueForValidSelection();

    void test_isSelected_shouldReturnTrueForSelectedPosition();
    void test_isSelected_shouldReturnFalseForNonSelectedPosition();

    void test_requestSelectionStart_shouldSetStartPosition();
    void test_requestSelectionEnd_shouldSetEndPosition();

    void test_clear_shouldResetSelection();
};

} // namespace noteahead

#endif // SELECTION_SERVICE_TEST_HPP
