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

#include "automation_command.hpp"
#include "../service/automation_service.hpp"

namespace noteahead {

namespace {

// One overload per kind, so that AutomationSet::forEach() can hand each automation to the right one.

void add(AutomationService & service, MidiCcAutomation & automation)
{
    automation.setId(service.addMidiCcAutomation(automation));
}

void add(AutomationService & service, PitchBendAutomation & automation)
{
    automation.setId(service.addPitchBendAutomation(automation));
}

void add(AutomationService & service, ParameterAutomation & automation)
{
    automation.setId(service.addParameterAutomation(automation));
}

void addWithId(AutomationService & service, const MidiCcAutomation & automation)
{
    service.addMidiCcAutomationWithId(automation);
}

void addWithId(AutomationService & service, const PitchBendAutomation & automation)
{
    service.addPitchBendAutomationWithId(automation);
}

void addWithId(AutomationService & service, const ParameterAutomation & automation)
{
    service.addParameterAutomationWithId(automation);
}

void remove(AutomationService & service, const MidiCcAutomation & automation)
{
    service.deleteMidiCcAutomation(automation);
}

void remove(AutomationService & service, const PitchBendAutomation & automation)
{
    service.deletePitchBendAutomation(automation);
}

void remove(AutomationService & service, const ParameterAutomation & automation)
{
    service.deleteParameterAutomation(automation);
}

} // namespace

AutomationCommand::AutomationCommand(AutomationServiceS automationService, AutomationSet additions, AutomationSet deletions)
  : m_automationService { std::move(automationService) }
  , m_additions { std::move(additions) }
  , m_deletions { std::move(deletions) }
{
}

void AutomationCommand::undo()
{
    auto & service = *m_automationService;
    m_additions.forEach([&service](auto && automation) { remove(service, automation); });
    m_deletions.forEach([&service](auto && automation) { addWithId(service, automation); });
}

void AutomationCommand::redo()
{
    auto & service = *m_automationService;
    m_deletions.forEach([&service](auto && automation) { remove(service, automation); });

    // The first time through the additions are given ids of their own, which every redo after it
    // reuses, so that an undo finds them by the id they were added under.
    if (m_idsAssigned) {
        m_additions.forEach([&service](auto && automation) { addWithId(service, automation); });
    } else {
        m_additions.forEach([&service](auto && automation) { add(service, automation); });
        m_idsAssigned = true;
    }
}

} // namespace noteahead
