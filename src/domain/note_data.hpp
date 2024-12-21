// This file is part of Cacophony.
// Copyright (C) 2024 Jussi Lind <jussi.lind@iki.fi>
//
// Cacophony is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// Cacophony is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Cacophony. If not, see <http://www.gnu.org/licenses/>.

#ifndef NOTE_DATA_HPP
#define NOTE_DATA_HPP

#include <cstdint>
#include <string>

namespace cacophony {

class NoteData
{
public:
    enum class Type
    {
        None,
        NoteOn,
        NoteOff
    };

    NoteData() = default;

    NoteData(uint32_t track, uint32_t column);

    void setAsNoteOn(uint8_t note, uint8_t velocity);

    void setAsNoteOff(uint8_t note);

    NoteData::Type type() const;

    uint8_t note() const;

    uint8_t velocity() const;

    void setVelocity(uint8_t velocity);

    std::string toString() const;

    uint32_t track() const;

    uint32_t column() const;

private:
    Type m_type = Type::None;

    uint8_t m_note = 0;

    uint8_t m_velocity = 0;

    uint32_t m_track = 0;

    uint32_t m_column = 0;
};

} // namespace cacophony

#endif // NOTE_DATA_HPP
