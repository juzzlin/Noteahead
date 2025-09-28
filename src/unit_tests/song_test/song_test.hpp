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

#ifndef SONG_TEST_HPP
#define SONG_TEST_HPP

#include <QTest>

namespace noteahead {

class SongTest : public QObject
{
    Q_OBJECT

private slots:

    void test_autoNoteOffOffset_shouldCalculateCorrectOffset();

    void test_createPattern_columnAdded_shouldCreatePattern();

    void test_hasData_emptySong_hasNoData();
    void test_hasData_noteOnAdded_shouldHaveData();
    void test_hasData_noteOffAdded_shouldHaveData();

    void test_nextNoteDataOnSameColumn_noteOn_shouldFindNoteData();
    void test_nextNoteDataOnSameColumn_noteOff_shouldFindNoteData();

    void test_prevNoteDataOnSameColumn_noteOn_shouldFindNoteData();
    void test_prevNoteDataOnSameColumn_noteOff_shouldFindNoteData();

    void test_renderToEvents_clockEvents_shouldRenderClockEvents();
    void test_renderToEvents_positiveDelaySet_shouldApplyDelay();
    void test_renderToEvents_negativeDelaySet_shouldApplyShiftedDelay();
    void test_renderToEvents_noEvents_shouldAddStartAndEndOfSong();
    void test_renderToEvents_noEvents_transportEnabled_shouldAddStartAndEndOfSong();
    void test_renderToEvents_noteOff_shouldMapNoteOff();
    void test_renderToEvents_playOrderSet_shouldRenderMultiplePatterns();
    void test_renderToEvents_singleEvent_shouldRenderEvent();
    void test_renderToEvents_sameColumn_shouldAddNoteOff();

    void test_renderToEvents_chordAutomation_shouldRenderChordNotes();

    void test_renderToEvents_transposeSet_shouldApplyTranspose();
    void test_renderToEvents_velocityJitterSet_shouldApplyVelocityJitter();
    void test_renderToEvents_customNoteOffOffsetSet_shouldApplyCorrectOffset();

    void test_addTrack_shouldUseSmallestFreeId();

    void test_trackByName_shouldReturnTrack();
    void test_columnByName_shouldReturnColumn();
};

} // namespace noteahead

#endif // SONG_TEST_HPP
