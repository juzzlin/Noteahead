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

#include "midi_importer.hpp"

#include "../../../contrib/SimpleLogger/src/simple_logger.hpp"
#include "../../../domain/event.hpp"
#include "../../../domain/midi_cc_data.hpp"
#include "../../../domain/note_data.hpp"
#include "../../../domain/pattern.hpp"
#include "../../../domain/pitch_bend_data.hpp"
#include "../../../domain/song.hpp"
#include "../../../domain/track.hpp"

#include <algorithm>
#include <array>
#include <fstream>
#include <iostream>
#include <map>
#include <regex>

namespace noteahead {

namespace {
const auto TAG = "MidiImporter";

// MIDI constants
constexpr uint8_t META_EVENT = 0xff;
constexpr uint8_t MIDI_PORT_EVENT = 0x21;
constexpr uint8_t TRACK_NAME_EVENT = 0x03;
constexpr uint8_t END_OF_TRACK_EVENT = 0x2f;
constexpr uint8_t SET_TEMPO_EVENT = 0x51;

constexpr uint8_t NOTE_ON_STATUS = 0x90;
constexpr uint8_t NOTE_OFF_STATUS = 0x80;
constexpr uint8_t CONTROL_CHANGE_STATUS = 0xb0;
constexpr uint8_t PITCH_BEND_STATUS = 0xe0;

constexpr uint8_t MIDI_CHANNEL_MASK = 0x0f;
constexpr uint8_t MIDI_STATUS_MASK = 0xf0;
constexpr uint8_t MIDI_STATUS_BIT = 0x80;

constexpr size_t CHUNK_TYPE_SIZE = 4;
constexpr size_t MIDI_HEADER_EXPECTED_SIZE = 6;
constexpr uint32_t MICROSECONDS_PER_MINUTE = 60'000'000;
constexpr uint32_t DEFAULT_BPM = 120;

std::optional<size_t> tryExtractTrackIndex(const std::string & trackName)
{
    static const std::regex trackRegex("^Track ([0-9]+)$");
    std::smatch match;
    if (std::regex_search(trackName, match, trackRegex)) {
        try {
            const auto val = std::stoul(match[1].str());
            if (val > 0) {
                return val - 1; // Noteahead uses 1-based names ("Track 1" -> index 0)
            }
        } catch (...) {
        }
    }
    return std::nullopt;
}
}

MidiImporter::MidiImporter() = default;

MidiImporter::MidiFileData MidiImporter::parseMidiFile(const std::string & fileName) const
{
    MidiFileData fileData;
    std::ifstream in { fileName, std::ios::binary };
    if (!in.is_open()) {
        juzzlin::L(TAG).error() << "Failed to open file for reading: " << fileName;
        return fileData;
    }

    std::array<char, CHUNK_TYPE_SIZE> chunkType;
    in.read(chunkType.data(), CHUNK_TYPE_SIZE);
    if (std::string(chunkType.data(), CHUNK_TYPE_SIZE) != "MThd") {
        juzzlin::L(TAG).error() << "Invalid MIDI file header";
        return fileData;
    }

    const auto headerSize = readBeU32(in);
    fileData.format = readBeU16(in);
    fileData.numTracks = readBeU16(in);
    fileData.ticksPerQuarterNote = readBeU16(in);

    // Skip any extra header bytes
    if (headerSize > MIDI_HEADER_EXPECTED_SIZE) {
        in.ignore(static_cast<std::streamsize>(headerSize - MIDI_HEADER_EXPECTED_SIZE));
    }

    juzzlin::L(TAG).info() << "MIDI Format: " << fileData.format << ", Tracks: " << fileData.numTracks << ", Ticks/QN: " << fileData.ticksPerQuarterNote;

    bool initialTempoFound = false;
    for (int i = 0; i < fileData.numTracks; ++i) {
        in.read(chunkType.data(), CHUNK_TYPE_SIZE);
        if (std::string(chunkType.data(), CHUNK_TYPE_SIZE) == "MTrk") {
            const auto trackSize = readBeU32(in);
            auto track = parseTrack(in, trackSize);

            // Check for initial tempo in this track if we don't have one yet
            if (!initialTempoFound) {
                for (const auto & event : track.events) {
                    if (event.isMeta && event.metaType == SET_TEMPO_EVENT && event.data.size() >= 3) {
                        const uint32_t mpqn = (static_cast<uint32_t>(event.data[0]) << 16) | (static_cast<uint32_t>(event.data[1]) << 8) | static_cast<uint32_t>(event.data[2]);
                        fileData.initialTempoBpm = mpqn > 0 ? MICROSECONDS_PER_MINUTE / mpqn : DEFAULT_BPM;
                        juzzlin::L(TAG).info() << "Setting project initial BPM to: " << fileData.initialTempoBpm;
                        initialTempoFound = true;
                        break;
                    }
                }
            }

            fileData.tracks.push_back(std::move(track));
        } else {
            juzzlin::L(TAG).error() << "Expected MTrk chunk, found " << std::string(chunkType.data(), CHUNK_TYPE_SIZE);
            break;
        }
    }

    return fileData;
}

MidiImporter::MidiTrack MidiImporter::parseTrack(std::istream & in, uint32_t trackSize) const
{
    MidiTrack track;
    uint32_t currentTick = 0;
    uint8_t lastStatus = 0;

    const auto startPos = in.tellg();
    while (static_cast<uint32_t>(in.tellg() - startPos) < trackSize) {
        const auto deltaTick = readVlq(in);
        currentTick += deltaTick;

        auto status = static_cast<uint8_t>(in.peek());
        if (!(status & MIDI_STATUS_BIT)) {
            status = lastStatus;
        } else {
            in.get();
            lastStatus = status;
        }

        MidiEvent event;
        event.tick = currentTick;
        event.status = status;

        if (status == META_EVENT) {
            event.isMeta = true;
            event.metaType = static_cast<uint8_t>(in.get());
            const auto length = readVlq(in);
            event.data.resize(length);
            in.read(reinterpret_cast<char *>(event.data.data()), static_cast<std::streamsize>(length));

            if (event.metaType == TRACK_NAME_EVENT) {
                track.name = std::string(reinterpret_cast<char *>(event.data.data()), length);
            } else if (event.metaType == MIDI_PORT_EVENT && length >= 1) {
                track.port = event.data[0];
            } else if (event.metaType == SET_TEMPO_EVENT && length >= 3) {
                const uint32_t mpqn = (static_cast<uint32_t>(event.data[0]) << 16) | (static_cast<uint32_t>(event.data[1]) << 8) | static_cast<uint32_t>(event.data[2]);
                const uint32_t bpm = mpqn > 0 ? MICROSECONDS_PER_MINUTE / mpqn : DEFAULT_BPM;
                juzzlin::L(TAG).info() << "Found tempo change: " << bpm << " BPM";
            }
            track.events.push_back(event);
        } else if ((status & MIDI_STATUS_MASK) == 0xf0) {
            const auto length = readVlq(in);
            in.ignore(static_cast<std::streamsize>(length));
        } else {
            const auto type = static_cast<uint8_t>(status & MIDI_STATUS_MASK);
            if (type == NOTE_ON_STATUS || type == NOTE_OFF_STATUS || type == CONTROL_CHANGE_STATUS || type == 0xa0 || type == 0xe0) {
                event.data.push_back(static_cast<uint8_t>(in.get()));
                event.data.push_back(static_cast<uint8_t>(in.get()));
            } else if (type == 0xc0 || type == 0xd0) {
                event.data.push_back(static_cast<uint8_t>(in.get()));
            }
            track.events.push_back(event);
        }
    }

    return track;
}

void MidiImporter::initializeSong(SongS song, const MidiFileData & data, int importMode, int patternLength) const
{
    if (!importMode) {
        song->initialize();
        song->setBeatsPerMinute(data.initialTempoBpm);
        if (patternLength > 0) {
            song->setLineCount(0, static_cast<size_t>(patternLength));
        }
    }

    if (!song->hasPattern(0)) {
        song->createPattern(0);
        if (patternLength > 0) {
            song->setLineCount(0, static_cast<size_t>(patternLength));
        }
    }
}

MidiImporter::PatternLine MidiImporter::getPatternLine(SongS song, size_t scaledTick, int patternLength) const
{
    const auto ticksPerLine = static_cast<uint32_t>(song->ticksPerLine());
    auto lineIndex = static_cast<size_t>(scaledTick / ticksPerLine);
    size_t patternIndex = 0;

    bool finished = false;
    while (!finished) {
        if (!song->hasPattern(patternIndex)) {
            song->createPattern(patternIndex);
            if (patternLength > 0) {
                song->setLineCount(patternIndex, static_cast<size_t>(patternLength));
            }
        }
        const auto pLineCount = song->lineCount(patternIndex);
        if (lineIndex < pLineCount) {
            finished = true;
        } else {
            lineIndex -= pLineCount;
            patternIndex++;
        }
    }
    return { patternIndex, lineIndex };
}

size_t MidiImporter::getFreeColumn(SongS song, size_t patternIndex, size_t trackIndex, size_t lineIndex, const std::map<uint8_t, size_t> & activeNotes) const
{
    size_t col = 0;
    bool found = false;
    while (!found) {
        if (col >= song->columnCount(trackIndex)) {
            song->addColumn(trackIndex);
        }

        bool columnBusy = false;
        for (const auto & [activePitch, assignedCol] : activeNotes) {
            if (assignedCol == col) {
                columnBusy = true;
                break;
            }
        }

        if (!columnBusy) {
            const auto existingData = song->noteDataAtPosition({ patternIndex, trackIndex, col, lineIndex });
            if (!existingData || existingData->type() == NoteData::Type::None) {
                found = true;
            }
        }

        if (!found) {
            col++;
        }
    }
    return col;
}

void MidiImporter::finalizePlayOrder(SongS song, size_t maxPatternIndex, int importMode, [[maybe_unused]] int patternLength) const
{
    if (!importMode) {
        song->setLength(maxPatternIndex + 1);
        for (size_t i = 0; i <= maxPatternIndex; ++i) {
            song->setPatternAtSongPosition(i, i);
        }
    }
}

void MidiImporter::importTo(const MidiFileData & data, SongS song, int importMode, int patternLength, bool quantizeNoteOn, bool quantizeNoteOff) const
{
    initializeSong(song, data, importMode, patternLength);

    const auto ticksPerLine = static_cast<uint32_t>(song->ticksPerLine());
    const auto linesPerBeat = static_cast<uint32_t>(song->linesPerBeat());
    const auto scaleFactor = static_cast<double>(ticksPerLine * linesPerBeat) / data.ticksPerQuarterNote;

    juzzlin::L(TAG).info() << "Importing " << data.tracks.size() << " MIDI tracks. Scale factor: " << scaleFactor;

    size_t nextImportTrackIndex = 0;
    size_t maxPatternIndex = 0;

    for (const auto & midiTrack : data.tracks) {
        const auto it = std::find_if(midiTrack.events.begin(), midiTrack.events.end(), [](const auto & e) {
            const auto type = static_cast<uint8_t>(e.status & MIDI_STATUS_MASK);
            return type == NOTE_ON_STATUS || type == NOTE_OFF_STATUS;
        });

        if (it != midiTrack.events.end()) {
            const auto trackIndex = tryExtractTrackIndex(midiTrack.name).value_or(nextImportTrackIndex);

            while (song->trackCount() <= trackIndex) {
                song->addTrackToRightOf(song->trackCount() - 1);
            }

            song->setTrackName(trackIndex, midiTrack.name.empty() ? "Imported " + std::to_string(trackIndex) : midiTrack.name);

            std::map<uint8_t, size_t> activeNotes;

            for (const auto & midiEvent : midiTrack.events) {
                const auto type = static_cast<uint8_t>(midiEvent.status & MIDI_STATUS_MASK);
                const auto scaledTick = static_cast<uint32_t>(midiEvent.tick * scaleFactor);
                const auto [patternIndex, lineIndex] = getPatternLine(song, scaledTick, patternLength);

                maxPatternIndex = std::max(maxPatternIndex, patternIndex);

                if (type == NOTE_ON_STATUS && midiEvent.data.size() >= 2 && midiEvent.data[1] > 0) {
                    const auto pitch = midiEvent.data[0];
                    const auto velocity = midiEvent.data[1];
                    const auto col = getFreeColumn(song, patternIndex, trackIndex, lineIndex, activeNotes);
                    activeNotes[pitch] = col;
                    NoteData noteData { trackIndex, col };
                    noteData.setAsNoteOn(pitch, velocity);
                    const auto delay = quantizeNoteOn ? static_cast<uint8_t>(0) : static_cast<uint8_t>(scaledTick % ticksPerLine);
                    noteData.setDelay(delay);
                    song->setNoteDataAtPosition(noteData, { patternIndex, trackIndex, col, lineIndex });
                } else if ((type == NOTE_OFF_STATUS || (type == NOTE_ON_STATUS && midiEvent.data[1] == 0)) && midiEvent.data.size() >= 1) {
                    const auto pitch = midiEvent.data[0];
                    if (activeNotes.contains(pitch)) {
                        const auto col = activeNotes[pitch];
                        activeNotes.erase(pitch);
                        NoteData noteData { trackIndex, col };
                        noteData.setAsNoteOff(pitch);
                        const auto delay = quantizeNoteOff ? static_cast<uint8_t>(0) : static_cast<uint8_t>(scaledTick % ticksPerLine);
                        noteData.setDelay(delay);
                        song->setNoteDataAtPosition(noteData, { patternIndex, trackIndex, col, lineIndex });
                    }
                }
            }

            if (!tryExtractTrackIndex(midiTrack.name).has_value()) {
                nextImportTrackIndex++;
            }
        }
    }

    finalizePlayOrder(song, maxPatternIndex, importMode, patternLength);
}

uint32_t MidiImporter::readBeU32(std::istream & in) const
{
    uint32_t value = 0;
    for (int i = 0; i < 4; ++i) {
        value = (value << 8) | static_cast<uint8_t>(in.get());
    }
    return value;
}

uint16_t MidiImporter::readBeU16(std::istream & in) const
{
    uint16_t value = 0;
    for (int i = 0; i < 2; ++i) {
        value = (value << 8) | static_cast<uint8_t>(in.get());
    }
    return value;
}

uint32_t MidiImporter::readVlq(std::istream & in) const
{
    uint32_t value = 0;
    uint8_t b;
    do {
        b = static_cast<uint8_t>(in.get());
        value = (value << 7) | (b & 0x7f);
    } while (b & 0x80);
    return value;
}

} // namespace noteahead
