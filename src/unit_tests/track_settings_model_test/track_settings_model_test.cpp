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

#include "track_settings_model_test.hpp"

#include "../../application/models/track_settings_model.hpp"

#include <QSignalSpy>
#include <QTest>

namespace noteahead {

void TrackSettingsModelTest::test_initialState_shouldHaveExpectedDefaults()
{
    TrackSettingsModel model;

    QCOMPARE(model.channel(), static_cast<uint8_t>(0));
    QCOMPARE(model.delay(), 0);
    QVERIFY(!model.bankEnabled());
    QCOMPARE(model.midiCcModel()->rowCount(), 0);
}

void TrackSettingsModelTest::test_setPortName_shouldUpdatePortName()
{
    TrackSettingsModel model;

    QSignalSpy spy { &model, &TrackSettingsModel::portNameChanged };
    model.setPortName("My MIDI Device");

    QCOMPARE(model.portName(), QString { "My MIDI Device" });
    QCOMPARE(spy.count(), 1);
}

void TrackSettingsModelTest::test_setChannel_shouldUpdateChannel()
{
    TrackSettingsModel model;

    QSignalSpy spy { &model, &TrackSettingsModel::channelChanged };
    model.setChannel(10);

    QCOMPARE(model.channel(), static_cast<uint8_t>(10));
    QCOMPARE(spy.count(), 1);
}

void TrackSettingsModelTest::test_setTrackIndex_shouldUpdateTrackIndex()
{
    TrackSettingsModel model;

    QSignalSpy spy { &model, &TrackSettingsModel::trackIndexChanged };
    model.setTrackIndex(2);

    QCOMPARE(model.trackIndex(), size_t { 2 });
    QCOMPARE(spy.count(), 1);
}

void TrackSettingsModelTest::test_setInstrumentData_shouldUpdateRelevantFields()
{
    TrackSettingsModel model;

    Instrument instrument { "TestPort" };
    auto address = instrument.midiAddress();
    address.setChannel(5);
    instrument.setMidiAddress(address);

    InstrumentSettings instrumentSettings;
    instrumentSettings.patch = 42;
    instrumentSettings.bank = InstrumentSettings::Bank { 1, 2, true };
    instrumentSettings.timing.sendMidiClock = true;
    instrumentSettings.timing.sendTransport = true;
    instrumentSettings.timing.delay = std::chrono::milliseconds(120);
    instrumentSettings.midiCcSettings.emplace_back(true, 10, 20);
    instrument.setSettings(instrumentSettings);

    QSignalSpy spy { &model, &TrackSettingsModel::instrumentDataReceived };

    model.setInstrumentData(instrument);

    QCOMPARE(model.portName(), QString { "TestPort" });
    QCOMPARE(model.channel(), static_cast<uint8_t>(5));
    QCOMPARE(model.patchEnabled(), true);
    QCOMPARE(model.patch(), static_cast<uint8_t>(42));
    QCOMPARE(model.bankEnabled(), true);
    QCOMPARE(model.bankLsb(), static_cast<uint8_t>(1));
    QCOMPARE(model.bankMsb(), static_cast<uint8_t>(2));
    QCOMPARE(model.bankByteOrderSwapped(), true);
    QCOMPARE(model.sendMidiClock(), true);
    QCOMPARE(model.sendTransport(), true);
    QCOMPARE(model.delay(), 120);
    QCOMPARE(model.midiCcModel()->rowCount(), 1);
    QCOMPARE(model.midiCcModel()->data(model.midiCcModel()->index(0), static_cast<int>(MidiCcSelectionModel::Roles::ControllerRole)).toInt(), 10);

    QCOMPARE(spy.count(), 1);
}

void TrackSettingsModelTest::test_toInstrument_shouldReturnInstrumentWithDefaultSettings()
{
    TrackSettingsModel model;
    model.setPortName("Test Port");
    model.setChannel(1);

    const auto instrument = model.toInstrument();

    QCOMPARE(instrument->midiAddress().channel(), 1);
    QVERIFY(!instrument->settings().patch.has_value());
    QVERIFY(!instrument->settings().bank.has_value());
    QVERIFY(instrument->settings().midiCcSettings.empty());
}

void TrackSettingsModelTest::test_toInstrument_shouldApplyPatchWhenPatchEnabled()
{
    TrackSettingsModel model;
    model.setPortName("Test Port");
    model.setChannel(1);
    model.setPatchEnabled(true);
    model.setPatch(5);

    const auto instrument = model.toInstrument();

    QCOMPARE(instrument->settings().patch, 5);
}

void TrackSettingsModelTest::test_toInstrument_shouldApplyBankSettingsWhenBankEnabled()
{
    TrackSettingsModel model;
    model.setPortName("Test Port");
    model.setChannel(1);
    model.setBankEnabled(true);
    model.setBankLsb(10);
    model.setBankMsb(20);
    model.setBankByteOrderSwapped(true);

    const auto instrument = model.toInstrument();

    QVERIFY(instrument->settings().bank.has_value());
    QCOMPARE(instrument->settings().bank->lsb, 10);
    QCOMPARE(instrument->settings().bank->msb, 20);
    QCOMPARE(instrument->settings().bank->byteOrderSwapped, true);
}

void TrackSettingsModelTest::test_toInstrument_shouldApplyMidiClockAndDelayWhenEnabled()
{
    TrackSettingsModel model;
    model.setPortName("Test Port");
    model.setChannel(1);
    model.setSendMidiClock(true);
    model.setDelay(200);

    const auto instrument = model.toInstrument();

    QCOMPARE(instrument->settings().timing.sendMidiClock, true);
    QCOMPARE(instrument->settings().timing.delay, std::chrono::milliseconds { 200 });
}

void TrackSettingsModelTest::test_toInstrument_setMidiCc_shouldEnableMidiCcSetting()
{
    TrackSettingsModel model;
    const auto ccModel = model.midiCcModel();

    ccModel->addMidiCcSetting(0, 0);
    ccModel->setData(ccModel->index(0), 1, static_cast<int>(MidiCcSelectionModel::Roles::ControllerRole));
    ccModel->setData(ccModel->index(0), 42, static_cast<int>(MidiCcSelectionModel::Roles::ValueRole));

    auto instrument = model.toInstrument();

    QCOMPARE(instrument->settings().midiCcSettings.at(0).enabled(), true);
    QCOMPARE(instrument->settings().midiCcSettings.at(0).controller(), 1);
    QCOMPARE(instrument->settings().midiCcSettings.at(0).value(), 42);

    ccModel->setData(ccModel->index(0), false, static_cast<int>(MidiCcSelectionModel::Roles::EnabledRole));
    instrument = model.toInstrument();
    QCOMPARE(instrument->settings().midiCcSettings.at(0).enabled(), false);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::TrackSettingsModelTest)
