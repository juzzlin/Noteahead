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
#include "../../domain/note_data.hpp"
#include "../../domain/song.hpp"

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

    QVERIFY(!editorService.canBeSaved());
    QVERIFY(!editorService.isModified());
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
    QSignalSpy positionChangedSpy { &editorService, &EditorService::positionChanged };

    // Min size
    editorService.setCurrentLineCount(0);
    QCOMPARE(currentLineCountChangedSpy.count(), 1);
    QCOMPARE(positionChangedSpy.count(), 1);
    QCOMPARE(editorService.currentLineCount(), editorService.minLineCount());
    QVERIFY(editorService.isModified());

    // No change
    editorService.setCurrentLineCount(editorService.minLineCount());
    QCOMPARE(currentLineCountChangedSpy.count(), 1);
    QCOMPARE(positionChangedSpy.count(), 1);
    QCOMPARE(editorService.currentLineCount(), editorService.minLineCount());

    // Expand
    editorService.setCurrentLineCount(256);
    QCOMPARE(currentLineCountChangedSpy.count(), 2);
    QCOMPARE(positionChangedSpy.count(), 2);
    QCOMPARE(editorService.currentLineCount(), 256);

    // Shrink with cursor line over the bounds
    editorService.requestPosition(0, 0, 0, 200, 0);
    editorService.setCurrentLineCount(64);
    QCOMPARE(positionChangedSpy.count(), 5);
    QCOMPARE(currentLineCountChangedSpy.count(), 3);
    QCOMPARE(editorService.currentLineCount(), 64);
    QCOMPARE(editorService.position().line, 63);

    // Max size
    editorService.setCurrentLineCount(editorService.maxLineCount());
    QCOMPARE(positionChangedSpy.count(), 6);
    QCOMPARE(currentLineCountChangedSpy.count(), 4);
    QCOMPARE(editorService.currentLineCount(), editorService.maxLineCount());

    // Beyond max size
    editorService.setCurrentLineCount(editorService.maxLineCount() + 1);
    QCOMPARE(positionChangedSpy.count(), 7);
    QCOMPARE(currentLineCountChangedSpy.count(), 5);
    QCOMPARE(editorService.currentLineCount(), editorService.maxLineCount());
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

void EditorServiceTest::test_toXmlFromXml_songProperties()
{
    EditorService editorServiceOut;
    editorServiceOut.setBeatsPerMinute(666);
    editorServiceOut.setLinesPerBeat(42);

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
    QCOMPARE(editorServiceIn.beatsPerMinute(), 666);
    QCOMPARE(editorServiceIn.linesPerBeat(), 42);
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

} // namespace cacophony

QTEST_GUILESS_MAIN(cacophony::EditorServiceTest)
