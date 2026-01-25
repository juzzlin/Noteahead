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

#ifndef TRACK_SETTINGS_MODEL_TEST_HPP
#define TRACK_SETTINGS_MODEL_TEST_HPP

#include <QObject>

namespace noteahead {

class TrackSettingsModelTest : public QObject
{
    Q_OBJECT

private slots:
    void test_initialState_shouldHaveExpectedDefaults();

    void test_setPortName_shouldUpdatePortName();
    void test_setChannel_shouldUpdateChannel();
    void test_setTrackIndex_shouldUpdateTrackIndex();
    void test_setInstrumentData_shouldUpdateRelevantFields();

    void test_toInstrument_shouldReturnInstrumentWithDefaultSettings();
    void test_toInstrument_shouldApplyPatchWhenPatchEnabled();
    void test_toInstrument_shouldApplyBankSettingsWhenBankEnabled();
    void test_toInstrument_shouldApplyMidiClockAndDelayWhenEnabled();
    void test_toInstrument_setMidiCc_shouldEnableMidiCcSetting();
};

} // namespace noteahead

#endif // TRACK_SETTINGS_MODEL_TEST_HPP
