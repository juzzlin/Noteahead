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

#include "editor_service_delay_test.hpp"

#include "../../application/service/automation_service.hpp"
#include "../../application/service/editor_service.hpp"
#include "../../application/service/property_service.hpp"
#include "../../application/service/selection_service.hpp"
#include "../../application/service/settings_service.hpp"
#include "../../domain/tracker/note_data.hpp"
#include "../../domain/tracker/song.hpp"
#include "../../infra/data_service.hpp"

#include <QTest>

namespace noteahead {

void EditorServiceDelayTest::test_requestDigitSetAtCurrentPosition_shouldSetDelay_whenAtDelayColumn()
{
    EditorService editorService { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    editorService.requestNewTrackToRight(); // Ensure we have a track
    editorService.requestNewColumn(0); // Ensure we have a column

    // Insert a note first at the note column
    editorService.requestPosition(0, 0, 0, 0, 0);
    QVERIFY(editorService.requestNoteOnAtCurrentPosition(1, 4, 100));

    // Navigate to delay column (tens)
    editorService.requestPosition(0, 0, 0, 0, 4);
    QVERIFY(editorService.isAtDelayColumn());

    // Set tens digit
    QVERIFY(editorService.requestDigitSetAtCurrentPosition(1));
    QCOMPARE(editorService.delayAtCurrentPosition(), 10);

    // Navigate to delay column (ones)
    editorService.requestPosition(0, 0, 0, 0, 5);
    QVERIFY(editorService.isAtDelayColumn());

    // Set ones digit
    QVERIFY(editorService.requestDigitSetAtCurrentPosition(5));
    QCOMPARE(editorService.delayAtCurrentPosition(), 15);

    // Test clamping (max 24)
    editorService.requestPosition(0, 0, 0, 0, 4);
    QVERIFY(!editorService.requestDigitSetAtCurrentPosition(3)); // Try to set 30, should return false for tens > 2
    QCOMPARE(editorService.delayAtCurrentPosition(), 15); // Should still be 15
}

void EditorServiceDelayTest::test_cursorNavigation_shouldIncludeDelayColumns()
{
    EditorService editorService { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    editorService.requestNewTrackToRight();

    // Start at note column
    editorService.requestPosition(0, 0, 0, 0, 0);
    QVERIFY(editorService.isAtNoteColumn());

    // Move right to velocity columns
    editorService.requestCursorRight();
    QCOMPARE(editorService.position().lineColumn, 1);
    editorService.requestCursorRight();
    QCOMPARE(editorService.position().lineColumn, 2);
    editorService.requestCursorRight();
    QCOMPARE(editorService.position().lineColumn, 3);
    QVERIFY(editorService.isAtVelocityColumn());

    // Move right to delay columns
    editorService.requestCursorRight();
    QCOMPARE(editorService.position().lineColumn, 4);
    QVERIFY(editorService.isAtDelayColumn());
    editorService.requestCursorRight();
    QCOMPARE(editorService.position().lineColumn, 5);
    QVERIFY(editorService.isAtDelayColumn());

    // Move right to pan columns (column 0 only)
    editorService.requestCursorRight();
    QCOMPARE(editorService.position().lineColumn, 6);
    QVERIFY(editorService.isAtPanColumn());
    editorService.requestCursorRight();
    QCOMPARE(editorService.position().lineColumn, 7);
    editorService.requestCursorRight();
    QCOMPARE(editorService.position().lineColumn, 8);
    QVERIFY(editorService.isAtPanColumn());

    // Move right to next column (column 0's last pan digit -> column 1)
    editorService.requestNewColumn(0);
    // Reset to first column, last pan digit
    editorService.requestPosition(0, 0, 0, 0, 8);

    editorService.requestCursorRight();
    QCOMPARE(editorService.position().column, 1);
    QCOMPARE(editorService.position().lineColumn, 0);

    // Move left back to pan column of previous column
    editorService.requestCursorLeft();
    QCOMPARE(editorService.position().column, 0);
    QCOMPARE(editorService.position().lineColumn, 8);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::EditorServiceDelayTest)
