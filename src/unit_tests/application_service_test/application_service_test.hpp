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

#ifndef APPLICATION_SERVICE_TEST_HPP
#define APPLICATION_SERVICE_TEST_HPP

#include <QObject>

namespace noteahead {

class ApplicationServiceTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void test_initialState_shouldBeCorrect();
    void test_applicationProperties_shouldMatchConstants();
    void test_editMode_shouldToggleCorrectly();
    void test_recentFiles_shouldBeManagedCorrectly();
    void test_stateMachineInteractions_shouldTriggerCorrectActions();
    void test_liveNoteLogic_shouldTriggerCorrectSignals();
    void test_importMidiFile_shouldAddRecentFile();
    void test_isMidiFile_shouldDetectCorrectExtensions();
    void test_requestAlertDialog_shouldEmitSignal();
};

} // namespace noteahead

#endif // APPLICATION_SERVICE_TEST_HPP
