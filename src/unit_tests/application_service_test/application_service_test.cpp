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

#include "application_service_test.hpp"

#include "../../application/service/application_service.hpp"
#include "../../application/service/editor_service.hpp"
#include "../../application/service/player_service.hpp"
#include "../../application/service/recent_files_manager.hpp"
#include "../../application/state_machine.hpp"
#include "../../common/constants.hpp"

#include "../../application/note_converter.hpp"
#include "../../domain/midi/midi_note_data.hpp"
#include "../../domain/tracker/column_settings.hpp"
#include "../../domain/tracker/instrument.hpp"
#include "../../domain/tracker/song.hpp"

#include <QSignalSpy>
#include <QTest>

namespace noteahead {

class MockEditorService : public EditorService
{
public:
    Position position() const override
    {
        return m_position;
    }

    void setMockPosition(const Position & position)
    {
        m_position = position;
    }

    InstrumentS instrument(quint64 trackIndex) const override
    {
        if (const auto it = m_instruments.find(trackIndex); it != m_instruments.end()) {
            return it->second;
        }
        return nullptr;
    }

    void setMockInstrument(quint64 trackIndex, InstrumentS instrument)
    {
        m_instruments[trackIndex] = instrument;
    }

    ColumnSettingsS columnSettings(quint64 trackIndex, quint64 columnIndex) const override
    {
        if (const auto it = m_columnSettings.find({ trackIndex, columnIndex }); it != m_columnSettings.end()) {
            return it->second;
        }
        return nullptr;
    }

    void setMockColumnSettings(quint64 trackIndex, quint64 columnIndex, ColumnSettingsS settings)
    {
        m_columnSettings[{ trackIndex, columnIndex }] = settings;
    }

    SongS song() const override
    {
        return m_mockSong;
    }

    void setMockSong(SongS song)
    {
        m_mockSong = song;
    }

private:
    Position m_position;
    std::map<quint64, InstrumentS> m_instruments;
    std::map<std::pair<quint64, quint64>, ColumnSettingsS> m_columnSettings;
    SongS m_mockSong;
};

class MockPlayerService : public PlayerService
{
public:
    MockPlayerService()
      : PlayerService { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr }
    {
    }

    bool isPlaying() const override
    {
        return m_isPlaying;
    }

    void setIsPlaying(bool playing)
    {
        m_isPlaying = playing;
        emit isPlayingChanged();
    }

private:
    bool m_isPlaying = false;
};

class MockRecentFilesManager : public RecentFilesManager
{
public:
    QStringList recentFiles() const override
    {
        return m_recentFiles;
    }

    void addRecentFile(QString filePath) override
    {
        m_recentFiles.push_front(filePath);
        emit recentFilesChanged(m_recentFiles);
    }

private:
    QStringList m_recentFiles;
};

class MockStateMachine : public StateMachine
{
public:
    MockStateMachine()
      : StateMachine { nullptr, nullptr }
    {
    }

    void calculateState(StateMachine::Action action) override
    {
        m_lastAction = action;
    }

    StateMachine::Action lastAction() const
    {
        return m_lastAction;
    }

private:
    StateMachine::Action m_lastAction = static_cast<StateMachine::Action>(-1);
};

void ApplicationServiceTest::initTestCase()
{
    qRegisterMetaType<noteahead::Position>("Position");
    qRegisterMetaType<noteahead::MidiNoteData>("MidiNoteData");
    qRegisterMetaType<noteahead::MidiNoteData>("MidiNoteDataCR");
    qRegisterMetaType<std::shared_ptr<noteahead::Instrument>>("InstrumentS");
}

void ApplicationServiceTest::test_initialState_shouldBeCorrect()
{
    ApplicationService service;
    QVERIFY(!service.editMode());
}

void ApplicationServiceTest::test_applicationProperties_shouldMatchConstants()
{
    ApplicationService service;
    QCOMPARE(service.applicationName(), Constants::applicationName());
    QCOMPARE(service.applicationVersion(), Constants::applicationVersion());
    QCOMPARE(service.copyright(), Constants::copyright());
    QCOMPARE(service.license(), Constants::license());
    QCOMPARE(service.fileFormatExtension(), Constants::fileFormatExtension());
    QCOMPARE(service.midiFileExtension(), Constants::midiFileExtension());
    QCOMPARE(service.webSiteUrl(), Constants::webSiteUrl());
}

void ApplicationServiceTest::test_changeLog_shouldOpenTheBundledResource()
{
    // The resource path is the part that has broken twice, and the parsing tests below cannot see
    // it because they run on string literals. The QML module's own prefix is Qt's to choose and it
    // moved in 6.5, so the CHANGELOG carries a prefix of its own; this is what checks it is really
    // where the code goes looking.
    ApplicationService service;

    const auto changeLog = service.changeLog();

    QVERIFY2(!changeLog.isEmpty(), "The bundled CHANGELOG resource did not open");
    QVERIFY2(changeLog.contains("====="), "The bundled CHANGELOG does not look like a CHANGELOG");
}

void ApplicationServiceTest::test_stripUnreleasedSection_shouldStartAtTheFirstRelease()
{
    const QString changeLog {
        "x.y.z\n"
        "=====\n"
        "\n"
        "New features:\n"
        "\n"
        "* Something still in progress\n"
        "\n"
        "6.0.0\n"
        "=====\n"
        "\n"
        "Release date:\n"
        "\n"
        "* Something that shipped\n"
    };

    const auto stripped = ApplicationService::stripUnreleasedSection(changeLog);

    // What's New is about what shipped, so the section still being worked on is dropped
    QVERIFY(!stripped.contains("x.y.z"));
    QVERIFY(!stripped.contains("Something still in progress"));
    QVERIFY(stripped.startsWith("6.0.0\n=====\n"));
    QVERIFY(stripped.contains("Something that shipped"));
}

void ApplicationServiceTest::test_stripUnreleasedSection_versionInsideText_shouldNotCut()
{
    const QString changeLog {
        "x.y.z\n"
        "=====\n"
        "\n"
        "* Fix a thing that broke in 5.0.0\n"
        "  - 1.2.3\n"
        "\n"
        "6.0.0\n"
        "=====\n"
        "\n"
        "* Shipped\n"
    };

    // A version has to be a heading, i.e. underlined, before it counts. Matching the bare number
    // would cut the file at the bullet above.
    const auto stripped = ApplicationService::stripUnreleasedSection(changeLog);

    QVERIFY(stripped.startsWith("6.0.0\n"));
    QVERIFY(!stripped.contains("broke in 5.0.0"));
}

void ApplicationServiceTest::test_stripUnreleasedSection_nothingReleased_shouldBeEmpty()
{
    const QString changeLog {
        "x.y.z\n"
        "=====\n"
        "\n"
        "* The very first release is not out yet\n"
    };

    // Better to say there is nothing to show than to present unreleased work as shipped
    QVERIFY(ApplicationService::stripUnreleasedSection(changeLog).isEmpty());
    QVERIFY(ApplicationService::stripUnreleasedSection({}).isEmpty());
}

void ApplicationServiceTest::test_editMode_shouldToggleCorrectly()
{
    ApplicationService service;
    const auto playerService = std::make_shared<MockPlayerService>();
    service.setPlayerService(playerService);

    QSignalSpy spy { &service, &ApplicationService::editModeChanged };

    service.setEditMode(true);
    QVERIFY(service.editMode());
    QCOMPARE(spy.count(), 1);

    service.setEditMode(true); // No change
    QCOMPARE(spy.count(), 1);

    service.toggleEditMode();
    QVERIFY(!service.editMode());
    QCOMPARE(spy.count(), 2);

    // If playing, edit mode should NOT be set to false and can still be set to true
    service.setEditMode(true);
    playerService->setIsPlaying(true);
    QVERIFY(service.editMode());

    service.setEditMode(false);
    QVERIFY(!service.editMode());

    service.setEditMode(true);
    QVERIFY(service.editMode());
}

void ApplicationServiceTest::test_recentFiles_shouldBeManagedCorrectly()
{
    ApplicationService service;
    auto recentFilesManager = std::make_shared<MockRecentFilesManager>();
    service.setRecentFilesManager(recentFilesManager);

    QCOMPARE(service.recentFiles(), QStringList {});

    const auto filePath = QDir::current().absoluteFilePath("test.nahd");
    recentFilesManager->addRecentFile(filePath);
    QCOMPARE(service.recentFiles(), QStringList { filePath });
}

void ApplicationServiceTest::test_stateMachineInteractions_shouldTriggerCorrectActions()
{
    ApplicationService service;
    const auto stateMachine = std::make_shared<MockStateMachine>();
    service.setStateMachine(stateMachine);

    service.requestNewProject();
    QCOMPARE(stateMachine->lastAction(), StateMachine::Action::NewProjectRequested);

    service.requestOpenProject();
    QCOMPARE(stateMachine->lastAction(), StateMachine::Action::OpenProjectRequested);

    service.requestSaveProject();
    QCOMPARE(stateMachine->lastAction(), StateMachine::Action::SaveProjectRequested);

    service.requestQuit();
    QCOMPARE(stateMachine->lastAction(), StateMachine::Action::QuitSelected);

    service.acceptUnsavedChangesDialog();
    QCOMPARE(stateMachine->lastAction(), StateMachine::Action::UnsavedChangesDialogAccepted);

    service.discardUnsavedChangesDialog();
    QCOMPARE(stateMachine->lastAction(), StateMachine::Action::UnsavedChangesDialogDiscarded);

    service.rejectUnsavedChangesDialog();
    QCOMPARE(stateMachine->lastAction(), StateMachine::Action::UnsavedChangesDialogCanceled);
}

void ApplicationServiceTest::test_liveNoteLogic_shouldTriggerCorrectSignals()
{
    ApplicationService service;
    const auto editorService = std::make_shared<MockEditorService>();
    service.setEditorService(editorService);

    const auto instrument = std::make_shared<Instrument>("Test Port");
    editorService->setMockInstrument(0, instrument);
    editorService->setMockPosition({ 0, 0, 0, 0, 0 });

    QSignalSpy onSpy { &service, &ApplicationService::liveNoteOnRequested };
    QSignalSpy offSpy { &service, &ApplicationService::liveNoteOffRequested };
    QSignalSpy currentSpy { &service, &ApplicationService::liveNoteOnAtCurrentPositionRequested };

    // Test liveNoteOn
    service.requestLiveNoteOn(1, 3, 100); // C-3 is 36
    QCOMPARE(onSpy.count(), 1);
    const auto onArgs = onSpy.takeFirst();
    QCOMPARE(onArgs.at(0).value<std::shared_ptr<Instrument>>(), instrument);
    QCOMPARE(onArgs.at(1).value<MidiNoteData>().note(), static_cast<uint8_t>(36));
    QCOMPARE(onArgs.at(1).value<MidiNoteData>().velocity(), static_cast<uint8_t>(100));

    // Test liveNoteOff
    service.requestLiveNoteOff(1, 3); // C-3 is 36
    QCOMPARE(offSpy.count(), 1);
    const auto offArgs = offSpy.takeFirst();
    QCOMPARE(offArgs.at(0).value<std::shared_ptr<Instrument>>(), instrument);
    QCOMPARE(offArgs.at(1).value<MidiNoteData>().note(), static_cast<uint8_t>(36));

    // Test liveNoteOnAtCurrentPosition
    service.requestLiveNoteOnAtCurrentPosition();
    QCOMPARE(currentSpy.count(), 1);
    QCOMPARE(currentSpy.takeFirst().at(0).value<std::shared_ptr<Instrument>>(), instrument);
}

void ApplicationServiceTest::test_livePlay_withColumnTranspose_shouldApplyTranspose()
{
    ApplicationService service;
    const auto editorService = std::make_shared<MockEditorService>();
    service.setEditorService(editorService);

    const auto instrument = std::make_shared<Instrument>("Test Port");
    InstrumentSettings instrSettings;
    instrSettings.transpose = 2;
    instrument->setSettings(instrSettings);
    editorService->setMockInstrument(0, instrument);
    editorService->setMockPosition({ 0, 0, 0, 0, 0 });

    const auto columnSettings = std::make_shared<ColumnSettings>();
    columnSettings->transpose = 3;
    editorService->setMockColumnSettings(0, 0, columnSettings);

    QSignalSpy onSpy { &service, &ApplicationService::liveNoteOnRequested };
    QSignalSpy offSpy { &service, &ApplicationService::liveNoteOffRequested };

    // Play note 60 (total transpose = +5 -> 65)
    service.requestLiveMidiNoteOn(60, 100);
    QCOMPARE(onSpy.count(), 1);
    const auto onArgs = onSpy.takeFirst();
    QCOMPARE(onArgs.at(0).value<std::shared_ptr<Instrument>>(), instrument);
    QCOMPARE(onArgs.at(1).value<MidiNoteData>().note(), static_cast<uint8_t>(65));
    QCOMPARE(onArgs.at(1).value<MidiNoteData>().velocity(), static_cast<uint8_t>(100));

    service.requestLiveMidiNoteOff(60);
    QCOMPARE(offSpy.count(), 1);
    const auto offArgs = offSpy.takeFirst();
    QCOMPARE(offArgs.at(0).value<std::shared_ptr<Instrument>>(), instrument);
    QCOMPARE(offArgs.at(1).value<MidiNoteData>().note(), static_cast<uint8_t>(65));
}

void ApplicationServiceTest::test_livePlay_withChordAutomation_shouldPlayChordNotes()
{
    ApplicationService service;
    const auto editorService = std::make_shared<MockEditorService>();
    service.setEditorService(editorService);

    const auto instrument = std::make_shared<Instrument>("Test Port");
    editorService->setMockInstrument(0, instrument);
    editorService->setMockPosition({ 0, 0, 0, 0, 0 });

    const auto columnSettings = std::make_shared<ColumnSettings>();
    columnSettings->chordAutomationSettings.note1.offset = 4;
    columnSettings->chordAutomationSettings.note1.velocity = 100;
    columnSettings->chordAutomationSettings.note1.delay = 0;
    columnSettings->chordAutomationSettings.note2.offset = 7;
    columnSettings->chordAutomationSettings.note2.velocity = 80;
    columnSettings->chordAutomationSettings.note2.delay = 0;
    editorService->setMockColumnSettings(0, 0, columnSettings);

    QSignalSpy onSpy { &service, &ApplicationService::liveNoteOnRequested };
    QSignalSpy offSpy { &service, &ApplicationService::liveNoteOffRequested };

    // Play note 60, velocity 100
    service.requestLiveMidiNoteOn(60, 100);
    QCOMPARE(onSpy.count(), 3);

    // Root note: 60, velocity 100
    const auto rootArgs = onSpy.at(0);
    QCOMPARE(rootArgs.at(1).value<MidiNoteData>().note(), static_cast<uint8_t>(60));
    QCOMPARE(rootArgs.at(1).value<MidiNoteData>().velocity(), static_cast<uint8_t>(100));

    // Chord note 1: 64, velocity 100
    const auto chord1Args = onSpy.at(1);
    QCOMPARE(chord1Args.at(1).value<MidiNoteData>().note(), static_cast<uint8_t>(64));
    QCOMPARE(chord1Args.at(1).value<MidiNoteData>().velocity(), static_cast<uint8_t>(100));

    // Chord note 2: 67, velocity 80
    const auto chord2Args = onSpy.at(2);
    QCOMPARE(chord2Args.at(1).value<MidiNoteData>().note(), static_cast<uint8_t>(67));
    QCOMPARE(chord2Args.at(1).value<MidiNoteData>().velocity(), static_cast<uint8_t>(80));

    // Release root note 60 -> should turn off all sounding notes (60, 64, 67)
    service.requestLiveMidiNoteOff(60);
    QCOMPARE(offSpy.count(), 3);
    QCOMPARE(offSpy.at(0).at(1).value<MidiNoteData>().note(), static_cast<uint8_t>(60));
    QCOMPARE(offSpy.at(1).at(1).value<MidiNoteData>().note(), static_cast<uint8_t>(64));
    QCOMPARE(offSpy.at(2).at(1).value<MidiNoteData>().note(), static_cast<uint8_t>(67));
}

void ApplicationServiceTest::test_livePlay_withChordAutomationDelay_shouldPlayDelayedChordNote()
{
    ApplicationService service;
    const auto editorService = std::make_shared<MockEditorService>();
    service.setEditorService(editorService);

    const auto instrument = std::make_shared<Instrument>("Test Port");
    editorService->setMockInstrument(0, instrument);
    editorService->setMockPosition({ 0, 0, 0, 0, 0 });

    const auto columnSettings = std::make_shared<ColumnSettings>();
    columnSettings->chordAutomationSettings.note1.offset = 4;
    columnSettings->chordAutomationSettings.note1.velocity = 100;
    columnSettings->chordAutomationSettings.note1.delay = 30; // 30 ms delay
    editorService->setMockColumnSettings(0, 0, columnSettings);

    QSignalSpy onSpy { &service, &ApplicationService::liveNoteOnRequested };
    QSignalSpy offSpy { &service, &ApplicationService::liveNoteOffRequested };

    service.requestLiveMidiNoteOn(60, 100);
    // Initially only root note
    QCOMPARE(onSpy.count(), 1);
    QCOMPARE(onSpy.at(0).at(1).value<MidiNoteData>().note(), static_cast<uint8_t>(60));

    // Wait for the delayed note
    QTRY_COMPARE_WITH_TIMEOUT(onSpy.count(), 2, 500);
    QCOMPARE(onSpy.at(1).at(1).value<MidiNoteData>().note(), static_cast<uint8_t>(64));

    // Note off should release both notes
    service.requestLiveMidiNoteOff(60);
    QCOMPARE(offSpy.count(), 2);
    QCOMPARE(offSpy.at(0).at(1).value<MidiNoteData>().note(), static_cast<uint8_t>(60));
    QCOMPARE(offSpy.at(1).at(1).value<MidiNoteData>().note(), static_cast<uint8_t>(64));
}

void ApplicationServiceTest::test_livePlay_fromMidiController_shouldApplyAllEffects()
{
    ApplicationService service;
    const auto editorService = std::make_shared<MockEditorService>();
    service.setEditorService(editorService);

    const auto instrument = std::make_shared<Instrument>("Test Port");
    editorService->setMockInstrument(0, instrument);
    editorService->setMockPosition({ 0, 0, 0, 0, 0 });

    const auto columnSettings = std::make_shared<ColumnSettings>();
    columnSettings->transpose = 12; // +1 octave
    columnSettings->chordAutomationSettings.note1.offset = 7;
    columnSettings->chordAutomationSettings.note1.velocity = 100;
    columnSettings->chordAutomationSettings.note1.delay = 0;
    editorService->setMockColumnSettings(0, 0, columnSettings);

    QSignalSpy onSpy { &service, &ApplicationService::liveNoteOnRequested };
    QSignalSpy offSpy { &service, &ApplicationService::liveNoteOffRequested };

    // Raw MIDI note 48 (C-3) from controller
    service.requestLiveMidiNoteOn(48, 90);
    QCOMPARE(onSpy.count(), 2);
    // Transposed root note: 48 + 12 = 60
    QCOMPARE(onSpy.at(0).at(1).value<MidiNoteData>().note(), static_cast<uint8_t>(60));
    // Chord note 1: 60 + 7 = 67
    QCOMPARE(onSpy.at(1).at(1).value<MidiNoteData>().note(), static_cast<uint8_t>(67));

    service.requestLiveMidiNoteOff(48);
    QCOMPARE(offSpy.count(), 2);
    QCOMPARE(offSpy.at(0).at(1).value<MidiNoteData>().note(), static_cast<uint8_t>(60));
    QCOMPARE(offSpy.at(1).at(1).value<MidiNoteData>().note(), static_cast<uint8_t>(67));
}

void ApplicationServiceTest::test_livePlay_noteOff_shouldStopAllSoundingNotes()
{
    ApplicationService service;
    const auto editorService = std::make_shared<MockEditorService>();
    service.setEditorService(editorService);

    const auto instrument = std::make_shared<Instrument>("Test Port");
    editorService->setMockInstrument(0, instrument);
    editorService->setMockPosition({ 0, 0, 0, 0, 0 });

    const auto columnSettings = std::make_shared<ColumnSettings>();
    columnSettings->chordAutomationSettings.note1.offset = 4;
    columnSettings->chordAutomationSettings.note1.velocity = 100;
    columnSettings->chordAutomationSettings.note1.delay = 500; // delayed by 500ms
    editorService->setMockColumnSettings(0, 0, columnSettings);

    QSignalSpy onSpy { &service, &ApplicationService::liveNoteOnRequested };
    QSignalSpy offSpy { &service, &ApplicationService::liveNoteOffRequested };

    service.requestLiveMidiNoteOn(60, 100);
    QCOMPARE(onSpy.count(), 1);

    // Release note before delayed note fires: delayed timer should be cancelled
    service.requestLiveMidiNoteOff(60);
    QCOMPARE(offSpy.count(), 1);
    QCOMPARE(offSpy.at(0).at(1).value<MidiNoteData>().note(), static_cast<uint8_t>(60));

    // Wait to ensure delayed note was indeed cancelled and never triggers
    QTest::qWait(600);
    QCOMPARE(onSpy.count(), 1);
    QCOMPARE(offSpy.count(), 1);

    // Also verify stopAllLiveNotes
    service.requestLiveMidiNoteOn(60, 100);
    service.requestLiveMidiNoteOn(64, 100);
    service.stopAllLiveNotes();
    // Stop all should have turned off both
    QVERIFY(offSpy.count() >= 3);
}

void ApplicationServiceTest::test_livePlayAtCurrentPosition_afterScroll_shouldStopTheNoteItStarted()
{
    // Typing a velocity plays the note under the cursor and scrolls before the key comes up. The
    // release used to stop the note under the cursor by then, and the one played rang on for good.
    ApplicationService service;
    const auto editorService = std::make_shared<MockEditorService>();
    service.setEditorService(editorService);

    const auto instrument = std::make_shared<Instrument>("Test Port");
    editorService->setMockInstrument(0, instrument);
    const auto song = std::make_shared<Song>();
    editorService->setMockSong(song);
    NoteData first { 0, 0 };
    first.setAsNoteOn(60, 100);
    song->setNoteDataAtPosition(first, { 0, 0, 0, 0, 0 });
    NoteData second { 0, 0 };
    second.setAsNoteOn(64, 100);
    song->setNoteDataAtPosition(second, { 0, 0, 0, 1, 0 });
    editorService->setMockPosition({ 0, 0, 0, 0, 0 });

    QSignalSpy onSpy { &service, &ApplicationService::liveNoteOnRequested };
    QSignalSpy offSpy { &service, &ApplicationService::liveNoteOffRequested };

    service.requestLiveNoteOnAtCurrentPosition();
    editorService->setMockPosition({ 0, 0, 0, 1, 0 });
    service.requestLiveNoteOffAtCurrentPosition();

    QCOMPARE(onSpy.count(), 1);
    QCOMPARE(onSpy.at(0).at(1).value<MidiNoteData>().note(), static_cast<uint8_t>(60));
    QCOMPARE(offSpy.count(), 1);
    QCOMPARE(offSpy.at(0).at(1).value<MidiNoteData>().note(), static_cast<uint8_t>(60));
}

void ApplicationServiceTest::test_importMidiFile_shouldAddRecentFile()
{
    ApplicationService service;
    auto recentFilesManager = std::make_shared<MockRecentFilesManager>();
    service.setRecentFilesManager(recentFilesManager);

    const auto filePath = QDir::current().absoluteFilePath("test.mid");
    service.addRecentFile(filePath);
    QCOMPARE(service.recentFiles(), QStringList { filePath });
}

void ApplicationServiceTest::test_isMidiFile_shouldDetectCorrectExtensions()
{
    ApplicationService service;
    QVERIFY(service.isMidiFile("test.mid"));
    QVERIFY(service.isMidiFile("test.midi"));
    QVERIFY(service.isMidiFile("TEST.MID"));
    QVERIFY(service.isMidiFile("TEST.MIDI"));
    QVERIFY(!service.isMidiFile("test.nahd"));
    QVERIFY(!service.isMidiFile("test.txt"));
    QVERIFY(!service.isMidiFile(""));
}

void ApplicationServiceTest::test_requestAlertDialog_shouldEmitSignal()
{
    ApplicationService service;
    QSignalSpy spy { &service, &ApplicationService::alertDialogRequested };

    const auto message = "Test Message";
    service.requestAlertDialog(message);

    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).toString(), message);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::ApplicationServiceTest)
