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

#include <chrono>
#include <cstddef>
#include <memory>

#include "midi_cc_data.hpp"
#include "note_data.hpp"
#include "pitch_bend_data.hpp"

namespace noteahead {

class Instrument;
class InstrumentSettings;
class NoteData;

//! A MIDI event class - but works at a higher level and drives the actual player.
//! Can have optional data attached or just a valueless event, e.g. EndOfSong.
class Event
{
public:
    enum class Type
    {
        EndOfSong,
        InstrumentSettings,
        MidiCcData,
        MidiClockOut,
        None,
        NoteData,
        PitchBendData,
        StartOfSong,
    };

    //! Builds a default None event.
    explicit Event(size_t tick);

    //! Builds a MIDI note data event.
    using NoteDataCR = const NoteData &;
    Event(size_t tick, NoteDataCR noteData);

    using MidiCcDataCR = const MidiCcData &;
    //! Builds a MIDI CC data event.
    Event(size_t tick, MidiCcDataCR midiCcData);

    using PitchBendDataCR = const PitchBendData &;
    //! Builds a Pitch Bend data event.
    Event(size_t tick, PitchBendDataCR pitchBendData);

    //! Builds an instrument settings event.
    using InstrumentSettingsS = std::shared_ptr<InstrumentSettings>;
    Event(size_t tick, InstrumentSettingsS instrumentSettings);

    size_t tick() const;
    void setTick(size_t tick);

    Type type() const;

    //! Applies delay as ticks.
    void applyDelay(std::chrono::milliseconds delay, double msPerTick);

    //! Applies random velocity jitter.
    void applyVelocityJitter(int percentage);

    //! Applies velocity key track scaling.
    void applyVelocityKeyTrack(int percentage, int offset);

    //! Convenience function to transpose note data (if set).
    void transpose(int semitones);

    //! Set as a MIDI clock event.
    void setAsMidiClockOut();

    //! Set as a StartOfSong event.
    void setAsStartOfSong();

    //! Set as an EndOfSong event.
    void setAsEndOfSong();

    using NoteDataOpt = std::optional<NoteData>;
    NoteDataOpt noteData() const;

    using MidiCcDataOpt = std::optional<MidiCcData>;
    MidiCcDataOpt midiCcData() const;

    using PitchBendDataOpt = std::optional<PitchBendData>;
    PitchBendDataOpt pitchBendData() const;

    using InstrumentS = std::shared_ptr<Instrument>;
    InstrumentS instrument() const;
    void setInstrument(InstrumentS instrument);

    InstrumentSettingsS instrumentSettings() const;
    void setInstrumentSettings(InstrumentSettingsS instrumentSettings);

private:
    size_t m_tick;

    Type m_type;

    //! Data for data-like events
    NoteDataOpt m_noteData;
    MidiCcDataOpt m_midiCcData;
    PitchBendDataOpt m_pitchBendData;
    InstrumentS m_instrument;
    InstrumentSettingsS m_instrumentSettings;
};

} // namespace noteahead

#endif // EVENT_HPP
