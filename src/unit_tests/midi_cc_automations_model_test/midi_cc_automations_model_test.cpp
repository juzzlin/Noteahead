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

#include "midi_cc_automations_model_test.hpp"

#include "../../application/models/midi_cc_automations_model.hpp"
#include "../../domain/midi_cc_automation.hpp"

#include <QSignalSpy>

namespace noteahead {

void MidiCcAutomationsModelTest::test_addMidiCcAutomations_shouldAddAutomations()
{
    const AutomationLocation location { 1, 2, 3 };
    const MidiCcAutomation::InterpolationParameters interpolation { 11, 22, 33, 44 };
    MidiCcAutomation midiCcAutomation { 42, location, 7, interpolation, "Comment", true };

    MidiCcAutomationsModel model;
    model.setMidiCcAutomations({ midiCcAutomation });

    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(model.data(model.index(0), static_cast<int>(MidiCcAutomationsModel::DataRole::Column)), static_cast<quint64>(midiCcAutomation.location().column));
    QCOMPARE(model.data(model.index(0), static_cast<int>(MidiCcAutomationsModel::DataRole::Comment)), midiCcAutomation.comment());
    QCOMPARE(model.data(model.index(0), static_cast<int>(MidiCcAutomationsModel::DataRole::Controller)), midiCcAutomation.controller());
    QCOMPARE(model.data(model.index(0), static_cast<int>(MidiCcAutomationsModel::DataRole::Enabled)), midiCcAutomation.enabled());
    QCOMPARE(model.data(model.index(0), static_cast<int>(MidiCcAutomationsModel::DataRole::Id)), static_cast<quint64>(midiCcAutomation.id()));
    QCOMPARE(model.data(model.index(0), static_cast<int>(MidiCcAutomationsModel::DataRole::Line0)), static_cast<quint64>(midiCcAutomation.interpolation().line0));
    QCOMPARE(model.data(model.index(0), static_cast<int>(MidiCcAutomationsModel::DataRole::Line1)), static_cast<quint64>(midiCcAutomation.interpolation().line1));
    QCOMPARE(model.data(model.index(0), static_cast<int>(MidiCcAutomationsModel::DataRole::Pattern)), static_cast<quint64>(midiCcAutomation.location().pattern));
    QCOMPARE(model.data(model.index(0), static_cast<int>(MidiCcAutomationsModel::DataRole::Track)), static_cast<quint64>(midiCcAutomation.location().track));
    QCOMPARE(model.data(model.index(0), static_cast<int>(MidiCcAutomationsModel::DataRole::Value0)), static_cast<quint64>(midiCcAutomation.interpolation().value0));
    QCOMPARE(model.data(model.index(0), static_cast<int>(MidiCcAutomationsModel::DataRole::Value1)), static_cast<quint64>(midiCcAutomation.interpolation().value1));
}

void MidiCcAutomationsModelTest::test_requestMidiCcAutomations_shouldFilterAutomations()
{
    const MidiCcAutomation::InterpolationParameters interpolation { 11, 22, 33, 44 };
    MidiCcAutomation midiCcAutomation { 42, { 1, 2, 3 }, 7, interpolation, {} };

    MidiCcAutomationsModel model;
    QSignalSpy midiCcAutomationsRequestedSpy { &model, &MidiCcAutomationsModel::midiCcAutomationsRequested };
    model.requestMidiCcAutomationsByColumn(1, 2, 3);
    model.setMidiCcAutomations({ midiCcAutomation });
    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(midiCcAutomationsRequestedSpy.count(), 1);

    model.requestMidiCcAutomationsByColumn(1, 2, 4);
    model.setMidiCcAutomations({ midiCcAutomation });
    QCOMPARE(model.rowCount(), 0);
    QCOMPARE(midiCcAutomationsRequestedSpy.count(), 2);

    model.requestMidiCcAutomationsByColumn(2, 2, 3);
    model.setMidiCcAutomations({ midiCcAutomation });
    QCOMPARE(model.rowCount(), 0);
    QCOMPARE(midiCcAutomationsRequestedSpy.count(), 3);

    model.requestMidiCcAutomationsByTrack(1, 2);
    model.setMidiCcAutomations({ midiCcAutomation });
    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(midiCcAutomationsRequestedSpy.count(), 4);

    model.requestMidiCcAutomationsByTrack(1, 3);
    model.setMidiCcAutomations({ midiCcAutomation });
    QCOMPARE(model.rowCount(), 0);
    QCOMPARE(midiCcAutomationsRequestedSpy.count(), 5);

    model.requestMidiCcAutomationsByTrack(2, 2);
    model.setMidiCcAutomations({ midiCcAutomation });
    QCOMPARE(model.rowCount(), 0);
    QCOMPARE(midiCcAutomationsRequestedSpy.count(), 6);

    model.requestMidiCcAutomationsByPattern(1);
    model.setMidiCcAutomations({ midiCcAutomation });
    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(midiCcAutomationsRequestedSpy.count(), 7);

    model.requestMidiCcAutomationsByPattern(2);
    model.setMidiCcAutomations({ midiCcAutomation });
    QCOMPARE(model.rowCount(), 0);
    QCOMPARE(midiCcAutomationsRequestedSpy.count(), 8);

    model.requestMidiCcAutomations();
    model.setMidiCcAutomations({ midiCcAutomation });
    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(midiCcAutomationsRequestedSpy.count(), 9);
}

void MidiCcAutomationsModelTest::test_setData_shouldUpdateAutomationData()
{
    using Role = MidiCcAutomationsModel::DataRole;

    const AutomationLocation location { 1, 2, 3 };
    const MidiCcAutomation::InterpolationParameters interpolation { 11, 22, 33, 44 };
    MidiCcAutomation midiCcAutomation { 42, location, 7, interpolation, "Old Comment" };
    std::optional<MidiCcAutomation> updatedAutomation;
    MidiCcAutomationsModel model;
    model.setMidiCcAutomations({ midiCcAutomation });
    QSignalSpy midiCcAutomationChangedSpy { &model, &MidiCcAutomationsModel::midiCcAutomationChanged };
    connect(&model, &MidiCcAutomationsModel::midiCcAutomationChanged, this, [&updatedAutomation](auto && automation) {
        updatedAutomation = automation;
    });
    QModelIndex index = model.index(0);

    // Try setting a new comment
    const QString newComment = "New Comment";
    QVERIFY(model.setData(index, newComment, static_cast<int>(Role::Comment)));
    QCOMPARE(model.data(index, static_cast<int>(Role::Comment)).toString(), newComment);
    QVERIFY(!model.setData(index, newComment, static_cast<int>(Role::Comment)));

    // Try setting a new controller value
    const int newController = 99;
    QVERIFY(model.setData(index, newController, static_cast<int>(Role::Controller)));
    QCOMPARE(model.data(index, static_cast<int>(Role::Controller)).toInt(), newController);
    QVERIFY(!model.setData(index, newController, static_cast<int>(Role::Controller)));

    // Try setting enabled
    QVERIFY(model.setData(index, false, static_cast<int>(Role::Enabled)));
    QCOMPARE(model.data(index, static_cast<int>(Role::Enabled)).toBool(), false);
    QVERIFY(model.setData(index, true, static_cast<int>(Role::Enabled)));
    QCOMPARE(model.data(index, static_cast<int>(Role::Enabled)).toBool(), true);
    QVERIFY(!model.setData(index, true, static_cast<int>(Role::Enabled)));

    // Try setting new line0 and line1 values
    QVERIFY(model.setData(index, 123u, static_cast<int>(Role::Line0)));
    QCOMPARE(model.data(index, static_cast<int>(Role::Line0)).toUInt(), 123u);
    QVERIFY(!model.setData(index, 123u, static_cast<int>(Role::Line0)));
    QVERIFY(model.setData(index, 321u, static_cast<int>(Role::Line1)));
    QCOMPARE(model.data(index, static_cast<int>(Role::Line1)).toUInt(), 321u);
    QVERIFY(!model.setData(index, 321u, static_cast<int>(Role::Line1)));

    // Try setting new value0 and value1
    QVERIFY(model.setData(index, 75u, static_cast<int>(Role::Value0)));
    QCOMPARE(model.data(index, static_cast<int>(Role::Value0)).toUInt(), 75u);
    QVERIFY(!model.setData(index, 75u, static_cast<int>(Role::Value0)));
    QVERIFY(model.setData(index, 25u, static_cast<int>(Role::Value1)));
    QCOMPARE(model.data(index, static_cast<int>(Role::Value1)).toUInt(), 25u);
    QVERIFY(!model.setData(index, 25u, static_cast<int>(Role::Value1)));

    // These should not be settable
    QVERIFY(!model.setData(index, 8u, static_cast<int>(Role::Pattern)));
    QCOMPARE(model.data(index, static_cast<int>(Role::Pattern)).toUInt(), midiCcAutomation.location().pattern);
    QVERIFY(!model.setData(index, 5u, static_cast<int>(Role::Track)));
    QCOMPARE(model.data(index, static_cast<int>(Role::Track)).toUInt(), midiCcAutomation.location().track);
    QVERIFY(!model.setData(index, 2u, static_cast<int>(Role::Column)));
    QCOMPARE(model.data(index, static_cast<int>(Role::Column)).toUInt(), midiCcAutomation.location().column);
    QVERIFY(!model.setData(index, 99u, static_cast<int>(Role::Id)));
    QCOMPARE(model.data(index, static_cast<int>(Role::Id)).toUInt(), midiCcAutomation.id());

    // Verify that correct automation data gets emitted on apply
    model.applyAll();
    QVERIFY(updatedAutomation.has_value());
    QCOMPARE(midiCcAutomationChangedSpy.count(), 1);
    QCOMPARE(updatedAutomation->comment(), model.data(index, static_cast<int>(Role::Comment)).toString());
    QCOMPARE(updatedAutomation->controller(), model.data(index, static_cast<int>(Role::Controller)).toInt());
    QCOMPARE(updatedAutomation->enabled(), model.data(index, static_cast<int>(Role::Enabled)).toBool());
    QCOMPARE(updatedAutomation->interpolation().line0, model.data(index, static_cast<int>(Role::Line0)).toUInt());
    QCOMPARE(updatedAutomation->interpolation().line1, model.data(index, static_cast<int>(Role::Line1)).toUInt());
    QCOMPARE(updatedAutomation->interpolation().value0, model.data(index, static_cast<int>(Role::Value0)).toUInt());
    QCOMPARE(updatedAutomation->interpolation().value1, model.data(index, static_cast<int>(Role::Value1)).toUInt());
    QCOMPARE(updatedAutomation->location().pattern, model.data(index, static_cast<int>(Role::Pattern)).toUInt());
    QCOMPARE(updatedAutomation->location().track, model.data(index, static_cast<int>(Role::Track)).toUInt());
    QCOMPARE(updatedAutomation->location().column, model.data(index, static_cast<int>(Role::Column)).toUInt());
}

void MidiCcAutomationsModelTest::test_removeAt_shouldRemoveAutomationData()
{
    const MidiCcAutomation::InterpolationParameters interpolation { 11, 22, 33, 44 };
    MidiCcAutomation midiCcAutomation { 42, { 1, 2, 3 }, 7, interpolation, "Old Comment" };

    MidiCcAutomationsModel model;
    model.setMidiCcAutomations({ midiCcAutomation });
    QSignalSpy midiCcAutomationDeletedSpy { &model, &MidiCcAutomationsModel::midiCcAutomationDeleted };

    model.removeAt(0);
    QCOMPARE(model.rowCount(), 0);

    model.applyAll();
    QCOMPARE(midiCcAutomationDeletedSpy.count(), 1);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::MidiCcAutomationsModelTest)
