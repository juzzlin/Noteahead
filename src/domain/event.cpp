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

#include "event.hpp"

#include "instrument.hpp"
#include "instrument_settings.hpp"

namespace noteahead {

Event::Event(size_t tick, NoteDataS noteData)
  : m_tick { tick }
  , m_type { Event::Type::NoteData }
  , m_noteData { noteData }
{
}

Event::Event(size_t tick, InstrumentSettingsS instrumentSettings)
  : m_tick { tick }
  , m_type { Event::Type::InstrumentSettings }
  , m_instrumentSettings { instrumentSettings }
{
}

Event::Event(size_t tick)
  : m_tick { tick }
  , m_type { Event::Type::None }
{
}

void Event::applyDelay(size_t beatsPerMinute, size_t linesPerBeat, size_t ticksPerLine)
{
    if (m_instrument) {
        const double msPerTick = 60000.0 / static_cast<double>(beatsPerMinute * linesPerBeat * ticksPerLine);
        m_tick += static_cast<size_t>(std::round(static_cast<double>(m_instrument->settings().delay.count()) / msPerTick));
    }
}

void Event::setAsMidiClockOut()
{
    m_type = Type::MidiClockOut;
}

void Event::setAsStartOfSong()
{
    m_type = Type::StartOfSong;
}

void Event::setAsEndOfSong()
{
    m_type = Type::EndOfSong;
}

size_t Event::tick() const
{
    return m_tick;
}

Event::Type Event::type() const
{
    return m_type;
}

Event::NoteDataS Event::noteData() const
{
    return m_noteData;
}

Event::InstrumentS Event::instrument() const
{
    return m_instrument;
}

void Event::setInstrument(InstrumentS instrument)
{
    m_instrument = instrument;
}

Event::InstrumentSettingsS Event::instrumentSettings() const
{
    return m_instrumentSettings;
}

void Event::setInstrumentSettings(InstrumentSettingsS instrumentSettings)
{
    m_instrumentSettings = instrumentSettings;
}

} // namespace noteahead
