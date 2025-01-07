// This file is part of Cacophony.
// Copyright (C) 2024 Jussi Lind <jussi.lind@iki.fi>
//
// Cacophony is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// Cacophony is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Cacophony. If not, see <http://www.gnu.org/licenses/>.

#include "editor_service_test.hpp"

#include "../../application/editor_service.hpp"

#include <QSignalSpy>

namespace cacophony {

void EditorServiceTest::test_defaultSong_shouldReturnCorrectProperties()
{
    EditorService editorService;

    const auto trackCount = 8;
    QCOMPARE(editorService.trackCount(), trackCount);
    for (uint32_t trackId = 0; trackId < trackCount; trackId++) {
        QCOMPARE(editorService.columnCount(trackId), 1);
        QCOMPARE(editorService.trackName(trackId), QString { "Track %1" }.arg(trackId + 1));
    }

    for (uint32_t trackId = 0; trackId < trackCount; trackId++) {
        QCOMPARE(editorService.columnCount(trackId), 1);
    }

    QCOMPARE(editorService.patternCount(), 1);
    QCOMPARE(editorService.lineCount(0), 64);
}

void EditorServiceTest::test_defaultSong_shouldNotHaveNoteData()
{
    EditorService editorService;

    for (uint8_t pattern = 0; pattern < editorService.patternCount(); pattern++) {
        for (uint8_t track = 0; track < editorService.trackCount(); track++) {
            for (uint8_t column = 0; column < editorService.columnCount(track); column++) {
                for (uint8_t line = 0; line < editorService.lineCount(pattern); line++) {
                    QCOMPARE(editorService.displayNoteAtPosition(pattern, track, column, line), editorService.noDataString());
                    QCOMPARE(editorService.displayVelocityAtPosition(pattern, track, column, line), editorService.noDataString());
                }
            }
        }
    }
}

void EditorServiceTest::test_requestDigitSetAtCurrentPosition_velocity_shouldChangeVelocity()
{
    EditorService editorService;
    QSignalSpy noteDataChangedSpy { &editorService, &EditorService::noteDataAtPositionChanged };
    QVERIFY(editorService.requestPosition(0, 0, 0, 0, 0));
    QVERIFY(editorService.requestNoteOnAtCurrentPosition(1, 3, 64));

    editorService.requestCursorRight();
    QVERIFY(!editorService.requestDigitSetAtCurrentPosition(4)); // 464 not possible
    QVERIFY(editorService.requestDigitSetAtCurrentPosition(1)); // 164 not possible => 127
    QCOMPARE(editorService.displayVelocityAtPosition(0, 0, 0, 0), "127");
    QVERIFY(editorService.requestDigitSetAtCurrentPosition(0)); // 027 possible
    QCOMPARE(editorService.displayVelocityAtPosition(0, 0, 0, 0), "027");
    editorService.requestCursorRight();
    QVERIFY(editorService.requestDigitSetAtCurrentPosition(6));
    QCOMPARE(editorService.displayVelocityAtPosition(0, 0, 0, 0), "067");
    editorService.requestCursorRight();
    QVERIFY(editorService.requestDigitSetAtCurrentPosition(8));
    QCOMPARE(editorService.displayVelocityAtPosition(0, 0, 0, 0), "068");
    editorService.requestCursorLeft();
    editorService.requestCursorLeft();
    QVERIFY(editorService.requestDigitSetAtCurrentPosition(1)); // 168 not possible => 127
    QCOMPARE(editorService.displayVelocityAtPosition(0, 0, 0, 0), "127");
    editorService.requestCursorRight();
    QVERIFY(editorService.requestDigitSetAtCurrentPosition(0));
    editorService.requestCursorRight();
    QVERIFY(editorService.requestDigitSetAtCurrentPosition(0));
    QCOMPARE(editorService.displayVelocityAtPosition(0, 0, 0, 0), "100");
    editorService.requestCursorLeft();
    QVERIFY(editorService.requestDigitSetAtCurrentPosition(5)); // 150 not possible => 050
    QCOMPARE(editorService.displayVelocityAtPosition(0, 0, 0, 0), "050");
}

void EditorServiceTest::test_requestNoteDeletionAtCurrentPosition_shouldDeleteNoteData()
{
    EditorService editorService;
    QSignalSpy noteDataChangedSpy { &editorService, &EditorService::noteDataAtPositionChanged };
    QVERIFY(editorService.requestPosition(0, 0, 0, 0, 0));

    QVERIFY(editorService.requestNoteOnAtCurrentPosition(1, 3, 64));
    editorService.requestNoteDeletionAtCurrentPosition();

    QCOMPARE(noteDataChangedSpy.count(), 2);
    QCOMPARE(editorService.displayNoteAtPosition(0, 0, 0, 4), editorService.noDataString());
    QCOMPARE(editorService.displayVelocityAtPosition(0, 0, 0, 4), editorService.noDataString());
}

void EditorServiceTest::test_requestNoteOnAtCurrentPosition_shouldChangeNoteData()
{
    EditorService editorService;
    QSignalSpy noteDataChangedSpy { &editorService, &EditorService::noteDataAtPositionChanged };
    QVERIFY(editorService.requestPosition(0, 0, 0, 0, 0));
    QVERIFY(editorService.requestNoteOnAtCurrentPosition(1, 3, 64));

    QCOMPARE(noteDataChangedSpy.count(), 1);
    QCOMPARE(editorService.displayNoteAtPosition(0, 0, 0, 0), "C-3");
    QCOMPARE(editorService.displayVelocityAtPosition(0, 0, 0, 0), "064");

    QVERIFY(editorService.requestNoteOnAtCurrentPosition(3, 3, 88));
    QCOMPARE(noteDataChangedSpy.count(), 2);
    QCOMPARE(editorService.displayNoteAtPosition(0, 0, 0, 0), "D-3");
    QCOMPARE(editorService.displayVelocityAtPosition(0, 0, 0, 0), "064"); // Should not change velocity on existing note

    editorService.requestScroll(4);
    editorService.requestNoteOnAtCurrentPosition(2, 4, 72);
    QCOMPARE(noteDataChangedSpy.count(), 3);
    QCOMPARE(editorService.displayNoteAtPosition(0, 0, 0, 4), "C#4");
    QCOMPARE(editorService.displayVelocityAtPosition(0, 0, 0, 4), "072");
}

void EditorServiceTest::test_requestNoteOnAtCurrentPosition_notOnNoteColumn_shouldNotChangeNoteData()
{
    EditorService editorService;
    QSignalSpy noteDataChangedSpy { &editorService, &EditorService::noteDataAtPositionChanged };

    QVERIFY(editorService.requestPosition(0, 0, 0, 0, 1));
    QVERIFY(!editorService.requestNoteOnAtCurrentPosition(1, 3, 64));
    QCOMPARE(noteDataChangedSpy.count(), 0);

    QCOMPARE(editorService.displayNoteAtPosition(0, 0, 0, 0), editorService.noDataString());
    QCOMPARE(editorService.displayVelocityAtPosition(0, 0, 0, 0), editorService.noDataString());
}

void EditorServiceTest::test_requestPosition_invalidPosition_shouldNotChangePosition()
{
    EditorService editorService;
    QSignalSpy positionChangedSpy { &editorService, &EditorService::positionChanged };

    const auto neg = static_cast<uint32_t>(-1);
    QVERIFY(!editorService.requestPosition(neg, 0, 0, 0, 0));
    QVERIFY(!editorService.requestPosition(0, neg, 0, 0, 0));
    QVERIFY(!editorService.requestPosition(0, 0, neg, 0, 0));
    QVERIFY(!editorService.requestPosition(0, 0, 0, neg, 0));
    QVERIFY(!editorService.requestPosition(0, 0, 0, 0, neg));
    QCOMPARE(positionChangedSpy.count(), 0);
}

void EditorServiceTest::test_requestPosition_validPosition_shouldChangePosition()
{
    EditorService editorService;
    QSignalSpy positionChangedSpy { &editorService, &EditorService::positionChanged };

    QVERIFY(editorService.requestPosition(0, 0, 0, 0, 0));
    QVERIFY(editorService.requestPosition(0, 0, 0, 0, 1));
    QVERIFY(editorService.requestPosition(0, 0, 0, 0, 2));
    QVERIFY(editorService.requestPosition(0, 0, 0, 0, 3));
    QCOMPARE(positionChangedSpy.count(), 4);
}

void EditorServiceTest::test_requestScroll_shouldChangePosition()
{
    EditorService editorService;
    QSignalSpy positionChangedSpy { &editorService, &EditorService::positionChanged };

    QCOMPARE(editorService.position().line, 0);

    editorService.requestScroll(1);
    QCOMPARE(editorService.position().line, 1);
    QCOMPARE(positionChangedSpy.count(), 1);

    editorService.requestScroll(0);
    QCOMPARE(editorService.position().line, 1);
    QCOMPARE(positionChangedSpy.count(), 2);

    editorService.requestScroll(-1);
    QCOMPARE(editorService.position().line, 0);
    QCOMPARE(positionChangedSpy.count(), 3);

    editorService.requestScroll(-10);
    QCOMPARE(editorService.position().line, editorService.lineCount(editorService.currentPatternId()) - 10);
    QCOMPARE(positionChangedSpy.count(), 4);

    editorService.requestScroll(10);
    QCOMPARE(editorService.position().line, 0);
    QCOMPARE(positionChangedSpy.count(), 5);

    editorService.requestScroll(static_cast<int>(editorService.lineCount(editorService.currentPatternId()) + 10));
    QCOMPARE(editorService.position().line, 10);
    QCOMPARE(positionChangedSpy.count(), 6);
}

void EditorServiceTest::test_requestTrackFocus_shouldChangePosition()
{
    EditorService editorService;
    QSignalSpy positionChangedSpy { &editorService, &EditorService::positionChanged };

    editorService.requestTrackFocus(0);
    QCOMPARE(positionChangedSpy.count(), 1);

    editorService.requestTrackFocus(editorService.trackCount() - 1);
    QCOMPARE(positionChangedSpy.count(), 2);
    QCOMPARE(editorService.position().track, editorService.trackCount() - 1);
}

void EditorServiceTest::test_requestTrackFocus_shouldNotChangePosition()
{
    EditorService editorService;
    QSignalSpy positionChangedSpy { &editorService, &EditorService::positionChanged };

    editorService.requestTrackFocus(editorService.trackCount());

    QCOMPARE(positionChangedSpy.count(), 0);
    QCOMPARE(editorService.position().track, 0);
}

void EditorServiceTest::test_setTrackName_shouldChangeTrackName()
{
    EditorService editorService;

    editorService.setTrackName(0, "Foo");
    editorService.setTrackName(1, "Bar");

    QCOMPARE(editorService.trackName(0), "Foo");
    QCOMPARE(editorService.trackName(1), "Bar");
}

} // namespace cacophony

QTEST_GUILESS_MAIN(cacophony::EditorServiceTest)
