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

namespace noteahead {

void TrackSettingsModelTest::test_initialState_shouldHaveExpectedDefaults()
{
    TrackSettingsModel model;

    QCOMPARE(model.channel(), static_cast<uint8_t>(0));
    QCOMPARE(model.volume(), static_cast<uint8_t>(127));
    QCOMPARE(model.cutoff(), static_cast<uint8_t>(127));
    QCOMPARE(model.pan(), static_cast<uint8_t>(64));
    QCOMPARE(model.delay(), 0);
    QVERIFY(!model.bankEnabled());
    QVERIFY(!model.volumeEnabled());
    QVERIFY(!model.cutoffEnabled());
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

void TrackSettingsModelTest::test_setVolumeEnabled_shouldUpdateAndEmitSignal()
{
    TrackSettingsModel model;

    QSignalSpy spy { &model, &TrackSettingsModel::volumeEnabledChanged };
    model.setVolumeEnabled(true);

    QVERIFY(model.volumeEnabled());
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
    instrumentSettings.predefinedMidiCcSettings.cutoff = 90;
    instrumentSettings.predefinedMidiCcSettings.pan = 64;
    instrumentSettings.predefinedMidiCcSettings.volume = 100;
    instrumentSettings.sendMidiClock = true;
    instrumentSettings.delay = std::chrono::milliseconds(120);
    instrument.setSettings(instrumentSettings);

    QSignalSpy spy { &model, &TrackSettingsModel::instrumentDataReceived };

    model.setInstrumentData(instrument);

    QCOMPARE(model.portName(), QString("TestPort"));
    QCOMPARE(model.channel(), static_cast<uint8_t>(5));
    QCOMPARE(model.patchEnabled(), true);
    QCOMPARE(model.patch(), static_cast<uint8_t>(42));
    QCOMPARE(model.bankEnabled(), true);
    QCOMPARE(model.bankLsb(), static_cast<uint8_t>(1));
    QCOMPARE(model.bankMsb(), static_cast<uint8_t>(2));
    QCOMPARE(model.bankByteOrderSwapped(), true);
    QCOMPARE(model.cutoffEnabled(), true);
    QCOMPARE(model.cutoff(), static_cast<uint8_t>(90));
    QCOMPARE(model.panEnabled(), true);
    QCOMPARE(model.pan(), static_cast<uint8_t>(64));
    QCOMPARE(model.volumeEnabled(), true);
    QCOMPARE(model.volume(), static_cast<uint8_t>(100));
    QCOMPARE(model.sendMidiClock(), true);
    QCOMPARE(model.delay(), 120);

    QCOMPARE(spy.count(), 1);
}

void TrackSettingsModelTest::test_toInstrument_shouldReturnInstrumentWithDefaultSettings()
{
    TrackSettingsModel model;
    // Setup default values for TrackSettingsModel (or use mock data)
    model.setPortName("Test Port");
    model.setChannel(1);

    const auto instrument = model.toInstrument();

    // Check if the instrument was created correctly
    QCOMPARE(instrument->midiAddress().channel(), 1); // Check channel
    QVERIFY(!instrument->settings().patch.has_value()); // Check default patch
    QVERIFY(!instrument->settings().bank.has_value()); // Check bank values
}

void TrackSettingsModelTest::test_toInstrument_shouldApplyPatchWhenPatchEnabled()
{
    TrackSettingsModel model;
    model.setPortName("Test Port");
    model.setChannel(1);
    model.setPatchEnabled(true);
    model.setPatch(5); // Set patch value to test

    const auto instrument = model.toInstrument();

    // Verify patch is set correctly
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

    // Verify bank settings
    QVERIFY(instrument->settings().bank.has_value());
    QCOMPARE(instrument->settings().bank->lsb, 10);
    QCOMPARE(instrument->settings().bank->msb, 20);
    QCOMPARE(instrument->settings().bank->byteOrderSwapped, true);
}

void TrackSettingsModelTest::test_toInstrument_shouldApplyCutoffWhenCutoffEnabled()
{
    TrackSettingsModel model;
    model.setPortName("Test Port");
    model.setChannel(1);
    model.setCutoffEnabled(true);
    model.setCutoff(50);

    const auto instrument = model.toInstrument();

    // Verify cutoff setting
    QCOMPARE(instrument->settings().predefinedMidiCcSettings.cutoff, 50);
}

void TrackSettingsModelTest::test_toInstrument_shouldApplyPanWhenPanEnabled()
{
    TrackSettingsModel model;
    model.setPortName("Test Port");
    model.setChannel(1);
    model.setPanEnabled(true);
    model.setPan(100);

    const auto instrument = model.toInstrument();

    // Verify pan setting
    QCOMPARE(instrument->settings().predefinedMidiCcSettings.pan, 100);
}

void TrackSettingsModelTest::test_toInstrument_shouldApplyVolumeWhenVolumeEnabled()
{
    TrackSettingsModel model;
    model.setPortName("Test Port");
    model.setChannel(1);
    model.setVolumeEnabled(true);
    model.setVolume(80);

    const auto instrument = model.toInstrument();

    // Verify volume setting
    QCOMPARE(instrument->settings().predefinedMidiCcSettings.volume, 80);
}

void TrackSettingsModelTest::test_toInstrument_shouldApplyMidiClockAndDelayWhenEnabled()
{
    TrackSettingsModel model;
    model.setPortName("Test Port");
    model.setChannel(1);
    model.setSendMidiClock(true);
    model.setDelay(200);

    const auto instrument = model.toInstrument();

    // Verify MIDI clock and delay settings
    QCOMPARE(instrument->settings().sendMidiClock, true);
    QCOMPARE(instrument->settings().delay, std::chrono::milliseconds { 200 });
}

void TrackSettingsModelTest::test_toInstrument_setMidiCc_shouldEnableMidiCcSetting()
{
    TrackSettingsModel model;
    model.setMidiCcController(0, 1);
    model.setMidiCcValue(0, 42);

    auto instrument = model.toInstrument();

    QCOMPARE(instrument->settings().midiCcSettings.at(0).enabled(), false);
    QCOMPARE(instrument->settings().midiCcSettings.at(0).controller(), 1);
    QCOMPARE(instrument->settings().midiCcSettings.at(0).value(), 42);

    model.setMidiCcEnabled(0, true);

    instrument = model.toInstrument();

    QCOMPARE(instrument->settings().midiCcSettings.at(0).enabled(), true);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::TrackSettingsModelTest)
