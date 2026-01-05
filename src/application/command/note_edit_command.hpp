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

#ifndef NOTE_EDIT_COMMAND_HPP
#define NOTE_EDIT_COMMAND_HPP

#include "command.hpp"

#include <functional>
#include <memory>
#include <vector>
#include <tuple>

#include "../../domain/note_data.hpp"
#include "../position.hpp"

namespace noteahead {

class Song;

class NoteEditCommand : public Command
{
public:
    using SongS = std::shared_ptr<Song>;
    using Change = std::tuple<Position, NoteData, NoteData>; // Position, Old, New
    using ChangeList = std::vector<Change>;
    using Callback = std::function<void(const Position &)>;

    NoteEditCommand(SongS song, ChangeList changes, Callback callback);

    void undo() override;
    void redo() override;

private:
    SongS m_song;
    ChangeList m_changes;
    Callback m_callback;
};

} // namespace noteahead

#endif // NOTE_EDIT_COMMAND_HPP
