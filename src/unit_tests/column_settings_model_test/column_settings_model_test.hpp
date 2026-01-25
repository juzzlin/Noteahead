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

#ifndef COLUMN_SETTINGS_MODEL_TEST_HPP
#define COLUMN_SETTINGS_MODEL_TEST_HPP

#include <QObject>

namespace noteahead {

class ColumnSettingsModelTest : public QObject
{
    Q_OBJECT

private slots:
    void test_initialState_shouldHaveExpectedDefaults();
    void test_setTrackIndex_shouldUpdateAndEmitSignal();
    void test_setColumnIndex_shouldUpdateAndEmitSignal();
    void test_setDelay_shouldUpdateAndEmitSignal();
    void test_setChordNote1Offset_shouldUpdateAndEmitSignal();
    void test_setChordNote1Velocity_shouldUpdateAndEmitSignal();
    void test_setChordNote1Delay_shouldUpdateAndEmitSignal();
    void test_setChordNote2Offset_shouldUpdateAndEmitSignal();
    void test_setChordNote2Velocity_shouldUpdateAndEmitSignal();
    void test_setChordNote2Delay_shouldUpdateAndEmitSignal();
    void test_setChordNote3Offset_shouldUpdateAndEmitSignal();
    void test_setChordNote3Velocity_shouldUpdateAndEmitSignal();
    void test_setChordNote3Delay_shouldUpdateAndEmitSignal();
    void test_save_shouldEmitSaveRequestedWithCorrectData();
};

} // namespace noteahead

#endif // COLUMN_SETTINGS_MODEL_TEST_HPP
