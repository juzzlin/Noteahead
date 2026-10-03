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

#include "../../domain/tracker/automation_set.hpp"

namespace noteahead {

class AutomationService;

//! Adds and deletes automations of every kind as one undoable step.
class AutomationCommand : public Command
{
public:
    using AutomationServiceS = std::shared_ptr<AutomationService>;

    AutomationCommand(AutomationServiceS automationService, AutomationSet additions, AutomationSet deletions);

    void undo() override;
    void redo() override;

private:
    AutomationServiceS m_automationService;
    AutomationSet m_additions;
    AutomationSet m_deletions;
    bool m_idsAssigned = false;
};

} // namespace noteahead

#endif // AUTOMATION_COMMAND_HPP
