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

#include "midi_exporter.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <optional>
#include <vector>
#include <utility>

#include "../../../application/service/mixer_service.hpp"
#include "../../../contrib/SimpleLogger/src/simple_logger.hpp"
#include "../../../domain/event.hpp"
#include "../../../domain/instrument.hpp"
#include "../../../domain/midi_cc_data.hpp"
#include "../../../domain/note_data.hpp"
#include "../../../domain/pitch_bend_data.hpp"
#include "../../../domain/song.hpp"

namespace noteahead {

namespace {
    const auto TAG = "MidiExporter";

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

    void writeBeU32(std::ostream & out, uint32_t value)
    {
        out.put(static_cast<uint8_t>(value >> 24) & 0xff);
        out.put(static_cast<uint8_t>(value >> 16) & 0xff);
        out.put(static_cast<uint8_t>(value >> 8) & 0xff);
        out.put(static_cast<uint8_t>(value) & 0xff);
    }

    void writeBeU16(std::ostream & out, uint16_t value)
    {
        out.put((value >> 8) & 0xff);
        out.put(value & 0xff);
    }

    void writeVlq(MidiExporter::ByteVector & out, uint32_t value)
    {
        MidiExporter::ByteVector buffer;
        buffer.push_back(static_cast<char>(value & 0x7f));
        value >>= 7;
        while (value > 0) {
            buffer.push_back(static_cast<char>((value & 0x7f) | 0x80));
            value >>= 7;
        }
        std::reverse(buffer.begin(), buffer.end());
        out.insert(out.end(), buffer.begin(), buffer.end());
    }

} // namespace

MidiExporter::MidiExporter(AutomationServiceS automationService)
  : m_automationService { automationService }
{
}

void MidiExporter::sortEvents(std::vector<EventS> & events)
{
    std::ranges::sort(events, [](const EventS & a, const EventS & b) {
        if (a->tick() != b->tick()) {
            return a->tick() < b->tick();
        }
        return a->type() < b->type();
    });
}

void MidiExporter::exportTo(std::string fileName, SongW songW, MixerServiceS mixerService, size_t startPosition, size_t endPosition) const
{
    auto song = songW.lock();
    if (!song) {
        return;
    }

    std::ofstream out(fileName, std::ios::binary);
    if (!out.is_open()) {
        juzzlin::L(TAG).error() << "Failed to open file for writing: " << fileName;
        return;
    }

    auto renderedEvents = song->renderToEvents(m_automationService, startPosition, endPosition);
    sortEvents(renderedEvents);

    if (startPosition > 0 && !renderedEvents.empty()) {
        const auto startTick = renderedEvents.front()->tick();
        for (auto & event : renderedEvents) {
            event->setTick(event->tick() - startTick);
        }
    }

    auto filteredEvents = filterEvents(renderedEvents, mixerService);

    const auto activeTracks = discoverActiveTracks(song, filteredEvents);
    const auto trackData = buildTrackData(filteredEvents, activeTracks);

    writeMidiHeader(out, song, activeTracks.trackToInstrument.size());
    writeTempoTrack(out, song);
    writeNoteTracks(out, trackData);
}

std::vector<MidiExporter::EventS> MidiExporter::filterEvents(const std::vector<EventS> & events, MixerServiceS mixerService) const
{
    std::vector<EventS> filteredEvents;
    for (const auto & event : events) {
        if (const auto noteData = event->noteData(); noteData) {
            if (mixerService->shouldColumnPlay(noteData->track(), noteData->column())) {
                filteredEvents.push_back(event);
            }
        } else if (const auto ccData = event->midiCcData(); ccData) {
            if (mixerService->shouldColumnPlay(ccData->track(), ccData->column())) {
                filteredEvents.push_back(event);
            }
        } else if (const auto pitchBendData = event->pitchBendData(); pitchBendData) {
            if (mixerService->shouldColumnPlay(pitchBendData->track(), pitchBendData->column())) {
                filteredEvents.push_back(event);
            }
        } else {
            // Other events like tempo, etc, are not filtered
            filteredEvents.push_back(event);
        }
    }
    return filteredEvents;
}

void MidiExporter::writeMidiHeader(std::ostream & out, const SongS & song, size_t numNoteTracks) const
{
    out.write("MThd", 4);
    writeBeU32(out, 6); // Header size
    writeBeU16(out, 1); // Format 1 (Multiple tracks, synchronous)
    writeBeU16(out, static_cast<uint16_t>(numNoteTracks + 1)); // Number of tracks + tempo track
    const auto ticksPerQuarterNote = static_cast<uint16_t>(song->ticksPerLine() * song->linesPerBeat());
    writeBeU16(out, ticksPerQuarterNote);
}

MidiExporter::ActiveTracks MidiExporter::discoverActiveTracks(const SongS & song, const std::vector<EventS> & events) const
{
    ActiveTracks activeTracks;
    std::set<std::string> portNames;
    std::set<size_t> activeTrackIndices;

    for (const auto & event : events) {
        if (auto noteData = event->noteData()) {
            activeTrackIndices.insert(noteData->track());
        } else if (auto ccData = event->midiCcData()) {
            activeTrackIndices.insert(ccData->track());
        } else if (auto pitchBendData = event->pitchBendData()) {
            activeTrackIndices.insert(pitchBendData->track());
        }
    }

    for (const auto & trackIndex : activeTrackIndices) {
        auto instrument = song->instrument(trackIndex);
        if (!instrument) {
            instrument = std::make_shared<Instrument>("Default Instrument");
        }
        juzzlin::L(TAG).info() << "Discovered active track: " << trackIndex;
        activeTracks.trackToInstrument[trackIndex] = instrument;
        std::string port = instrument->midiAddress().portName().toStdString();
        activeTracks.trackToPortName[trackIndex] = port;
        portNames.insert(port);
    }

    std::vector<std::string> sortedPortNames(portNames.begin(), portNames.end());
    std::sort(sortedPortNames.begin(), sortedPortNames.end());

    uint8_t nextPortIndex = 0;
    for (const auto & name : sortedPortNames) {
        activeTracks.portNameToIndex[name] = nextPortIndex++;
    }

    return activeTracks;
}

MidiExporter::ByteVector MidiExporter::initializeTrack(size_t trackIndex, const ActiveTracks & activeTracks) const
{
    ByteVector data;

    const auto & portName = activeTracks.trackToPortName.at(trackIndex);
    const uint8_t portIndex = activeTracks.portNameToIndex.at(portName);

    // Add MIDI Port Meta-Event
    data.push_back(static_cast<char>(0x00)); // Delta time
    data.push_back(static_cast<char>(META_EVENT));
    data.push_back(static_cast<char>(MIDI_PORT_EVENT));
    data.push_back(static_cast<char>(0x01)); // Length
    data.push_back(static_cast<char>(portIndex));
    juzzlin::L(TAG).info() << "Writing MIDI Port Meta-Event for track " << trackIndex << ", portIndex: " << static_cast<int>(portIndex);

    // Add Track Name Meta-Event
    data.push_back(static_cast<char>(0x00)); // Delta time
    data.push_back(static_cast<char>(META_EVENT));
    data.push_back(static_cast<char>(TRACK_NAME_EVENT));
    const std::string trackName = "Track " + std::to_string(trackIndex);
    writeVlq(data, static_cast<uint32_t>(trackName.length()));
    data.insert(data.end(), trackName.begin(), trackName.end());

    return data;
}

void MidiExporter::writeNoteOnEvent(ByteVector & dataOut, uint8_t channel, const NoteData & noteData) const
{
    if (const auto note = noteData.note(); note.has_value()) {
        dataOut.push_back(static_cast<char>(NOTE_ON_STATUS | channel));
        dataOut.push_back(static_cast<char>(*note));
        dataOut.push_back(static_cast<char>(noteData.velocity()));
    }
}

void MidiExporter::writeNoteOffEvent(ByteVector & dataOut, uint8_t channel, const NoteData & noteData) const
{
    if (const auto note = noteData.note(); note.has_value()) {
        dataOut.push_back(static_cast<char>(NOTE_OFF_STATUS | channel));
        dataOut.push_back(static_cast<char>(*note));
        dataOut.push_back(static_cast<char>(0)); // Velocity 0 for note off
    }
}

void MidiExporter::writeControlChangeEvent(ByteVector & dataOut, uint8_t channel, const MidiCcData & ccData) const
{
    dataOut.push_back(static_cast<char>(CONTROL_CHANGE_STATUS | channel));
    dataOut.push_back(static_cast<char>(ccData.controller()));
    dataOut.push_back(static_cast<char>(ccData.value()));
}

void MidiExporter::writePitchBendEvent(ByteVector & dataOut, uint8_t channel, const PitchBendData & pitchBendData) const
{
    dataOut.push_back(static_cast<char>(PITCH_BEND_STATUS | channel));
    dataOut.push_back(static_cast<char>(pitchBendData.lsb()));
    dataOut.push_back(static_cast<char>(pitchBendData.msb()));
}

std::map<size_t, MidiExporter::ByteVector> MidiExporter::buildTrackData(const std::vector<EventS> & events, const ActiveTracks & activeTracks) const
{
    auto initialState = initializeTracks(activeTracks);
    auto processedState = processEvents(std::move(initialState), events);
    return finalizeTracks(std::move(processedState));
}

MidiExporter::TrackProcessingState MidiExporter::initializeTracks(const ActiveTracks & activeTracks) const
{
    TrackProcessingState state;
    std::map<std::string, uint8_t> portChannelCounters;

    for (const auto & [trackIndex, instrument] : activeTracks.trackToInstrument) {
        state.lastTicks[trackIndex] = 0;

        const auto & portName = activeTracks.trackToPortName.at(trackIndex);
        uint8_t & channel = portChannelCounters[portName];
        if (channel == 9) { // Skip drum channel 10
            channel++;
        }
        state.trackToChannelMap[trackIndex] = channel++;

        state.allTracksData[trackIndex] = initializeTrack(trackIndex, activeTracks);
    }
    return state;
}

MidiExporter::TrackProcessingState MidiExporter::processEvents(TrackProcessingState state, const std::vector<EventS> & events) const
{
    for (const auto & event : events) {
        size_t trackIndex = 0;
        if (const auto noteData = event->noteData(); noteData) {
            trackIndex = noteData->track();
        } else if (const auto ccData = event->midiCcData(); ccData) {
            trackIndex = ccData->track();
        } else if (const auto pitchBendData = event->pitchBendData(); pitchBendData) {
            trackIndex = pitchBendData->track();
        } else {
            continue;
        }

        if (!state.allTracksData.contains(trackIndex)) {
            continue;
        }

        auto & currentTrackData = state.allTracksData.at(trackIndex);
        const uint32_t lastTick = state.lastTicks.at(trackIndex);
        const uint8_t midiChannel = state.trackToChannelMap.at(trackIndex);

        const uint32_t deltaTick = static_cast<uint32_t>(event->tick()) - lastTick;
        writeVlq(currentTrackData, deltaTick);
        state.lastTicks.at(trackIndex) = static_cast<uint32_t>(event->tick());

        if (const auto noteData = event->noteData(); noteData) {
            if (noteData->type() == NoteData::Type::NoteOn) {
                writeNoteOnEvent(currentTrackData, midiChannel, *noteData);
            } else {
                writeNoteOffEvent(currentTrackData, midiChannel, *noteData);
            }
        } else if (const auto ccData = event->midiCcData(); ccData) {
            writeControlChangeEvent(currentTrackData, midiChannel, *ccData);
        } else if (const auto pitchBendData = event->pitchBendData(); pitchBendData) {
            writePitchBendEvent(currentTrackData, midiChannel, *pitchBendData);
        }
    }
    return state;
}

std::map<size_t, MidiExporter::ByteVector> MidiExporter::finalizeTracks(TrackProcessingState state) const
{
    for (auto & [trackIndex, data] : state.allTracksData) {
        data.push_back(static_cast<char>(0x00)); // Delta time
        data.push_back(static_cast<char>(META_EVENT));
        data.push_back(static_cast<char>(END_OF_TRACK_EVENT));
        data.push_back(static_cast<char>(0x00)); // Length
    }
    return state.allTracksData;
}

void MidiExporter::writeTempoTrack(std::ostream & out, const SongS & song) const
{
    ByteVector tempoTrackData;
    const uint32_t tempo = 60'000'000 / song->beatsPerMinute();

    // Set Tempo Meta-Event
    tempoTrackData.push_back(static_cast<char>(0x00)); // Delta time
    tempoTrackData.push_back(static_cast<char>(META_EVENT));
    tempoTrackData.push_back(static_cast<char>(SET_TEMPO_EVENT));
    tempoTrackData.push_back(static_cast<char>(0x03)); // Length
    tempoTrackData.push_back(static_cast<char>((tempo >> 16) & 0xff));
    tempoTrackData.push_back(static_cast<char>((tempo >> 8) & 0xff));
    tempoTrackData.push_back(static_cast<char>(tempo & 0xff));

    // End of Track Meta-Event
    tempoTrackData.push_back(static_cast<char>(0x00)); // Delta time
    tempoTrackData.push_back(static_cast<char>(META_EVENT));
    tempoTrackData.push_back(static_cast<char>(END_OF_TRACK_EVENT));
    tempoTrackData.push_back(static_cast<char>(0x00)); // Length

    out.write("MTrk", 4);
    writeBeU32(out, static_cast<uint32_t>(tempoTrackData.size()));
    out.write(tempoTrackData.data(), static_cast<uint32_t>(tempoTrackData.size()));
}

void MidiExporter::writeNoteTracks(std::ostream & out, const std::map<size_t, ByteVector> & allTracksData) const
{
    for (const auto & [trackIndex, trackData] : allTracksData) {
        juzzlin::L(TAG).info() << "Writing track: " << trackIndex;
        out.write("MTrk", 4);
        writeBeU32(out, static_cast<uint32_t>(trackData.size()));
        out.write(trackData.data(), static_cast<uint32_t>(trackData.size()));
    }
}

} // namespace noteahead
