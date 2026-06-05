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

#include "../application/service/random_service.hpp"
#include "../common/utils.hpp"

#include <algorithm>

namespace noteahead {

Event::Event(size_t tick, NoteDataCR noteData)
  : m_tick { tick }
  , m_type { Event::Type::NoteData }
  , m_data { noteData }
{
}

Event::Event(size_t tick, MidiCcDataCR midiCcData)
  : m_tick { tick }
  , m_type { Event::Type::MidiCcData }
  , m_data { midiCcData }
{
}

Event::Event(size_t tick, PitchBendDataCR pitchBendData)
  : m_tick { tick }
  , m_type { Event::Type::PitchBendData }
  , m_data { pitchBendData }
{
}

Event::Event(size_t tick, InstrumentSettingsS instrumentSettings)
  : m_tick { tick }
  , m_type { Event::Type::InstrumentSettings }
  , m_data { instrumentSettings }
{
}

Event::Event(size_t tick)
  : m_tick { tick }
  , m_type { Event::Type::None }
  , m_data { None {} }
{
}

void Event::applyDelay(std::chrono::milliseconds delay, double msPerTick)
{
    m_tick += static_cast<size_t>(std::round(static_cast<double>(delay.count()) / msPerTick));
}

void Event::applyVelocityJitter(int percentage)
{
    if (auto noteData = std::get_if<NoteData>(&m_data); noteData && percentage > 0) {

        const int velocity = noteData->velocity();

        // Jitter range in absolute velocity units
        const int maxDelta = (velocity * percentage) / 100;

        // Uniform distribution between -maxDelta and +maxDelta
        std::uniform_int_distribution<int> dist { -maxDelta, 0 };
        const int jittered = velocity + dist(RandomService::generator());
        noteData->setVelocity(static_cast<uint8_t>(std::clamp(jittered, 0, 127)));
    }
}

void Event::applyVelocityKeyTrack(int percentage, int offset)
{
    if (auto noteData = std::get_if<NoteData>(&m_data); noteData && percentage > 0 && noteData->type() == NoteData::Type::NoteOn && noteData->note().has_value()) {
        noteData->setVelocity(Utils::Midi::scaleVelocityByKey(noteData->velocity(), *noteData->note(), percentage, offset));
    }
}

void Event::transpose(int semitones)
{
    if (auto noteData = std::get_if<NoteData>(&m_data)) {
        noteData->transpose(semitones);
    }
}

void Event::setAsMidiClockOut()
{
    m_type = Type::MidiClockOut;
    m_data = MidiClockOut {};
}

void Event::setAsStartOfSong()
{
    m_type = Type::StartOfSong;
    m_data = StartOfSong {};
}

void Event::setAsEndOfSong()
{
    m_type = Type::EndOfSong;
    m_data = EndOfSong {};
}

size_t Event::tick() const
{
    return m_tick;
}

void Event::setTick(size_t tick)
{
    m_tick = tick;
}

Event::Type Event::type() const
{
    return m_type;
}

Event::NoteDataOpt Event::noteData() const
{
    if (auto noteData = std::get_if<NoteData>(&m_data)) {
        return *noteData;
    }
    return std::nullopt;
}

Event::MidiCcDataOpt Event::midiCcData() const
{
    if (auto midiCcData = std::get_if<MidiCcData>(&m_data)) {
        return *midiCcData;
    }
    return std::nullopt;
}

Event::PitchBendDataOpt Event::pitchBendData() const
{
    if (auto pitchBendData = std::get_if<PitchBendData>(&m_data)) {
        return *pitchBendData;
    }
    return std::nullopt;
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
    if (auto instrumentSettings = std::get_if<InstrumentSettingsS>(&m_data)) {
        return *instrumentSettings;
    }
    return nullptr;
}

void Event::setInstrumentSettings(InstrumentSettingsS instrumentSettings)
{
    m_type = Type::InstrumentSettings;
    m_data = instrumentSettings;
}

} // namespace noteahead
