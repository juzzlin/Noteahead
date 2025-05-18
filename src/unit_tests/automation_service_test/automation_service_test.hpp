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

#ifndef AUTOMATION_SERVICE_TEST_HPP
#define AUTOMATION_SERVICE_TEST_HPP

#include <QTest>

namespace noteahead {

class AutomationServiceTest : public QObject
{
    Q_OBJECT

private slots:

    void initTestCase();

    void test_addMidiCcAutomation_shouldAddAutomation();
    void test_deleteMidiCcAutomation_shouldDeleteAutomation();
    void test_addPitchBendAutomation_shouldAddAutomation();
    void test_deletePitchBendAutomation_shouldDeleteAutomation();

    void test_automationWeight_midiCc_shouldCalculateCorrectWeight();
    void test_automationWeight_pitchBendUp_shouldCalculateCorrectWeight();
    void test_automationWeight_pitchBendDown_shouldCalculateCorrectWeight();

    void test_renderToEventsByLine_shouldRenderToEvents();
    void test_renderToEventsByLine_disableAutomation_shouldNotRenderEvents();
    void test_renderToEventsByColumn_shouldRenderToEvents();
    void test_renderToEventsByColumn_shouldPruneRepeatingEvents();
    void test_renderToEventsByColumn_disableAutomation_shouldNotRenderEvents();
};

} // namespace noteahead

#endif // AUTOMATION_SERVICE_TEST_HPP
