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

#include "../../application/models/column_settings_model.hpp"
#include "../../domain/column_settings.hpp"

#include <QSignalSpy>

namespace noteahead {

void ColumnSettingsModelTest::test_initialState_shouldHaveExpectedDefaults()
{
    ColumnSettingsModel model;
    QCOMPARE(model.trackIndex(), quint64(0));
    QCOMPARE(model.columnIndex(), quint64(0));
    QCOMPARE(model.chordNote1Offset(), qint8(0));
    QCOMPARE(model.chordNote1Velocity(), quint8(100));
    QCOMPARE(model.chordNote2Offset(), qint8(0));
    QCOMPARE(model.chordNote2Velocity(), quint8(100));
    QCOMPARE(model.chordNote3Offset(), qint8(0));
    QCOMPARE(model.chordNote3Velocity(), quint8(100));
}

void ColumnSettingsModelTest::test_setTrackIndex_shouldUpdateAndEmitSignal()
{
    ColumnSettingsModel model;
    QSignalSpy spy { &model, &ColumnSettingsModel::trackIndexChanged };
    model.setTrackIndex(5);
    QCOMPARE(model.trackIndex(), quint64(5));
    QCOMPARE(spy.count(), 1);
}

void ColumnSettingsModelTest::test_setColumnIndex_shouldUpdateAndEmitSignal()
{
    ColumnSettingsModel model;
    QSignalSpy spy { &model, &ColumnSettingsModel::columnIndexChanged };
    model.setColumnIndex(3);
    QCOMPARE(model.columnIndex(), quint64(3));
    QCOMPARE(spy.count(), 1);
}

void ColumnSettingsModelTest::test_setChordNote1Offset_shouldUpdateAndEmitSignal()
{
    ColumnSettingsModel model;
    QSignalSpy spy { &model, &ColumnSettingsModel::chordNote1OffsetChanged };
    model.setChordNote1Offset(-5);
    QCOMPARE(model.chordNote1Offset(), qint8(-5));
    QCOMPARE(spy.count(), 1);
}

void ColumnSettingsModelTest::test_setChordNote1Velocity_shouldUpdateAndEmitSignal()
{
    ColumnSettingsModel model;
    QSignalSpy spy { &model, &ColumnSettingsModel::chordNote1VelocityChanged };
    model.setChordNote1Velocity(80);
    QCOMPARE(model.chordNote1Velocity(), 80);
    QCOMPARE(spy.count(), 1);
}

void ColumnSettingsModelTest::test_setChordNote2Offset_shouldUpdateAndEmitSignal()
{
    ColumnSettingsModel model;
    QSignalSpy spy { &model, &ColumnSettingsModel::chordNote2OffsetChanged };
    model.setChordNote2Offset(7);
    QCOMPARE(model.chordNote2Offset(), 7);
    QCOMPARE(spy.count(), 1);
}

void ColumnSettingsModelTest::test_setChordNote2Velocity_shouldUpdateAndEmitSignal()
{
    ColumnSettingsModel model;
    QSignalSpy spy { &model, &ColumnSettingsModel::chordNote2VelocityChanged };
    model.setChordNote2Velocity(60);
    QCOMPARE(model.chordNote2Velocity(), 60);
    QCOMPARE(spy.count(), 1);
}

void ColumnSettingsModelTest::test_setChordNote3Offset_shouldUpdateAndEmitSignal()
{
    ColumnSettingsModel model;
    QSignalSpy spy { &model, &ColumnSettingsModel::chordNote3OffsetChanged };
    model.setChordNote3Offset(12);
    QCOMPARE(model.chordNote3Offset(), 12);
    QCOMPARE(spy.count(), 1);
}

void ColumnSettingsModelTest::test_setChordNote3Velocity_shouldUpdateAndEmitSignal()
{
    ColumnSettingsModel model;
    QSignalSpy spy { &model, &ColumnSettingsModel::chordNote3VelocityChanged };
    model.setChordNote3Velocity(90);
    QCOMPARE(model.chordNote3Velocity(), 90);
    QCOMPARE(spy.count(), 1);
}

void ColumnSettingsModelTest::test_save_shouldEmitSaveRequestedWithCorrectData()
{
    qRegisterMetaType<ColumnSettings>("ColumnSettings");

    ColumnSettingsModel model;
    model.setTrackIndex(1);
    model.setColumnIndex(2);
    model.setChordNote1Offset(4);
    model.setChordNote1Velocity(80);
    model.setChordNote2Offset(7);
    model.setChordNote2Velocity(60);
    model.setChordNote3Offset(12);
    model.setChordNote3Velocity(90);

    QSignalSpy spy { &model, &ColumnSettingsModel::saveRequested };
    model.save();

    QCOMPARE(spy.count(), 1);
    const auto arguments = spy.takeFirst();
    QCOMPARE(arguments.at(0).toULongLong(), 1);
    QCOMPARE(arguments.at(1).toULongLong(), 2);

    const auto settings = qvariant_cast<ColumnSettings>(arguments.at(2));
    QCOMPARE(settings.chordAutomationSettings.note1.offset, 4);
    QCOMPARE(settings.chordAutomationSettings.note1.velocity, 80);
    QCOMPARE(settings.chordAutomationSettings.note2.offset, 7);
    QCOMPARE(settings.chordAutomationSettings.note2.velocity, 60);
    QCOMPARE(settings.chordAutomationSettings.note3.offset, 12);
    QCOMPARE(settings.chordAutomationSettings.note3.velocity, 90);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::ColumnSettingsModelTest)
