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

#ifndef SONG_OVERVIEW_BUILDER_TEST_HPP
#define SONG_OVERVIEW_BUILDER_TEST_HPP

#include <QObject>

namespace noteahead {

class SongOverviewBuilderTest : public QObject
{
    Q_OBJECT

private slots:
    void test_build_note_shouldLastUntilTheNextNote();
    void test_build_note_shouldEndAtANoteOff();
    void test_build_note_shouldLastUntilThePatternEnd();
    void test_build_note_limitLine_shouldBeTheEventAfterItsNoteOff();
    void test_build_note_withDelay_shouldStartInsideItsLine();
    void test_build_columnsAndTracks_shouldKeepTheirOwnNotes();
    void test_build_playOrder_shouldBuildARepeatedPatternOnce();
    void test_build_patternsOfDifferentLengths_shouldFollowOnTheTimeline();
    void test_build_skippedPosition_shouldBeMarked();
    void test_build_pitchRange_shouldSpanTheTracksNotes();
    void test_build_emptySong_shouldHaveADefaultRange();
    void test_build_automation_shouldBeACurveOfItsColumn();
    void test_build_automation_disabled_shouldBeLeftOut();
    void test_build_automation_shouldNameItsAutomation();
    void test_build_automation_repeatedPattern_shouldBeBuiltOnce();
    void test_build_automation_withoutAutomationService_shouldHaveNoCurves();
    void test_controller_automationAdded_shouldRebuild();

    void test_noteAt_overlappingNotes_shouldReturnTheTopOne();
    void test_noteAt_outsideTolerance_shouldReturnNothing();

    void test_controller_targetAt_note_shouldGoToItsTrackColumnAndStart();
    void test_controller_targetAt_emptyLane_shouldGoToTheLanesTrack();
    void test_controller_targetAt_strip_shouldKeepTheCursorsTrack();
    void test_controller_targetAt_repeatedPattern_shouldGoToTheClickedRow();
    void test_controller_playhead_whenStopped_shouldFollowTheEditor();
    void test_controller_inactive_shouldNotBuild();
    void test_controller_notesStartingIn_shouldReturnTheNotesOfTheLines();
};

} // namespace noteahead

#endif // SONG_OVERVIEW_BUILDER_TEST_HPP
