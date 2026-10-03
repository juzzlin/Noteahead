// This file is part of Noteahead.
// Copyright (C) 2026 Jussi Lind <jussi.lind@iki.fi>
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

#include "parameter_automations_model_test.hpp"

#include "../../application/models/parameter_automations_model.hpp"

#include <QSignalSpy>
#include <QTest>

namespace noteahead {

namespace {

using Role = ParameterAutomationsModel::DataRole;

ParameterAutomation automation(size_t id, AutomationLocation location, size_t line0, size_t line1)
{
    return { id, location, { ParameterAutomation::Target::Owner::Effect, "effect-1", "cutoff" }, ParameterAutomation::Mode::Relative, { line0, line1, 0.2, 0.8 }, {}, "Comment", true };
}

QVariant dataOf(const ParameterAutomationsModel & model, int row, Role role)
{
    return model.data(model.index(row), static_cast<int>(role));
}

} // namespace

void ParameterAutomationsModelTest::test_setParameterAutomations_shouldExposeEveryRole()
{
    ParameterAutomationsModel model;
    model.setParameterAutomations({ automation(7, { 1, 2, 3 }, 4, 12) });

    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(dataOf(model, 0, Role::Id).toULongLong(), quint64 { 7 });
    QCOMPARE(dataOf(model, 0, Role::Pattern).toULongLong(), quint64 { 1 });
    QCOMPARE(dataOf(model, 0, Role::Track).toULongLong(), quint64 { 2 });
    QCOMPARE(dataOf(model, 0, Role::Column).toULongLong(), quint64 { 3 });
    QCOMPARE(dataOf(model, 0, Role::Owner).toString(), QString { "effect" });
    QCOMPARE(dataOf(model, 0, Role::OwnerId).toString(), QString { "effect-1" });
    QCOMPARE(dataOf(model, 0, Role::Parameter).toString(), QString { "cutoff" });
    QCOMPARE(dataOf(model, 0, Role::Mode).toInt(), static_cast<int>(ParameterAutomation::Mode::Relative));
    QCOMPARE(dataOf(model, 0, Role::Line0).toULongLong(), quint64 { 4 });
    QCOMPARE(dataOf(model, 0, Role::Line1).toULongLong(), quint64 { 12 });
    QCOMPARE(dataOf(model, 0, Role::Value0).toDouble(), 0.2);
    QCOMPARE(dataOf(model, 0, Role::Value1).toDouble(), 0.8);
    QCOMPARE(dataOf(model, 0, Role::Comment).toString(), QString { "Comment" });
    QVERIFY(dataOf(model, 0, Role::Enabled).toBool());
}

void ParameterAutomationsModelTest::test_requestParameterAutomationsByLine_shouldFilter()
{
    ParameterAutomationsModel model;
    QSignalSpy requestedSpy { &model, &ParameterAutomationsModel::parameterAutomationsRequested };

    model.requestParameterAutomationsByLine(1, 2, 3, 13);
    model.setParameterAutomations({ automation(1, { 1, 2, 3 }, 4, 12), automation(2, { 1, 2, 3 }, 10, 20), automation(3, { 1, 2, 4 }, 10, 20) });

    QCOMPARE(requestedSpy.count(), 1);
    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(dataOf(model, 0, Role::Id).toULongLong(), quint64 { 2 });
}

void ParameterAutomationsModelTest::test_requestParameterAutomationsBySelection_shouldFilterOnOverlap()
{
    ParameterAutomationsModel model;

    // Columns 2..3, lines 8..12 of track 1
    model.requestParameterAutomationsBySelection(0, 1, 2, 3, 8, 12);
    model.setParameterAutomations({
      automation(1, { 0, 1, 2 }, 0, 8), // Ends on the first selected line
      automation(2, { 0, 1, 3 }, 12, 20), // Starts on the last one
      automation(3, { 0, 1, 3 }, 0, 7), // Ends just before
      automation(4, { 0, 1, 4 }, 8, 12), // A column outside
      automation(5, { 0, 2, 2 }, 8, 12), // Another track
      automation(6, { 1, 1, 2 }, 8, 12) // Another pattern
    });

    QCOMPARE(model.rowCount(), 2);
    QCOMPARE(dataOf(model, 0, Role::Id).toULongLong(), quint64 { 1 });
    QCOMPARE(dataOf(model, 1, Role::Id).toULongLong(), quint64 { 2 });
}

void ParameterAutomationsModelTest::test_setData_shouldHoldTheEditUntilApplied()
{
    ParameterAutomationsModel model;
    model.setParameterAutomations({ automation(1, { 0, 0, 0 }, 0, 8) });
    QSignalSpy changedSpy { &model, &ParameterAutomationsModel::parameterAutomationChanged };

    QVERIFY(model.setData(model.index(0), 0.4, static_cast<int>(Role::Value1)));
    QVERIFY(model.setData(model.index(0), static_cast<int>(ParameterAutomation::Mode::Absolute), static_cast<int>(Role::Mode)));
    // A position never lies outside the travel
    QVERIFY(model.setData(model.index(0), 3.0, static_cast<int>(Role::Value0)));
    QCOMPARE(dataOf(model, 0, Role::Value0).toDouble(), 1.0);
    QCOMPARE(changedSpy.count(), 0);

    model.applyAll();

    // Both edits in one, the latest state of the automation
    QCOMPARE(changedSpy.count(), 1);
    const auto changed = changedSpy.at(0).at(0).value<ParameterAutomation>();
    QCOMPARE(changed.interpolation().value1, 0.4);
    QCOMPARE(changed.mode(), ParameterAutomation::Mode::Absolute);
}

void ParameterAutomationsModelTest::test_setData_target_shouldBeRefused()
{
    ParameterAutomationsModel model;
    model.setParameterAutomations({ automation(1, { 0, 0, 0 }, 0, 8) });

    QVERIFY(!model.setData(model.index(0), "other", static_cast<int>(Role::OwnerId)));
    QVERIFY(!model.setData(model.index(0), "resonance", static_cast<int>(Role::Parameter)));
    QCOMPARE(dataOf(model, 0, Role::Parameter).toString(), QString { "cutoff" });
}

void ParameterAutomationsModelTest::test_removeAt_shouldDeleteOnApply()
{
    ParameterAutomationsModel model;
    model.setParameterAutomations({ automation(1, { 0, 0, 0 }, 0, 8), automation(2, { 0, 0, 0 }, 0, 8) });
    QSignalSpy deletedSpy { &model, &ParameterAutomationsModel::parameterAutomationDeleted };
    QSignalSpy changedSpy { &model, &ParameterAutomationsModel::parameterAutomationChanged };

    // An edit to a row that is then removed must not be written back
    model.setData(model.index(0), 0.5, static_cast<int>(Role::Value0));
    QVERIFY(model.removeAt(0));
    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(deletedSpy.count(), 0);

    model.applyAll();

    QCOMPARE(deletedSpy.count(), 1);
    QCOMPARE(changedSpy.count(), 0);
    QCOMPARE(deletedSpy.at(0).at(0).value<ParameterAutomation>().id(), size_t { 1 });
}

void ParameterAutomationsModelTest::test_applyValues_shouldKeepLocationEnabledAndTarget()
{
    ParameterAutomationsModel model;
    model.setParameterAutomations({ automation(1, { 0, 1, 2 }, 0, 8) });
    QSignalSpy replacedSpy { &model, &ParameterAutomationsModel::automationReplaced };

    model.applyValues(0, { { "pattern", 5 }, { "track", 5 }, { "enabled", false }, { "owner", "device" }, { "ownerId", "other" }, { "parameter", "resonance" }, { "mode", 0 }, { "line0", 2 }, { "line1", 6 }, { "value0", 0.3 }, { "value1", 0.6 }, { "comment", "Copied" } });

    QCOMPARE(replacedSpy.count(), 1);
    QCOMPARE(replacedSpy.at(0).at(0).toInt(), 0);
    // What the automation does comes from the copy...
    QCOMPARE(dataOf(model, 0, Role::Mode).toInt(), 0);
    QCOMPARE(dataOf(model, 0, Role::Line0).toULongLong(), quint64 { 2 });
    QCOMPARE(dataOf(model, 0, Role::Value1).toDouble(), 0.6);
    QCOMPARE(dataOf(model, 0, Role::Comment).toString(), QString { "Copied" });
    // ...where it lives, whether it is on and what it drives do not
    QCOMPARE(dataOf(model, 0, Role::Pattern).toULongLong(), quint64 { 0 });
    QCOMPARE(dataOf(model, 0, Role::Track).toULongLong(), quint64 { 1 });
    QVERIFY(dataOf(model, 0, Role::Enabled).toBool());
    QCOMPARE(dataOf(model, 0, Role::OwnerId).toString(), QString { "effect-1" });
    QCOMPARE(dataOf(model, 0, Role::Parameter).toString(), QString { "cutoff" });
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::ParameterAutomationsModelTest)
