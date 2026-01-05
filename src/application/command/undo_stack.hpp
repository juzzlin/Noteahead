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

#ifndef UNDO_STACK_HPP
#define UNDO_STACK_HPP

#include "command.hpp"

#include <memory>
#include <vector>
#include <functional>

namespace noteahead {

class UndoStack
{
public:
    using CommandS = std::shared_ptr<Command>;
    using Callback = std::function<void()>;

    UndoStack();

    void push(CommandS command);

    void undo();
    void redo();

    bool canUndo() const;
    bool canRedo() const;

    void clear();

    void setCanUndoChangedCallback(Callback callback);
    void setCanRedoChangedCallback(Callback callback);

private:
    std::vector<CommandS> m_commands;
    size_t m_index { 0 };

    Callback m_canUndoChangedCallback;
    Callback m_canRedoChangedCallback;
};

} // namespace noteahead

#endif // UNDO_STACK_HPP
