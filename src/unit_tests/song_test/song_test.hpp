// This file is part of Cacophony.
// Copyright (C) 2025 Jussi Lind <jussi.lind@iki.fi>
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

#ifndef SONG_TEST_HPP
#define SONG_TEST_HPP

#include <QTest>

namespace cacophony {

class SongTest : public QObject
{
    Q_OBJECT

private slots:

    void test_hasData_emptySong_hasNoData();

    void test_hasData_noteOnAdded_shouldHaveData();

    void test_hasData_noteOffAdded_shouldHaveData();

    void test_nextNoteDataOnSameColumn_noteOn_shouldFindNoteData();

    void test_nextNoteDataOnSameColumn_noteOff_shouldFindNoteData();

    void test_prevNoteDataOnSameColumn_noteOn_shouldFindNoteData();

    void test_prevNoteDataOnSameColumn_noteOff_shouldFindNoteData();

    void test_renderToEvents_noEvents_shouldAddStartAndEndOfSong();

    void test_renderToEvents_singleEvent_shouldRenderEvent();

    void test_renderToEvents_sameColumn_shouldAddNoteOff();
};

} // namespace cacophony

#endif // SONG_TEST_HPP
