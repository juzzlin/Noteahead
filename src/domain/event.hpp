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

#ifndef EVENT_HPP
#define EVENT_HPP

#include <cstddef>
#include <memory>

namespace noteahead {

class Instrument;
class InstrumentSettings;
class NoteData;

class Event
{
public:
    enum class Type
    {
        None,
        NoteData,
        MidiClockOut,
        InstrumentSettings,
        StartOfSong,
        EndOfSong
    };

    Event(size_t tick);

    using NoteDataS = std::shared_ptr<NoteData>;
    Event(size_t tick, NoteDataS noteData);

    using InstrumentSettingsS = std::shared_ptr<InstrumentSettings>;
    Event(size_t tick, InstrumentSettingsS instrumentSettings);

    void setAsMidiClockOut();

    void setAsStartOfSong();

    void setAsEndOfSong();

    size_t tick() const;

    Type type() const;

    NoteDataS noteData() const;

    using InstrumentS = std::shared_ptr<Instrument>;
    InstrumentS instrument() const;
    void setInstrument(InstrumentS instrument);

    InstrumentSettingsS instrumentSettings() const;
    void setInstrumentSettings(InstrumentSettingsS instrumentSettings);

private:
    size_t m_tick;

    Type m_type;

    NoteDataS m_noteData;

    InstrumentS m_instrument;

    InstrumentSettingsS m_instrumentSettings;
};

} // namespace noteahead

#endif // EVENT_HPP
