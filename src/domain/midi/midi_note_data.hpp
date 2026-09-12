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
#include <optional>

namespace noteahead {

class MidiNoteData
{
public:
    //! \param noteBeats How long the note lasts, in beats, where that is known.
    //!
    //! Known when the song is playing, because the note-off is already in the rendered event list
    //! by then, and not known when somebody presses a key. Defaulted for that reason: the callers
    //! that have no timeline do not have to say so.
    MidiNoteData(uint8_t note, uint8_t velocity, std::optional<double> noteBeats = std::nullopt);
    MidiNoteData() = default;

    uint8_t note() const;
    uint8_t velocity() const;
    std::optional<double> noteBeats() const;

private:
    uint8_t m_note = 0;
    uint8_t m_velocity = 0;
    std::optional<double> m_noteBeats;
};

} // namespace noteahead

#endif // MIDI_NOTE_DATA_HPP
