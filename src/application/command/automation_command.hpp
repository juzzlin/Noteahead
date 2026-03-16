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

#ifndef AUTOMATION_COMMAND_HPP
#define AUTOMATION_COMMAND_HPP

#include "command.hpp"

#include <memory>
#include <vector>

#include "../../domain/midi_cc_automation.hpp"
#include "../../domain/pitch_bend_automation.hpp"

namespace noteahead {

class AutomationService;

class AutomationCommand : public Command
{
public:
    using AutomationServiceS = std::shared_ptr<AutomationService>;
    using MidiCcAutomationList = std::vector<MidiCcAutomation>;
    using PitchBendAutomationList = std::vector<PitchBendAutomation>;

    AutomationCommand(AutomationServiceS automationService, MidiCcAutomationList additions, PitchBendAutomationList pbAdditions, MidiCcAutomationList deletions, PitchBendAutomationList pbDeletions);

    void undo() override;
    void redo() override;

private:
    AutomationServiceS m_automationService;
    MidiCcAutomationList m_additions;
    PitchBendAutomationList m_pitchBendAdditions;
    MidiCcAutomationList m_deletions;
    PitchBendAutomationList m_pitchBendDeletions;
    bool m_idsAssigned = false;
};

} // namespace noteahead

#endif // AUTOMATION_COMMAND_HPP
