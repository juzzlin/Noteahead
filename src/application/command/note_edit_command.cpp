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

#include "note_edit_command.hpp"
#include "../../domain/song.hpp"

namespace noteahead {

NoteEditCommand::NoteEditCommand(SongS song, ChangeList changes, Callback callback)
    : m_song { std::move(song) }
    , m_changes { std::move(changes) }
    , m_callback { std::move(callback) }
{
}

void NoteEditCommand::undo()
{
    for (const auto & [position, oldData, newData] : m_changes) {
        m_song->setNoteDataAtPosition(oldData, position);
        if (m_callback) {
            m_callback(position);
        }
    }
}

void NoteEditCommand::redo()
{
    for (const auto & [position, oldData, newData] : m_changes) {
        m_song->setNoteDataAtPosition(newData, position);
        if (m_callback) {
            m_callback(position);
        }
    }
}

} // namespace noteahead
