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

#include "undo_stack.hpp"

namespace noteahead {

UndoStack::UndoStack() = default;

void UndoStack::push(CommandS command)
{
    if (m_index < m_commands.size()) {
        m_commands.erase(m_commands.begin() + static_cast<long>(m_index), m_commands.end());
    }
    m_commands.push_back(command);
    command->redo();
    m_index++;
    if (m_canUndoChangedCallback) {
        m_canUndoChangedCallback();
    }
    if (m_canRedoChangedCallback) {
        m_canRedoChangedCallback();
    }
}

void UndoStack::undo()
{
    if (canUndo()) {
        m_index--;
        m_commands[m_index]->undo();
        if (m_canUndoChangedCallback) {
            m_canUndoChangedCallback();
        }
        if (m_canRedoChangedCallback) {
            m_canRedoChangedCallback();
        }
    }
}

void UndoStack::redo()
{
    if (canRedo()) {
        m_commands[m_index]->redo();
        m_index++;
        if (m_canUndoChangedCallback) {
            m_canUndoChangedCallback();
        }
        if (m_canRedoChangedCallback) {
            m_canRedoChangedCallback();
        }
    }
}

bool UndoStack::canUndo() const
{
    return m_index > 0;
}

bool UndoStack::canRedo() const
{
    return m_index < m_commands.size();
}

void UndoStack::clear()
{
    m_commands.clear();
    m_index = 0;
    if (m_canUndoChangedCallback) {
        m_canUndoChangedCallback();
    }
    if (m_canRedoChangedCallback) {
        m_canRedoChangedCallback();
    }
}

void UndoStack::setCanUndoChangedCallback(Callback callback)
{
    m_canUndoChangedCallback = std::move(callback);
}

void UndoStack::setCanRedoChangedCallback(Callback callback)
{
    m_canRedoChangedCallback = std::move(callback);
}

} // namespace noteahead
