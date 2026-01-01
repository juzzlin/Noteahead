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

#include "midi_cc_selection_model_test.hpp"

#include "../../application/models/midi_cc_selection_model.hpp"

#include <QSignalSpy>

namespace noteahead {

void MidiCcSelectionModelTest::initTestCase()
{
    model = new MidiCcSelectionModel;
}

void MidiCcSelectionModelTest::cleanupTestCase()
{
    delete model;
}

void MidiCcSelectionModelTest::test_setData_shouldUpdateModel()
{
    model->setMidiCcSettings({});
    model->addMidiCcSetting(0, 0);

    QCOMPARE(model->rowCount(), 1);
    auto index = model->index(0, 0);

    // Verify initial state from addMidiCcSetting
    QCOMPARE(model->data(index, static_cast<int>(MidiCcSelectionModel::Roles::EnabledRole)).toBool(), true);

    // Test changing values
    QVERIFY(model->setData(index, 74, static_cast<int>(MidiCcSelectionModel::Roles::ControllerRole)));
    QVERIFY(model->setData(index, 127, static_cast<int>(MidiCcSelectionModel::Roles::ValueRole)));
    QVERIFY(model->setData(index, false, static_cast<int>(MidiCcSelectionModel::Roles::EnabledRole)));

    QCOMPARE(model->data(index, static_cast<int>(MidiCcSelectionModel::Roles::ControllerRole)).toInt(), 74);
    QCOMPARE(model->data(index, static_cast<int>(MidiCcSelectionModel::Roles::ValueRole)).toInt(), 127);
    QCOMPARE(model->data(index, static_cast<int>(MidiCcSelectionModel::Roles::EnabledRole)).toBool(), false);
}

void MidiCcSelectionModelTest::test_toString_shouldReturnNonEmptyString()
{
    const auto result = model->midiCcToString(1);
    QVERIFY(!result.isEmpty());
}

void MidiCcSelectionModelTest::test_settings_shouldRoundTripCorrectly()
{
    MidiCcSelectionModel::MidiCcSettingList settings = {
        MidiCcSetting { true, 10, 64 },
        MidiCcSetting { false, 74, 127 },
    };

    model->setMidiCcSettings(settings);

    QCOMPARE(model->rowCount(), 2);
    const auto result = model->midiCcSettings();
    for (size_t i = 0; i < result.size(); i++) {
        bool found = false;
        for(const auto& res : result) {
            if(res.controller() == settings[i].controller() && res.value() == settings[i].value()) {
                found = true;
                break;
            }
        }
        QVERIFY(found);
    }
}

void MidiCcSelectionModelTest::test_addAndRemoveSetting()
{
    model->setMidiCcSettings({});
    QCOMPARE(model->rowCount(), 0);

    model->addMidiCcSetting(10, 20);
    QCOMPARE(model->rowCount(), 1);
    QCOMPARE(model->data(model->index(0), static_cast<int>(MidiCcSelectionModel::Roles::ControllerRole)).toInt(), 10);
    QCOMPARE(model->data(model->index(0), static_cast<int>(MidiCcSelectionModel::Roles::ValueRole)).toInt(), 20);

    model->removeMidiCcSetting(0);
    QCOMPARE(model->rowCount(), 0);
}

void MidiCcSelectionModelTest::test_addDuplicateSetting()
{
    model->setMidiCcSettings({});
    model->addMidiCcSetting(94, 0);
    model->addMidiCcSetting(94, 4);
    
    QCOMPARE(model->rowCount(), 1);
    QCOMPARE(model->data(model->index(0), static_cast<int>(MidiCcSelectionModel::Roles::ValueRole)).toInt(), 4);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::MidiCcSelectionModelTest)
