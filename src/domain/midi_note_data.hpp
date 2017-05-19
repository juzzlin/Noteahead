// This file is part of Noteahead.
// Copyright (C) 2025 Jussi Lind <jussi.lind@iki.fi>
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

#ifndef MIDI_NOTE_DATA_HPP
#define MIDI_NOTE_DATA_HPP

#include <cstdint>

namespace noteahead {

class MidiNoteData
{
public:
    MidiNoteData(uint8_t note, uint8_t velocity);

    uint8_t note() const;
    uint8_t velocity() const;

private:
    uint8_t m_note = 0;
    uint8_t m_velocity = 0;
};

} // namespace noteahead

#endif // MIDI_NOTE_DATA_HPP
