#include "midi_exporter_test.hpp"

#include "../../application/position.hpp"
#include "../../application/service/automation_service.hpp"
#include "../../application/service/mixer_service.hpp"
#include "../../application/service/side_chain_service.hpp"
#include "../../domain/note_data.hpp"
#include "../../domain/song.hpp"
#include "../../infra/midi/export/midi_exporter.hpp"

#include <algorithm> // For std::ranges::sort
#include <cstddef> // For size_t
#include <fstream>
#include <iostream> // For std::cerr and std::endl
#include <memory> // For std::make_shared
#include <vector> // For std::vector

#include <QFile>
#include <QTemporaryFile>

namespace noteahead {

// Helper struct to represent a MIDI event for testing
struct MidiTestEvent
{
    enum class Type
    {
        NoteOff,
        NoteOn
    };

    uint32_t tick;

    Type type;

    uint8_t note;
    uint8_t velocity;
    uint8_t channel;
    uint8_t port; // Added to verify port assignment

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
                    events.push_back({ currentTrackTick, MidiTestEvent::Type::NoteOn, note, velocity, channel, currentPort });
                } else {
                    events.push_back({ currentTrackTick, MidiTestEvent::Type::NoteOff, note, velocity, channel, currentPort });
                }
            } else if ((statusByte & 0xF0) == 0x80) { // Note Off
                const auto channel = static_cast<uint8_t>(statusByte & 0x0F);
                const auto note = static_cast<uint8_t>(in.get());
                const auto velocity = static_cast<uint8_t>(in.get());
                events.push_back({ currentTrackTick, MidiTestEvent::Type::NoteOff, note, velocity, channel, currentPort });
            } else if ((statusByte & 0xF0) == 0xC0 || (statusByte & 0xF0) == 0xD0) {
                in.seekg(1, std::ios_base::cur);
            } else if ((statusByte & 0xF0) == 0xA0 || (statusByte & 0xF0) == 0xB0 || (statusByte & 0xF0) == 0xE0) {
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
    auto song = std::make_shared<Song>();
    song->setBeatsPerMinute(120);
    song->setLinesPerBeat(4);

    auto instrument = std::make_shared<Instrument>("TestInstrument");
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

    const auto automationService = std::make_shared<AutomationService>();
    const auto mixerService = std::make_shared<MixerService>();
    const auto sideChainService = std::make_shared<SideChainService>();
    MidiExporter exporter { automationService, mixerService, sideChainService };
    exporter.exportTo(fileName.toStdString(), song);

    QVERIFY(QFile::exists(fileName));
    QVERIFY(QFile { fileName }.size() > 0);

    const auto exportedEvents = readMidiFile(fileName.toStdString());

    const std::vector<MidiTestEvent> expectedEvents = {
        { 0, MidiTestEvent::Type::NoteOn, 60, 100, 0, 0 },
        { static_cast<uint32_t>(song->ticksPerLine()), MidiTestEvent::Type::NoteOff, 60, 0, 0, 0 }
    };

    QCOMPARE(exportedEvents.size(), expectedEvents.size());
    for (size_t i = 0; i < exportedEvents.size(); ++i) {
        QCOMPARE(exportedEvents[i], expectedEvents[i]);
    }
}

void MidiExporterTest::test_exportTo_multipleNotesAndTracks_shouldExportCorrectly()
{
    auto song = std::make_shared<Song>();
    song->setBeatsPerMinute(120);
    song->setLinesPerBeat(4);

    auto instrument0 = std::make_shared<Instrument>("Track0Instrument");
    auto midiAddress0 = instrument0->midiAddress();
    midiAddress0.setPort("PortA");
    instrument0->setMidiAddress(midiAddress0);
    song->setInstrument(0, instrument0);

    auto instrument1 = std::make_shared<Instrument>("Track1Instrument");
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

    const auto automationService = std::make_shared<AutomationService>();
    const auto mixerService = std::make_shared<MixerService>();
    const auto sideChainService = std::make_shared<SideChainService>();
    MidiExporter exporter { automationService, mixerService, sideChainService };
    exporter.exportTo(fileName.toStdString(), song);

    QVERIFY(QFile::exists(fileName));
    QVERIFY(QFile { fileName }.size() > 0);

    const auto exportedEvents = readMidiFile(fileName.toStdString());

    const std::vector<MidiTestEvent> expectedEvents = {
        { 0, MidiTestEvent::Type::NoteOn, 60, 100, 0, 0 }, // PortA, Chan0
        { ticksPerLine, MidiTestEvent::Type::NoteOff, 60, 0, 0, 0 }, // PortA, Chan0
        { ticksPerLine, MidiTestEvent::Type::NoteOn, 64, 80, 0, 1 }, // PortB, Chan0
        { 2 * ticksPerLine, MidiTestEvent::Type::NoteOff, 64, 0, 0, 1 }, // PortB, Chan0
        { 2 * ticksPerLine, MidiTestEvent::Type::NoteOn, 62, 90, 0, 0 }, // PortA, Chan0
        { 3 * ticksPerLine, MidiTestEvent::Type::NoteOn, 65, 70, 0, 1 }, // PortB, Chan0
        { 4 * ticksPerLine, MidiTestEvent::Type::NoteOff, 62, 0, 0, 0 }, // PortA, Chan0
        { 4 * ticksPerLine, MidiTestEvent::Type::NoteOff, 65, 0, 0, 1 } // PortB, Chan0
    };

    QCOMPARE(exportedEvents.size(), expectedEvents.size());
    for (size_t i = 0; i < exportedEvents.size(); ++i) {
        QCOMPARE(exportedEvents[i], expectedEvents[i]);
    }
}

void MidiExporterTest::test_exportTo_timing_shouldBeCorrect()
{
    auto song = std::make_shared<Song>();
    song->setBeatsPerMinute(120);
    song->setLinesPerBeat(4);

    auto instrument = std::make_shared<Instrument>("TestInstrument");
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

    const auto automationService = std::make_shared<AutomationService>();
    const auto mixerService = std::make_shared<MixerService>();
    const auto sideChainService = std::make_shared<SideChainService>();
    MidiExporter exporter { automationService, mixerService, sideChainService };
    exporter.exportTo(fileName.toStdString(), song);

    QVERIFY(QFile::exists(fileName));
    QVERIFY(QFile(fileName).size() > 0);

    const auto exportedEvents = readMidiFile(fileName.toStdString());

    const auto ticksPerLine = song->ticksPerLine();
    const auto linesPerBeat = song->linesPerBeat();
    const auto ticksPerBeat = static_cast<uint32_t>(ticksPerLine * linesPerBeat);

    const std::vector<MidiTestEvent> expectedEvents = {
        { 0, MidiTestEvent::Type::NoteOn, 60, 100, 0, 0 },
        { ticksPerBeat, MidiTestEvent::Type::NoteOff, 60, 0, 0, 0 }
    };

    QCOMPARE(exportedEvents.size(), expectedEvents.size());
    for (size_t i = 0; i < exportedEvents.size(); ++i) {
        QCOMPARE(exportedEvents[i], expectedEvents[i]);
    }
}

void MidiExporterTest::test_exportTo_mutedAndSoloedTracks_shouldExportCorrectly()
{
    auto song = std::make_shared<Song>();
    song->setBeatsPerMinute(120);
    song->setLinesPerBeat(4);

    auto instrument0 = std::make_shared<Instrument>("Track0Instrument");
    song->setInstrument(0, instrument0);

    auto instrument1 = std::make_shared<Instrument>("Track1Instrument");
    song->setInstrument(1, instrument1);
    
    auto instrument2 = std::make_shared<Instrument>("Track2Instrument");
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

    const auto automationService = std::make_shared<AutomationService>();
    const auto mixerService = std::make_shared<MixerService>();
    mixerService->muteTrack(1, true);
    mixerService->soloTrack(2, true);

    QTemporaryFile tempFile;
    QVERIFY(tempFile.open());
    const auto fileName = tempFile.fileName();
    tempFile.close();

    const auto sideChainService = std::make_shared<SideChainService>();
    MidiExporter exporter { automationService, mixerService, sideChainService };
    exporter.exportTo(fileName.toStdString(), song);

    const auto exportedEvents = readMidiFile(fileName.toStdString());

    const std::vector<MidiTestEvent> expectedEvents = {
        { 0, MidiTestEvent::Type::NoteOn, 64, 80, 0, 0 },
        { ticksPerLine, MidiTestEvent::Type::NoteOff, 64, 0, 0, 0 }
    };

    QCOMPARE(exportedEvents.size(), expectedEvents.size());
    for (size_t i = 0; i < exportedEvents.size(); ++i) {
        QCOMPARE(exportedEvents[i], expectedEvents[i]);
    }
}

void MidiExporterTest::test_exportTo_rangedExport_shouldExportCorrectRange()
{
    auto song = std::make_shared<Song>();
    song->setBeatsPerMinute(120);
    song->setLinesPerBeat(4);

    auto instrument = std::make_shared<Instrument>("TestInstrument");
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

    const auto automationService = std::make_shared<AutomationService>();
    const auto mixerService = std::make_shared<MixerService>();
    const auto sideChainService = std::make_shared<SideChainService>();
    MidiExporter exporter { automationService, mixerService, sideChainService };
    exporter.exportTo(fileName.toStdString(), song);

    const auto startPosition = 1;
    const auto endPosition = 2;
    exporter.exportTo(fileName.toStdString(), song, startPosition, endPosition);

    QVERIFY(QFile::exists(fileName));
    QVERIFY(QFile { fileName }.size() > 0);

    const auto exportedEvents = readMidiFile(fileName.toStdString());
    const auto ticksPerLine = song->ticksPerLine();

    const std::vector<MidiTestEvent> expectedEvents = {
        { 0, MidiTestEvent::Type::NoteOn, 62, 100, 0, 0 },
        { static_cast<uint32_t>(ticksPerLine), MidiTestEvent::Type::NoteOff, 62, 0, 0, 0 },
    };

    QCOMPARE(exportedEvents.size(), expectedEvents.size());
    for (size_t i = 0; i < exportedEvents.size(); ++i) {
        QCOMPARE(exportedEvents[i], expectedEvents[i]);
    }
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::MidiExporterTest)
