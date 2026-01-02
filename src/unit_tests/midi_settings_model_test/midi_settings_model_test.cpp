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

#include "midi_settings_model_test.hpp"

#include "../../application/models/midi_settings_model.hpp"
#include "../../application/service/settings_service.hpp"

#include <QSignalSpy>

namespace noteahead {

void MidiSettingsModelTest::initTestCase()
{
    QCoreApplication::setOrganizationName("NoteaheadTest");
    QCoreApplication::setApplicationName("MidiSettingsModelTest");
}

void MidiSettingsModelTest::cleanupTestCase()
{
    QSettings settings {};
    settings.clear();
}

void MidiSettingsModelTest::test_initialState_shouldHaveExpectedDefaults()
{
    const auto settingsService { std::make_shared<SettingsService>() };
    const MidiSettingsModel model { settingsService };

    QCOMPARE(model.midiInPorts(), QStringList {});
    QCOMPARE(model.controllerPort(), settingsService->controllerPort());
    QCOMPARE(model.debugData(), QString { "" });
}

void MidiSettingsModelTest::test_setMidiInPorts_shouldUpdateAndEmitSignal()
{
    const auto settingsService { std::make_shared<SettingsService>() };
    MidiSettingsModel model { settingsService };
    QSignalSpy spy { &model, &MidiSettingsModel::midiInPortsChanged };

    const QStringList ports { "Port 1", "Port 2" };
    model.setMidiInPorts(ports);

    QCOMPARE(model.midiInPorts(), ports);
    QCOMPARE(spy.count(), 1);
}

void MidiSettingsModelTest::test_setControllerPort_shouldUpdateAndEmitSignalAndSaveToSettings()
{
    const auto settingsService { std::make_shared<SettingsService>() };
    MidiSettingsModel model { settingsService };
    QSignalSpy spy { &model, &MidiSettingsModel::controllerPortChanged };

    const QString portName { "Test Port" };
    model.setControllerPort(portName);

    QCOMPARE(model.controllerPort(), portName);
    QCOMPARE(settingsService->controllerPort(), portName);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.first().at(0).toString(), portName);
}

void MidiSettingsModelTest::test_setDebugData_shouldUpdateAndEmitSignal()
{
    const auto settingsService { std::make_shared<SettingsService>() };
    MidiSettingsModel model { settingsService };
    QSignalSpy spy { &model, &MidiSettingsModel::debugDataChanged };

    const QString debugData { "Some debug data" };
    model.setDebugData(debugData);

    QCOMPARE(model.debugData(), debugData);
    QCOMPARE(spy.count(), 1);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::MidiSettingsModelTest)
