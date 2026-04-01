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

#ifndef ARPEGGIATOR_HPP
#define ARPEGGIATOR_HPP

#include <cstdint>
#include <vector>

namespace noteahead {

class Arpeggiator
{
public:
    enum class Pattern
    {
        Up,
        Down,
        UpDown,
        DownUp,
        Random
    };

    struct Settings
    {
        bool enabled = false;
        Pattern pattern = Pattern::Up;
        uint8_t eventsPerBeat = 4;
    };

    struct NoteInfo
    {
        uint8_t note = 0;
        uint8_t velocity = 0;
    };

    using NoteInfoList = std::vector<NoteInfo>;
    static NoteInfoList generate(Pattern pattern, const NoteInfoList & inputNotes);
};

} // namespace noteahead

#endif // ARPEGGIATOR_HPP
