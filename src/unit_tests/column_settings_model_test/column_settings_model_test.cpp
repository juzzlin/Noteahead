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

#include "column_settings_model_test.hpp"

#include <QSignalSpy>

namespace noteahead {

void ColumnSettingsModelTest::test_initialValues()
{
    ColumnSettingsModel model;
    QCOMPARE(model.trackIndex(), 0);
    QCOMPARE(model.columnIndex(), 0);
    QCOMPARE(model.delay(), 0);
    QCOMPARE(model.chordNote1Offset(), 0);
    QCOMPARE(model.chordNote1Velocity(), 100);
    QCOMPARE(model.chordNote1Delay(), 0);
}

void ColumnSettingsModelTest::test_settersAndGetters()
{
    ColumnSettingsModel model;

    model.setTrackIndex(42);
    QCOMPARE(model.trackIndex(), 42);

    model.setColumnIndex(7);
    QCOMPARE(model.columnIndex(), 7);

    model.setDelay(123);
    QCOMPARE(model.delay(), 123);

    model.setChordNote1Offset(12);
    QCOMPARE(model.chordNote1Offset(), 12);

    model.setChordNote1Velocity(150);
    QCOMPARE(model.chordNote1Velocity(), 150);

    model.setChordNote1Delay(500);
    QCOMPARE(model.chordNote1Delay(), 500);

    model.setChordNote2Offset(-12);
    QCOMPARE(model.chordNote2Offset(), -12);

    model.setChordNote2Velocity(50);
    QCOMPARE(model.chordNote2Velocity(), 50);

    model.setChordNote2Delay(250);
    QCOMPARE(model.chordNote2Delay(), 250);

    model.setChordNote3Offset(24);
    QCOMPARE(model.chordNote3Offset(), 24);

    model.setChordNote3Velocity(200);
    QCOMPARE(model.chordNote3Velocity(), 200);

    model.setChordNote3Delay(750);
    QCOMPARE(model.chordNote3Delay(), 750);

    model.setArpeggiatorEnabled(true);
    QCOMPARE(model.arpeggiatorEnabled(), true);

    model.setArpeggiatorPattern(2); // UpDown
    QCOMPARE(model.arpeggiatorPattern(), 2);

    model.setArpeggiatorEventsPerBeat(8);
    QCOMPARE(model.arpeggiatorEventsPerBeat(), 8);
}

void ColumnSettingsModelTest::test_signals()
{
    ColumnSettingsModel model;

    QSignalSpy trackIndexSpy { &model, &ColumnSettingsModel::trackIndexChanged };
    model.setTrackIndex(1);
    QCOMPARE(trackIndexSpy.count(), 1);

    QSignalSpy columnIndexSpy { &model, &ColumnSettingsModel::columnIndexChanged };
    model.setColumnIndex(1);
    QCOMPARE(columnIndexSpy.count(), 1);

    QSignalSpy delaySpy { &model, &ColumnSettingsModel::delayChanged };
    model.setDelay(1);
    QCOMPARE(delaySpy.count(), 1);

    QSignalSpy note1OffsetSpy { &model, &ColumnSettingsModel::chordNote1OffsetChanged };
    model.setChordNote1Offset(1);
    QCOMPARE(note1OffsetSpy.count(), 1);

    QSignalSpy note1VelocitySpy { &model, &ColumnSettingsModel::chordNote1VelocityChanged };
    model.setChordNote1Velocity(1);
    QCOMPARE(note1VelocitySpy.count(), 1);

    QSignalSpy note1DelaySpy { &model, &ColumnSettingsModel::chordNote1DelayChanged };
    model.setChordNote1Delay(1);
    QCOMPARE(note1DelaySpy.count(), 1);

    QSignalSpy arpeggiatorEnabledSpy { &model, &ColumnSettingsModel::arpeggiatorEnabledChanged };
    model.setArpeggiatorEnabled(true);
    QCOMPARE(arpeggiatorEnabledSpy.count(), 1);

    QSignalSpy arpeggiatorPatternSpy { &model, &ColumnSettingsModel::arpeggiatorPatternChanged };
    model.setArpeggiatorPattern(1);
    QCOMPARE(arpeggiatorPatternSpy.count(), 1);

    QSignalSpy arpeggiatorEventsPerBeatSpy { &model, &ColumnSettingsModel::arpeggiatorEventsPerBeatChanged };
    model.setArpeggiatorEventsPerBeat(1);
    QCOMPARE(arpeggiatorEventsPerBeatSpy.count(), 1);
}

void ColumnSettingsModelTest::test_reset_shouldResetToDefaultValues()
{
    ColumnSettingsModel model;
    model.setDelay(123);
    model.setChordNote1Offset(12);
    model.setArpeggiatorEnabled(true);

    model.reset();

    QCOMPARE(model.delay(), 0);
    QCOMPARE(model.chordNote1Offset(), 0);
    QCOMPARE(model.arpeggiatorEnabled(), false);
}

void ColumnSettingsModelTest::test_save_shouldEmitSaveRequestedWithCorrectData()
{
    qRegisterMetaType<ColumnSettings>("ColumnSettings");

    ColumnSettingsModel model;
    model.setTrackIndex(1);
    model.setColumnIndex(2);
    model.setDelay(666);
    model.setChordNote1Offset(4);
    model.setChordNote1Velocity(80);
    model.setChordNote1Delay(11);
    model.setChordNote2Offset(7);
    model.setChordNote2Velocity(60);
    model.setChordNote2Delay(22);
    model.setChordNote3Offset(12);
    model.setChordNote3Velocity(90);
    model.setChordNote3Delay(33);
    model.setArpeggiatorEnabled(true);
    model.setArpeggiatorPattern(1); // Down
    model.setArpeggiatorEventsPerBeat(8);

    QSignalSpy spy { &model, &ColumnSettingsModel::saveRequested };
    model.save();

    QCOMPARE(spy.count(), 1);
    const auto arguments = spy.takeFirst();
    QCOMPARE(arguments.at(0).toULongLong(), 1);
    QCOMPARE(arguments.at(1).toULongLong(), 2);

    const auto settings = qvariant_cast<ColumnSettings>(arguments.at(2));
    QCOMPARE(settings.delay.count(), 666);
    QCOMPARE(settings.chordAutomationSettings.note1.offset, 4);
    QCOMPARE(settings.chordAutomationSettings.note1.velocity, 80);
    QCOMPARE(settings.chordAutomationSettings.note1.delay, 11);
    QCOMPARE(settings.chordAutomationSettings.note2.offset, 7);
    QCOMPARE(settings.chordAutomationSettings.note2.velocity, 60);
    QCOMPARE(settings.chordAutomationSettings.note2.delay, 22);
    QCOMPARE(settings.chordAutomationSettings.note3.offset, 12);
    QCOMPARE(settings.chordAutomationSettings.note3.velocity, 90);
    QCOMPARE(settings.chordAutomationSettings.note3.delay, 33);
    QCOMPARE(settings.chordAutomationSettings.arpeggiator.enabled, true);
    QCOMPARE(static_cast<int>(settings.chordAutomationSettings.arpeggiator.pattern), 1);
    QCOMPARE(settings.chordAutomationSettings.arpeggiator.eventsPerBeat, 8);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::ColumnSettingsModelTest)
