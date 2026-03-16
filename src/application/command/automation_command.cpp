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

AutomationCommand::AutomationCommand(AutomationServiceS automationService, MidiCcAutomationList additions, PitchBendAutomationList pbAdditions, MidiCcAutomationList deletions, PitchBendAutomationList pbDeletions)
  : m_automationService { std::move(automationService) }
  , m_additions { std::move(additions) }
  , m_pitchBendAdditions { std::move(pbAdditions) }
  , m_deletions { std::move(deletions) }
  , m_pitchBendDeletions { std::move(pbDeletions) }
{
}

void AutomationCommand::undo()
{
    for (auto && automation : m_additions) {
        m_automationService->deleteMidiCcAutomation(automation);
    }
    for (auto && automation : m_pitchBendAdditions) {
        m_automationService->deletePitchBendAutomation(automation);
    }
    for (auto && automation : m_deletions) {
        m_automationService->addMidiCcAutomationWithId(automation);
    }
    for (auto && automation : m_pitchBendDeletions) {
        m_automationService->addPitchBendAutomationWithId(automation);
    }
}

void AutomationCommand::redo()
{
    for (auto && automation : m_deletions) {
        m_automationService->deleteMidiCcAutomation(automation);
    }
    for (auto && automation : m_pitchBendDeletions) {
        m_automationService->deletePitchBendAutomation(automation);
    }

    if (m_idsAssigned) {
        for (auto && automation : m_additions) {
            m_automationService->addMidiCcAutomationWithId(automation);
        }
        for (auto && automation : m_pitchBendAdditions) {
            m_automationService->addPitchBendAutomationWithId(automation);
        }
    } else {
        for (auto && automation : m_additions) {
            const auto id = m_automationService->addMidiCcAutomation(automation);
            automation.setId(id);
        }
        for (auto && automation : m_pitchBendAdditions) {
            const auto id = m_automationService->addPitchBendAutomation(automation);
            automation.setId(id);
        }
        m_idsAssigned = true;
    }
}

} // namespace noteahead
