// This file is part of Noteahead.
// Copyright (C) 2024 Jussi Lind <jussi.lind@iki.fi>
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

#ifndef NOTE_DATA_HPP
#define NOTE_DATA_HPP

#include <cstdint>
#include <memory>
#include <optional>
#include <string>

class QXmlStreamWriter;
class QXmlStreamReader;

namespace noteahead {

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
    NoteData(size_t track, size_t column);

    void setAsNoteOn(uint8_t note, uint8_t velocity);
    void setAsNoteOff(uint8_t note);
    void setAsNoteOff();

    void transpose(int semitones);

    NoteData::Type type() const;

    std::optional<uint8_t> note() const;

    uint8_t velocity() const;
    void setVelocity(uint8_t velocity);

    size_t track() const;
    void setTrack(size_t track);

    size_t column() const;
    void setColumn(size_t column);

    uint8_t delay() const;
    void setDelay(uint8_t ticks);

    std::string toString() const;
    void serializeToXml(QXmlStreamWriter & writer) const;
    using NoteDataS = std::shared_ptr<NoteData>;
    static NoteDataS deserializeFromXml(QXmlStreamReader & reader, size_t trackIndex, size_t columnIndex);

private:
    Type m_type = Type::None;

    std::optional<uint8_t> m_note;
    std::optional<uint8_t> m_delay; // In ticks per line
    uint8_t m_velocity = 0;

    size_t m_track = 0;
    size_t m_column = 0;
};

} // namespace noteahead

#endif // NOTE_DATA_HPP
