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

#include "pitch_bend_automations_model_test.hpp"

#include "../../application/models/pitch_bend_automations_model.hpp"
#include "../../domain/pitch_bend_automation.hpp"

#include <QSignalSpy>

namespace noteahead {

void PitchBendAutomationsModelTest::test_addPitchBendAutomations_shouldAddAutomations()
{
    const AutomationLocation location { 1, 2, 3 };
    const PitchBendAutomation::InterpolationParameters interpolation { 11, 22, 33, 44 };
    PitchBendAutomation automation { 42, location, interpolation, "Comment", true };

    PitchBendAutomationsModel model;
    model.setPitchBendAutomations({ automation });

    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(model.data(model.index(0), static_cast<int>(PitchBendAutomationsModel::DataRole::Column)), static_cast<quint64>(automation.location().column));
    QCOMPARE(model.data(model.index(0), static_cast<int>(PitchBendAutomationsModel::DataRole::Comment)), automation.comment());
    QCOMPARE(model.data(model.index(0), static_cast<int>(PitchBendAutomationsModel::DataRole::Enabled)), automation.enabled());
    QCOMPARE(model.data(model.index(0), static_cast<int>(PitchBendAutomationsModel::DataRole::Id)), static_cast<quint64>(automation.id()));
    QCOMPARE(model.data(model.index(0), static_cast<int>(PitchBendAutomationsModel::DataRole::Line0)), static_cast<quint64>(automation.interpolation().line0));
    QCOMPARE(model.data(model.index(0), static_cast<int>(PitchBendAutomationsModel::DataRole::Line1)), static_cast<quint64>(automation.interpolation().line1));
    QCOMPARE(model.data(model.index(0), static_cast<int>(PitchBendAutomationsModel::DataRole::Pattern)), static_cast<quint64>(automation.location().pattern));
    QCOMPARE(model.data(model.index(0), static_cast<int>(PitchBendAutomationsModel::DataRole::Track)), static_cast<quint64>(automation.location().track));
    QCOMPARE(model.data(model.index(0), static_cast<int>(PitchBendAutomationsModel::DataRole::Value0)), static_cast<quint64>(automation.interpolation().value0));
    QCOMPARE(model.data(model.index(0), static_cast<int>(PitchBendAutomationsModel::DataRole::Value1)), static_cast<quint64>(automation.interpolation().value1));
}

void PitchBendAutomationsModelTest::test_requestPitchBendAutomations_shouldFilterAutomations()
{
    const PitchBendAutomation::InterpolationParameters interpolation { 11, 22, 33, 44 };
    PitchBendAutomation automation { 42, { 1, 2, 3 }, interpolation, {} };

    PitchBendAutomationsModel model;
    QSignalSpy PitchBendAutomationsRequestedSpy { &model, &PitchBendAutomationsModel::pitchBendAutomationsRequested };
    model.requestPitchBendAutomationsByColumn(1, 2, 3);
    model.setPitchBendAutomations({ automation });
    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(PitchBendAutomationsRequestedSpy.count(), 1);

    model.requestPitchBendAutomationsByColumn(1, 2, 4);
    model.setPitchBendAutomations({ automation });
    QCOMPARE(model.rowCount(), 0);
    QCOMPARE(PitchBendAutomationsRequestedSpy.count(), 2);

    model.requestPitchBendAutomationsByColumn(2, 2, 3);
    model.setPitchBendAutomations({ automation });
    QCOMPARE(model.rowCount(), 0);
    QCOMPARE(PitchBendAutomationsRequestedSpy.count(), 3);

    model.requestPitchBendAutomationsByTrack(1, 2);
    model.setPitchBendAutomations({ automation });
    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(PitchBendAutomationsRequestedSpy.count(), 4);

    model.requestPitchBendAutomationsByTrack(1, 3);
    model.setPitchBendAutomations({ automation });
    QCOMPARE(model.rowCount(), 0);
    QCOMPARE(PitchBendAutomationsRequestedSpy.count(), 5);

    model.requestPitchBendAutomationsByTrack(2, 2);
    model.setPitchBendAutomations({ automation });
    QCOMPARE(model.rowCount(), 0);
    QCOMPARE(PitchBendAutomationsRequestedSpy.count(), 6);

    model.requestPitchBendAutomationsByPattern(1);
    model.setPitchBendAutomations({ automation });
    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(PitchBendAutomationsRequestedSpy.count(), 7);

    model.requestPitchBendAutomationsByPattern(2);
    model.setPitchBendAutomations({ automation });
    QCOMPARE(model.rowCount(), 0);
    QCOMPARE(PitchBendAutomationsRequestedSpy.count(), 8);

    model.requestPitchBendAutomations();
    model.setPitchBendAutomations({ automation });
    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(PitchBendAutomationsRequestedSpy.count(), 9);
}

void PitchBendAutomationsModelTest::test_setData_shouldUpdateAutomationData()
{
    using Role = PitchBendAutomationsModel::DataRole;

    const AutomationLocation location { 1, 2, 3 };
    const PitchBendAutomation::InterpolationParameters interpolation { 11, 22, 33, 44 };
    PitchBendAutomation automation { 42, location, interpolation, "Old Comment" };

    PitchBendAutomationsModel model;
    model.setPitchBendAutomations({ automation });
    QSignalSpy PitchBendAutomationChangedSpy { &model, &PitchBendAutomationsModel::pitchBendAutomationChanged };

    QModelIndex index = model.index(0);

    // Try setting a new comment
    const QString newComment = "New Comment";
    QVERIFY(model.setData(index, newComment, static_cast<int>(Role::Comment)));
    QCOMPARE(model.data(index, static_cast<int>(Role::Comment)).toString(), newComment);
    QVERIFY(!model.setData(index, newComment, static_cast<int>(Role::Comment)));

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
    QCOMPARE(model.data(index, static_cast<int>(Role::Pattern)).toUInt(), automation.location().pattern);
    QVERIFY(!model.setData(index, 5u, static_cast<int>(Role::Track)));
    QCOMPARE(model.data(index, static_cast<int>(Role::Track)).toUInt(), automation.location().track);
    QVERIFY(!model.setData(index, 2u, static_cast<int>(Role::Column)));
    QCOMPARE(model.data(index, static_cast<int>(Role::Column)).toUInt(), automation.location().column);
    QVERIFY(!model.setData(index, 99u, static_cast<int>(Role::Id)));
    QCOMPARE(model.data(index, static_cast<int>(Role::Id)).toUInt(), automation.id());

    model.applyAll();
    QCOMPARE(PitchBendAutomationChangedSpy.count(), 1);
}

void PitchBendAutomationsModelTest::test_removeAt_shouldRemoveAutomationData()
{
    const PitchBendAutomation::InterpolationParameters interpolation { 11, 22, 33, 44 };
    PitchBendAutomation automation { 42, { 1, 2, 3 }, interpolation, "Old Comment" };

    PitchBendAutomationsModel model;
    model.setPitchBendAutomations({ automation });
    QSignalSpy PitchBendAutomationDeletedSpy { &model, &PitchBendAutomationsModel::pitchBendAutomationDeleted };

    model.removeAt(0);
    QCOMPARE(model.rowCount(), 0);

    model.applyAll();
    QCOMPARE(PitchBendAutomationDeletedSpy.count(), 1);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::PitchBendAutomationsModelTest)
