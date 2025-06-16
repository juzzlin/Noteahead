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

#ifndef EDITOR_SERVICE_TEST_HPP
#define EDITOR_SERVICE_TEST_HPP

#include <QTest>

namespace noteahead {

class EditorServiceTest : public QObject
{
    Q_OBJECT

private slots:

    void test_initialize_shouldInitializeCorrectly();

    void test_defaultSong_shouldReturnCorrectProperties();
    void test_defaultSong_shouldNotHaveNoteData();

    void test_insertPattern_shouldInsertPattern();
    void test_removePattern_shouldRemovePattern();

    void test_columnCutPaste_equalSizes_shouldCopyColumn();
    void test_columnCutPaste_shorterTarget_shouldCopyColumn();
    void test_columnCopyPaste_equalSizes_shouldCopyColumn();
    void test_columnCopyPaste_shorterTarget_shouldCopyColumn();

    void test_trackCutPaste_equalSizes_shouldCopyTrack();
    void test_trackCutPaste_shorterTarget_shouldCopyTrack();
    void test_trackCopyPaste_equalSizes_shouldCopyTrack();
    void test_trackCopyPaste_shorterTarget_shouldCopyTrack();

    void test_patternCutPaste_equalSizes_shouldCopyPattern();
    void test_patternCutPaste_shorterTarget_shouldCopyPattern();
    void test_patternCopyPaste_equalSizes_shouldCopyPattern();
    void test_patternCopyPaste_shorterTarget_shouldCopyPattern();
    void test_patternCopyPaste_trackDeleted_shouldCopyPattern();

    void test_selectionCutPaste_shouldCopySelection();
    void test_selectionCopyPaste_shouldCopySelection();

    void test_requestCursorLeft_shouldWrapCorrectly();

    void test_requestDigitSetAtCurrentPosition_velocity_shouldChangeVelocity();

    void test_requestHorizontalScrollPositionChange_shouldChangePosition();

    void test_requestNewColumn_shouldAddNewColumn();
    void test_requestColumnDeletion_shouldDeleteColumn();

    void test_requestNewTrackToRight_shouldAddNewTrack();
    void test_requestTrackDeletion_shouldDeleteTrack();

    void test_requestNoteDeletionAtCurrentPosition_shouldDeleteNoteData();
    void test_requestNoteDeletionAtCurrentPosition_shouldDeleteNoteData_shouldShiftNotes();
    void test_requestNoteInsertionAtCurrentPosition_shouldInsertNoteData();
    void test_requestNoteOnAtCurrentPosition_shouldChangeNoteData();
    void test_requestNoteOnAtCurrentPosition_notOnNoteColumn_shouldNotChangeNoteData();

    void test_requestColumnTranspose_shouldTransposeColumn();
    void test_requestTrackTranspose_shouldTransposeTrack();
    void test_requestPatternTranspose_shouldTransposePattern();
    void test_requestSelectionTranspose_shouldTransposeSelection();

    void test_requestLinearVelocityInterpolation_shouldInterpolateVelocities();

    void test_requestPosition_invalidPosition_shouldNotChangePosition();
    void test_requestPosition_validPosition_shouldChangePosition();
    void test_resetSongPosition_firstTrackRemoved_shouldResetPosition();

    void test_requestScroll_shouldChangePosition();
    void test_requestScroll_shouldChangeCurrentTime();

    void test_requestTrackFocus_shouldChangePosition();
    void test_requestTrackFocus_shouldNotChangePosition();

    void test_setCurrentLineCount_shouldSetLineCount();

    void test_setCurrentPattern_shouldCreatePattern();
    void test_setCurrentPattern_addColumnFirst_shouldCreatePattern();

    void test_setInstrumentSettings_shouldSetInstrumentSettings();
    void test_removeInstrumentSettings_shouldRemoveInstrumentSettings();

    void test_setPatternAtSongPosition_shouldCreatePattern();
    void test_setSongPosition_shouldChangePattern();
    void test_setSongPosition_trackDeleted_shouldCreatePattern();

    void test_setPatternName_shouldChangePatternName();
    void test_setTrackName_shouldChangeTrackName();
    void test_setColumnName_shouldChangeColumnName();

    void test_toXmlFromXml_addTrack_shouldLoadSong();
    void test_toXmlFromXml_columnName_shouldLoadColumnName();
    void test_toXmlFromXml_instrumentSettings_shouldParseInstrumentSettings();
    void test_toXmlFromXml_instrument_shouldParseInstrument();
    void test_toXmlFromXml_automationService_midiCc_shouldLoadAutomationService();
    void test_toXmlFromXml_automationService_pitchBend_shouldLoadAutomationService();
    void test_toXmlFromXml_mixerService_shouldLoadMixerService();
    void test_toXmlFromXml_noteData_noteOff();
    void test_toXmlFromXml_noteData_noteOn();
    void test_toXmlFromXml_noteData_delay_shouldSaveAndLoadDelay();
    void test_toXmlFromXml_playOrder();
    void test_toXmlFromXml_removeTrack_shouldLoadSong();
    void test_toXmlFromXml_songProperties();
    void test_toXmlFromXml_trackName_shouldLoadTrackName();

    void test_toXmlFromXml_template_shouldLoadTemplate();
    void test_toXmlFromXml_differentSongs_shouldLoadSongs();

    void test_velocityAtPosition_shouldReturnCorrectVelocity();
};

} // namespace noteahead

#endif // EDITOR_SERVICE_TEST_HPP
