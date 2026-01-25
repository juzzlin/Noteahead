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

#include "event_selection_model_test.hpp"

#include "../../application/models/event_selection_model.hpp"
#include "../../domain/instrument_settings.hpp"

#include <QSignalSpy>
#include <QTest>

namespace noteahead {

void EventSelectionModelTest::test_initialState_shouldHaveExpectedDefaults()
{
    const EventSelectionModel model {};
    QCOMPARE(model.patchEnabled(), false);
    QCOMPARE(model.patch(), quint8 { 0 });
    QCOMPARE(model.bankEnabled(), false);
    QCOMPARE(model.bankLsb(), quint8 { 0 });
    QCOMPARE(model.bankMsb(), quint8 { 0 });
    QCOMPARE(model.bankByteOrderSwapped(), false);
}

void EventSelectionModelTest::test_setPatchEnabled_shouldUpdateAndEmitSignal()
{
    EventSelectionModel model {};
    QSignalSpy spy { &model, &EventSelectionModel::patchEnabledChanged };
    model.setPatchEnabled(true);
    QCOMPARE(model.patchEnabled(), true);
    QCOMPARE(spy.count(), 1);
}

void EventSelectionModelTest::test_setPatch_shouldUpdateAndEmitSignal()
{
    EventSelectionModel model {};
    QSignalSpy spy { &model, &EventSelectionModel::patchChanged };
    model.setPatch(42);
    QCOMPARE(model.patch(), quint8 { 42 });
    QCOMPARE(spy.count(), 1);
}

void EventSelectionModelTest::test_setBankEnabled_shouldUpdateAndEmitSignal()
{
    EventSelectionModel model {};
    QSignalSpy spy { &model, &EventSelectionModel::bankEnabledChanged };
    model.setBankEnabled(true);
    QCOMPARE(model.bankEnabled(), true);
    QCOMPARE(spy.count(), 1);
}

void EventSelectionModelTest::test_setBankLsb_shouldUpdateAndEmitSignal()
{
    EventSelectionModel model {};
    QSignalSpy spy { &model, &EventSelectionModel::bankLsbChanged };
    model.setBankLsb(10);
    QCOMPARE(model.bankLsb(), quint8 { 10 });
    QCOMPARE(spy.count(), 1);
}

void EventSelectionModelTest::test_setBankMsb_shouldUpdateAndEmitSignal()
{
    EventSelectionModel model {};
    QSignalSpy spy { &model, &EventSelectionModel::bankMsbChanged };
    model.setBankMsb(20);
    QCOMPARE(model.bankMsb(), quint8 { 20 });
    QCOMPARE(spy.count(), 1);
}

void EventSelectionModelTest::test_setBankByteOrderSwapped_shouldUpdateAndEmitSignal()
{
    EventSelectionModel model {};
    QSignalSpy spy { &model, &EventSelectionModel::bankByteOrderSwappedChanged };
    model.setBankByteOrderSwapped(true);
    QCOMPARE(model.bankByteOrderSwapped(), true);
    QCOMPARE(spy.count(), 1);
}

void EventSelectionModelTest::test_reset_shouldResetToDefaults()
{
    EventSelectionModel model {};
    model.setPatchEnabled(true);
    model.setPatch(42);
    model.setBankEnabled(true);
    model.setBankLsb(10);
    model.setBankMsb(20);
    model.setBankByteOrderSwapped(true);

    QSignalSpy spy { &model, &EventSelectionModel::dataReceived };
    model.reset();

    QCOMPARE(model.patchEnabled(), false);
    QCOMPARE(model.patch(), quint8 { 0 });
    QCOMPARE(model.bankEnabled(), false);
    QCOMPARE(model.bankLsb(), quint8 { 0 });
    QCOMPARE(model.bankMsb(), quint8 { 0 });
    QCOMPARE(model.bankByteOrderSwapped(), false);
    QCOMPARE(spy.count(), 1);
}

void EventSelectionModelTest::test_toInstrumentSettings_shouldReturnCorrectData()
{
    EventSelectionModel model {};
    model.setPatchEnabled(true);
    model.setPatch(42);
    model.setBankEnabled(true);
    model.setBankLsb(10);
    model.setBankMsb(20);
    model.setBankByteOrderSwapped(true);

    const auto settings { model.toInstrumentSettings() };
    QCOMPARE(settings->patch.has_value(), true);
    QCOMPARE(*settings->patch, quint8 { 42 });
    QCOMPARE(settings->bank.has_value(), true);
    QCOMPARE(settings->bank->lsb, quint8 { 10 });
    QCOMPARE(settings->bank->msb, quint8 { 20 });
    QCOMPARE(settings->bank->byteOrderSwapped, true);
}

void EventSelectionModelTest::test_fromInstrumentSettings_shouldUpdateState()
{
    EventSelectionModel model {};
    InstrumentSettings settings {};
    settings.patch = 42;
    settings.bank = { 10, 20, true };

    QSignalSpy spy { &model, &EventSelectionModel::dataReceived };
    model.fromInstrumentSettings(settings);

    QCOMPARE(model.patchEnabled(), true);
    QCOMPARE(model.patch(), quint8 { 42 });
    QCOMPARE(model.bankEnabled(), true);
    QCOMPARE(model.bankLsb(), quint8 { 10 });
    QCOMPARE(model.bankMsb(), quint8 { 20 });
    QCOMPARE(model.bankByteOrderSwapped(), true);
    QCOMPARE(spy.count(), 1);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::EventSelectionModelTest)
