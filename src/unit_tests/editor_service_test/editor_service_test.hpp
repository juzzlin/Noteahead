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

#ifndef EDITOR_SERVICE_TEST_HPP
#define EDITOR_SERVICE_TEST_HPP

#include <QTest>

namespace cacophony {

class EditorServiceTest : public QObject
{
    Q_OBJECT

private slots:

    void test_defaultSong_shouldReturnCorrectProperties();

    void test_defaultSong_shouldNotHaveNoteData();

    void test_requestDigitSetAtCurrentPosition_velocity_shouldChangeVelocity();

    void test_requestNewColumn_shouldAddNewColumn();

    void test_requestNoteDeletionAtCurrentPosition_shouldDeleteNoteData();

    void test_requestNoteOnAtCurrentPosition_shouldChangeNoteData();

    void test_requestNoteOnAtCurrentPosition_notOnNoteColumn_shouldNotChangeNoteData();

    void test_requestPosition_invalidPosition_shouldNotChangePosition();

    void test_requestPosition_validPosition_shouldChangePosition();

    void test_requestScroll_shouldChangePosition();

    void test_requestTrackFocus_shouldChangePosition();

    void test_requestTrackFocus_shouldNotChangePosition();

    void test_setCurrentLineCount_shouldSetLineCount();

    void test_setTrackName_shouldChangeTrackName();

    void test_toXmlFromXml_songProperties();

    void test_toXmlFromXml_noteData_noteOn();

    void test_toXmlFromXml_noteData_noteOff();
};

} // namespace cacophony

#endif // EDITOR_SERVICE_TEST_HPP
