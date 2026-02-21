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

#include "midi_importer_test.hpp"

#include "../../application/service/automation_service.hpp"
#include "../../application/service/mixer_service.hpp"
#include "../../application/service/side_chain_service.hpp"
#include "../../domain/note_data.hpp"
#include "../../domain/song.hpp"
#include "../../infra/midi/export/midi_exporter.hpp"
#include "../../infra/midi/import/midi_importer.hpp"

#include <memory>

#include <QTemporaryFile>
#include <QTest>

namespace noteahead {

void MidiImporterTest::test_import_exportedFile_shouldRestoreNotes()
{
    const auto automationService = std::make_shared<AutomationService>();
    const auto mixerService = std::make_shared<MixerService>();
    const auto sideChainService = std::make_shared<SideChainService>();
    const MidiExporter exporter { automationService, mixerService, sideChainService };
    const MidiImporter importer;

    const auto song = std::make_shared<Song>();
    song->setBeatsPerMinute(140);
    song->setLinesPerBeat(4);

    const auto instrument = std::make_shared<Instrument>("TestInstrument");
    song->setInstrument(0, instrument);

    NoteData noteOn;
    noteOn.setAsNoteOn(60, 100);
    song->setNoteDataAtPosition(noteOn, { 0, 0, 0, 0, 0 });

    NoteData noteOff;
    noteOff.setAsNoteOff(60);
    song->setNoteDataAtPosition(noteOff, { 0, 0, 0, 2, 0 });

    QTemporaryFile tempFile;
    QVERIFY(tempFile.open());
    const auto fileName = tempFile.fileName();
    tempFile.close();

    exporter.exportTo(fileName.toStdString(), song);

    const auto importedSong = std::make_shared<Song>();
    importedSong->setBeatsPerMinute(120);
    importedSong->setLinesPerBeat(4);

    const auto midiData = importer.parseMidiFile(fileName.toStdString());
    importer.importTo(midiData, importedSong, 0, 64, false, false);

    QCOMPARE(importedSong->beatsPerMinute(), 140);
    // Noteahead initializes with 8 tracks by default
    QCOMPARE(importedSong->trackCount(), 8);

    const auto importedNoteOn = importedSong->noteDataAtPosition({ 0, 0, 0, 0 });
    QVERIFY(importedNoteOn->type() == NoteData::Type::NoteOn);
    QCOMPARE(*importedNoteOn->note(), 60);
    QCOMPARE(importedNoteOn->velocity(), 100);

    const auto importedNoteOff = importedSong->noteDataAtPosition({ 0, 0, 0, 2 });
    QVERIFY(importedNoteOff->type() == NoteData::Type::NoteOff);
    QCOMPARE(*importedNoteOff->note(), 60);
}

void MidiImporterTest::test_import_multipleTracksAndPatterns_shouldRestoreCorrectly()
{
    const auto automationService = std::make_shared<AutomationService>();
    const auto mixerService = std::make_shared<MixerService>();
    const auto sideChainService = std::make_shared<SideChainService>();
    const MidiExporter exporter { automationService, mixerService, sideChainService };
    const MidiImporter importer;

    const auto song = std::make_shared<Song>();
    song->setBeatsPerMinute(120);
    song->setLinesPerBeat(4);
    song->setLineCount(0, 4);
    song->createPattern(1);
    song->setLineCount(1, 4);
    song->setLength(2);
    song->setPatternAtSongPosition(0, 0);
    song->setPatternAtSongPosition(1, 1);

    song->setTrackName(0, "Melody");
    song->setTrackName(1, "Bass");

    // Track 0, Pattern 0, Line 0
    NoteData n1;
    n1.setAsNoteOn(60, 100);
    song->setNoteDataAtPosition(n1, { 0, 0, 0, 0 });

    // Track 1, Pattern 1, Line 0 (which is absolute line 4)
    NoteData n2;
    n2.setAsNoteOn(62, 110);
    song->setNoteDataAtPosition(n2, { 1, 1, 0, 0 });

    QTemporaryFile tempFile;
    QVERIFY(tempFile.open());
    const auto fileName = tempFile.fileName();
    tempFile.close();

    exporter.exportTo(fileName.toStdString(), song);

    const auto importedSong = std::make_shared<Song>();
    importedSong->setBeatsPerMinute(120);
    importedSong->setLinesPerBeat(4);

    const auto midiData = importer.parseMidiFile(fileName.toStdString());
    // Use pattern length 4 to match original structure
    importer.importTo(midiData, importedSong, 0, 4, false, false);

    QCOMPARE(importedSong->trackCount(), 8);
    QCOMPARE(QString::fromStdString(importedSong->trackName(0)), QString { "Melody" });
    QCOMPARE(QString::fromStdString(importedSong->trackName(1)), QString { "Bass" });

    // Verify Pattern 0
    const auto importedN1 = importedSong->noteDataAtPosition({ 0, 0, 0, 0 });
    QVERIFY(importedN1->type() == NoteData::Type::NoteOn);
    QCOMPARE(*importedN1->note(), 60);

    // Verify Pattern 1
    const auto importedN2 = importedSong->noteDataAtPosition({ 1, 1, 0, 0 });
    QVERIFY(importedN2->type() == NoteData::Type::NoteOn);
    QCOMPARE(*importedN2->note(), 62);
    QCOMPARE(importedN2->velocity(), 110);
}

void MidiImporterTest::test_import_polyphony_shouldCreateNewColumns()
{
    const auto automationService = std::make_shared<AutomationService>();
    const auto mixerService = std::make_shared<MixerService>();
    const auto sideChainService = std::make_shared<SideChainService>();
    const MidiExporter exporter { automationService, mixerService, sideChainService };
    const MidiImporter importer;

    const auto song = std::make_shared<Song>();
    song->setBeatsPerMinute(120);
    song->setLinesPerBeat(4);

    // Track 0: Play a chord (C4, E4, G4) at the same time
    // We need 3 columns
    song->addColumn(0);
    song->addColumn(0);

    NoteData n1;
    n1.setAsNoteOn(60, 100);
    song->setNoteDataAtPosition(n1, { 0, 0, 0, 0 });
    NoteData n2;
    n2.setAsNoteOn(64, 100);
    song->setNoteDataAtPosition(n2, { 0, 0, 1, 0 });
    NoteData n3;
    n3.setAsNoteOn(67, 100);
    song->setNoteDataAtPosition(n3, { 0, 0, 2, 0 });

    QTemporaryFile tempFile;
    QVERIFY(tempFile.open());
    const auto fileName = tempFile.fileName();
    tempFile.close();

    exporter.exportTo(fileName.toStdString(), song);

    const auto importedSong = std::make_shared<Song>();
    importedSong->setBeatsPerMinute(120);
    importedSong->setLinesPerBeat(4);

    const auto midiData = importer.parseMidiFile(fileName.toStdString());
    importer.importTo(midiData, importedSong, 0, 64, false, false);

    // Should have 3 columns on track 0
    QCOMPARE(importedSong->columnCount(0), 3);

    const auto imp1 = importedSong->noteDataAtPosition({ 0, 0, 0, 0 });
    const auto imp2 = importedSong->noteDataAtPosition({ 0, 0, 1, 0 });
    const auto imp3 = importedSong->noteDataAtPosition({ 0, 0, 2, 0 });

    QVERIFY(imp1->type() == NoteData::Type::NoteOn);
    QVERIFY(imp2->type() == NoteData::Type::NoteOn);
    QVERIFY(imp3->type() == NoteData::Type::NoteOn);

    std::vector<uint8_t> notes = { *imp1->note(), *imp2->note(), *imp3->note() };
    std::ranges::sort(notes);
    const std::vector<uint8_t> expected = { 60, 64, 67 };
    QCOMPARE(notes, expected);
}

void MidiImporterTest::test_import_quantization_shouldZeroDelays()
{
    const auto automationService = std::make_shared<AutomationService>();
    const auto mixerService = std::make_shared<MixerService>();
    const auto sideChainService = std::make_shared<SideChainService>();
    const MidiExporter exporter { automationService, mixerService, sideChainService };
    const MidiImporter importer;

    const auto song = std::make_shared<Song>();
    song->setBeatsPerMinute(120);
    song->setLinesPerBeat(4);

    // Note with delay 10
    NoteData n1;
    n1.setAsNoteOn(60, 100);
    n1.setDelay(10);
    song->setNoteDataAtPosition(n1, { 0, 0, 0, 0 });

    NoteData n1Off;
    n1Off.setAsNoteOff(60);
    n1Off.setDelay(5);
    song->setNoteDataAtPosition(n1Off, { 0, 0, 0, 1 });

    QTemporaryFile tempFile;
    QVERIFY(tempFile.open());
    const auto fileName = tempFile.fileName();
    tempFile.close();

    exporter.exportTo(fileName.toStdString(), song);

    const auto importedSong = std::make_shared<Song>();
    importedSong->setBeatsPerMinute(120);
    importedSong->setLinesPerBeat(4);
    const auto midiData = importer.parseMidiFile(fileName.toStdString());

    // Import with quantization enabled
    importer.importTo(midiData, importedSong, 0, 64, true, true);

    const auto noteOnQuantized = importedSong->noteDataAtPosition({ 0, 0, 0, 0 });
    const auto noteOffQuantized = importedSong->noteDataAtPosition({ 0, 0, 0, 1 });

    QCOMPARE(noteOnQuantized->delay(), 0);
    QCOMPARE(noteOffQuantized->delay(), 0);

    // Re-import without quantization to verify delays are actually there in the file
    const auto importedSongNoQuantize = std::make_shared<Song>();
    importedSongNoQuantize->setBeatsPerMinute(120);
    importedSongNoQuantize->setLinesPerBeat(4);
    importer.importTo(midiData, importedSongNoQuantize, 0, 64, false, false);

    const auto noteOnNotQuantized = importedSongNoQuantize->noteDataAtPosition({ 0, 0, 0, 0 });
    const auto noteOffNotQuantized = importedSongNoQuantize->noteDataAtPosition({ 0, 0, 0, 1 });

    QCOMPARE(noteOnNotQuantized->delay(), 10);
    QCOMPARE(noteOffNotQuantized->delay(), 5);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::MidiImporterTest)
