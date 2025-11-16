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

#ifndef MIDI_EXPORTER_HPP
#define MIDI_EXPORTER_HPP

#include <memory>
#include <map>
#include <string>
#include <vector>
#include <ostream>

namespace noteahead {

class AutomationService;
class Song;
class Instrument;
class Event;
class NoteData;
class MidiCcData;
class PitchBendData;

class MidiExporter {
public:
    using AutomationServiceS = std::shared_ptr<AutomationService>;
    using ByteVector = std::vector<char>;
    using EventS = std::shared_ptr<Event>;
    using InstrumentS = std::shared_ptr<Instrument>;
    using SongS = std::shared_ptr<Song>;
    using SongW = std::weak_ptr<Song>;

    struct ActiveTracks {
        std::map<size_t, InstrumentS> trackToInstrument;
        std::map<size_t, std::string> trackToPortName;
        std::map<std::string, uint8_t> portNameToIndex;
    };

    MidiExporter(AutomationServiceS automationService);

    void exportTo(std::string fileName, SongW songW) const;

private:
    struct TrackProcessingState {
        std::map<size_t, ByteVector> allTracksData;
        std::map<size_t, uint32_t> lastTicks;
        std::map<size_t, uint8_t> trackToChannelMap;
    };

    void writeMidiHeader(std::ostream & out, const SongS & song, size_t numNoteTracks) const;
    ActiveTracks discoverActiveTracks(const SongS& song) const;
    
    std::map<size_t, ByteVector> buildTrackData(const std::vector<EventS> & events, const ActiveTracks & activeTracks) const;
    TrackProcessingState initializeTracks(const ActiveTracks & activeTracks) const;
    TrackProcessingState processEvents(TrackProcessingState state, const std::vector<EventS> & events) const;
    std::map<size_t, ByteVector> finalizeTracks(TrackProcessingState state) const;

    ByteVector initializeTrack(size_t trackIndex, const ActiveTracks & activeTracks) const;
    void writeNoteOnEvent(ByteVector & data, uint8_t channel, const NoteData & noteData) const;
    void writeNoteOffEvent(ByteVector & data, uint8_t channel, const NoteData & noteData) const;
    void writeControlChangeEvent(ByteVector & data, uint8_t channel, const MidiCcData & ccData) const;
    void writePitchBendEvent(ByteVector & data, uint8_t channel, const PitchBendData & pitchBendData) const;
    void writeTempoTrack(std::ostream& out, const SongS& song) const;
    void writeNoteTracks(std::ostream & out, const std::map<size_t, ByteVector> & allTracksData) const;

    static void sortEvents(std::vector<EventS> & events);

    AutomationServiceS m_automationService;
};

} // namespace noteahead

#endif // MIDI_EXPORTER_HPP
