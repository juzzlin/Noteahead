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
#include "../../application/service/recent_files_manager.hpp"
#include "../../application/service/player_service.hpp"
#include "../../application/service/editor_service.hpp"
#include "../../application/state_machine.hpp"
#include "../../common/constants.hpp"

#include "../../application/note_converter.hpp"
#include "../../domain/midi_note_data.hpp"
#include "../../domain/instrument.hpp"

#include <QSignalSpy>

namespace noteahead {

class MockEditorService : public EditorService
{
public:
    Position position() const override { return m_position; }
    void setMockPosition(const Position & position) { m_position = position; }

    InstrumentS instrument(quint64 trackIndex) const override
    {
        if (const auto it = m_instruments.find(trackIndex); it != m_instruments.end()) {
            return it->second;
        }
        return nullptr;
    }
    void setMockInstrument(quint64 trackIndex, InstrumentS instrument) { m_instruments[trackIndex] = instrument; }

private:
    Position m_position;
    std::map<quint64, InstrumentS> m_instruments;
};

class MockPlayerService : public PlayerService
{
public:
    MockPlayerService()
      : PlayerService { nullptr, nullptr, nullptr, nullptr, nullptr }
    {
    }
    bool isPlaying() const override { return m_isPlaying; }
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
    QStringList recentFiles() const override { return m_recentFiles; }
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
    StateMachine::Action lastAction() const { return m_lastAction; }

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

void ApplicationServiceTest::test_initialState()
{
    ApplicationService service;
    QVERIFY(!service.editMode());
}

void ApplicationServiceTest::test_applicationProperties()
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

void ApplicationServiceTest::test_editMode()
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

    // If playing, edit mode should be set to false and cannot be set to true
    playerService->setIsPlaying(true);
    // Connection in ApplicationService sets edit mode to false when playing
    QVERIFY(!service.editMode());

    service.setEditMode(true);
    QVERIFY(!service.editMode());
}

void ApplicationServiceTest::test_recentFiles()
{
    ApplicationService service;
    auto recentFilesManager = std::make_shared<MockRecentFilesManager>();
    service.setRecentFilesManager(recentFilesManager);

    QCOMPARE(service.recentFiles(), QStringList {});

    const auto filePath = QDir::current().absoluteFilePath("test.nahd");
    recentFilesManager->addRecentFile(filePath);
    QCOMPARE(service.recentFiles(), QStringList { filePath });
}

void ApplicationServiceTest::test_stateMachineInteractions()
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

void ApplicationServiceTest::test_liveNoteLogic()
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

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::ApplicationServiceTest)
