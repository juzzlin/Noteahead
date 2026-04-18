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

#include <algorithm> // Required for std::sort
#include <vector>    // Required for std::vector

#include "../../application/service/automation_service.hpp"
#include "../../application/service/property_service.hpp"
#include "../../application/service/editor_service.hpp"
#include "../../application/service/mixer_service.hpp"
#include "../../infra/settings.hpp"
#include "../../application/service/selection_service.hpp"
#include "../../application/service/settings_service.hpp"
#include "../../application/service/side_chain_service.hpp"
#include "../../domain/column_settings.hpp"
#include "../../domain/instrument.hpp"
#include "../../domain/note_data.hpp"
#include "../../domain/pattern.hpp"
#include "../../domain/side_chain_settings.hpp"
#include "../../domain/song.hpp"
#include "../../domain/track.hpp"

#include <QSignalSpy>
#include <QTest>

namespace noteahead {

void EditorServiceTest::test_initialize_shouldInitializeCorrectly()
{
    EditorService editorService { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };

    // Track emitted signals
    QSignalSpy spyAboutToInitialize(&editorService, &EditorService::aboutToInitialize);
    QSignalSpy spyInitialized(&editorService, &EditorService::initialized);
    QSignalSpy spyStatusText(&editorService, &EditorService::statusTextRequested);
    QSignalSpy spySongChanged(&editorService, &EditorService::songChanged);
    QSignalSpy spyBpmChanged(&editorService, &EditorService::beatsPerMinuteChanged);
    QSignalSpy spyLpbChanged(&editorService, &EditorService::linesPerBeatChanged);
    QSignalSpy spyPatternChanged(&editorService, &EditorService::currentPatternChanged);
    QSignalSpy spyPatternAtCurrentPositionChanged(&editorService, &EditorService::patternAtCurrentSongPositionChanged);

    editorService.setSongPosition(1); // Alter song length and position

    QSignalSpy spySongLengthChanged(&editorService, &EditorService::songLengthChanged);
    QSignalSpy spySongPositionChanged(&editorService, &EditorService::songPositionChanged);

    QCOMPARE(spyPatternAtCurrentPositionChanged.count(), 1);

    editorService.initialize();

    // Assert: Check emitted signals
    QCOMPARE(spyAboutToInitialize.count(), 1);
    QCOMPARE(spyInitialized.count(), 1);
    QCOMPARE(spySongChanged.count(), 1);
    QCOMPARE(spyBpmChanged.count(), 1);
    QCOMPARE(spyLpbChanged.count(), 1);
    QCOMPARE(spyPatternChanged.count(), 1);
    QCOMPARE(spySongLengthChanged.count(), 1);
    QCOMPARE(spySongPositionChanged.count(), 1);
    QCOMPARE(spyPatternAtCurrentPositionChanged.count(), 2);

    // Verify status message
    QCOMPARE(spyStatusText.count(), 1);
    QCOMPARE(spyStatusText.takeFirst().at(0).toString(), QStringLiteral("An empty song initialized"));

    // Verify song initialization
    QVERIFY(editorService.song());
    QCOMPARE(editorService.songPosition(), 0);
}

void EditorServiceTest::test_defaultSong_shouldReturnCorrectProperties()
{
    EditorService editorService { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };

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
    EditorService editorService { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };

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

void EditorServiceTest::test_insertPattern_shouldInsertPattern()
{
    EditorService editorService { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };
    editorService.setPatternAtSongPosition(0, 1);
    editorService.insertPatternToPlayOrder();

    editorService.setSongPosition(0);
    QCOMPARE(editorService.patternAtCurrentSongPosition(), 0);
    editorService.setSongPosition(1);
    QCOMPARE(editorService.patternAtCurrentSongPosition(), 1);
}

void EditorServiceTest::test_removePattern_shouldRemovePattern()
{
    EditorService editorService { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };
    editorService.setPatternAtSongPosition(0, 1);
    editorService.insertPatternToPlayOrder();

    editorService.setSongPosition(0);
    editorService.removePatternFromPlayOrder();
    QCOMPARE(editorService.patternAtCurrentSongPosition(), 1);
}

void EditorServiceTest::test_removePatternFromMiddle_shouldNotRemoveLastPattern()
{
    EditorService editorService { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };

    // Initial length is 1.
    // Add two more patterns to have 3 patterns in total.
    editorService.setSongPosition(0);
    editorService.insertPatternToPlayOrder(); // Now 2
    editorService.insertPatternToPlayOrder(); // Now 3

    QCOMPARE(editorService.songLength(), 3);

    // Set some distinct patterns to verify they are still there
    editorService.setPatternAtSongPosition(0, 0);
    editorService.setPatternAtSongPosition(1, 1);
    editorService.setPatternAtSongPosition(2, 2);

    // Remove pattern at position 1 (the middle one)
    editorService.setSongPosition(1);
    editorService.removePatternFromPlayOrder();

    // Expect length to be 2 (original 3 - 1)
    QCOMPARE(editorService.songLength(), 2);

    // Verify remaining patterns: position 0 should be 0, position 1 should be 2
    QCOMPARE(editorService.patternAtSongPosition(0), 0);
    QCOMPARE(editorService.patternAtSongPosition(1), 2);
}

void EditorServiceTest::test_columnCutPaste_equalSizes_shouldCopyColumn()
{
    EditorService editorService { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };
    QSignalSpy noteDataChangedSpy { &editorService, &EditorService::noteDataAtPositionChanged };
    const Position sourcePosition = { 0, 1, 0, 0, 0 };
    QVERIFY(editorService.requestPosition(sourcePosition));
    QVERIFY(editorService.requestNoteOnAtCurrentPosition(1, 3, 64));
    QCOMPARE(noteDataChangedSpy.count(), 1);

    editorService.setIsModified(false);
    editorService.requestColumnCut();
    QVERIFY(editorService.isModified());
    QCOMPARE(noteDataChangedSpy.count(), 65);
    editorService.setCurrentPattern(1);
    const Position targetPosition = { 1, 2, 0, 0, 0 };
    QVERIFY(editorService.requestPosition(targetPosition));
    editorService.requestColumnPaste();

    QCOMPARE(noteDataChangedSpy.count(), 129);
    QCOMPARE(editorService.displayNoteAtPosition(sourcePosition), editorService.noDataString());
    QCOMPARE(editorService.displayVelocityAtPosition(sourcePosition), editorService.noDataString());
    QCOMPARE(editorService.displayNoteAtPosition(targetPosition), "C-3");
    QCOMPARE(editorService.displayVelocityAtPosition(targetPosition), "064");
    QCOMPARE(editorService.song()->noteDataAtPosition(targetPosition)->column(), targetPosition.column);
    QCOMPARE(editorService.song()->noteDataAtPosition(targetPosition)->track(), targetPosition.track);
}

void EditorServiceTest::test_columnCutPaste_shorterTarget_shouldCopyColumn()
{
    EditorService editorService { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };
    QSignalSpy noteDataChangedSpy { &editorService, &EditorService::noteDataAtPositionChanged };
    const Position sourcePosition = { 0, 1, 0, 0, 0 };
    QVERIFY(editorService.requestPosition(sourcePosition));
    QVERIFY(editorService.requestNoteOnAtCurrentPosition(1, 3, 64));
    QCOMPARE(noteDataChangedSpy.count(), 1);

    editorService.setIsModified(false);
    editorService.requestColumnCut();
    QVERIFY(editorService.isModified());
    QCOMPARE(noteDataChangedSpy.count(), 65);
    editorService.setCurrentPattern(1);
    editorService.setCurrentLineCount(32);
    const Position targetPosition = { 1, 2, 0, 0, 0 };
    QVERIFY(editorService.requestPosition(targetPosition));
    editorService.requestColumnPaste();

    QCOMPARE(noteDataChangedSpy.count(), 97);
    QCOMPARE(editorService.displayNoteAtPosition(sourcePosition), editorService.noDataString());
    QCOMPARE(editorService.displayVelocityAtPosition(sourcePosition), editorService.noDataString());
    QCOMPARE(editorService.displayNoteAtPosition(targetPosition), "C-3");
    QCOMPARE(editorService.displayVelocityAtPosition(targetPosition), "064");
    QCOMPARE(editorService.song()->noteDataAtPosition(targetPosition)->column(), targetPosition.column);
    QCOMPARE(editorService.song()->noteDataAtPosition(targetPosition)->track(), targetPosition.track);
}

void EditorServiceTest::test_columnCopyPaste_equalSizes_shouldCopyColumn()
{
    EditorService editorService { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };
    QSignalSpy noteDataChangedSpy { &editorService, &EditorService::noteDataAtPositionChanged };
    const Position sourcePosition = { 0, 1, 0, 0, 0 };
    QVERIFY(editorService.requestPosition(sourcePosition));
    QVERIFY(editorService.requestNoteOnAtCurrentPosition(1, 3, 64));
    QCOMPARE(noteDataChangedSpy.count(), 1);

    editorService.setIsModified(false);
    editorService.requestColumnCopy();
    QVERIFY(!editorService.isModified());
    editorService.setCurrentPattern(1);
    const Position targetPosition = { 1, 2, 0, 0, 0 };
    QVERIFY(editorService.requestPosition(targetPosition));
    editorService.requestColumnPaste();

    QCOMPARE(noteDataChangedSpy.count(), 65);
    QCOMPARE(editorService.displayNoteAtPosition(sourcePosition), "C-3");
    QCOMPARE(editorService.displayVelocityAtPosition(sourcePosition), "064");
    QCOMPARE(editorService.displayNoteAtPosition(targetPosition), "C-3");
    QCOMPARE(editorService.displayVelocityAtPosition(targetPosition), "064");
    QCOMPARE(editorService.song()->noteDataAtPosition(targetPosition)->column(), targetPosition.column);
    QCOMPARE(editorService.song()->noteDataAtPosition(targetPosition)->track(), targetPosition.track);
}

void EditorServiceTest::test_columnCopyPaste_shorterTarget_shouldCopyColumn()
{
    EditorService editorService { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };
    QSignalSpy noteDataChangedSpy { &editorService, &EditorService::noteDataAtPositionChanged };
    const Position sourcePosition = { 0, 1, 0, 0, 0 };
    QVERIFY(editorService.requestPosition(sourcePosition));
    QVERIFY(editorService.requestNoteOnAtCurrentPosition(1, 3, 64));
    QCOMPARE(noteDataChangedSpy.count(), 1);

    editorService.setIsModified(false);
    editorService.requestColumnCopy();
    QVERIFY(!editorService.isModified());
    editorService.setCurrentPattern(1);
    editorService.setCurrentLineCount(32);
    const Position targetPosition = { 1, 2, 0, 0, 0 };
    QVERIFY(editorService.requestPosition(targetPosition));
    editorService.requestColumnPaste();

    QCOMPARE(noteDataChangedSpy.count(), 33);
    QCOMPARE(editorService.displayNoteAtPosition(sourcePosition), "C-3");
    QCOMPARE(editorService.displayVelocityAtPosition(sourcePosition), "064");
    QCOMPARE(editorService.displayNoteAtPosition(targetPosition), "C-3");
    QCOMPARE(editorService.displayVelocityAtPosition(targetPosition), "064");
    QCOMPARE(editorService.song()->noteDataAtPosition(targetPosition)->column(), targetPosition.column);
    QCOMPARE(editorService.song()->noteDataAtPosition(targetPosition)->track(), targetPosition.track);
}

void EditorServiceTest::test_trackCutPaste_equalSizes_shouldCopyTrack()
{
    EditorService editorService { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };
    QSignalSpy noteDataChangedSpy { &editorService, &EditorService::noteDataAtPositionChanged };
    const Position sourcePosition = { 0, 1, 0, 0, 0 };
    QVERIFY(editorService.requestPosition(sourcePosition));
    QVERIFY(editorService.requestNoteOnAtCurrentPosition(1, 3, 64));
    QCOMPARE(noteDataChangedSpy.count(), 1);

    editorService.setIsModified(false);
    editorService.requestTrackCut();
    QVERIFY(editorService.isModified());
    QCOMPARE(noteDataChangedSpy.count(), 65);
    editorService.setCurrentPattern(1);
    const Position targetPosition = { 1, 2, 0, 0, 0 };
    QVERIFY(editorService.requestPosition(targetPosition));
    editorService.requestTrackPaste();

    QCOMPARE(noteDataChangedSpy.count(), 129);
    QCOMPARE(editorService.displayNoteAtPosition(sourcePosition), editorService.noDataString());
    QCOMPARE(editorService.displayVelocityAtPosition(sourcePosition), editorService.noDataString());
    QCOMPARE(editorService.displayNoteAtPosition(targetPosition), "C-3");
    QCOMPARE(editorService.displayVelocityAtPosition(targetPosition), "064");
    QCOMPARE(editorService.song()->noteDataAtPosition(targetPosition)->column(), targetPosition.column);
    QCOMPARE(editorService.song()->noteDataAtPosition(targetPosition)->track(), targetPosition.track);
}

void EditorServiceTest::test_trackCutPaste_shorterTarget_shouldCopyTrack()
{
    EditorService editorService { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };
    QSignalSpy noteDataChangedSpy { &editorService, &EditorService::noteDataAtPositionChanged };
    const Position sourcePosition = { 0, 1, 0, 0, 0 };
    QVERIFY(editorService.requestPosition(sourcePosition));
    QVERIFY(editorService.requestNoteOnAtCurrentPosition(1, 3, 64));
    QCOMPARE(noteDataChangedSpy.count(), 1);

    editorService.setIsModified(false);
    editorService.requestTrackCut();
    QVERIFY(editorService.isModified());
    QCOMPARE(noteDataChangedSpy.count(), 65);
    editorService.setCurrentPattern(1);
    editorService.setCurrentLineCount(32);
    const Position targetPosition = { 1, 2, 0, 0, 0 };
    QVERIFY(editorService.requestPosition(targetPosition));
    editorService.requestTrackPaste();

    QCOMPARE(noteDataChangedSpy.count(), 97);
    QCOMPARE(editorService.displayNoteAtPosition(sourcePosition), editorService.noDataString());
    QCOMPARE(editorService.displayVelocityAtPosition(sourcePosition), editorService.noDataString());
    QCOMPARE(editorService.displayNoteAtPosition(targetPosition), "C-3");
    QCOMPARE(editorService.displayVelocityAtPosition(targetPosition), "064");
    QCOMPARE(editorService.song()->noteDataAtPosition(targetPosition)->column(), targetPosition.column);
    QCOMPARE(editorService.song()->noteDataAtPosition(targetPosition)->track(), targetPosition.track);
}

void EditorServiceTest::test_trackCopyPaste_equalSizes_shouldCopyTrack()
{
    EditorService editorService { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };
    QSignalSpy noteDataChangedSpy { &editorService, &EditorService::noteDataAtPositionChanged };
    const Position sourcePosition = { 0, 1, 0, 0, 0 };
    QVERIFY(editorService.requestPosition(sourcePosition));
    QVERIFY(editorService.requestNoteOnAtCurrentPosition(1, 3, 64));
    QCOMPARE(noteDataChangedSpy.count(), 1);

    editorService.setIsModified(false);
    editorService.requestTrackCopy();
    QVERIFY(!editorService.isModified());
    editorService.setCurrentPattern(1);
    const Position targetPosition = { 1, 2, 0, 0, 0 };
    QVERIFY(editorService.requestPosition(targetPosition));
    editorService.requestTrackPaste();

    QCOMPARE(noteDataChangedSpy.count(), 65);
    QCOMPARE(editorService.displayNoteAtPosition(sourcePosition), "C-3");
    QCOMPARE(editorService.displayVelocityAtPosition(sourcePosition), "064");
    QCOMPARE(editorService.displayNoteAtPosition(targetPosition), "C-3");
    QCOMPARE(editorService.displayVelocityAtPosition(targetPosition), "064");
    QCOMPARE(editorService.song()->noteDataAtPosition(targetPosition)->column(), targetPosition.column);
    QCOMPARE(editorService.song()->noteDataAtPosition(targetPosition)->track(), targetPosition.track);
}

void EditorServiceTest::test_trackCopyPaste_shorterTarget_shouldCopyTrack()
{
    EditorService editorService { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };
    QSignalSpy noteDataChangedSpy { &editorService, &EditorService::noteDataAtPositionChanged };
    const Position sourcePosition = { 0, 1, 0, 0, 0 };
    QVERIFY(editorService.requestPosition(sourcePosition));
    QVERIFY(editorService.requestNoteOnAtCurrentPosition(1, 3, 64));
    QCOMPARE(noteDataChangedSpy.count(), 1);

    editorService.setIsModified(false);
    editorService.requestTrackCopy();
    QVERIFY(!editorService.isModified());
    editorService.setCurrentPattern(1);
    editorService.setCurrentLineCount(32);
    const Position targetPosition = { 1, 2, 0, 0, 0 };
    QVERIFY(editorService.requestPosition(targetPosition));
    editorService.requestTrackPaste();

    QCOMPARE(noteDataChangedSpy.count(), 33);
    QCOMPARE(editorService.displayNoteAtPosition(sourcePosition), "C-3");
    QCOMPARE(editorService.displayVelocityAtPosition(sourcePosition), "064");
    QCOMPARE(editorService.displayNoteAtPosition(targetPosition), "C-3");
    QCOMPARE(editorService.displayVelocityAtPosition(targetPosition), "064");
    QCOMPARE(editorService.song()->noteDataAtPosition(targetPosition)->column(), targetPosition.column);
    QCOMPARE(editorService.song()->noteDataAtPosition(targetPosition)->track(), targetPosition.track);
}

void EditorServiceTest::test_trackCopyPaste_withAutomations()
{
    EditorService editorService { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };
    auto automationService = std::make_shared<AutomationService>(std::make_shared<PropertyService>());
    editorService.setAutomationService(automationService);

    const quint64 sourcePattern = 0;
    const quint64 sourceTrack = 1;
    const quint64 sourceColumn = 0;
    const quint64 sourceLine = 0;

    const Position sourcePosition = { sourcePattern, sourceTrack, sourceColumn, sourceLine, 0 };
    QVERIFY(editorService.requestPosition(sourcePosition));
    QVERIFY(editorService.requestNoteOnAtCurrentPosition(1, 3, 64));

    // Add MIDI CC automation
    automationService->addMidiCcAutomation(sourcePattern, sourceTrack, sourceColumn, 7, 0, 8, 0, 127, "Test CC", true, 8, 0);
    QCOMPARE(automationService->midiCcAutomationsByTrack(sourcePattern, sourceTrack).size(), 1);

    editorService.requestTrackCopy();

    const quint64 targetPattern = 1;
    const quint64 targetTrack = 2;
    editorService.setCurrentPattern(targetPattern);
    const Position targetPosition = { targetPattern, targetTrack, 0, 0, 0 };
    QVERIFY(editorService.requestPosition(targetPosition));

    editorService.requestTrackPaste();

    // Verify automation is pasted
    auto pastedAutomations = automationService->midiCcAutomationsByTrack(targetPattern, targetTrack);
    QCOMPARE(pastedAutomations.size(), 1);
    QCOMPARE(pastedAutomations.at(0).location().pattern(), targetPattern);
    QCOMPARE(pastedAutomations.at(0).location().track(), targetTrack);
    QCOMPARE(pastedAutomations.at(0).controller(), 7);
    QCOMPARE(pastedAutomations.at(0).comment(), "Test CC");

    // Test Undo
    editorService.undo();
    QCOMPARE(automationService->midiCcAutomationsByTrack(targetPattern, targetTrack).size(), 0);

    // Test Redo
    editorService.redo();
    QCOMPARE(automationService->midiCcAutomationsByTrack(targetPattern, targetTrack).size(), 1);
}

void EditorServiceTest::test_patternCutPaste_equalSizes_shouldCopyPattern()
{
    EditorService editorService { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };
    QSignalSpy noteDataChangedSpy { &editorService, &EditorService::noteDataAtPositionChanged };
    const Position sourcePosition = { 0, 0, 0, 0, 0 };
    QVERIFY(editorService.requestPosition(sourcePosition));
    QVERIFY(editorService.requestNoteOnAtCurrentPosition(1, 3, 64));
    QCOMPARE(noteDataChangedSpy.count(), 1);

    editorService.setIsModified(false);
    editorService.requestPatternCut();
    QVERIFY(editorService.isModified());
    QCOMPARE(noteDataChangedSpy.count(), 513);
    editorService.setCurrentPattern(1);
    editorService.requestPatternPaste();

    QCOMPARE(noteDataChangedSpy.count(), 1025);
    QCOMPARE(editorService.displayNoteAtPosition(sourcePosition), editorService.noDataString());
    QCOMPARE(editorService.displayVelocityAtPosition(sourcePosition), editorService.noDataString());
    const Position targetPosition = { 1, 0, 0, 0, 0 };
    QCOMPARE(editorService.displayNoteAtPosition(targetPosition), "C-3");
    QCOMPARE(editorService.displayVelocityAtPosition(targetPosition), "064");
}

void EditorServiceTest::test_patternCutPaste_shorterTarget_shouldCopyPattern()
{
    EditorService editorService { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };
    QSignalSpy noteDataChangedSpy { &editorService, &EditorService::noteDataAtPositionChanged };
    const Position sourcePosition = { 0, 0, 0, 0, 0 };
    QVERIFY(editorService.requestPosition(sourcePosition));
    QVERIFY(editorService.requestNoteOnAtCurrentPosition(1, 3, 64));
    QCOMPARE(noteDataChangedSpy.count(), 1);

    editorService.setIsModified(false);
    editorService.requestPatternCut();
    QVERIFY(editorService.isModified());
    QCOMPARE(noteDataChangedSpy.count(), 513);
    editorService.setCurrentPattern(1);
    editorService.setCurrentLineCount(32);
    editorService.requestPatternPaste();

    QCOMPARE(noteDataChangedSpy.count(), 769);
    QCOMPARE(editorService.displayNoteAtPosition(sourcePosition), editorService.noDataString());
    QCOMPARE(editorService.displayVelocityAtPosition(sourcePosition), editorService.noDataString());
    const Position targetPosition = { 1, 0, 0, 0, 0 };
    QCOMPARE(editorService.displayNoteAtPosition(targetPosition), "C-3");
    QCOMPARE(editorService.displayVelocityAtPosition(targetPosition), "064");
}

void EditorServiceTest::test_patternCopyPaste_equalSizes_shouldCopyPattern()
{
    EditorService editorService { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };
    QSignalSpy noteDataChangedSpy { &editorService, &EditorService::noteDataAtPositionChanged };
    const Position sourcePosition = { 0, 0, 0, 0, 0 };
    QVERIFY(editorService.requestPosition(sourcePosition));
    QVERIFY(editorService.requestNoteOnAtCurrentPosition(1, 3, 64));
    QCOMPARE(noteDataChangedSpy.count(), 1);

    editorService.setIsModified(false);
    editorService.requestPatternCopy();
    QVERIFY(!editorService.isModified());
    editorService.setCurrentPattern(1);
    editorService.requestPatternPaste();

    QCOMPARE(noteDataChangedSpy.count(), 513);
    QCOMPARE(editorService.displayNoteAtPosition(sourcePosition), "C-3");
    QCOMPARE(editorService.displayVelocityAtPosition(sourcePosition), "064");
    const Position targetPosition = { 1, 0, 0, 0, 0 };
    QCOMPARE(editorService.displayNoteAtPosition(targetPosition), "C-3");
    QCOMPARE(editorService.displayVelocityAtPosition(targetPosition), "064");
}

void EditorServiceTest::test_patternCopyPaste_trackDeleted_shouldCopyPattern()
{
    EditorService editorService { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };
    QSignalSpy noteDataChangedSpy { &editorService, &EditorService::noteDataAtPositionChanged };
    const Position sourcePosition = { 0, 0, 0, 0, 0 };
    QVERIFY(editorService.requestPosition(sourcePosition));
    QVERIFY(editorService.requestNoteOnAtCurrentPosition(1, 3, 64));
    QCOMPARE(noteDataChangedSpy.count(), 1);
    QVERIFY(editorService.requestPosition({ 0, 1, 0, 0, 0 }));
    editorService.requestTrackDeletion();

    editorService.setIsModified(false);
    editorService.requestPatternCopy();
    QVERIFY(!editorService.isModified());
    editorService.setCurrentPattern(1);
    editorService.requestPatternPaste();

    QCOMPARE(noteDataChangedSpy.count(), 449);
    QCOMPARE(editorService.displayNoteAtPosition(sourcePosition), "C-3");
    QCOMPARE(editorService.displayVelocityAtPosition(sourcePosition), "064");
    const Position targetPosition = { 1, 0, 0, 0, 0 };
    QCOMPARE(editorService.displayNoteAtPosition(targetPosition), "C-3");
    QCOMPARE(editorService.displayVelocityAtPosition(targetPosition), "064");
}

void EditorServiceTest::test_patternCopyPaste_shorterTarget_shouldCopyPattern()
{
    EditorService editorService { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };
    QSignalSpy noteDataChangedSpy { &editorService, &EditorService::noteDataAtPositionChanged };
    const Position sourcePosition = { 0, 0, 0, 0, 0 };
    QVERIFY(editorService.requestPosition(sourcePosition));
    QVERIFY(editorService.requestNoteOnAtCurrentPosition(1, 3, 64));
    QCOMPARE(noteDataChangedSpy.count(), 1);

    editorService.setIsModified(false);
    editorService.requestPatternCopy();
    QVERIFY(!editorService.isModified());
    editorService.setCurrentPattern(1);
    editorService.setCurrentLineCount(32);
    editorService.requestPatternPaste();

    QCOMPARE(noteDataChangedSpy.count(), 257);
    QCOMPARE(editorService.displayNoteAtPosition(sourcePosition), "C-3");
    QCOMPARE(editorService.displayVelocityAtPosition(sourcePosition), "064");
    const Position targetPosition = { 1, 0, 0, 0, 0 };
    QCOMPARE(editorService.displayNoteAtPosition(targetPosition), "C-3");
    QCOMPARE(editorService.displayVelocityAtPosition(targetPosition), "064");
}

void EditorServiceTest::test_selectionCutPaste_shouldCopySelection()
{
    const auto selectionService = std::make_shared<SelectionService>();
    const auto settingsService = std::make_shared<SettingsService>();
    EditorService editorService { selectionService, settingsService, std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };
    QSignalSpy noteDataChangedSpy { &editorService, &EditorService::noteDataAtPositionChanged };
    const Position sourcePosition = { 0, 1, 0, 8, 0 };
    QVERIFY(editorService.requestPosition(sourcePosition));
    QVERIFY(editorService.requestNoteOnAtCurrentPosition(1, 3, 64));
    QCOMPARE(noteDataChangedSpy.count(), 1);

    editorService.setIsModified(false);
    selectionService->requestSelectionStart(0, 1, 0, 4);
    selectionService->requestSelectionEnd(0, 1, 0, 12);
    editorService.requestSelectionCut();
    QVERIFY(editorService.isModified());
    QCOMPARE(noteDataChangedSpy.count(), 10);
    const Position targetPosition = { 0, 2, 0, 16, 0 };
    QVERIFY(editorService.requestPosition(targetPosition));
    editorService.requestSelectionPaste();

    QCOMPARE(noteDataChangedSpy.count(), 19);
    QCOMPARE(editorService.displayNoteAtPosition(sourcePosition), editorService.noDataString());
    QCOMPARE(editorService.displayVelocityAtPosition(sourcePosition), editorService.noDataString());
    const Position pastedNotePosition = { 0, 2, 0, targetPosition.line + sourcePosition.line - selectionService->selectedPositions().at(0).line, 0 };
    QCOMPARE(editorService.displayNoteAtPosition(pastedNotePosition), "C-3");
    QCOMPARE(editorService.displayVelocityAtPosition(pastedNotePosition), "064");
    QCOMPARE(editorService.song()->noteDataAtPosition(pastedNotePosition)->column(), targetPosition.column);
    QCOMPARE(editorService.song()->noteDataAtPosition(pastedNotePosition)->track(), targetPosition.track);
}

void EditorServiceTest::test_selectionCopyPaste_shouldCopySelection()
{
    const auto selectionService = std::make_shared<SelectionService>();
    const auto settingsService = std::make_shared<SettingsService>();
    EditorService editorService { selectionService, settingsService, std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };
    QSignalSpy noteDataChangedSpy { &editorService, &EditorService::noteDataAtPositionChanged };
    const Position sourcePosition = { 0, 1, 0, 8, 0 };
    QVERIFY(editorService.requestPosition(sourcePosition));
    QVERIFY(editorService.requestNoteOnAtCurrentPosition(1, 3, 64));
    QCOMPARE(noteDataChangedSpy.count(), 1);

    editorService.setIsModified(false);
    selectionService->requestSelectionStart(0, 1, 0, 4);
    selectionService->requestSelectionEnd(0, 1, 0, 12);
    editorService.requestSelectionCopy();
    QVERIFY(!editorService.isModified());
    QCOMPARE(noteDataChangedSpy.count(), 1);
    const Position targetPosition = { 0, 2, 0, 16, 0 };
    QVERIFY(editorService.requestPosition(targetPosition));
    editorService.requestSelectionPaste();

    QCOMPARE(noteDataChangedSpy.count(), 10);
    QCOMPARE(editorService.displayNoteAtPosition(sourcePosition), "C-3");
    QCOMPARE(editorService.displayVelocityAtPosition(sourcePosition), "064");
    const Position pastedNotePosition = { 0, 2, 0, targetPosition.line + sourcePosition.line - selectionService->selectedPositions().at(0).line, 0 };
    QCOMPARE(editorService.displayNoteAtPosition(pastedNotePosition), "C-3");
    QCOMPARE(editorService.displayVelocityAtPosition(pastedNotePosition), "064");
    QCOMPARE(editorService.song()->noteDataAtPosition(pastedNotePosition)->column(), targetPosition.column);
    QCOMPARE(editorService.song()->noteDataAtPosition(pastedNotePosition)->track(), targetPosition.track);
}

void EditorServiceTest::test_requestCursorLeft_shouldWrapCorrectly()
{
    EditorService editorService { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };
    editorService.requestPosition({ 0, 0, 0, 0, 0 });
    editorService.requestCursorLeft();

    QCOMPARE(editorService.position().track, editorService.trackIndices().back());
}

void EditorServiceTest::test_requestDigitSetAtCurrentPosition_velocity_shouldChangeVelocity()
{
    EditorService editorService { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };
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

void EditorServiceTest::test_velocity_input_hundreds_digit()
{
    EditorService editorService { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };
    QVERIFY(editorService.requestPosition(0, 0, 0, 0, 0));

    // Move to Hundreds digit of velocity (LineColumn 1)
    editorService.requestCursorRight();

    // Helper to set velocity and try input '1'
    auto checkInput = [&](uint8_t startVelocity, QString expectedDisplay) {
        // Reset to start velocity using note on
        editorService.requestCursorLeft(); // Back to note column
        editorService.requestNoteOnAtCurrentPosition(1, 3, startVelocity);
        editorService.requestCursorRight(); // Back to velocity hundreds

        QVERIFY(editorService.requestDigitSetAtCurrentPosition(1));
        QCOMPARE(editorService.displayVelocityAtPosition(0, 0, 0, 0), expectedDisplay);
    };

    checkInput(0, "100");
    checkInput(27, "127");
    checkInput(28, "127");
    checkInput(50, "127");
    checkInput(99, "127");
    checkInput(100, "100");
    checkInput(127, "127");
}

void EditorServiceTest::test_requestHorizontalScrollPositionChange_shouldChangePosition()
{
    EditorService editorService { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };
    QSignalSpy horizontalScrollChangeSpy(&editorService, &EditorService::horizontalScrollChanged);
    QSignalSpy positionChangedSpy { &editorService, &EditorService::positionChanged };

    editorService.requestHorizontalScrollBarPositionChange(0);
    QCOMPARE(editorService.horizontalScrollPosition(), 0);
    QVERIFY(editorService.isColumnVisible(0, 0));

    editorService.requestHorizontalScrollBarPositionChange(0.5);
    QCOMPARE(editorService.horizontalScrollPosition(), 2);
    QVERIFY(!editorService.isColumnVisible(0, 0));

    editorService.requestNewColumn(0);
    QCOMPARE(positionChangedSpy.count(), 2);
    editorService.requestHorizontalScrollBarPositionChange(0.5); // Not the exact threshold, but "enough"

    QCOMPARE(editorService.horizontalScrollPosition(), 3);
    QCOMPARE(horizontalScrollChangeSpy.count(), 2);
    QCOMPARE(positionChangedSpy.count(), 3);
}

void EditorServiceTest::test_requestNewColumn_shouldAddNewColumn()
{
    EditorService editorService { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };
    QVERIFY(editorService.requestPosition(0, 0, 0, 0, 0));
    QSignalSpy positionChangedSpy { &editorService, &EditorService::positionChanged };
    QSignalSpy columnAddedSpy { &editorService, &EditorService::columnAdded };
    QSignalSpy scrollBarSizeChangedSpy { &editorService, &EditorService::scrollBarHandleSizeChanged };
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
    EditorService editorService { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };
    QVERIFY(editorService.requestPosition(0, 0, 0, 0, 0));
    QSignalSpy columnDeletedSpy { &editorService, &EditorService::columnDeleted };
    QSignalSpy positionChangedSpy { &editorService, &EditorService::positionChanged };
    QSignalSpy scrollBarSizeChangedSpy { &editorService, &EditorService::scrollBarHandleSizeChanged };
    QSignalSpy scrollBarStepSizeChangedSpy { &editorService, &EditorService::scrollBarStepSizeChanged };

    editorService.requestNewColumn(0);
    QCOMPARE(editorService.columnCount(0), 2);
    QVERIFY(editorService.requestPosition(0, 0, 1, 0, 0));
    QCOMPARE(scrollBarSizeChangedSpy.count(), 1);

    editorService.requestColumnDeletion(0);

    QVERIFY(editorService.isModified());
    QCOMPARE(columnDeletedSpy.count(), 1);
    QCOMPARE(positionChangedSpy.count(), 3);
    QCOMPARE(editorService.position().column, 0);
    QCOMPARE(scrollBarSizeChangedSpy.count(), 2);
    QCOMPARE(scrollBarStepSizeChangedSpy.count(), 2);
    QCOMPARE(editorService.columnCount(0), 1);
}

void EditorServiceTest::test_requestNewTrackToRight_shouldAddNewTrack()
{
    EditorService editorService { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };
    const auto initialTrackCount = editorService.trackCount();
    QVERIFY(editorService.requestPosition(0, 1, 0, 0, 0));
    QSignalSpy trackAddedSpy { &editorService, &EditorService::trackAdded };
    editorService.requestNewTrackToRight();

    const auto newIndex = initialTrackCount;
    QCOMPARE(editorService.trackCount(), initialTrackCount + 1);
    QCOMPARE(editorService.trackName(newIndex), "Track " + QString::number(newIndex + 1));
    QCOMPARE(editorService.trackPositionByIndex(newIndex), 2);
    QCOMPARE(editorService.trackIndexByPosition(2), newIndex);
    QCOMPARE(trackAddedSpy.count(), 1);
    QCOMPARE(trackAddedSpy.at(0).at(0).toUInt(), newIndex);

    editorService.requestNewColumn(newIndex);
    QCOMPARE(editorService.columnCount(newIndex), 2);

    QVERIFY(editorService.requestPosition(0, newIndex, 1, 0, 0));
    QVERIFY(editorService.requestNoteOnAtCurrentPosition(1, 3, 64));
    editorService.requestColumnTranspose(1);
    editorService.requestTrackTranspose(1);
    const auto noteAndVelocity = editorService.displayNoteAndVelocityAtPosition(0, newIndex, 1, 0);
    QCOMPARE(noteAndVelocity.at(0), "D-3");
    QCOMPARE(noteAndVelocity.at(1), "064");

    editorService.requestColumnDeletion(newIndex);
    QCOMPARE(editorService.columnCount(newIndex), 1);

    editorService.initialize();
    QCOMPARE(editorService.trackCount(), initialTrackCount);
}

void EditorServiceTest::test_requestNewTrackToLeft_shouldAddNewTrack()
{
    EditorService editorService { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };
    const auto initialTrackCount = editorService.trackCount();
    QVERIFY(editorService.requestPosition(0, 1, 0, 0, 0));
    QSignalSpy trackAddedSpy { &editorService, &EditorService::trackAdded };
    editorService.requestNewTrackToLeft();

    const auto newIndex = initialTrackCount;
    QCOMPARE(editorService.trackCount(), initialTrackCount + 1);
    QCOMPARE(editorService.trackName(newIndex), "Track " + QString::number(newIndex + 1));
    QCOMPARE(editorService.trackPositionByIndex(newIndex), 1);
    QCOMPARE(editorService.trackIndexByPosition(1), newIndex);
    QCOMPARE(trackAddedSpy.count(), 1);
    QCOMPARE(trackAddedSpy.at(0).at(0).toUInt(), newIndex);

    editorService.requestNewColumn(newIndex);
    QCOMPARE(editorService.columnCount(newIndex), 2);

    QVERIFY(editorService.requestPosition(0, newIndex, 1, 0, 0));
    QVERIFY(editorService.requestNoteOnAtCurrentPosition(1, 3, 64));
    editorService.requestColumnTranspose(1);
    editorService.requestTrackTranspose(1);
    const auto noteAndVelocity = editorService.displayNoteAndVelocityAtPosition(0, newIndex, 1, 0);
    QCOMPARE(noteAndVelocity.at(0), "D-3");
    QCOMPARE(noteAndVelocity.at(1), "064");

    editorService.requestColumnDeletion(newIndex);
    QCOMPARE(editorService.columnCount(newIndex), 1);

    editorService.initialize();
    QCOMPARE(editorService.trackCount(), initialTrackCount);
}

void EditorServiceTest::test_requestNewTrackToLeft_firstTrack_shouldAddNewTrack()
{
    EditorService editorService { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };
    const auto initialTrackCount = editorService.trackCount();
    QVERIFY(editorService.requestPosition(0, 0, 0, 0, 0));
    QSignalSpy trackAddedSpy { &editorService, &EditorService::trackAdded };
    editorService.requestNewTrackToLeft();

    const auto newIndex = initialTrackCount;
    QCOMPARE(editorService.trackCount(), initialTrackCount + 1);
    QCOMPARE(editorService.trackName(newIndex), "Track " + QString::number(newIndex + 1));
    QCOMPARE(editorService.trackPositionByIndex(newIndex), 0);
    QCOMPARE(editorService.trackIndexByPosition(0), newIndex);
    QCOMPARE(trackAddedSpy.count(), 1);
    QCOMPARE(trackAddedSpy.at(0).at(0).toUInt(), newIndex);

    editorService.initialize();
    QCOMPARE(editorService.trackCount(), initialTrackCount);
}

void EditorServiceTest::test_requestTrackDeletion_firstTrack_shouldDeleteTrack()
{
    EditorService editorService { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };
    const auto initialTrackCount = editorService.trackCount();
    QSignalSpy trackDeletedSpy { &editorService, &EditorService::trackDeleted };
    QSignalSpy positionChangedSpy { &editorService, &EditorService::positionChanged };
    QSignalSpy scrollBarSizeChangedSpy { &editorService, &EditorService::scrollBarHandleSizeChanged };
    QSignalSpy scrollBarStepSizeChangedSpy { &editorService, &EditorService::scrollBarStepSizeChanged };

    QVERIFY(editorService.requestPosition(0, 0, 0, 0, 0));
    editorService.requestTrackDeletion();

    QCOMPARE(editorService.trackCount(), initialTrackCount - 1);
    QVERIFY(editorService.isModified());
    QCOMPARE(trackDeletedSpy.count(), 1);
    QCOMPARE(positionChangedSpy.count(), 2);
    QCOMPARE(editorService.position().track, editorService.trackIndices().at(0));
    QCOMPARE(editorService.position().column, 0);
    QCOMPARE(editorService.position().lineColumn, 0);
    QCOMPARE(scrollBarSizeChangedSpy.count(), 1);
    QCOMPARE(scrollBarStepSizeChangedSpy.count(), 1);
}

void EditorServiceTest::test_requestTrackDeletion_lastTrack_shouldDeleteTrack()
{
    EditorService editorService { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };
    const auto initialTrackCount = editorService.trackCount();
    QSignalSpy trackDeletedSpy { &editorService, &EditorService::trackDeleted };
    QSignalSpy positionChangedSpy { &editorService, &EditorService::positionChanged };
    QSignalSpy scrollBarSizeChangedSpy { &editorService, &EditorService::scrollBarHandleSizeChanged };
    QSignalSpy scrollBarStepSizeChangedSpy { &editorService, &EditorService::scrollBarStepSizeChanged };

    QVERIFY(editorService.requestPosition(0, editorService.trackCount() - 1, 0, 0, 0));
    editorService.requestTrackDeletion();

    QCOMPARE(editorService.trackCount(), initialTrackCount - 1);
    QVERIFY(editorService.isModified());
    QCOMPARE(trackDeletedSpy.count(), 1);
    QCOMPARE(positionChangedSpy.count(), 2);
    QCOMPARE(editorService.position().track, editorService.trackIndices().at(initialTrackCount - 2));
    QCOMPARE(editorService.position().column, 0);
    QCOMPARE(editorService.position().lineColumn, 0);
    QCOMPARE(scrollBarSizeChangedSpy.count(), 1);
    QCOMPARE(scrollBarStepSizeChangedSpy.count(), 1);
}

void EditorServiceTest::test_requestNoteDeletionAtCurrentPosition_shouldDeleteNoteData()
{
    EditorService editorService { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };
    QSignalSpy noteDataChangedSpy { &editorService, &EditorService::noteDataAtPositionChanged };
    QVERIFY(editorService.requestPosition(0, 0, 0, 0, 0));
    QVERIFY(editorService.requestNoteOnAtCurrentPosition(1, 3, 64));

    editorService.setIsModified(false);
    editorService.requestNoteDeletionAtCurrentPosition(false);

    QVERIFY(editorService.isModified());
    QCOMPARE(noteDataChangedSpy.count(), 2);
    QCOMPARE(editorService.displayNoteAtPosition(0, 0, 0, 4), editorService.noDataString());
    QCOMPARE(editorService.displayVelocityAtPosition(0, 0, 0, 4), editorService.noDataString());
}

void EditorServiceTest::test_requestNoteDeletionAtCurrentPosition_shouldDeleteNoteData_shouldShiftNotes()
{
    EditorService editorService { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };
    QSignalSpy noteDataChangedSpy { &editorService, &EditorService::noteDataAtPositionChanged };
    QVERIFY(editorService.requestPosition(0, 0, 0, 10, 0));
    QVERIFY(editorService.requestNoteOnAtCurrentPosition(1, 3, 64));

    editorService.setIsModified(false);
    QVERIFY(editorService.requestPosition(0, 0, 0, 10, 0));
    QSignalSpy positionChangedSpy { &editorService, &EditorService::positionChanged };
    editorService.requestNoteDeletionAtCurrentPosition(true);

    QVERIFY(editorService.isModified());
    QCOMPARE(noteDataChangedSpy.count(), 56);
    QCOMPARE(positionChangedSpy.count(), 2);
    QCOMPARE(editorService.position().line, 9);
    QCOMPARE(editorService.displayNoteAtPosition(0, 0, 0, 9), "C-3");
    QCOMPARE(editorService.displayVelocityAtPosition(0, 0, 0, 9), "064");
}

void EditorServiceTest::test_requestNoteInsertionAtCurrentPosition_shouldInsertNoteData()
{
    EditorService editorService { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };
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
    EditorService editorService { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };
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
    QCOMPARE(editorService.displayVelocityAtPosition(0, 0, 0, 0), "088");

    editorService.requestScroll(4);
    editorService.requestNoteOnAtCurrentPosition(2, 4, 72);
    QCOMPARE(noteDataChangedSpy.count(), 3);
    QCOMPARE(editorService.displayNoteAtPosition(0, 0, 0, 4), "C#4");
    QCOMPARE(editorService.displayVelocityAtPosition(0, 0, 0, 4), "072");
}

void EditorServiceTest::test_requestNoteOnAtCurrentPosition_notOnNoteColumn_shouldNotChangeNoteData()
{
    EditorService editorService { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };
    QSignalSpy noteDataChangedSpy { &editorService, &EditorService::noteDataAtPositionChanged };

    QVERIFY(editorService.requestPosition(0, 0, 0, 0, 1));
    QVERIFY(!editorService.requestNoteOnAtCurrentPosition(1, 3, 64));
    QCOMPARE(noteDataChangedSpy.count(), 0);
    QVERIFY(!editorService.isModified());

    QCOMPARE(editorService.displayNoteAtPosition(0, 0, 0, 0), editorService.noDataString());
    QCOMPARE(editorService.displayVelocityAtPosition(0, 0, 0, 0), editorService.noDataString());
}

void EditorServiceTest::test_requestNoteOffAtColumnFirstLine_shouldAddNoteOff()
{
    EditorService editorService { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };
    QSignalSpy noteDataChangedSpy { &editorService, &EditorService::noteDataAtPositionChanged };

    editorService.requestPosition(0, 0, 0, 4, 0); // Move to line 4
    editorService.requestNoteOffAtColumnFirstLine();

    QCOMPARE(noteDataChangedSpy.count(), 1);
    QCOMPARE(editorService.displayNoteAtPosition(0, 0, 0, 0), "OFF");
    QCOMPARE(editorService.displayNoteAtPosition(0, 0, 0, 4), editorService.noDataString());
    QVERIFY(editorService.isModified());

    editorService.undo();
    QCOMPARE(editorService.displayNoteAtPosition(0, 0, 0, 0), editorService.noDataString());
}

void EditorServiceTest::test_requestNoteOffAtTrackFirstLine_shouldAddNoteOff()
{
    EditorService editorService { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };
    QSignalSpy noteDataChangedSpy { &editorService, &EditorService::noteDataAtPositionChanged };

    editorService.requestNewColumn(0); // Track 0 now has 2 columns
    QCOMPARE(static_cast<int>(editorService.columnCount(0)), 2);

    editorService.requestPosition(0, 0, 0, 5, 0); // Move to line 5
    editorService.requestNoteOffAtTrackFirstLine();

    QCOMPARE(noteDataChangedSpy.count(), 2);
    QCOMPARE(editorService.displayNoteAtPosition(0, 0, 0, 0), "OFF");
    QCOMPARE(editorService.displayNoteAtPosition(0, 0, 1, 0), "OFF");
    QVERIFY(editorService.isModified());

    editorService.undo();
    QCOMPARE(editorService.displayNoteAtPosition(0, 0, 0, 0), editorService.noDataString());
    QCOMPARE(editorService.displayNoteAtPosition(0, 0, 1, 0), editorService.noDataString());
}

void EditorServiceTest::test_requestNoteOffAtPatternFirstLine_shouldAddNoteOff()
{
    EditorService editorService { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };
    QSignalSpy noteDataChangedSpy { &editorService, &EditorService::noteDataAtPositionChanged };

    const auto numTracks = editorService.trackCount();
    editorService.requestPosition(0, 1, 0, 2, 0); // Track 1, Line 2
    editorService.requestNoteOffAtPatternFirstLine();

    QCOMPARE(noteDataChangedSpy.count(), static_cast<int>(numTracks));
    for (quint64 trk = 0; trk < numTracks; trk++) {
        QCOMPARE(editorService.displayNoteAtPosition(0, trk, 0, 0), "OFF");
    }
    QVERIFY(editorService.isModified());

    editorService.undo();
    for (quint64 trk = 0; trk < numTracks; trk++) {
        QCOMPARE(editorService.displayNoteAtPosition(0, trk, 0, 0), editorService.noDataString());
    }
}

void EditorServiceTest::test_requestColumnTranspose_shouldTransposeColumn()
{
    EditorService editorService { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };
    QSignalSpy noteDataChangedSpy { &editorService, &EditorService::noteDataAtPositionChanged };

    QVERIFY(editorService.requestPosition(0, 0, 0, 0, 0));
    QVERIFY(editorService.requestNoteOnAtCurrentPosition(1, 3, 64));
    editorService.requestNewColumn(0);
    QVERIFY(editorService.requestPosition(0, 0, 1, 0, 0));
    QVERIFY(editorService.requestNoteOnAtCurrentPosition(3, 3, 64));

    editorService.requestColumnTranspose(1);

    QCOMPARE(noteDataChangedSpy.count(), 3);
    QCOMPARE(editorService.displayNoteAtPosition(0, 0, 0, 0), "C-3");
    QCOMPARE(editorService.displayVelocityAtPosition(0, 0, 0, 0), "064");
    QCOMPARE(editorService.displayNoteAtPosition(0, 0, 1, 0), "D#3");
    QCOMPARE(editorService.displayVelocityAtPosition(0, 0, 1, 0), "064");
}

void EditorServiceTest::test_requestTrackTranspose_shouldTransposeTrack()
{
    EditorService editorService { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };
    QSignalSpy noteDataChangedSpy { &editorService, &EditorService::noteDataAtPositionChanged };

    QVERIFY(editorService.requestPosition(0, 0, 0, 0, 0));
    QVERIFY(editorService.requestNoteOnAtCurrentPosition(1, 3, 64));
    editorService.requestNewColumn(0);
    QVERIFY(editorService.requestPosition(0, 0, 1, 0, 0));
    QVERIFY(editorService.requestNoteOnAtCurrentPosition(3, 3, 64));

    editorService.requestTrackTranspose(1);

    QCOMPARE(noteDataChangedSpy.count(), 4);
    QCOMPARE(editorService.displayNoteAtPosition(0, 0, 0, 0), "C#3");
    QCOMPARE(editorService.displayVelocityAtPosition(0, 0, 0, 0), "064");
    QCOMPARE(editorService.displayNoteAtPosition(0, 0, 1, 0), "D#3");
    QCOMPARE(editorService.displayVelocityAtPosition(0, 0, 1, 0), "064");
}

void EditorServiceTest::test_requestPatternTranspose_shouldTransposePattern()
{
    EditorService editorService { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };
    QSignalSpy noteDataChangedSpy { &editorService, &EditorService::noteDataAtPositionChanged };

    QVERIFY(editorService.requestPosition(0, 0, 0, 0, 0));
    QVERIFY(editorService.requestNoteOnAtCurrentPosition(1, 3, 64));
    QVERIFY(editorService.requestPosition(0, 1, 0, 0, 0));
    QVERIFY(editorService.requestNoteOnAtCurrentPosition(3, 3, 64));

    editorService.requestPatternTranspose(1);

    QCOMPARE(noteDataChangedSpy.count(), 4);
    QCOMPARE(editorService.displayNoteAtPosition(0, 0, 0, 0), "C#3");
    QCOMPARE(editorService.displayVelocityAtPosition(0, 0, 0, 0), "064");
    QCOMPARE(editorService.displayNoteAtPosition(0, 1, 0, 0), "D#3");
    QCOMPARE(editorService.displayVelocityAtPosition(0, 1, 0, 0), "064");
}

void EditorServiceTest::test_requestSongTranspose_shouldTransposeSong()
{
    EditorService editorService { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };
    QSignalSpy noteDataChangedSpy { &editorService, &EditorService::noteDataAtPositionChanged };

    QVERIFY(editorService.requestPosition(0, 0, 0, 0, 0));
    QVERIFY(editorService.requestNoteOnAtCurrentPosition(1, 3, 64)); // C-3

    editorService.setCurrentPattern(1);
    QVERIFY(editorService.requestPosition(1, 1, 0, 0, 0));
    QVERIFY(editorService.requestNoteOnAtCurrentPosition(3, 3, 64)); // D-3

    editorService.requestSongTranspose(1);

    QCOMPARE(editorService.displayNoteAtPosition(0, 0, 0, 0), "C#3");
    QCOMPARE(editorService.displayNoteAtPosition(1, 1, 0, 0), "D#3");
}

void EditorServiceTest::test_requestSelectionTranspose_shouldTransposeSelection()
{
    const auto selectionService = std::make_shared<SelectionService>();
    const auto settingsService = std::make_shared<SettingsService>();
    EditorService editorService { selectionService, settingsService, std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };
    QSignalSpy noteDataChangedSpy { &editorService, &EditorService::noteDataAtPositionChanged };

    editorService.requestNewColumn(0);

    QVERIFY(editorService.requestPosition(0, 0, 1, 0, 0));
    QVERIFY(editorService.requestNoteOnAtCurrentPosition(1, 3, 64));
    QVERIFY(editorService.requestPosition(0, 0, 1, 4, 0));
    QVERIFY(editorService.requestNoteOnAtCurrentPosition(1, 3, 64));
    QVERIFY(editorService.requestPosition(0, 0, 1, 8, 0));
    QVERIFY(editorService.requestNoteOnAtCurrentPosition(1, 3, 64));

    editorService.requestSelectionTranspose(1); // No selection => should do nothing

    QCOMPARE(editorService.displayNoteAtPosition(0, 0, 1, 0), "C-3");
    QCOMPARE(editorService.displayVelocityAtPosition(0, 0, 1, 0), "064");
    QCOMPARE(editorService.displayNoteAtPosition(0, 0, 1, 4), "C-3");
    QCOMPARE(editorService.displayVelocityAtPosition(0, 0, 1, 4), "064");
    QCOMPARE(editorService.displayNoteAtPosition(0, 0, 1, 8), "C-3");
    QCOMPARE(editorService.displayVelocityAtPosition(0, 0, 1, 8), "064");

    selectionService->requestSelectionStart(0, 0, 1, 0);
    selectionService->requestSelectionEnd(0, 0, 1, 4);

    editorService.requestSelectionTranspose(1);

    QCOMPARE(noteDataChangedSpy.count(), 5);
    QCOMPARE(editorService.displayNoteAtPosition(0, 0, 1, 0), "C#3");
    QCOMPARE(editorService.displayVelocityAtPosition(0, 0, 1, 0), "064");
    QCOMPARE(editorService.displayNoteAtPosition(0, 0, 1, 4), "C#3");
    QCOMPARE(editorService.displayVelocityAtPosition(0, 0, 1, 4), "064");
    QCOMPARE(editorService.displayNoteAtPosition(0, 0, 1, 8), "C-3");
    QCOMPARE(editorService.displayVelocityAtPosition(0, 0, 1, 8), "064");
}

void EditorServiceTest::test_requestLinearVelocityInterpolationOnColumn_shouldInterpolateVelocities()
{
    EditorService editorService { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };
    QSignalSpy noteDataChangedSpy { &editorService, &EditorService::noteDataAtPositionChanged };

    QVERIFY(editorService.requestPosition(0, 0, 0, 0, 0));
    QVERIFY(editorService.requestNoteOnAtCurrentPosition(1, 3, 64));
    QVERIFY(editorService.requestPosition(0, 0, 0, 3, 0));
    QVERIFY(editorService.requestNoteOnAtCurrentPosition(1, 3, 64));
    QVERIFY(editorService.requestPosition(0, 0, 0, 7, 0));
    QVERIFY(editorService.requestNoteOnAtCurrentPosition(1, 3, 64));

    editorService.requestLinearVelocityInterpolationOnColumn(0, 7, 0, 100, false);

    QCOMPARE(noteDataChangedSpy.count(), 6);
    QCOMPARE(editorService.displayNoteAtPosition(0, 0, 0, 0), "C-3");
    QCOMPARE(editorService.displayVelocityAtPosition(0, 0, 0, 0), "000");
    QCOMPARE(editorService.displayNoteAtPosition(0, 0, 0, 3), "C-3");
    QCOMPARE(editorService.displayVelocityAtPosition(0, 0, 0, 3), "042");
    QCOMPARE(editorService.displayNoteAtPosition(0, 0, 0, 7), "C-3");
    QCOMPARE(editorService.displayVelocityAtPosition(0, 0, 0, 7), "100");
}

void EditorServiceTest::test_requestLinearVelocityInterpolationOnTrack_shouldInterpolateVelocities()
{
    EditorService editorService { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };
    QSignalSpy noteDataChangedSpy { &editorService, &EditorService::noteDataAtPositionChanged };

    QVERIFY(editorService.requestPosition(0, 0, 0, 0, 0));
    QVERIFY(editorService.requestNoteOnAtCurrentPosition(1, 3, 64));
    QVERIFY(editorService.requestPosition(0, 0, 0, 3, 0));
    QVERIFY(editorService.requestNoteOnAtCurrentPosition(1, 3, 64));
    QVERIFY(editorService.requestPosition(0, 0, 0, 7, 0));
    QVERIFY(editorService.requestNoteOnAtCurrentPosition(1, 3, 64));

    editorService.requestNewColumn(0);

    QVERIFY(editorService.requestPosition(0, 0, 1, 0, 0));
    QVERIFY(editorService.requestNoteOnAtCurrentPosition(1, 3, 64));
    QVERIFY(editorService.requestPosition(0, 0, 1, 3, 0));
    QVERIFY(editorService.requestNoteOnAtCurrentPosition(1, 3, 64));
    QVERIFY(editorService.requestPosition(0, 0, 1, 7, 0));
    QVERIFY(editorService.requestNoteOnAtCurrentPosition(1, 3, 64));

    editorService.requestLinearVelocityInterpolationOnTrack(0, 7, 0, 100, false);

    QCOMPARE(noteDataChangedSpy.count(), 12);

    QCOMPARE(editorService.displayNoteAtPosition(0, 0, 0, 0), "C-3");
    QCOMPARE(editorService.displayVelocityAtPosition(0, 0, 0, 0), "000");
    QCOMPARE(editorService.displayNoteAtPosition(0, 0, 0, 3), "C-3");
    QCOMPARE(editorService.displayVelocityAtPosition(0, 0, 0, 3), "042");
    QCOMPARE(editorService.displayNoteAtPosition(0, 0, 0, 7), "C-3");
    QCOMPARE(editorService.displayVelocityAtPosition(0, 0, 0, 7), "100");

    QCOMPARE(editorService.displayNoteAtPosition(0, 0, 1, 0), "C-3");
    QCOMPARE(editorService.displayVelocityAtPosition(0, 0, 1, 0), "000");
    QCOMPARE(editorService.displayNoteAtPosition(0, 0, 1, 3), "C-3");
    QCOMPARE(editorService.displayVelocityAtPosition(0, 0, 1, 3), "042");
    QCOMPARE(editorService.displayNoteAtPosition(0, 0, 1, 7), "C-3");
    QCOMPARE(editorService.displayVelocityAtPosition(0, 0, 1, 7), "100");
}

void EditorServiceTest::test_requestLinearVelocityInterpolationOnColumn_shouldInterpolateVelocitiesAsPercentages()
{
    EditorService editorService { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };
    QSignalSpy noteDataChangedSpy { &editorService, &EditorService::noteDataAtPositionChanged };

    QVERIFY(editorService.requestPosition(0, 0, 0, 0, 0));
    QVERIFY(editorService.requestNoteOnAtCurrentPosition(1, 3, 100)); // Initial velocity 100
    QVERIFY(editorService.requestPosition(0, 0, 0, 3, 0));
    QVERIFY(editorService.requestNoteOnAtCurrentPosition(1, 3, 50));  // Initial velocity 50
    QVERIFY(editorService.requestPosition(0, 0, 0, 7, 0));
    QVERIFY(editorService.requestNoteOnAtCurrentPosition(1, 3, 20));  // Initial velocity 20

    // Interpolate from 50% to 150%
    editorService.requestLinearVelocityInterpolationOnColumn(0, 7, 50, 150, true);

    QCOMPARE(noteDataChangedSpy.count(), 6);
    QCOMPARE(editorService.displayNoteAtPosition(0, 0, 0, 0), "C-3");
    // Line 0: 100 * 0.50 = 50
    QCOMPARE(editorService.displayVelocityAtPosition(0, 0, 0, 0), "050");
    QCOMPARE(editorService.displayNoteAtPosition(0, 0, 0, 3), "C-3");
    // Line 3: Interpolated percentage at line 3: 50 + (150 - 50) * (3.0 / 7.0) = 50 + 100 * 0.42857 = 50 + 42.857 = 92.857
    //         Velocity at line 3: 50 * 92.857 / 100 = 46.4285 -> 46
    QCOMPARE(editorService.displayVelocityAtPosition(0, 0, 0, 3), "046");
    QCOMPARE(editorService.displayNoteAtPosition(0, 0, 0, 7), "C-3");
    // Line 7: 20 * 1.50 = 30
    QCOMPARE(editorService.displayVelocityAtPosition(0, 0, 0, 7), "030");
}

void EditorServiceTest::test_requestPosition_invalidPosition_shouldNotChangePosition()
{
    EditorService editorService { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };
    QSignalSpy positionChangedSpy { &editorService, &EditorService::positionChanged };
    QSignalSpy currentTimeChangedSpy { &editorService, &EditorService::currentTimeChanged };

    const auto neg = static_cast<size_t>(-1);
    QVERIFY(!editorService.requestPosition(neg, 0, 0, 0, 0));
    QVERIFY(!editorService.requestPosition(0, neg, 0, 0, 0));
    QVERIFY(!editorService.requestPosition(0, 0, neg, 0, 0));
    QVERIFY(!editorService.requestPosition(0, 0, 0, 0, neg));
    QCOMPARE(positionChangedSpy.count(), 0);
    QCOMPARE(currentTimeChangedSpy.count(), 0);
}

void EditorServiceTest::test_requestPosition_validPosition_shouldChangePosition()
{
    EditorService editorService { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };
    QSignalSpy positionChangedSpy { &editorService, &EditorService::positionChanged };
    QSignalSpy currentTimeChangedSpy { &editorService, &EditorService::currentTimeChanged };

    QVERIFY(editorService.requestPosition(0, 0, 0, 0, 0));
    QVERIFY(editorService.requestPosition(0, 0, 0, 1, 0));
    QVERIFY(editorService.requestPosition(0, 0, 0, 2, 0));
    QVERIFY(editorService.requestPosition(0, 0, 0, 3, 0));
    QCOMPARE(positionChangedSpy.count(), 4);
    QCOMPARE(currentTimeChangedSpy.count(), 3);
}

void EditorServiceTest::test_resetSongPosition_firstTrackRemoved_shouldResetPosition()
{
    EditorService editorService { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };

    QVERIFY(editorService.requestPosition(0, 0, 0, 0, 0));
    editorService.requestTrackDeletion();
    editorService.resetSongPosition();

    QCOMPARE(editorService.position().pattern, 0);
    QCOMPARE(editorService.position().track, 1);
    QCOMPARE(editorService.position().column, 0);
    QCOMPARE(editorService.position().line, 0);
}

void EditorServiceTest::test_requestScroll_shouldChangePosition()
{
    EditorService editorService { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };

    editorService.setCurrentLineCount(65);
    editorService.requestPosition(0, 0, 0, 0, 0);

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
    EditorService editorService { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };
    QSignalSpy currentTimeChangedSpy { &editorService, &EditorService::currentTimeChanged };
    QCOMPARE(currentTimeChangedSpy.count(), 0);
    QCOMPARE(editorService.position().line, 0);
    QCOMPARE(editorService.currentTime(), "00:00:00.000");

    editorService.requestScroll(1);

    QCOMPARE(currentTimeChangedSpy.count(), 1);
    QCOMPARE(editorService.position().line, 1);
    QCOMPARE(editorService.currentTime(), "00:00:00.062");
}

void EditorServiceTest::test_requestPosition_shouldChangePosition()
{
    EditorService editorService { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };
    QSignalSpy positionChangedSpy { &editorService, &EditorService::positionChanged };

    editorService.requestPosition(0, 0, 0, 0, 0);
    QCOMPARE(positionChangedSpy.count(), 1);

    editorService.requestPosition(0, 0, 1, 0, 0);
    QCOMPARE(positionChangedSpy.count(), 1);

    editorService.requestNewColumn(0);
    QCOMPARE(positionChangedSpy.count(), 2);
    editorService.requestPosition(0, 0, 1, 0, 0);
    QCOMPARE(positionChangedSpy.count(), 3);
    QCOMPARE(editorService.position().track, 0);
    QCOMPARE(editorService.position().column, 1);

    editorService.requestPosition(0, editorService.trackCount() - 1, 0, 0, 0);
    QCOMPARE(positionChangedSpy.count(), 4);
    QCOMPARE(editorService.position().track, editorService.trackCount() - 1);
}

void EditorServiceTest::test_requestPosition_shouldNotChangePosition()
{
    EditorService editorService { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };
    QSignalSpy positionChangedSpy { &editorService, &EditorService::positionChanged };

    editorService.requestPosition(0, editorService.trackCount(), 0, 0, 0);

    QCOMPARE(positionChangedSpy.count(), 0);
    QCOMPARE(editorService.position().track, 0);
}

void EditorServiceTest::test_setCurrentLineCount_shouldSetLineCount()
{
    EditorService editorService { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };
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
    EditorService editorService { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };

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
    EditorService editorService { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };

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

void EditorServiceTest::test_setInstrumentSettings_shouldSetInstrumentSettings()
{
    EditorService editorService { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };
    QSignalSpy lineDataChangedSpy { &editorService, &EditorService::lineDataChanged };
    editorService.requestPosition(0, 1, 0, 0, 0);
    auto instrumentSettings = std::make_shared<InstrumentSettings>();
    instrumentSettings->patch = 42;

    editorService.setInstrumentSettingsAtCurrentPosition(instrumentSettings);

    QCOMPARE(lineDataChangedSpy.count(), 1);
    editorService.requestPosition(0, 1, 0, 0, 0);
    instrumentSettings = editorService.instrumentSettingsAtCurrentPosition();
    QVERIFY(instrumentSettings);
    QCOMPARE(instrumentSettings->patch, 42);
    QCOMPARE(instrumentSettings->track(), 1);
}

void EditorServiceTest::test_removeInstrumentSettings_shouldRemoveInstrumentSettings()
{
    EditorService editorService { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };
    QSignalSpy lineDataChangedSpy { &editorService, &EditorService::lineDataChanged };
    editorService.requestPosition(0, 1, 0, 0, 0);
    auto instrumentSettings = std::make_shared<InstrumentSettings>();
    instrumentSettings->patch = 42;
    editorService.setInstrumentSettingsAtCurrentPosition(instrumentSettings);
    QCOMPARE(lineDataChangedSpy.count(), 1);

    editorService.removeInstrumentSettingsAtCurrentPosition();
    QCOMPARE(lineDataChangedSpy.count(), 2);

    editorService.requestPosition(0, 1, 0, 0, 0);
    instrumentSettings = editorService.instrumentSettingsAtCurrentPosition();
    QVERIFY(!instrumentSettings);
}

void EditorServiceTest::test_setPatternAtSongPosition_shouldCreatePattern()
{
    EditorService editorService { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };

    QSignalSpy currentPatternChangedSpy { &editorService, &EditorService::currentPatternChanged };
    QSignalSpy durationChangedSpy { &editorService, &EditorService::durationChanged };
    QSignalSpy patternAtCurrentSongPositionChangedSpy { &editorService, &EditorService::patternAtCurrentSongPositionChanged };
    QSignalSpy patternCreatedChangedSpy { &editorService, &EditorService::patternCreated };
    QSignalSpy positionChangedSpy { &editorService, &EditorService::positionChanged };
    QSignalSpy songPositionChangedSpy { &editorService, &EditorService::songPositionChanged };

    editorService.setPatternAtSongPosition(0, 0);

    QCOMPARE(editorService.currentPattern(), 0);
    QCOMPARE(editorService.patternAtCurrentSongPosition(), 0);
    QCOMPARE(editorService.patternCount(), 1);

    QCOMPARE(currentPatternChangedSpy.count(), 0);
    QCOMPARE(patternAtCurrentSongPositionChangedSpy.count(), 0);
    QCOMPARE(patternCreatedChangedSpy.count(), 0);
    QCOMPARE(positionChangedSpy.count(), 0);
    QCOMPARE(songPositionChangedSpy.count(), 0);
    QCOMPARE(durationChangedSpy.count(), 0);
    QCOMPARE(editorService.duration(), "00:00:04.000");

    editorService.setPatternAtSongPosition(0, 1);

    QCOMPARE(editorService.currentPattern(), 1);
    QCOMPARE(editorService.patternAtCurrentSongPosition(), 1);
    QCOMPARE(editorService.patternCount(), 2);

    QCOMPARE(currentPatternChangedSpy.count(), 1);
    QCOMPARE(patternAtCurrentSongPositionChangedSpy.count(), 1);
    QCOMPARE(patternCreatedChangedSpy.count(), 1);
    QCOMPARE(positionChangedSpy.count(), 1);
    QCOMPARE(songPositionChangedSpy.count(), 0);
    QCOMPARE(durationChangedSpy.count(), 0);
    QCOMPARE(editorService.duration(), "00:00:04.000");

    editorService.setPatternAtSongPosition(1, 1);

    QCOMPARE(editorService.currentPattern(), 1);
    QCOMPARE(editorService.patternAtCurrentSongPosition(), 1);
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
    EditorService editorService { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };

    QSignalSpy currentPatternChangedSpy { &editorService, &EditorService::currentPatternChanged };
    QSignalSpy patternAtCurrentSongPositionChangedSpy { &editorService, &EditorService::patternAtCurrentSongPositionChanged };
    QSignalSpy patternCreatedChangedSpy { &editorService, &EditorService::patternCreated };
    QSignalSpy positionChangedSpy { &editorService, &EditorService::positionChanged };
    QSignalSpy songPositionChangedSpy { &editorService, &EditorService::songPositionChanged };

    editorService.setPatternAtSongPosition(1, 1);
    editorService.setSongPosition(1);

    QCOMPARE(editorService.currentPattern(), 1);
    QCOMPARE(editorService.position().pattern, 1);
    QCOMPARE(editorService.patternAtSongPosition(0), 0);
    QCOMPARE(editorService.patternAtSongPosition(1), 1);
    QCOMPARE(editorService.patternAtCurrentSongPosition(), 1);
    QCOMPARE(editorService.patternCount(), 2);

    QCOMPARE(currentPatternChangedSpy.count(), 1);
    QCOMPARE(patternAtCurrentSongPositionChangedSpy.count(), 1);
    QCOMPARE(patternCreatedChangedSpy.count(), 1);
    QCOMPARE(positionChangedSpy.count(), 2);
    QCOMPARE(songPositionChangedSpy.count(), 1);

    editorService.setSongPosition(0);

    QCOMPARE(editorService.currentPattern(), 0);
    QCOMPARE(editorService.position().pattern, 0);
    QCOMPARE(editorService.position().line, 0);
    QCOMPARE(editorService.patternAtSongPosition(0), 0);
    QCOMPARE(editorService.patternAtSongPosition(1), 1);
    QCOMPARE(editorService.patternAtCurrentSongPosition(), 0);
    QCOMPARE(editorService.patternCount(), 2);

    QCOMPARE(currentPatternChangedSpy.count(), 3);
    QCOMPARE(patternAtCurrentSongPositionChangedSpy.count(), 2);
    QCOMPARE(patternCreatedChangedSpy.count(), 1);
    QCOMPARE(positionChangedSpy.count(), 4);
    QCOMPARE(songPositionChangedSpy.count(), 2);
}

void EditorServiceTest::test_setSongPosition_trackDeleted_shouldCreatePattern()
{
    EditorService editorService { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };
    editorService.setPatternAtSongPosition(0, 0);
    editorService.requestTrackDeletion();
    editorService.setPatternAtSongPosition(1, 1);
    QCOMPARE(editorService.patternCount(), 2);
}

void EditorServiceTest::test_setPatternName_shouldChangePatternName()
{
    EditorService editorService { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };

    editorService.setPatternName(0, "Foo");

    QVERIFY(editorService.isModified());
    QCOMPARE(editorService.patternName(0), "Foo");
    QCOMPARE(editorService.currentPatternName(), "Foo");

    editorService.setCurrentPatternName("Bar");

    QVERIFY(editorService.isModified());
    QCOMPARE(editorService.patternName(0), "Bar");
    QCOMPARE(editorService.currentPatternName(), "Bar");
}

void EditorServiceTest::test_setColumnName_shouldChangeColumnName()
{
    EditorService editorService { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };

    editorService.setColumnName(0, 0, "Foo");
    editorService.setColumnName(1, 0, "Bar");

    QVERIFY(editorService.isModified());
    QCOMPARE(editorService.columnName(0, 0), "Foo");
    QCOMPARE(editorService.columnName(1, 0), "Bar");
}

void EditorServiceTest::test_setTrackName_shouldChangeTrackName()
{
    EditorService editorService { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };

    editorService.setTrackName(0, "Foo");
    editorService.setTrackName(1, "Bar");

    QVERIFY(editorService.isModified());
    QCOMPARE(editorService.trackName(0), "Foo");
    QCOMPARE(editorService.trackName(1), "Bar");
}

void EditorServiceTest::test_velocityAtPosition_shouldReturnCorrectVelocity()
{
    EditorService editorService { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };

    editorService.requestPosition(0, 0, 0, 0, 0);
    editorService.requestNoteOnAtCurrentPosition(1, 1, 50);
    QCOMPARE(editorService.velocityAtPosition(0, 0, 0, 0), 50);

    editorService.requestNoteOffAtCurrentPosition();
    QCOMPARE(editorService.velocityAtPosition(0, 0, 0, 0), 0);

    QCOMPARE(editorService.velocityAtPosition(0, 1, 0, 0), 0);
}

void EditorServiceTest::test_requestPositionByTick_shouldRespectUiUpdatesDisabledSetting()
{
    EditorService editorService { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };
    QSignalSpy positionChangedSpy { &editorService, &EditorService::positionChanged };
    QSignalSpy currentTimeChangedSpy { &editorService, &EditorService::currentTimeChanged };
    QSignalSpy songPositionChangedSpy { &editorService, &EditorService::songPositionChanged };

    // Enable disabling UI updates
    editorService.settingsService()->setUiUpdatesDisabledDuringPlayback(true);

    // Initial position
    QCOMPARE(editorService.songPosition(), 0);

    // Populate song position map manually as we don't have a PlayerService rendering
    editorService.song()->updateTickToSongPositionMapping(0, 0, 0, editorService.lineCount(0));

    // Tick update that triggers a line change
    const auto tick = editorService.ticksPerLine(); // Move to next line
    editorService.requestPositionByTick(tick);

    // Assert: Position should NOT change (UI update suppressed)
    QCOMPARE(positionChangedSpy.count(), 0);
    // Assert: Time should change
    QCOMPARE(currentTimeChangedSpy.count(), 1);
    // Assert: Song position should NOT change (we are still in the same pattern)
    QCOMPARE(songPositionChangedSpy.count(), 0);
    QCOMPARE(editorService.songPosition(), 0);

    // Reset setting
    editorService.settingsService()->setUiUpdatesDisabledDuringPlayback(false);

    // Another tick
    const auto nextTick = tick * 2;
    editorService.requestPositionByTick(nextTick);

    // Assert: Position should change now
    QCOMPARE(positionChangedSpy.count(), 1);
    QCOMPARE(currentTimeChangedSpy.count(), 2);
    // Assert: Song position still 0
    QCOMPARE(songPositionChangedSpy.count(), 0);
}

void EditorServiceTest::test_midiNotesAtPosition_shouldReturnCorrectNotes()
{
    auto selectionService = std::make_shared<SelectionService>();
    auto settingsService = std::make_shared<SettingsService>();
    auto mixerService = std::make_shared<MixerService>();
    EditorService editorService { selectionService, settingsService, std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };
    editorService.setMixerService(mixerService);

    // Create a pattern with notes on different tracks/columns
    editorService.initialize();

    // Track 0, Column 0: Note 60
    editorService.requestPosition(0, 0, 0, 0, 0);
    editorService.requestNoteOnAtCurrentPosition(1, 5, 100); // C5 -> 60

    // Track 0, Column 1 (add column first)
    editorService.requestNewColumn(0);
    editorService.requestPosition(0, 0, 1, 0, 0);
    editorService.requestNoteOnAtCurrentPosition(3, 5, 100); // D5 -> 62

    // Track 1, Column 0: Note 64
    editorService.requestPosition(0, 1, 0, 0, 0);
    editorService.requestNoteOnAtCurrentPosition(5, 5, 100); // E5 -> 64

    // 1. Verify all notes are returned initially
    {
        const auto notesTrack0 = editorService.midiNotesAtPosition(0, 0, 0);
        QCOMPARE(notesTrack0.size(), 2);
        // Ordering depends on implementation, but likely 60, 62
        bool found60 = std::find(notesTrack0.begin(), notesTrack0.end(), 60) != notesTrack0.end();
        bool found62 = std::find(notesTrack0.begin(), notesTrack0.end(), 62) != notesTrack0.end();
        QVERIFY(found60);
        QVERIFY(found62);

        const auto notesTrack1 = editorService.midiNotesAtPosition(0, 1, 0);
        QCOMPARE(notesTrack1.size(), 1);
        QCOMPARE(notesTrack1[0], 64);
    }

    // 2. Mute Track 0 -> Should return empty list for Track 0
    mixerService->muteTrack(0, true);
    {
        const auto notesTrack0 = editorService.midiNotesAtPosition(0, 0, 0);
        QVERIFY(notesTrack0.empty());
    }
    mixerService->muteTrack(0, false);

    // 3. Mute Track 0, Column 0 -> Should return only Note 62 (from Column 1)
    mixerService->muteColumn(0, 0, true);
    {
        const auto notesTrack0 = editorService.midiNotesAtPosition(0, 0, 0);
        QCOMPARE(notesTrack0.size(), 1);
        QCOMPARE(notesTrack0[0], 62);
    }
    mixerService->muteColumn(0, 0, false);

    // 4. Solo Track 0, Column 0 -> Should return only Note 60
    mixerService->soloColumn(0, 0, true);
    {
        const auto notesTrack0 = editorService.midiNotesAtPosition(0, 0, 0);
        QCOMPARE(notesTrack0.size(), 1);
        QCOMPARE(notesTrack0[0], 60);
    }
    mixerService->soloColumn(0, 0, false);

    // 5. Solo Track 1 -> Should return notes for Track 1, but Track 0 might still return notes if logic only checks "shouldTrackPlay" for THAT track.
    // wait, mixerService->shouldTrackPlay(0) should be FALSE if Track 1 is soloed.
    mixerService->soloTrack(1, true);
    {
        const auto notesTrack0 = editorService.midiNotesAtPosition(0, 0, 0);
        QVERIFY(notesTrack0.empty());

        const auto notesTrack1 = editorService.midiNotesAtPosition(0, 1, 0);
        QCOMPARE(notesTrack1.size(), 1);
        QCOMPARE(notesTrack1[0], 64);
    }
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::EditorServiceTest)
