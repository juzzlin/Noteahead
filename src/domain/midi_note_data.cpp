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

#include "midi_note_data.hpp"

namespace noteahead {

MidiNoteData::MidiNoteData(uint8_t note, uint8_t velocity)
  : m_note { note }
  , m_velocity { velocity }
{
}

uint8_t MidiNoteData::note() const
{
    return m_note;
}

uint8_t MidiNoteData::velocity() const
{
    return m_velocity;
}

} // namespace noteahead
