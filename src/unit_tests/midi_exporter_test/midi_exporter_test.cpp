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

#include "midi_exporter_test.hpp"

#include "../../application/position.hpp"
#include "../../application/service/automation_service.hpp"
#include "../../application/service/mixer_service.hpp"
#include "../../application/service/property_service.hpp"
#include "../../application/service/side_chain_service.hpp"
#include "../../domain/note_data.hpp"
#include "../../domain/song.hpp"
#include "../../infra/midi/export/midi_exporter.hpp"

#include <algorithm>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <memory>
#include <vector>

#include <QFile>
#include <QTemporaryFile>
#include <QTest>

namespace noteahead {

// Helper struct to represent a MIDI event for testing
struct MidiTestEvent
{
    enum class Type
    {
        BankMsb,
        BankLsb,
        ProgramChange,
        NoteOff,
        NoteOn,
        ControlChange,
        PitchBend
    };

    uint32_t tick;

    Type type;

    uint8_t note = 0;
    uint8_t velocity = 0;
    uint8_t bankMsb = 0;
    uint8_t bankLsb = 0;
    uint8_t patch = 0;
    uint8_t channel = 0;
    uint8_t port = 0;
    uint8_t controller = 0;
    uint8_t value = 0;
    int16_t pitchBend = 0;

    auto operator<=>(const MidiTestEvent&) const = default;
};

// Helper function to read big-endian 16-bit unsigned integer
auto readBeU16(std::istream& in) -> uint16_t
{
    uint8_t b1 {};
    uint8_t b2 {};
    in.read(reinterpret_cast<char*>(&b1), 1);
    in.read(reinterpret_cast<char*>(&b2), 1);
    return (static_cast<uint16_t>(b1) << 8) | b2;
}

// Helper function to read big-endian 32-bit unsigned integer
auto readBeU32(std::istream& in) -> uint32_t
{
    uint8_t b1 {};
    uint8_t b2 {};
    uint8_t b3 {};
    uint8_t b4 {};
    in.read(reinterpret_cast<char*>(&b1), 1);
    in.read(reinterpret_cast<char*>(&b2), 1);
    in.read(reinterpret_cast<char*>(&b3), 1);
    in.read(reinterpret_cast<char*>(&b4), 1);
    return (static_cast<uint32_t>(b1) << 24) | (static_cast<uint32_t>(b2) << 16) | (static_cast<uint32_t>(b3) << 8) | b4;
}

// Helper function to read a variable-length quantity (VLQ)
auto readVlq(std::istream& in) -> uint32_t
{
    uint32_t value = 0;
    uint8_t byte {};
    do {
        in.read(reinterpret_cast<char*>(&byte), 1);
        if (!in.good()) {
            return 0; // Or throw an exception
        }
        value = (value << 7) | (byte & 0x7F);
    } while (byte & 0x80);
    return value;
}

// Helper function to parse a MIDI file and extract note events
auto readMidiFile(const std::string& fileName) -> std::vector<MidiTestEvent>
{
    std::vector<MidiTestEvent> events;
    std::ifstream in(fileName, std::ios::binary);
    if (!in.is_open()) {
        std::cerr << "readMidiFile: Failed to open file: " << fileName << std::endl;
        return events;
    }

    char chunkType[4];

    // Read MThd chunk
    in.read(chunkType, 4);
    if (std::string(chunkType, 4) != "MThd") {
        std::cerr << "readMidiFile: Expected MThd chunk, but got " << std::string(chunkType, 4) << std::endl;
        return {};
    }
    const auto chunkLength = readBeU32(in);
    if (chunkLength != 6) {
        std::cerr << "readMidiFile: Expected MThd chunk length 6, but got " << chunkLength << std::endl;
        return {};
    }

    const auto format = readBeU16(in);
    if (format != 1) {
        std::cerr << "readMidiFile: Expected MIDI format 1, but got " << format << std::endl;
        return {};
    }
    const auto numTracks = readBeU16(in);
    [[maybe_unused]] const auto division = readBeU16(in); // Ticks per quarter note

    for (int i = 0; i < numTracks; ++i) {
        in.read(chunkType, 4);
        if (std::string(chunkType, 4) != "MTrk") {
            std::cerr << "readMidiFile: Expected MTrk chunk, but got " << std::string(chunkType, 4) << ". Skipping chunk." << std::endl;
            const auto trackChunkLength = readBeU32(in);
            in.seekg(trackChunkLength, std::ios_base::cur);
            continue;
        }
        const auto trackChunkLength = readBeU32(in);
        const auto trackEndPos = static_cast<uint32_t>(in.tellg()) + trackChunkLength;

        uint8_t statusByte = 0;
        uint32_t currentTrackTick = 0;
        uint8_t currentPort = 0; // Default to port 0

        while (static_cast<uint32_t>(in.tellg()) < trackEndPos) {
            const auto deltaTime = readVlq(in);
            currentTrackTick += deltaTime;

            uint8_t currentByte;
            in.read(reinterpret_cast<char*>(&currentByte), 1);

            if (currentByte & 0x80) { // New status byte
                statusByte = currentByte;
            } else { // Running status
                in.seekg(-1, std::ios_base::cur);
            }

            if (statusByte == 0xFF) { // Meta event
                const auto metaType = in.get();
                const auto metaLength = readVlq(in);
                if (metaType == 0x21) { // MIDI Port
                    if (metaLength == 1) {
                        currentPort = static_cast<uint8_t>(in.get());
                         std::cerr << "readMidiFile: MIDI Port Meta-Event found, setting current_port to " << static_cast<int>(currentPort) << std::endl;
                    } else {
                        in.seekg(metaLength, std::ios_base::cur);
                    }
                } else {
                    in.seekg(metaLength, std::ios_base::cur);
                }

                if (metaType == 0x2F) { // End of Track
                    break;
                }
            } else if ((statusByte & 0xF0) == 0x90) { // Note On
                const auto channel = static_cast<uint8_t>(statusByte & 0x0F);
                const auto note = static_cast<uint8_t>(in.get());
                const auto velocity = static_cast<uint8_t>(in.get());
                if (velocity > 0) {
                    events.push_back({ .tick = currentTrackTick, .type = MidiTestEvent::Type::NoteOn, .note = note, .velocity = velocity, .channel = channel, .port = currentPort });
                } else {
                    events.push_back({ .tick = currentTrackTick, .type = MidiTestEvent::Type::NoteOff, .note = note, .velocity = velocity, .channel = channel, .port = currentPort });
                }
            } else if ((statusByte & 0xF0) == 0x80) { // Note Off
                const auto channel = static_cast<uint8_t>(statusByte & 0x0F);
                const auto note = static_cast<uint8_t>(in.get());
                const auto velocity = static_cast<uint8_t>(in.get());
                events.push_back({ .tick = currentTrackTick, .type = MidiTestEvent::Type::NoteOff, .note = note, .velocity = velocity, .channel = channel, .port = currentPort });
            } else if ((statusByte & 0xF0) == 0xB0) { // Control Change
                const auto channel = static_cast<uint8_t>(statusByte & 0x0F);
                const auto controller = static_cast<uint8_t>(in.get());
                const auto value = static_cast<uint8_t>(in.get());
                if (controller == 0x00) { // Bank Select MSB
                    events.push_back({ .tick = currentTrackTick, .type = MidiTestEvent::Type::BankMsb, .bankMsb = value, .channel = channel, .port = currentPort });
                } else if (controller == 0x20) { // Bank Select LSB
                    events.push_back({ .tick = currentTrackTick, .type = MidiTestEvent::Type::BankLsb, .bankLsb = value, .channel = channel, .port = currentPort });
                } else {
                    events.push_back({ .tick = currentTrackTick, .type = MidiTestEvent::Type::ControlChange, .channel = channel, .port = currentPort, .controller = controller, .value = value });
                }
            } else if ((statusByte & 0xF0) == 0xC0) { // Program Change
                const auto channel = static_cast<uint8_t>(statusByte & 0x0F);
                const auto patch = static_cast<uint8_t>(in.get());
                events.push_back({ .tick = currentTrackTick, .type = MidiTestEvent::Type::ProgramChange, .patch = patch, .channel = channel, .port = currentPort });
            } else if ((statusByte & 0xF0) == 0xD0) {
                in.seekg(1, std::ios_base::cur);
            } else if ((statusByte & 0xF0) == 0xE0) { // Pitch Bend
                const auto channel = static_cast<uint8_t>(statusByte & 0x0F);
                const auto lsb = static_cast<uint8_t>(in.get());
                const auto msb = static_cast<uint8_t>(in.get());
                const auto value = static_cast<int16_t>((msb << 7) | lsb);
                events.push_back({ .tick = currentTrackTick, .type = MidiTestEvent::Type::PitchBend, .channel = channel, .port = currentPort, .pitchBend = value });
            } else if ((statusByte & 0xF0) == 0xA0) {
                in.seekg(2, std::ios_base::cur);
            }
        }
    }

    std::ranges::sort(events, [](const auto& a, const auto& b) {
        if (a.tick != b.tick) {
            return a.tick < b.tick;
        }
        return a.type < b.type;
    });

    return events;
}

void MidiExporterTest::test_exportTo_singleNote_shouldExportCorrectly()
{
    const auto song = std::make_shared<Song>();
    song->setBeatsPerMinute(120);
    song->setLinesPerBeat(4);

    const auto instrument = std::make_shared<Instrument>("TestInstrument");
    song->setInstrument(0, instrument);

    const Position noteOnPosition = { 0, 0, 0, 0, 0 };
    NoteData noteOnData;
    noteOnData.setAsNoteOn(60, 100);
    song->setNoteDataAtPosition(noteOnData, noteOnPosition);

    const Position noteOffPosition = { 0, 0, 0, 1, 0 };
    NoteData noteOffData;
    noteOffData.setAsNoteOff(60);
    song->setNoteDataAtPosition(noteOffData, noteOffPosition);

    QTemporaryFile tempFile;
    QVERIFY(tempFile.open());
    const auto fileName = tempFile.fileName();
    tempFile.close();

    const auto automationService = std::make_shared<AutomationService>(std::make_shared<PropertyService>());
    const auto mixerService = std::make_shared<MixerService>();
    const auto sideChainService = std::make_shared<SideChainService>();
    MidiExporter exporter { automationService, mixerService, sideChainService };
    exporter.exportTo(fileName.toStdString(), song, 0, std::numeric_limits<size_t>::max(), MidiExportOptions());

    QVERIFY(QFile::exists(fileName));
    QVERIFY(QFile { fileName }.size() > 0);

    const auto exportedEvents = readMidiFile(fileName.toStdString());

    const std::vector<MidiTestEvent> expectedEvents = {
        { .tick = 0, .type = MidiTestEvent::Type::NoteOn, .note = 60, .velocity = 100, .channel = 0, .port = 0 },
        { .tick = static_cast<uint32_t>(song->ticksPerLine()), .type = MidiTestEvent::Type::NoteOff, .note = 60, .velocity = 0, .channel = 0, .port = 0 }
    };

    QCOMPARE(exportedEvents.size(), expectedEvents.size());
    for (size_t i = 0; i < exportedEvents.size(); ++i) {
        QCOMPARE(exportedEvents[i], expectedEvents[i]);
    }
}

void MidiExporterTest::test_exportTo_multipleNotesAndTracks_shouldExportCorrectly()
{
    const auto song = std::make_shared<Song>();
    song->setBeatsPerMinute(120);
    song->setLinesPerBeat(4);

    const auto instrument0 = std::make_shared<Instrument>("Track0Instrument");
    auto midiAddress0 = instrument0->midiAddress();
    midiAddress0.setPort("PortA");
    instrument0->setMidiAddress(midiAddress0);
    song->setInstrument(0, instrument0);

    const auto instrument1 = std::make_shared<Instrument>("Track1Instrument");
    auto midiAddress1 = instrument1->midiAddress();
    midiAddress1.setPort("PortB");
    instrument1->setMidiAddress(midiAddress1);
    song->setInstrument(1, instrument1);

    const auto ticksPerLine = static_cast<uint32_t>(song->ticksPerLine());

    NoteData noteData;
    noteData.setAsNoteOn(60, 100);
    song->setNoteDataAtPosition(noteData, { 0, 0, 0, 0, 0 });
    noteData.setAsNoteOff(60);
    song->setNoteDataAtPosition(noteData, { 0, 0, 0, 1, 0 });
    noteData.setAsNoteOn(62, 90);
    song->setNoteDataAtPosition(noteData, { 0, 0, 0, 2, 0 });
    noteData.setAsNoteOff(62);
    song->setNoteDataAtPosition(noteData, { 0, 0, 0, 4, 0 });

    // Track 1 (PortB, Channel 0): E4 at 1 line, F4 at 3 lines
    noteData.setAsNoteOn(64, 80);
    song->setNoteDataAtPosition(noteData, { 0, 1, 0, 1, 0 });
    noteData.setAsNoteOff(64);
    song->setNoteDataAtPosition(noteData, { 0, 1, 0, 2, 0 });
    noteData.setAsNoteOn(65, 70);
    song->setNoteDataAtPosition(noteData, { 0, 1, 0, 3, 0 });
    noteData.setAsNoteOff(65);
    song->setNoteDataAtPosition(noteData, { 0, 1, 0, 4, 0 });

    QTemporaryFile tempFile;
    QVERIFY(tempFile.open());
    const auto fileName = tempFile.fileName();
    tempFile.close();

    const auto automationService = std::make_shared<AutomationService>(std::make_shared<PropertyService>());
    const auto mixerService = std::make_shared<MixerService>();
    const auto sideChainService = std::make_shared<SideChainService>();
    MidiExporter exporter { automationService, mixerService, sideChainService };
    exporter.exportTo(fileName.toStdString(), song, 0, std::numeric_limits<size_t>::max(), MidiExportOptions());

    QVERIFY(QFile::exists(fileName));
    QVERIFY(QFile { fileName }.size() > 0);

    const auto exportedEvents = readMidiFile(fileName.toStdString());

    const std::vector<MidiTestEvent> expectedEvents = {
        { .tick = 0, .type = MidiTestEvent::Type::NoteOn, .note = 60, .velocity = 100, .channel = 0, .port = 0 }, // PortA, Chan0
        { .tick = ticksPerLine, .type = MidiTestEvent::Type::NoteOff, .note = 60, .velocity = 0, .channel = 0, .port = 0 }, // PortA, Chan0
        { .tick = ticksPerLine, .type = MidiTestEvent::Type::NoteOn, .note = 64, .velocity = 80, .channel = 0, .port = 1 }, // PortB, Chan0
        { .tick = 2 * ticksPerLine, .type = MidiTestEvent::Type::NoteOff, .note = 64, .velocity = 0, .channel = 0, .port = 1 }, // PortB, Chan0
        { .tick = 2 * ticksPerLine, .type = MidiTestEvent::Type::NoteOn, .note = 62, .velocity = 90, .channel = 0, .port = 0 }, // PortA, Chan0
        { .tick = 3 * ticksPerLine, .type = MidiTestEvent::Type::NoteOn, .note = 65, .velocity = 70, .channel = 0, .port = 1 }, // PortB, Chan0
        { .tick = 4 * ticksPerLine, .type = MidiTestEvent::Type::NoteOff, .note = 62, .velocity = 0, .channel = 0, .port = 0 }, // PortA, Chan0
        { .tick = 4 * ticksPerLine, .type = MidiTestEvent::Type::NoteOff, .note = 65, .velocity = 0, .channel = 0, .port = 1 } // PortB, Chan0
    };

    QCOMPARE(exportedEvents.size(), expectedEvents.size());
    for (size_t i = 0; i < exportedEvents.size(); ++i) {
        QCOMPARE(exportedEvents[i], expectedEvents[i]);
    }
}

void MidiExporterTest::test_exportTo_timing_shouldBeCorrect()
{
    const auto song = std::make_shared<Song>();
    song->setBeatsPerMinute(120);
    song->setLinesPerBeat(4);

    const auto instrument = std::make_shared<Instrument>("TestInstrument");
    song->setInstrument(0, instrument);

    const Position noteOnPosition = { 0, 0, 0, 0, 0 };
    NoteData noteOnData;
    noteOnData.setAsNoteOn(60, 100);
    song->setNoteDataAtPosition(noteOnData, noteOnPosition);

    const Position noteOffPosition = { 0, 0, 0, 4, 0 };
    NoteData noteOffData;
    noteOffData.setAsNoteOff(60);
    song->setNoteDataAtPosition(noteOffData, noteOffPosition);

    QTemporaryFile tempFile;
    QVERIFY(tempFile.open());
    const auto fileName = tempFile.fileName();
    tempFile.close();

    const auto automationService = std::make_shared<AutomationService>(std::make_shared<PropertyService>());
    const auto mixerService = std::make_shared<MixerService>();
    const auto sideChainService = std::make_shared<SideChainService>();
    MidiExporter exporter { automationService, mixerService, sideChainService };
    exporter.exportTo(fileName.toStdString(), song, 0, std::numeric_limits<size_t>::max(), MidiExportOptions());

    QVERIFY(QFile::exists(fileName));
    QVERIFY(QFile(fileName).size() > 0);

    const auto exportedEvents = readMidiFile(fileName.toStdString());

    const auto ticksPerLine = song->ticksPerLine();
    const auto linesPerBeat = song->linesPerBeat();
    const auto ticksPerBeat = static_cast<uint32_t>(ticksPerLine * linesPerBeat);

    const std::vector<MidiTestEvent> expectedEvents = {
        { .tick = 0, .type = MidiTestEvent::Type::NoteOn, .note = 60, .velocity = 100, .channel = 0, .port = 0 },
        { .tick = ticksPerBeat, .type = MidiTestEvent::Type::NoteOff, .note = 60, .velocity = 0, .channel = 0, .port = 0 }
    };

    QCOMPARE(exportedEvents.size(), expectedEvents.size());
    for (size_t i = 0; i < exportedEvents.size(); ++i) {
        QCOMPARE(exportedEvents[i], expectedEvents[i]);
    }
}

void MidiExporterTest::test_exportTo_mutedAndSoloedTracks_shouldExportCorrectly()
{
    const auto song = std::make_shared<Song>();
    song->setBeatsPerMinute(120);
    song->setLinesPerBeat(4);

    const auto instrument0 = std::make_shared<Instrument>("Track0Instrument");
    song->setInstrument(0, instrument0);

    const auto instrument1 = std::make_shared<Instrument>("Track1Instrument");
    song->setInstrument(1, instrument1);

    const auto instrument2 = std::make_shared<Instrument>("Track2Instrument");
    song->setInstrument(2, instrument2);

    const auto ticksPerLine = static_cast<uint32_t>(song->ticksPerLine());

    NoteData noteData;
    noteData.setAsNoteOn(60, 100);
    song->setNoteDataAtPosition(noteData, { 0, 0, 0, 0, 0 });
    noteData.setAsNoteOff(60);
    song->setNoteDataAtPosition(noteData, { 0, 0, 0, 1, 0 });

    noteData.setAsNoteOn(62, 90);
    song->setNoteDataAtPosition(noteData, { 0, 1, 0, 0, 0 });
    noteData.setAsNoteOff(62);
    song->setNoteDataAtPosition(noteData, { 0, 1, 0, 1, 0 });
    
    noteData.setAsNoteOn(64, 80);
    song->setNoteDataAtPosition(noteData, { 0, 2, 0, 0, 0 });
    noteData.setAsNoteOff(64);
    song->setNoteDataAtPosition(noteData, { 0, 2, 0, 1, 0 });

    const auto automationService = std::make_shared<AutomationService>(std::make_shared<PropertyService>());
    const auto mixerService = std::make_shared<MixerService>();
    mixerService->muteTrack(1, true);
    mixerService->soloTrack(2, true);

    QTemporaryFile tempFile;
    QVERIFY(tempFile.open());
    const auto fileName = tempFile.fileName();
    tempFile.close();

    const auto sideChainService = std::make_shared<SideChainService>();
    MidiExporter exporter { automationService, mixerService, sideChainService };
    exporter.exportTo(fileName.toStdString(), song, 0, std::numeric_limits<size_t>::max(), MidiExportOptions());

    const auto exportedEvents = readMidiFile(fileName.toStdString());

    const std::vector<MidiTestEvent> expectedEvents = {
        { .tick = 0, .type = MidiTestEvent::Type::NoteOn, .note = 64, .velocity = 80, .channel = 0, .port = 0 },
        { .tick = ticksPerLine, .type = MidiTestEvent::Type::NoteOff, .note = 64, .velocity = 0, .channel = 0, .port = 0 }
    };

    QCOMPARE(exportedEvents.size(), expectedEvents.size());
    for (size_t i = 0; i < exportedEvents.size(); ++i) {
        QCOMPARE(exportedEvents[i], expectedEvents[i]);
    }
}

void MidiExporterTest::test_exportTo_rangedExport_shouldExportCorrectRange()
{
    const auto song = std::make_shared<Song>();
    song->setBeatsPerMinute(120);
    song->setLinesPerBeat(4);

    const auto instrument = std::make_shared<Instrument>("TestInstrument");
    song->setInstrument(0, instrument);

    song->createPattern(1);
    song->setLength(2);
    song->setPatternAtSongPosition(0, 0);
    song->setPatternAtSongPosition(1, 1);

    // In pattern 0, put a note.
    NoteData p0noteOn;
    p0noteOn.setAsNoteOn(60, 100);
    song->setNoteDataAtPosition(p0noteOn, { 0, 0, 0, 0, 0 });
    NoteData p0noteOff;
    p0noteOff.setAsNoteOff(60);
    song->setNoteDataAtPosition(p0noteOff, { 0, 0, 0, 1, 0 });

    // In pattern 1, put a note.
    NoteData p1noteOn;
    p1noteOn.setAsNoteOn(62, 100);
    song->setNoteDataAtPosition(p1noteOn, { 1, 0, 0, 0, 0 });
    NoteData p1noteOff;
    p1noteOff.setAsNoteOff(62);
    song->setNoteDataAtPosition(p1noteOff, { 1, 0, 0, 1, 0 });

    QTemporaryFile tempFile;
    QVERIFY(tempFile.open());
    const auto fileName = tempFile.fileName();
    tempFile.close();

    const auto automationService = std::make_shared<AutomationService>(std::make_shared<PropertyService>());
    const auto mixerService = std::make_shared<MixerService>();
    const auto sideChainService = std::make_shared<SideChainService>();
    MidiExporter exporter { automationService, mixerService, sideChainService };
    exporter.exportTo(fileName.toStdString(), song, 0, std::numeric_limits<size_t>::max(), MidiExportOptions());

    const auto startPosition = 1;
    const auto endPosition = 2;
    exporter.exportTo(fileName.toStdString(), song, startPosition, endPosition, MidiExportOptions());

    QVERIFY(QFile::exists(fileName));
    QVERIFY(QFile { fileName }.size() > 0);

    const auto exportedEvents = readMidiFile(fileName.toStdString());
    const auto ticksPerLine = song->ticksPerLine();

    const std::vector<MidiTestEvent> expectedEvents = {
        { .tick = 0, .type = MidiTestEvent::Type::NoteOn, .note = 62, .velocity = 100, .channel = 0, .port = 0 },
        { .tick = static_cast<uint32_t>(ticksPerLine), .type = MidiTestEvent::Type::NoteOff, .note = 62, .velocity = 0, .channel = 0, .port = 0 },
    };

    QCOMPARE(exportedEvents.size(), expectedEvents.size());
    for (size_t i = 0; i < exportedEvents.size(); ++i) {
        QCOMPARE(exportedEvents[i], expectedEvents[i]);
    }
}

void MidiExporterTest::test_exportTo_bankAndProgramChange_shouldExportCorrectly()
{
    const auto song = std::make_shared<Song>();
    song->setBeatsPerMinute(120);
    song->setLinesPerBeat(4);

    const auto instrument = std::make_shared<Instrument>("TestInstrument");
    auto settings = instrument->settings();
    settings.bank = InstrumentSettings::Bank { .lsb = 10, .msb = 20 };
    settings.patch = 30;
    instrument->setSettings(settings);
    song->setInstrument(0, instrument);

    // Add a note so the track is considered active
    NoteData noteData;
    noteData.setAsNoteOn(60, 100);
    song->setNoteDataAtPosition(noteData, { 0, 0, 0, 0, 0 });

    const auto automationService = std::make_shared<AutomationService>(std::make_shared<PropertyService>());
    const auto mixerService = std::make_shared<MixerService>();
    const auto sideChainService = std::make_shared<SideChainService>();
    MidiExporter exporter { automationService, mixerService, sideChainService };

    {
        QTemporaryFile tempFile;
        QVERIFY(tempFile.open());
        const auto fileName = tempFile.fileName();
        tempFile.close();

        MidiExportOptions options;
        options.exportBank = true;
        options.exportProgramChange = true;
        exporter.exportTo(fileName.toStdString(), song, 0, song->length(), options);

        const auto exportedEvents = readMidiFile(fileName.toStdString());

        // Expected at tick 0: Bank MSB (20), Bank LSB (10), Program Change (30), then Note On
        QVERIFY(exportedEvents.size() >= 4);
        QCOMPARE(exportedEvents[0].type, MidiTestEvent::Type::BankMsb);
        QCOMPARE(exportedEvents[0].bankMsb, 20);
        QCOMPARE(exportedEvents[1].type, MidiTestEvent::Type::BankLsb);
        QCOMPARE(exportedEvents[1].bankLsb, 10);
        QCOMPARE(exportedEvents[2].type, MidiTestEvent::Type::ProgramChange);
        QCOMPARE(exportedEvents[2].patch, 30);
    }

    {
        QTemporaryFile tempFile;
        QVERIFY(tempFile.open());
        const auto fileName = tempFile.fileName();
        tempFile.close();

        MidiExportOptions options;
        options.exportBank = false;
        options.exportProgramChange = true;
        exporter.exportTo(fileName.toStdString(), song, 0, song->length(), options);

        const auto exportedEvents = readMidiFile(fileName.toStdString());
        
        // Expected at tick 0: Program Change (30), then Note On
        QVERIFY(exportedEvents.size() >= 2);
        QCOMPARE(exportedEvents[0].type, MidiTestEvent::Type::ProgramChange);
        QCOMPARE(exportedEvents[0].patch, 30);
        QCOMPARE(exportedEvents[1].type, MidiTestEvent::Type::NoteOn);
    }

    {
        QTemporaryFile tempFile;
        QVERIFY(tempFile.open());
        const auto fileName = tempFile.fileName();
        tempFile.close();

        MidiExportOptions options;
        options.exportBank = true;
        options.exportProgramChange = false;
        exporter.exportTo(fileName.toStdString(), song, 0, song->length(), options);

        const auto exportedEvents = readMidiFile(fileName.toStdString());
        
        // Expected at tick 0: Bank MSB (20), Bank LSB (10), then Note On
        QVERIFY(exportedEvents.size() >= 3);
        QCOMPARE(exportedEvents[0].type, MidiTestEvent::Type::BankMsb);
        QCOMPARE(exportedEvents[0].bankMsb, 20);
        QCOMPARE(exportedEvents[1].type, MidiTestEvent::Type::BankLsb);
        QCOMPARE(exportedEvents[1].bankLsb, 10);
        QCOMPARE(exportedEvents[2].type, MidiTestEvent::Type::NoteOn);
    }
}

void MidiExporterTest::test_exportTo_noNotesButSettings_shouldExportSettings()
{
    const auto song = std::make_shared<Song>();
    song->setBeatsPerMinute(120);
    song->setLinesPerBeat(4);

    // Track 0 has settings but NO notes
    const auto instrument = std::make_shared<Instrument>("TestInstrument");
    auto settings = instrument->settings();
    settings.bank = InstrumentSettings::Bank { .lsb = 15, .msb = 25 };
    settings.patch = 35;
    instrument->setSettings(settings);
    song->setInstrument(0, instrument);

    const auto automationService = std::make_shared<AutomationService>(std::make_shared<PropertyService>());
    const auto mixerService = std::make_shared<MixerService>();
    const auto sideChainService = std::make_shared<SideChainService>();
    MidiExporter exporter { automationService, mixerService, sideChainService };

    QTemporaryFile tempFile;
    QVERIFY(tempFile.open());
    const auto fileName = tempFile.fileName();
    tempFile.close();

    MidiExportOptions options;
    options.exportBank = true;
    options.exportProgramChange = true;
    exporter.exportTo(fileName.toStdString(), song, 0, song->length(), options);

    const auto exportedEvents = readMidiFile(fileName.toStdString());
    
    // Expected at tick 0: Bank MSB (25), Bank LSB (15), Program Change (35)
    // No NoteOn should be present
    QVERIFY(exportedEvents.size() >= 3);
    QCOMPARE(exportedEvents[0].type, MidiTestEvent::Type::BankMsb);
    QCOMPARE(exportedEvents[0].bankMsb, 25);
    QCOMPARE(exportedEvents[1].type, MidiTestEvent::Type::BankLsb);
    QCOMPARE(exportedEvents[1].bankLsb, 15);
    QCOMPARE(exportedEvents[2].type, MidiTestEvent::Type::ProgramChange);
    QCOMPARE(exportedEvents[2].patch, 35);

    for (const auto & event : exportedEvents) {
        QVERIFY(event.type != MidiTestEvent::Type::NoteOn);
    }
}

void MidiExporterTest::test_exportTo_midiCcAndPitchBend_shouldExportCorrectly()
{
    const auto song = std::make_shared<Song>();
    song->setBeatsPerMinute(120);
    song->setLinesPerBeat(4);

    const auto instrument = std::make_shared<Instrument>("TestInstrument");
    song->setInstrument(0, instrument);

    const auto automationService = std::make_shared<AutomationService>(std::make_shared<PropertyService>());

    // Add MIDI CC automation: Pattern 0, Track 0, Column 0, Controller 7, Line 0 to 1, Value 100 to 110
    automationService->addMidiCcAutomation(0, 0, 0, 7, 0, 1, 100, 110, "Test CC", true, 4, 0);

    // Add Pitch Bend automation: Pattern 0, Track 0, Column 0, Line 2 to 3, Value 50 to 60
    automationService->addPitchBendAutomation(0, 0, 0, 2, 3, 50, 60, "Test PB", true);

    const auto mixerService = std::make_shared<MixerService>();
    const auto sideChainService = std::make_shared<SideChainService>();
    MidiExporter exporter { automationService, mixerService, sideChainService };

    const auto ticksPerLine = static_cast<uint32_t>(song->ticksPerLine());

    {
        QTemporaryFile tempFile;
        QVERIFY(tempFile.open());
        const auto fileName = tempFile.fileName();
        tempFile.close();

        MidiExportOptions options;
        options.exportMidiCc = true;
        options.exportPitchBend = true;
        exporter.exportTo(fileName.toStdString(), song, 0, song->length(), options);

        const auto exportedEvents = readMidiFile(fileName.toStdString());

        // Expected:
        // CC 7 value 100 at tick 0
        // CC 7 value 110 at tick ticksPerLine
        // Pitch Bend 50% at tick 2*ticksPerLine
        // Pitch Bend 60% at tick 3*ticksPerLine

        QVERIFY(exportedEvents.size() >= 4);

        QCOMPARE(exportedEvents[0].type, MidiTestEvent::Type::ControlChange);
        QCOMPARE(exportedEvents[0].tick, 0u);
        QCOMPARE(exportedEvents[0].controller, 7);
        QCOMPARE(exportedEvents[0].value, 100);

        QCOMPARE(exportedEvents[1].type, MidiTestEvent::Type::ControlChange);
        QCOMPARE(exportedEvents[1].tick, ticksPerLine);
        QCOMPARE(exportedEvents[1].controller, 7);
        QCOMPARE(exportedEvents[1].value, 110);

        QCOMPARE(exportedEvents[2].type, MidiTestEvent::Type::PitchBend);
        QCOMPARE(exportedEvents[2].tick, 2 * ticksPerLine);
        QCOMPARE(exportedEvents[2].pitchBend, 12287);

        QCOMPARE(exportedEvents[3].type, MidiTestEvent::Type::PitchBend);
        QCOMPARE(exportedEvents[3].tick, 3 * ticksPerLine);
        QCOMPARE(exportedEvents[3].pitchBend, 13106);
    }

    {
        QTemporaryFile tempFile;
        QVERIFY(tempFile.open());
        const auto fileName = tempFile.fileName();
        tempFile.close();

        MidiExportOptions options;
        options.exportMidiCc = false;
        options.exportPitchBend = false;
        exporter.exportTo(fileName.toStdString(), song, 0, song->length(), options);

        const auto exportedEvents = readMidiFile(fileName.toStdString());
        QCOMPARE(exportedEvents.size(), 0u);
    }
}

void MidiExporterTest::test_exportTo_emptySong_shouldExportValidMidi()
{
    const auto song = std::make_shared<Song>();
    song->initialize();

    QTemporaryFile tempFile;
    QVERIFY(tempFile.open());
    const auto fileName = tempFile.fileName();
    tempFile.close();

    const auto automationService = std::make_shared<AutomationService>(std::make_shared<PropertyService>());
    const auto mixerService = std::make_shared<MixerService>();
    const auto sideChainService = std::make_shared<SideChainService>();
    MidiExporter exporter { automationService, mixerService, sideChainService };
    exporter.exportTo(fileName.toStdString(), song, 0, std::numeric_limits<size_t>::max(), MidiExportOptions());

    QVERIFY(QFile::exists(fileName));
    // Header (14) + Tempo track header (8) + Tempo track data (11) = 33 bytes
    QVERIFY(QFile { fileName }.size() >= 33);
}

void MidiExporterTest::test_exportTo_nonSequentialTracks_shouldExportCorrectly()
{
    const auto song = std::make_shared<Song>();
    song->initialize();

    // Default song has 8 tracks (0-7). Let's add more and delete some.
    const auto track8 = song->addTrackToRightOf(7);
    song->addTrackToRightOf(track8);

    // Indices should be 0-9. Let's delete 8.
    song->deleteTrack(track8);

    // trackCount() should return 9.
    QCOMPARE(song->trackCount(), 9u);
    const auto indices = song->trackIndices();
    QVERIFY(indices.size() == 9);
    // Index 8 should be missing
    QVERIFY(std::ranges::find(indices, 8) == indices.end());
    QVERIFY(std::ranges::find(indices, 9) != indices.end());

    // Add note to track 9
    NoteData noteData;
    noteData.setAsNoteOn(60, 100);
    song->setNoteDataAtPosition(noteData, { 0, 9, 0, 0, 0 });

    QTemporaryFile tempFile;
    QVERIFY(tempFile.open());
    const auto fileName = tempFile.fileName();
    tempFile.close();

    const auto automationService = std::make_shared<AutomationService>(std::make_shared<PropertyService>());
    const auto mixerService = std::make_shared<MixerService>();
    const auto sideChainService = std::make_shared<SideChainService>();
    MidiExporter exporter { automationService, mixerService, sideChainService };

    // This should not throw!
    exporter.exportTo(fileName.toStdString(), song, 0, std::numeric_limits<size_t>::max(), MidiExportOptions());

    QVERIFY(QFile::exists(fileName));
    QVERIFY(QFile { fileName }.size() > 0);
}

        } // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::MidiExporterTest)
