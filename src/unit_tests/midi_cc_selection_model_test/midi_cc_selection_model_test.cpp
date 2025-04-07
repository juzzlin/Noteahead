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

void MidiCcSelectionModelTest::test_controller_shouldBeSetAndRetrieved()
{
    model->setMidiCcController(0, 74);
    QCOMPARE(model->midiCcController(0), 74);
}

void MidiCcSelectionModelTest::test_value_shouldBeSetAndRetrieved()
{
    model->setMidiCcValue(0, 127);
    QCOMPARE(model->midiCcValue(0), 127);
}

void MidiCcSelectionModelTest::test_enabled_shouldBeSetAndRetrieved()
{
    model->setMidiCcEnabled(0, true);
    QVERIFY(model->midiCcEnabled(0));

    model->setMidiCcEnabled(0, false);
    QVERIFY(!model->midiCcEnabled(0));
}

void MidiCcSelectionModelTest::test_toString_shouldReturnNonEmptyString()
{
    model->setMidiCcController(1, 1);
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

    QCOMPARE(model->midiCcSettings().size(), 2);
    for (size_t i = 0; i < model->midiCcSettings().size(); i++) {
        const auto setting = model->midiCcSettings().at(i);
        QCOMPARE(setting.enabled(), settings.at(i).enabled());
        QCOMPARE(setting.controller(), settings.at(i).controller());
        QCOMPARE(setting.value(), settings.at(i).value());
    }
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::MidiCcSelectionModelTest)
