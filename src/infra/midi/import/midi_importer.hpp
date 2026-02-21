// This file is part of Noteahead.
// Copyright (C) 2026 Jussi Lind <jussi.lind@iki.fi>
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

#ifndef MIDI_IMPORTER_HPP
#define MIDI_IMPORTER_HPP

#include <cstdint>
#include <istream>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace noteahead {

class Event;
class MidiCcData;
class NoteData;
class PitchBendData;
class Song;

class MidiImporter
{
public:
    using EventS = std::shared_ptr<Event>;
    using SongS = std::shared_ptr<Song>;

    struct MidiEvent
    {
        uint32_t tick;
        uint8_t status;
        std::vector<uint8_t> data;
        bool isMeta = false;
        uint8_t metaType = 0;
    };

    struct MidiTrack
    {
        std::string name;
        std::vector<MidiEvent> events;
        uint8_t port = 0;
    };

    struct MidiFileData
    {
        uint16_t format;
        uint16_t numTracks;
        uint16_t ticksPerQuarterNote;
        std::vector<MidiTrack> tracks;
        uint32_t initialTempoBpm = 120;
    };

    MidiImporter();

    MidiFileData parseMidiFile(const std::string & fileName) const;

    void importTo(const MidiFileData & data, SongS song, int importMode, int patternLength, bool quantizeNoteOn, bool quantizeNoteOff) const;

private:
    struct PatternLine
    {
        size_t patternIndex;
        size_t lineIndex;
    };

    void initializeSong(SongS song, const MidiFileData & data, int importMode, int patternLength) const;
    PatternLine getPatternLine(SongS song, size_t scaledTick, int patternLength) const;
    size_t getFreeColumn(SongS song, size_t patternIndex, size_t trackIndex, size_t lineIndex, const std::map<uint8_t, size_t> & activeNotes) const;
    void finalizePlayOrder(SongS song, size_t maxPatternIndex, int importMode, int patternLength) const;

    uint32_t readBeU32(std::istream & in) const;
    uint16_t readBeU16(std::istream & in) const;
    uint32_t readVlq(std::istream & in) const;

    MidiTrack parseTrack(std::istream & in, uint32_t trackSize) const;
};

} // namespace noteahead

#endif // MIDI_IMPORTER_HPP
