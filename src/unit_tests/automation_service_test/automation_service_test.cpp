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

#include "automation_service_test.hpp"

#include "../../application/automation_service.hpp"
#include "../../domain/midi_cc_automation.hpp"

#include <QSignalSpy>

namespace noteahead {

void AutomationServiceTest::test_addMidiCcAutomation_shouldAddAutomation()
{
    AutomationService automationService;
    QSignalSpy lineDataChangedSpy { &automationService, &AutomationService::lineDataChanged };
    quint64 pattern = 0;
    quint64 track = 1;
    quint64 column = 2;
    quint8 controller = 64;
    quint8 line0 = 4;
    quint8 line1 = 12;
    quint8 value0 = 0;
    quint8 value1 = 100;
    automationService.addMidiCcAutomation(pattern, track, column, controller, line0, line1, value0, value1, "MIDI CC Automation Test");
    QCOMPARE(lineDataChangedSpy.count(), line1 - line0 + 1);
    QVERIFY(automationService.hasAutomations(pattern, track, column, line0));
    QVERIFY(automationService.hasAutomations(pattern, track, column, line1));
    QVERIFY(automationService.hasAutomations(pattern, track, column, (line0 + line1) / 2));
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::AutomationServiceTest)
