// This file is part of Noteahead.
// Copyright (C) 2024 Jussi Lind <jussi.lind@iki.fi>
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

#include "editor_service_test.hpp"

#include "../../application/editor_service.hpp"
#include "../../domain/instrument.hpp"
#include "../../domain/note_data.hpp"
#include "../../domain/song.hpp"

#include <QSignalSpy>

namespace noteahead {

void EditorServiceTest::test_defaultSong_shouldReturnCorrectProperties()
{
    EditorService editorService;

    const auto trackCount = 8;
    QCOMPARE(editorService.trackCount(), trackCount);
    for (size_t trackId = 0; trackId < trackCount; trackId++) {
        QCOMPARE(editorService.columnCount(trackId), 1);
        QCOMPARE(editorService.trackName(trackId), QString { "Track %1" }.arg(trackId + 1));
    }

    for (size_t trackId = 0; trackId < trackCount; trackId++) {
        QCOMPARE(editorService.columnCount(trackId), 1);
    }

    QCOMPARE(editorService.patternCount(), 1);
    QCOMPARE(editorService.lineCount(0), 64);

    QVERIFY(!editorService.canBeSaved());
    QVERIFY(!editorService.isModified());

    QCOMPARE(editorService.currentTime(), "00:00:00.000");
    QCOMPARE(editorService.duration(), "00:00:04.000");
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

void EditorServiceTest::test_patternCopyPaste_equalSizes_shouldCopyPattern()
{
    EditorService editorService;
    QSignalSpy noteDataChangedSpy { &editorService, &EditorService::noteDataAtPositionChanged };
    QVERIFY(editorService.requestPosition(0, 0, 0, 0, 0));
    QVERIFY(editorService.requestNoteOnAtCurrentPosition(1, 3, 64));
    QCOMPARE(noteDataChangedSpy.count(), 1);

    editorService.requestPatternCopy();
    editorService.setCurrentPattern(1);
    editorService.requestPatternPaste();

    QCOMPARE(noteDataChangedSpy.count(), 513);
    QCOMPARE(editorService.displayNoteAtPosition(0, 0, 0, 0), "C-3");
    QCOMPARE(editorService.displayVelocityAtPosition(0, 0, 0, 0), "064");
    QCOMPARE(editorService.displayNoteAtPosition(1, 0, 0, 0), "C-3");
    QCOMPARE(editorService.displayVelocityAtPosition(1, 0, 0, 0), "064");
}

void EditorServiceTest::test_patternCopyPaste_shorterTarget_shouldCopyPattern()
{
    EditorService editorService;
    QSignalSpy noteDataChangedSpy { &editorService, &EditorService::noteDataAtPositionChanged };
    QVERIFY(editorService.requestPosition(0, 0, 0, 0, 0));
    QVERIFY(editorService.requestNoteOnAtCurrentPosition(1, 3, 64));
    QCOMPARE(noteDataChangedSpy.count(), 1);

    editorService.requestPatternCopy();
    editorService.setCurrentPattern(1);
    editorService.setCurrentLineCount(32);
    editorService.requestPatternPaste();

    QCOMPARE(noteDataChangedSpy.count(), 257);
    QCOMPARE(editorService.displayNoteAtPosition(0, 0, 0, 0), "C-3");
    QCOMPARE(editorService.displayVelocityAtPosition(0, 0, 0, 0), "064");
    QCOMPARE(editorService.displayNoteAtPosition(1, 0, 0, 0), "C-3");
    QCOMPARE(editorService.displayVelocityAtPosition(1, 0, 0, 0), "064");
}

void EditorServiceTest::test_requestDigitSetAtCurrentPosition_velocity_shouldChangeVelocity()
{
    EditorService editorService;
    QSignalSpy noteDataChangedSpy { &editorService, &EditorService::noteDataAtPositionChanged };
    QVERIFY(editorService.requestPosition(0, 0, 0, 0, 0));
    QVERIFY(editorService.requestNoteOnAtCurrentPosition(1, 3, 64));
    QVERIFY(editorService.isModified());

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

void EditorServiceTest::test_requestHorizontalScrollPositionChange_shouldChangePosition()
{
    EditorService editorService;
    QSignalSpy horizontalScrollChangeSpy(&editorService, &EditorService::horizontalScrollChanged);
    QSignalSpy positionChangedSpy { &editorService, &EditorService::positionChanged };

    editorService.requestHorizontalScrollPositionChange(0);
    QCOMPARE(editorService.horizontalScrollPosition(), 0);

    editorService.requestHorizontalScrollPositionChange(0.5);
    QCOMPARE(editorService.horizontalScrollPosition(), 2);

    editorService.requestNewColumn(0);
    QCOMPARE(positionChangedSpy.count(), 2);
    editorService.requestHorizontalScrollPositionChange(0.5); // Not the exact threshold, but "enough"

    QCOMPARE(editorService.horizontalScrollPosition(), 3);
    QCOMPARE(horizontalScrollChangeSpy.count(), 2);
    QCOMPARE(positionChangedSpy.count(), 3);
}

void EditorServiceTest::test_requestNewColumn_shouldAddNewColumn()
{
    EditorService editorService;
    QVERIFY(editorService.requestPosition(0, 0, 0, 0, 0));
    QSignalSpy positionChangedSpy { &editorService, &EditorService::positionChanged };
    QSignalSpy columnAddedSpy { &editorService, &EditorService::columnAdded };
    QSignalSpy scrollBarSizeChangedSpy { &editorService, &EditorService::scrollBarSizeChanged };
    QSignalSpy scrollBarStepSizeChangedSpy { &editorService, &EditorService::scrollBarStepSizeChanged };

    editorService.requestNewColumn(0);

    QVERIFY(editorService.isModified());
    QCOMPARE(positionChangedSpy.count(), 1);
    QCOMPARE(columnAddedSpy.count(), 1);
    QCOMPARE(scrollBarSizeChangedSpy.count(), 1);
    QCOMPARE(scrollBarStepSizeChangedSpy.count(), 1);
    QCOMPARE(editorService.columnCount(0), 2);
}

void EditorServiceTest::test_requestColumnDeletion_shouldDeleteColumn()
{
    EditorService editorService;
    QVERIFY(editorService.requestPosition(0, 0, 0, 0, 0));
    QSignalSpy columnDeletedSpy { &editorService, &EditorService::columnDeleted };
    QSignalSpy positionChangedSpy { &editorService, &EditorService::positionChanged };
    QSignalSpy scrollBarSizeChangedSpy { &editorService, &EditorService::scrollBarSizeChanged };
    QSignalSpy scrollBarStepSizeChangedSpy { &editorService, &EditorService::scrollBarStepSizeChanged };

    editorService.requestNewColumn(0);
    QCOMPARE(editorService.columnCount(0), 2);
    QVERIFY(editorService.requestPosition(0, 0, 1, 0, 0));
    QCOMPARE(scrollBarSizeChangedSpy.count(), 1);

    editorService.requestColumnDeletion(0);

    QVERIFY(editorService.isModified());
    QCOMPARE(columnDeletedSpy.count(), 1);
    QCOMPARE(positionChangedSpy.count(), 4);
    QCOMPARE(editorService.position().column, 0);
    QCOMPARE(scrollBarSizeChangedSpy.count(), 2);
    QCOMPARE(scrollBarStepSizeChangedSpy.count(), 2);
    QCOMPARE(editorService.columnCount(0), 1);
}

void EditorServiceTest::test_requestNoteDeletionAtCurrentPosition_shouldDeleteNoteData()
{
    EditorService editorService;
    QSignalSpy noteDataChangedSpy { &editorService, &EditorService::noteDataAtPositionChanged };
    QVERIFY(editorService.requestPosition(0, 0, 0, 0, 0));
    QVERIFY(editorService.requestNoteOnAtCurrentPosition(1, 3, 64));

    editorService.setIsModified(false);
    editorService.requestNoteDeletionAtCurrentPosition();

    QVERIFY(editorService.isModified());
    QCOMPARE(noteDataChangedSpy.count(), 2);
    QCOMPARE(editorService.displayNoteAtPosition(0, 0, 0, 4), editorService.noDataString());
    QCOMPARE(editorService.displayVelocityAtPosition(0, 0, 0, 4), editorService.noDataString());
}

void EditorServiceTest::test_requestNoteInsertionAtCurrentPosition_shouldInsertNoteData()
{
    EditorService editorService;
    QVERIFY(editorService.requestPosition(0, 0, 0, 10, 0));
    QVERIFY(editorService.requestNoteOnAtCurrentPosition(1, 3, 64));
    QVERIFY(editorService.requestPosition(0, 0, 0, 11, 0));
    QVERIFY(editorService.requestNoteOnAtCurrentPosition(2, 3, 65));
    QCOMPARE(editorService.duration(), "00:00:04.000");

    editorService.setIsModified(false);
    QVERIFY(editorService.requestPosition(0, 0, 0, 10, 0));
    QSignalSpy noteDataChangedSpy { &editorService, &EditorService::noteDataAtPositionChanged };
    editorService.requestNoteInsertionAtCurrentPosition();

    QVERIFY(editorService.isModified());
    QCOMPARE(noteDataChangedSpy.count(), 54);
    QCOMPARE(editorService.displayNoteAtPosition(0, 0, 0, 10), editorService.noDataString());
    QCOMPARE(editorService.displayVelocityAtPosition(0, 0, 0, 10), editorService.noDataString());
    QCOMPARE(editorService.displayNoteAtPosition(0, 0, 0, 11), "C-3");
    QCOMPARE(editorService.displayVelocityAtPosition(0, 0, 0, 11), "064");
    QCOMPARE(editorService.displayNoteAtPosition(0, 0, 0, 12), "C#3");
    QCOMPARE(editorService.displayVelocityAtPosition(0, 0, 0, 12), "065");
    QCOMPARE(editorService.duration(), "00:00:04.000");
}

void EditorServiceTest::test_requestNoteOnAtCurrentPosition_shouldChangeNoteData()
{
    EditorService editorService;
    QSignalSpy noteDataChangedSpy { &editorService, &EditorService::noteDataAtPositionChanged };
    QVERIFY(editorService.requestPosition(0, 0, 0, 0, 0));
    QVERIFY(editorService.requestNoteOnAtCurrentPosition(1, 3, 64));
    QVERIFY(editorService.isModified());

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
    QVERIFY(!editorService.isModified());

    QCOMPARE(editorService.displayNoteAtPosition(0, 0, 0, 0), editorService.noDataString());
    QCOMPARE(editorService.displayVelocityAtPosition(0, 0, 0, 0), editorService.noDataString());
}

void EditorServiceTest::test_requestPosition_invalidPosition_shouldNotChangePosition()
{
    EditorService editorService;
    QSignalSpy positionChangedSpy { &editorService, &EditorService::positionChanged };

    const auto neg = static_cast<size_t>(-1);
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
    QCOMPARE(editorService.position().line, editorService.lineCount(editorService.currentPattern()) - 10);
    QCOMPARE(positionChangedSpy.count(), 4);

    editorService.requestScroll(10);
    QCOMPARE(editorService.position().line, 0);
    QCOMPARE(positionChangedSpy.count(), 5);

    editorService.requestScroll(static_cast<int>(editorService.lineCount(editorService.currentPattern()) + 10));
    QCOMPARE(editorService.position().line, 10);
    QCOMPARE(positionChangedSpy.count(), 6);
}

void EditorServiceTest::test_requestScroll_shouldChangeCurrentTime()
{
    EditorService editorService;
    QSignalSpy currentTimeChangedSpy { &editorService, &EditorService::currentTimeChanged };
    QCOMPARE(currentTimeChangedSpy.count(), 0);
    QCOMPARE(editorService.position().line, 0);
    QCOMPARE(editorService.currentTime(), "00:00:00.000");

    editorService.requestScroll(1);

    QCOMPARE(currentTimeChangedSpy.count(), 1);
    QCOMPARE(editorService.position().line, 1);
    QCOMPARE(editorService.currentTime(), "00:00:00.062");
}

void EditorServiceTest::test_requestTrackFocus_shouldChangePosition()
{
    EditorService editorService;
    QSignalSpy positionChangedSpy { &editorService, &EditorService::positionChanged };

    editorService.requestTrackFocus(0, 0);
    QCOMPARE(positionChangedSpy.count(), 1);

    editorService.requestTrackFocus(0, 1);
    QCOMPARE(positionChangedSpy.count(), 1);

    editorService.requestNewColumn(0);
    QCOMPARE(positionChangedSpy.count(), 2);
    editorService.requestTrackFocus(0, 1);
    QCOMPARE(positionChangedSpy.count(), 3);
    QCOMPARE(editorService.position().track, 0);
    QCOMPARE(editorService.position().column, 1);

    editorService.requestTrackFocus(editorService.trackCount() - 1, 0);
    QCOMPARE(positionChangedSpy.count(), 4);
    QCOMPARE(editorService.position().track, editorService.trackCount() - 1);
}

void EditorServiceTest::test_requestTrackFocus_shouldNotChangePosition()
{
    EditorService editorService;
    QSignalSpy positionChangedSpy { &editorService, &EditorService::positionChanged };

    editorService.requestTrackFocus(editorService.trackCount(), 0);

    QCOMPARE(positionChangedSpy.count(), 0);
    QCOMPARE(editorService.position().track, 0);
}

void EditorServiceTest::test_setCurrentLineCount_shouldSetLineCount()
{
    EditorService editorService;
    QSignalSpy currentLineCountChangedSpy { &editorService, &EditorService::currentLineCountChanged };
    QSignalSpy durationChangedSpy { &editorService, &EditorService::durationChanged };
    QSignalSpy positionChangedSpy { &editorService, &EditorService::positionChanged };

    // Min size
    editorService.setCurrentLineCount(0);
    QCOMPARE(currentLineCountChangedSpy.count(), 1);
    QCOMPARE(positionChangedSpy.count(), 1);
    QCOMPARE(editorService.currentLineCount(), editorService.minLineCount());
    QVERIFY(editorService.isModified());
    QCOMPARE(durationChangedSpy.count(), 1);
    QCOMPARE(editorService.duration(), "00:00:00.125");

    // No change
    editorService.setCurrentLineCount(editorService.minLineCount());
    QCOMPARE(currentLineCountChangedSpy.count(), 1);
    QCOMPARE(positionChangedSpy.count(), 1);
    QCOMPARE(editorService.currentLineCount(), editorService.minLineCount());
    QCOMPARE(durationChangedSpy.count(), 1);
    QCOMPARE(editorService.duration(), "00:00:00.125");

    // Expand
    editorService.setCurrentLineCount(256);
    QCOMPARE(currentLineCountChangedSpy.count(), 2);
    QCOMPARE(positionChangedSpy.count(), 2);
    QCOMPARE(editorService.currentLineCount(), 256);
    QCOMPARE(durationChangedSpy.count(), 2);
    QCOMPARE(editorService.duration(), "00:00:16.000");

    // Shrink with cursor line over the bounds
    editorService.requestPosition(0, 0, 0, 200, 0);
    editorService.setCurrentLineCount(64);
    QCOMPARE(positionChangedSpy.count(), 5);
    QCOMPARE(currentLineCountChangedSpy.count(), 3);
    QCOMPARE(editorService.currentLineCount(), 64);
    QCOMPARE(editorService.position().line, 63);
    QCOMPARE(durationChangedSpy.count(), 3);
    QCOMPARE(editorService.duration(), "00:00:04.000");

    // Max size
    editorService.setCurrentLineCount(editorService.maxLineCount());
    QCOMPARE(positionChangedSpy.count(), 6);
    QCOMPARE(currentLineCountChangedSpy.count(), 4);
    QCOMPARE(editorService.currentLineCount(), editorService.maxLineCount());
    QCOMPARE(durationChangedSpy.count(), 4);
    QCOMPARE(editorService.duration(), "00:01:02.437");

    // Beyond max size
    editorService.setCurrentLineCount(editorService.maxLineCount() + 1);
    QCOMPARE(positionChangedSpy.count(), 7);
    QCOMPARE(currentLineCountChangedSpy.count(), 5);
    QCOMPARE(editorService.currentLineCount(), editorService.maxLineCount());
    QCOMPARE(durationChangedSpy.count(), 4);
    QCOMPARE(editorService.duration(), "00:01:02.437");
}

void EditorServiceTest::test_setCurrentPattern_shouldCreatePattern()
{
    EditorService editorService;

    editorService.setCurrentLineCount(32); // New pattern should take the previous line count

    QSignalSpy positionChangedSpy { &editorService, &EditorService::positionChanged };
    QSignalSpy patternCreatedSpy { &editorService, &EditorService::patternCreated };

    editorService.setCurrentPattern(0);

    QCOMPARE(editorService.currentPattern(), 0);
    QCOMPARE(editorService.patternCount(), 1);
    QCOMPARE(positionChangedSpy.count(), 0);
    QCOMPARE(patternCreatedSpy.count(), 0);
    QCOMPARE(editorService.duration(), "00:00:02.000");

    editorService.setCurrentPattern(1);

    QCOMPARE(editorService.currentPattern(), 1);
    QCOMPARE(editorService.patternCount(), 2);
    QCOMPARE(editorService.lineCount(1), 32);
    QCOMPARE(positionChangedSpy.count(), 1);
    QCOMPARE(patternCreatedSpy.count(), 1);
    QCOMPARE(editorService.duration(), "00:00:02.000"); // Play order has not changed

    editorService.setCurrentPattern(0);

    QCOMPARE(editorService.currentPattern(), 0);
    QCOMPARE(editorService.patternCount(), 2);
    QCOMPARE(positionChangedSpy.count(), 2);
    QCOMPARE(patternCreatedSpy.count(), 1);
    QCOMPARE(editorService.duration(), "00:00:02.000");

    editorService.setCurrentPattern(1);
    editorService.setCurrentLineCount(128); // New pattern should take the previous line count

    editorService.setCurrentPattern(2);

    QCOMPARE(editorService.currentPattern(), 2);
    QCOMPARE(editorService.patternCount(), 3);
    QCOMPARE(editorService.lineCount(1), 128);
    QCOMPARE(positionChangedSpy.count(), 5);
    QCOMPARE(patternCreatedSpy.count(), 2);
    QCOMPARE(editorService.duration(), "00:00:02.000");
}

void EditorServiceTest::test_setCurrentPattern_addColumnFirst_shouldCreatePattern()
{
    EditorService editorService;

    QSignalSpy positionChangedSpy { &editorService, &EditorService::positionChanged };
    QSignalSpy patternCreatedSpy { &editorService, &EditorService::patternCreated };

    editorService.setCurrentPattern(0);
    editorService.requestNewColumn(0);
    const auto newColumnCount = editorService.columnCount(0);
    editorService.setCurrentPattern(1);

    QCOMPARE(editorService.currentPattern(), 1);
    QCOMPARE(editorService.columnCount(0), newColumnCount);
    QCOMPARE(editorService.patternCount(), 2);
    QCOMPARE(positionChangedSpy.count(), 2);
    QCOMPARE(patternCreatedSpy.count(), 1);
}

void EditorServiceTest::test_setPatternAtSongPosition_shouldCreatePattern()
{
    EditorService editorService;

    QSignalSpy currentPatternChangedSpy { &editorService, &EditorService::currentPatternChanged };
    QSignalSpy durationChangedSpy { &editorService, &EditorService::durationChanged };
    QSignalSpy patternAtCurrentSongPositionChangedSpy { &editorService, &EditorService::patternAtCurrentPlayOrderSongPositionChanged };
    QSignalSpy patternCreatedChangedSpy { &editorService, &EditorService::patternCreated };
    QSignalSpy positionChangedSpy { &editorService, &EditorService::positionChanged };
    QSignalSpy songPositionChangedSpy { &editorService, &EditorService::playOrderSongPositionChanged };

    editorService.setPatternAtPlayOrderSongPosition(0, 0);

    QCOMPARE(editorService.currentPattern(), 0);
    QCOMPARE(editorService.patternAtCurrentPlayOrderSongPosition(), 0);
    QCOMPARE(editorService.patternCount(), 1);

    QCOMPARE(currentPatternChangedSpy.count(), 0);
    QCOMPARE(patternAtCurrentSongPositionChangedSpy.count(), 0);
    QCOMPARE(patternCreatedChangedSpy.count(), 0);
    QCOMPARE(positionChangedSpy.count(), 0);
    QCOMPARE(songPositionChangedSpy.count(), 0);
    QCOMPARE(durationChangedSpy.count(), 0);
    QCOMPARE(editorService.duration(), "00:00:04.000");

    editorService.setPatternAtPlayOrderSongPosition(0, 1);

    QCOMPARE(editorService.currentPattern(), 1);
    QCOMPARE(editorService.patternAtCurrentPlayOrderSongPosition(), 1);
    QCOMPARE(editorService.patternCount(), 2);

    QCOMPARE(currentPatternChangedSpy.count(), 1);
    QCOMPARE(patternAtCurrentSongPositionChangedSpy.count(), 1);
    QCOMPARE(patternCreatedChangedSpy.count(), 1);
    QCOMPARE(positionChangedSpy.count(), 1);
    QCOMPARE(songPositionChangedSpy.count(), 0);
    QCOMPARE(durationChangedSpy.count(), 0);
    QCOMPARE(editorService.duration(), "00:00:04.000");

    editorService.setPatternAtPlayOrderSongPosition(1, 1);

    QCOMPARE(editorService.currentPattern(), 1);
    QCOMPARE(editorService.patternAtCurrentPlayOrderSongPosition(), 1);
    QCOMPARE(editorService.patternCount(), 2);

    QCOMPARE(currentPatternChangedSpy.count(), 1);
    QCOMPARE(patternAtCurrentSongPositionChangedSpy.count(), 1);
    QCOMPARE(patternCreatedChangedSpy.count(), 1);
    QCOMPARE(positionChangedSpy.count(), 1);
    QCOMPARE(songPositionChangedSpy.count(), 0);
    QCOMPARE(durationChangedSpy.count(), 1);
    QCOMPARE(editorService.duration(), "00:00:08.000");
}

void EditorServiceTest::test_setSongPosition_shouldChangePattern()
{
    EditorService editorService;

    QSignalSpy currentPatternChangedSpy { &editorService, &EditorService::currentPatternChanged };
    QSignalSpy patternAtCurrentSongPositionChangedSpy { &editorService, &EditorService::patternAtCurrentPlayOrderSongPositionChanged };
    QSignalSpy patternCreatedChangedSpy { &editorService, &EditorService::patternCreated };
    QSignalSpy positionChangedSpy { &editorService, &EditorService::positionChanged };
    QSignalSpy songPositionChangedSpy { &editorService, &EditorService::playOrderSongPositionChanged };

    editorService.setPatternAtPlayOrderSongPosition(1, 1);
    editorService.setPlayOrderSongPosition(1);

    QCOMPARE(editorService.currentPattern(), 1);
    QCOMPARE(editorService.position().pattern, 1);
    QCOMPARE(editorService.patternAtPlayOrderSongPosition(0), 0);
    QCOMPARE(editorService.patternAtPlayOrderSongPosition(1), 1);
    QCOMPARE(editorService.patternAtCurrentPlayOrderSongPosition(), 1);
    QCOMPARE(editorService.patternCount(), 2);

    QCOMPARE(currentPatternChangedSpy.count(), 1);
    QCOMPARE(patternAtCurrentSongPositionChangedSpy.count(), 1);
    QCOMPARE(patternCreatedChangedSpy.count(), 1);
    QCOMPARE(positionChangedSpy.count(), 1);
    QCOMPARE(songPositionChangedSpy.count(), 1);

    editorService.setPlayOrderSongPosition(0);

    QCOMPARE(editorService.currentPattern(), 1);
    QCOMPARE(editorService.position().pattern, 1);
    QCOMPARE(editorService.patternAtPlayOrderSongPosition(0), 0);
    QCOMPARE(editorService.patternAtPlayOrderSongPosition(1), 1);
    QCOMPARE(editorService.patternAtCurrentPlayOrderSongPosition(), 0);
    QCOMPARE(editorService.patternCount(), 2);

    QCOMPARE(currentPatternChangedSpy.count(), 1);
    QCOMPARE(patternAtCurrentSongPositionChangedSpy.count(), 2);
    QCOMPARE(patternCreatedChangedSpy.count(), 1);
    QCOMPARE(positionChangedSpy.count(), 1);
    QCOMPARE(songPositionChangedSpy.count(), 2);
}

void EditorServiceTest::test_setPatternName_shouldChangePatternName()
{
    EditorService editorService;

    editorService.setPatternName(0, "Foo");

    QVERIFY(editorService.isModified());
    QCOMPARE(editorService.patternName(0), "Foo");
    QCOMPARE(editorService.currentPatternName(), "Foo");

    editorService.setCurrentPatternName("Bar");

    QVERIFY(editorService.isModified());
    QCOMPARE(editorService.patternName(0), "Bar");
    QCOMPARE(editorService.currentPatternName(), "Bar");
}

void EditorServiceTest::test_setTrackName_shouldChangeTrackName()
{
    EditorService editorService;

    editorService.setTrackName(0, "Foo");
    editorService.setTrackName(1, "Bar");

    QVERIFY(editorService.isModified());
    QCOMPARE(editorService.trackName(0), "Foo");
    QCOMPARE(editorService.trackName(1), "Bar");
}

void EditorServiceTest::test_toXmlFromXml_playOrder()
{
    EditorService editorServiceOut;
    editorServiceOut.setPatternAtPlayOrderSongPosition(1, 11);
    editorServiceOut.setPatternAtPlayOrderSongPosition(2, 22);
    editorServiceOut.setPatternAtPlayOrderSongPosition(3, 33);

    const auto xml = editorServiceOut.toXml();

    EditorService editorServiceIn;
    editorServiceIn.fromXml(xml);

    QCOMPARE(editorServiceIn.patternAtPlayOrderSongPosition(0), 0);
    QCOMPARE(editorServiceIn.patternAtPlayOrderSongPosition(1), 11);
    QCOMPARE(editorServiceIn.patternAtPlayOrderSongPosition(2), 22);
    QCOMPARE(editorServiceIn.patternAtPlayOrderSongPosition(3), 33);
}

void EditorServiceTest::test_toXmlFromXml_songProperties()
{
    EditorService editorServiceOut;
    editorServiceOut.setBeatsPerMinute(666);
    editorServiceOut.setLinesPerBeat(42);
    editorServiceOut.setPatternName(0, "patternName");
    editorServiceOut.setTrackName(0, "trackName0");
    editorServiceOut.setTrackName(1, "trackName1");

    const auto xml = editorServiceOut.toXml();

    EditorService editorServiceIn;
    QSignalSpy songChangedSpy { &editorServiceIn, &EditorService::songChanged };
    QSignalSpy positionChangedSpy { &editorServiceIn, &EditorService::positionChanged };
    QSignalSpy beatsPerMinuteChangedSpy { &editorServiceIn, &EditorService::beatsPerMinuteChanged };
    QSignalSpy linesPerBeatChangedSpy { &editorServiceIn, &EditorService::linesPerBeatChanged };
    editorServiceIn.fromXml(xml);

    QCOMPARE(songChangedSpy.count(), 1);
    QCOMPARE(positionChangedSpy.count(), 1);
    QCOMPARE(beatsPerMinuteChangedSpy.count(), 1);
    QCOMPARE(linesPerBeatChangedSpy.count(), 1);
    QCOMPARE(editorServiceIn.beatsPerMinute(), editorServiceOut.beatsPerMinute());
    QCOMPARE(editorServiceIn.linesPerBeat(), editorServiceOut.linesPerBeat());
    QCOMPARE(editorServiceIn.patternName(0), editorServiceOut.patternName(0));
    QCOMPARE(editorServiceIn.trackName(0), editorServiceOut.trackName(0));
    QCOMPARE(editorServiceIn.trackName(1), editorServiceOut.trackName(1));
}

void EditorServiceTest::test_toXmlFromXml_noteData_noteOn()
{
    EditorService editorServiceOut;

    editorServiceOut.requestPosition(0, 0, 0, 0, 0);
    editorServiceOut.requestNoteOnAtCurrentPosition(1, 3, 64);

    editorServiceOut.requestPosition(0, 0, 0, 2, 0);
    editorServiceOut.requestNoteOnAtCurrentPosition(3, 3, 80);

    editorServiceOut.requestPosition(0, 1, 0, 0, 0);
    editorServiceOut.requestNoteOnAtCurrentPosition(2, 4, 100);

    editorServiceOut.requestPosition(0, 1, 0, 2, 0);
    editorServiceOut.requestNoteOnAtCurrentPosition(3, 4, 127);

    const auto xml = editorServiceOut.toXml();

    EditorService editorServiceIn;
    editorServiceIn.fromXml(xml);

    auto noteData = editorServiceIn.song()->noteDataAtPosition({ 0, 0, 0, 0, 0 });
    QCOMPARE(noteData->track(), 0);
    QCOMPARE(noteData->column(), 0);
    QCOMPARE(editorServiceIn.displayNoteAtPosition(0, 0, 0, 0), "C-3");
    QCOMPARE(editorServiceIn.displayVelocityAtPosition(0, 0, 0, 0), "064");

    noteData = editorServiceIn.song()->noteDataAtPosition({ 0, 0, 0, 2, 0 });
    QCOMPARE(noteData->track(), 0);
    QCOMPARE(noteData->column(), 0);
    QCOMPARE(editorServiceIn.displayNoteAtPosition(0, 0, 0, 2), "D-3");
    QCOMPARE(editorServiceIn.displayVelocityAtPosition(0, 0, 0, 2), "080");

    noteData = editorServiceIn.song()->noteDataAtPosition({ 0, 1, 0, 0, 0 });
    QCOMPARE(noteData->track(), 1);
    QCOMPARE(noteData->column(), 0);
    QCOMPARE(editorServiceIn.displayNoteAtPosition(0, 1, 0, 0), "C#4");
    QCOMPARE(editorServiceIn.displayVelocityAtPosition(0, 1, 0, 0), "100");

    noteData = editorServiceIn.song()->noteDataAtPosition({ 0, 1, 0, 2, 0 });
    QCOMPARE(noteData->track(), 1);
    QCOMPARE(noteData->column(), 0);
    QCOMPARE(editorServiceIn.displayNoteAtPosition(0, 1, 0, 2), "D-4");
    QCOMPARE(editorServiceIn.displayVelocityAtPosition(0, 1, 0, 2), "127");
}

void EditorServiceTest::test_toXmlFromXml_noteData_noteOff()
{
    EditorService editorServiceOut;

    editorServiceOut.requestPosition(0, 0, 0, 0, 0);
    editorServiceOut.requestNoteOffAtCurrentPosition();

    editorServiceOut.requestPosition(0, 0, 0, 2, 0);
    editorServiceOut.requestNoteOnAtCurrentPosition(3, 3, 80);

    editorServiceOut.requestPosition(0, 0, 0, 4, 0);
    editorServiceOut.requestNoteOffAtCurrentPosition();

    const auto xml = editorServiceOut.toXml();

    EditorService editorServiceIn;
    editorServiceIn.fromXml(xml);

    auto noteData = editorServiceIn.song()->noteDataAtPosition({ 0, 0, 0, 0, 0 });
    QCOMPARE(noteData->track(), 0);
    QCOMPARE(noteData->column(), 0);
    QCOMPARE(editorServiceIn.displayNoteAtPosition(0, 0, 0, 0), "OFF");
    QCOMPARE(editorServiceIn.displayVelocityAtPosition(0, 0, 0, 0), "---");

    noteData = editorServiceIn.song()->noteDataAtPosition({ 0, 0, 0, 2, 0 });
    QCOMPARE(noteData->track(), 0);
    QCOMPARE(noteData->column(), 0);
    QCOMPARE(editorServiceIn.displayNoteAtPosition(0, 0, 0, 2), "D-3");
    QCOMPARE(editorServiceIn.displayVelocityAtPosition(0, 0, 0, 2), "080");

    noteData = editorServiceIn.song()->noteDataAtPosition({ 0, 0, 0, 4, 0 });
    QCOMPARE(noteData->track(), 0);
    QCOMPARE(noteData->column(), 0);
    QCOMPARE(editorServiceIn.displayNoteAtPosition(0, 0, 0, 4), "OFF");
    QCOMPARE(editorServiceIn.displayVelocityAtPosition(0, 0, 0, 4), "---");
}

void EditorServiceTest::test_toXmlFromXml_instrument_shouldParseInstrument()
{
    EditorService editorServiceOut;

    // Set up the instrument with all possible properties
    const auto instrumentOut = std::make_shared<Instrument>("Test Port");
    instrumentOut->channel = 10; // Example channel
    instrumentOut->patch = 42; // Optional patch
    instrumentOut->bank = {
        static_cast<uint8_t>(21), // Bank LSB
        static_cast<uint8_t>(34), // Bank MSB
        true // Byte order swapped
    };

    editorServiceOut.setInstrument(0, instrumentOut);

    // Serialize to XML
    const auto xml = editorServiceOut.toXml();

    // Deserialize from XML
    EditorService editorServiceIn;
    editorServiceIn.fromXml(xml);

    // Retrieve the instrument
    const auto instrumentIn = editorServiceIn.instrument(0);

    // Validate the instrument
    QVERIFY(instrumentIn);
    QCOMPARE(instrumentIn->portName, instrumentOut->portName);
    QCOMPARE(instrumentIn->channel, instrumentOut->channel);

    // Validate optional properties
    QCOMPARE(instrumentIn->patch.has_value(), instrumentOut->patch.has_value());
    if (instrumentIn->patch && instrumentOut->patch) {
        QCOMPARE(*instrumentIn->patch, *instrumentOut->patch);
    }

    QCOMPARE(instrumentIn->bank.has_value(), instrumentOut->bank.has_value());
    if (instrumentIn->bank && instrumentOut->bank) {
        QCOMPARE(instrumentIn->bank->lsb, instrumentOut->bank->lsb);
        QCOMPARE(instrumentIn->bank->msb, instrumentOut->bank->msb);
        QCOMPARE(instrumentIn->bank->byteOrderSwapped, instrumentOut->bank->byteOrderSwapped);
    }
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::EditorServiceTest)
