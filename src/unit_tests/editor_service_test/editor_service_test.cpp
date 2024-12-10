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

void EditorServiceTest::testDefaultSong_shouldReturnCorrectProperties()
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

void EditorServiceTest::testDefaultSong_shouldNotHaveNoteData()
{
    EditorService editorService;

    for (uint8_t pattern = 0; pattern < editorService.patternCount(); pattern++) {
        for (uint8_t track = 0; track < editorService.trackCount(); track++) {
            for (uint8_t column = 0; column < editorService.columnCount(track); column++) {
                for (uint8_t line = 0; line < editorService.lineCount(pattern); line++) {
                    QCOMPARE(editorService.noteAtPosition(pattern, track, column, line), "---");
                }
            }
        }
    }
}

void EditorServiceTest::testRequestScroll_shouldChangePosition()
{
    EditorService editorService;

    QCOMPARE(editorService.position().line, 0);

    editorService.requestScroll(1);
    QCOMPARE(editorService.position().line, 1);

    editorService.requestScroll(0);
    QCOMPARE(editorService.position().line, 1);

    editorService.requestScroll(-1);
    QCOMPARE(editorService.position().line, 0);

    editorService.requestScroll(-10);
    QCOMPARE(editorService.position().line, editorService.lineCount(editorService.currentPatternId()) - 10);

    editorService.requestScroll(10);
    QCOMPARE(editorService.position().line, 0);

    editorService.requestScroll(static_cast<int>(editorService.lineCount(editorService.currentPatternId()) + 10));
    QCOMPARE(editorService.position().line, 10);
}

void EditorServiceTest::testRequestTrackFocus_shouldChangePosition()
{
    EditorService editorService;
    QSignalSpy positionChangedSpy { &editorService, &EditorService::positionChanged };

    editorService.requestTrackFocus(0);
    QCOMPARE(positionChangedSpy.count(), 1);

    editorService.requestTrackFocus(editorService.trackCount() - 1);
    QCOMPARE(positionChangedSpy.count(), 2);
    QCOMPARE(editorService.position().track, editorService.trackCount() - 1);
}

void EditorServiceTest::testRequestTrackFocus_shouldNotChangePosition()
{
    EditorService editorService;
    QSignalSpy positionChangedSpy { &editorService, &EditorService::positionChanged };

    editorService.requestTrackFocus(editorService.trackCount());

    QCOMPARE(positionChangedSpy.count(), 0);
    QCOMPARE(editorService.position().track, 0);
}

void EditorServiceTest::testSetTrackName_shouldChangeTrackName()
{
    EditorService editorService;

    editorService.setTrackName(0, "Foo");
    editorService.setTrackName(1, "Bar");

    QCOMPARE(editorService.trackName(0), "Foo");
    QCOMPARE(editorService.trackName(1), "Bar");
}

} // namespace cacophony

QTEST_GUILESS_MAIN(cacophony::EditorServiceTest)
