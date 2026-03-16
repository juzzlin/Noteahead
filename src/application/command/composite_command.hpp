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

#ifndef COMPOSITE_COMMAND_HPP
#define COMPOSITE_COMMAND_HPP

#include "command.hpp"

#include <memory>
#include <vector>

namespace noteahead {

class CompositeCommand : public Command
{
public:
    using CommandS = std::shared_ptr<Command>;
    using CommandList = std::vector<CommandS>;

    explicit CompositeCommand(CommandList commands);

    void undo() override;
    void redo() override;

private:
    CommandList m_commands;
};

} // namespace noteahead

#endif // COMPOSITE_COMMAND_HPP
