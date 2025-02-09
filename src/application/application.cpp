// This file is part of Noteahead.
// Copyright (C) 2020 Jussi Lind <jussi.lind@iki.fi>
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

#include "application.hpp"

#include "../contrib/Argengine/src/argengine.hpp"
#include "../contrib/SimpleLogger/src/simple_logger.hpp"
#include "application_service.hpp"
#include "config.hpp"
#include "editor_service.hpp"
#include "midi_service.hpp"
#include "mixer_service.hpp"
#include "models/recent_files_model.hpp"
#include "models/track_settings_model.hpp"
#include "player_service.hpp"
#include "recent_files_manager.hpp"
#include "state_machine.hpp"
#include "ui_logger.hpp"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

namespace noteahead {

static const auto TAG = "Application";

Application::Application(int & argc, char ** argv)
  : m_uiLogger { std::make_unique<UiLogger>() }
  , m_application { std::make_unique<QGuiApplication>(argc, argv) }
  , m_applicationService { std::make_unique<ApplicationService>() }
  , m_config { std::make_unique<Config>() }
  , m_editorService { std::make_unique<EditorService>() }
  , m_midiService { std::make_unique<MidiService>() }
  , m_mixerService { std::make_unique<MixerService>() }
  , m_playerService { std::make_unique<PlayerService>(m_midiService, m_config) }
  , m_stateMachine { std::make_unique<StateMachine>(m_applicationService, m_editorService) }
  , m_recentFilesManager { std::make_unique<RecentFilesManager>() }
  , m_recentFilesModel { std::make_unique<RecentFilesModel>() }
  , m_trackSettingsModel { std::make_unique<TrackSettingsModel>() }
  , m_engine { std::make_unique<QQmlApplicationEngine>() }
{
    qmlRegisterType<UiLogger>("Noteahead", 1, 0, "UiLogger");
    qmlRegisterType<ApplicationService>("Noteahead", 1, 0, "ApplicationService");
    qmlRegisterType<Config>("Noteahead", 1, 0, "Config");
    qmlRegisterType<EditorService>("Noteahead", 1, 0, "EditorService");
    qmlRegisterType<MidiService>("Noteahead", 1, 0, "MidiService");
    qmlRegisterType<MixerService>("Noteahead", 1, 0, "MixerService");
    qmlRegisterType<RecentFilesModel>("Noteahead", 1, 0, "RecentFilesModel");
    qmlRegisterType<TrackSettingsModel>("Noteahead", 1, 0, "TrackSettingsModel");

    qmlRegisterSingletonType(QUrl(QML_ROOT_DIR + QString { "/Constants.qml" }), "Noteahead", 1, 0, "Constants");
    qmlRegisterSingletonType(QUrl(QML_ROOT_DIR + QString { "/UiService.qml" }), "Noteahead", 1, 0, "UiService");

    handleCommandLineArguments(argc, argv); // Handle command-line arguments at initialization

    m_applicationService->setRecentFilesManager(m_recentFilesManager);
    m_applicationService->setStateMachine(m_stateMachine);
    m_applicationService->setEditorService(m_editorService);
    m_applicationService->setPlayerService(m_playerService);

    m_recentFilesModel->setRecentFiles(m_recentFilesManager->recentFiles());
}

void Application::handleCommandLineArguments(int & argc, char ** argv)
{
    juzzlin::Argengine ae { argc, argv };

    ae.addOption({ "--debug" }, [] {
        juzzlin::SimpleLogger::setLoggingLevel(juzzlin::SimpleLogger::Level::Debug);
    });

    ae.addOption({ "--trace" }, [] {
        juzzlin::SimpleLogger::setLoggingLevel(juzzlin::SimpleLogger::Level::Trace);
    });

    ae.parse();
}

void Application::setContextProperties()
{
    m_engine->rootContext()->setContextProperty("applicationService", m_applicationService.get());
    m_engine->rootContext()->setContextProperty("config", m_config.get());
    m_engine->rootContext()->setContextProperty("editorService", m_editorService.get());
    m_engine->rootContext()->setContextProperty("midiService", m_midiService.get());
    m_engine->rootContext()->setContextProperty("mixerService", m_mixerService.get());
    m_engine->rootContext()->setContextProperty("playerService", m_playerService.get());
    m_engine->rootContext()->setContextProperty("uiLogger", m_uiLogger.get());
    m_engine->rootContext()->setContextProperty("recentFilesModel", m_recentFilesModel.get());
    m_engine->rootContext()->setContextProperty("trackSettingsModel", m_trackSettingsModel.get());
}

void Application::connectServices()
{
    connect(m_applicationService.get(), &ApplicationService::applyAllTrackSettingsRequested, this, [this]() {
        for (size_t trackIndex = 0; trackIndex < m_editorService->trackCount(); trackIndex++) {
            if (const auto instrument = m_editorService->instrument(trackIndex); instrument) {
                const InstrumentRequest instrumentRequest { InstrumentRequest::Type::ApplyAll, instrument };
                juzzlin::L(TAG).info() << "Applying instrument: " << instrumentRequest.instrument()->toString().toStdString();
                m_midiService->handleInstrumentRequest(instrumentRequest);
            }
        }
    });

    connect(m_applicationService.get(), &ApplicationService::liveNoteOnRequested, m_midiService.get(), &MidiService::playNote);

    connect(m_applicationService.get(), &ApplicationService::liveNoteOffRequested, m_midiService.get(), &MidiService::stopNote);

    connect(m_editorService.get(), &EditorService::instrumentRequested, m_midiService.get(), &MidiService::handleInstrumentRequest);

    connect(m_editorService.get(), &EditorService::songPositionChanged, m_playerService.get(), &PlayerService::setSongPosition);

    connect(m_midiService.get(), &MidiService::availableMidiPortsChanged, this, [this] {
        m_trackSettingsModel->setAvailableMidiPorts(m_midiService->availableMidiPorts());
    });

    connect(m_mixerService.get(), &MixerService::trackMuted, m_playerService.get(), &PlayerService::muteTrack);

    connect(m_mixerService.get(), &MixerService::trackSoloed, m_playerService.get(), &PlayerService::soloTrack);

    connect(m_playerService.get(), &PlayerService::songRequested, this, [this] {
        m_playerService->setSong(m_editorService->song());
    });
    connect(m_playerService.get(), &PlayerService::tickUpdated, m_editorService.get(), &EditorService::requestPositionByTick);

    connect(m_playerService.get(), &PlayerService::isPlayingChanged, this, [this]() {
        m_midiService->setIsPlaying(m_playerService->isPlaying());
    });

    connect(m_stateMachine.get(), &StateMachine::stateChanged, this, &Application::applyState);

    connect(m_trackSettingsModel.get(), &TrackSettingsModel::instrumentDataRequested, this, [this]() {
        if (const auto instrument = m_editorService->instrument(m_trackSettingsModel->trackIndex()); instrument) {
            m_trackSettingsModel->setInstrumentData(*instrument);
        } else {
            m_trackSettingsModel->reset();
        }
    });

    connect(m_trackSettingsModel.get(), &TrackSettingsModel::applyAllRequested, this, [this]() {
        const InstrumentRequest instrumentRequest { InstrumentRequest::Type::ApplyAll, m_trackSettingsModel->toInstrument() };
        juzzlin::L(TAG).info() << "Applying ALL: " << instrumentRequest.instrument()->toString().toStdString();
        m_midiService->handleInstrumentRequest(instrumentRequest);
    });

    connect(m_trackSettingsModel.get(), &TrackSettingsModel::applyPatchRequested, this, [this]() {
        const InstrumentRequest instrumentRequest { InstrumentRequest::Type::ApplyPatch, m_trackSettingsModel->toInstrument() };
        juzzlin::L(TAG).info() << "Applying PATCH: " << instrumentRequest.instrument()->toString().toStdString();
        m_midiService->handleInstrumentRequest(instrumentRequest);
    });

    connect(m_trackSettingsModel.get(), &TrackSettingsModel::saveRequested, this, [this]() {
        m_editorService->setInstrument(m_trackSettingsModel->trackIndex(), m_trackSettingsModel->toInstrument());
    });

    connect(m_trackSettingsModel.get(), &TrackSettingsModel::testSoundRequested, this, [this](uint8_t velocity) {
        m_midiService->playAndStopMiddleC(m_trackSettingsModel->portName(), m_trackSettingsModel->channel(), velocity);
    });
}

void Application::initialize()
{
    initializeApplicationEngine();

    connectServices();

    m_stateMachine->calculateState(StateMachine::Action::ApplicationInitialized);
}

void Application::initializeApplicationEngine()
{
    setContextProperties();

    const auto entryPoint = QML_ROOT_DIR + QString { "/" } + QML_ENTRY_POINT;
    juzzlin::L(TAG).info() << "Loading entry point " << entryPoint.toStdString();
    m_engine->load(entryPoint);
    if (m_engine->rootObjects().isEmpty()) {
        throw std::runtime_error("Failed to initialize QML application engine!");
    }
}

int Application::run()
{
    initialize();

    return m_application->exec();
}

void Application::applyState(StateMachine::State state)
{
    juzzlin::L(TAG).info() << "Applying state: " << StateMachine::stateToString(state).toStdString();

    switch (state) {
    case StateMachine::State::Exit:
        if (m_playerService->isPlaying()) {
            m_playerService->stop();
        }
        m_application->exit(EXIT_SUCCESS);
        break;
    case StateMachine::State::InitializeNewProject:
        m_trackSettingsModel->reset();
        m_editorService->initialize();
        break;
    case StateMachine::State::OpenRecent:
        try {
            m_editorService->load(m_recentFilesManager->selectedFile());
            m_recentFilesManager->addRecentFile(m_recentFilesManager->selectedFile()); // Moves the loaded file to top
            m_stateMachine->calculateState(StateMachine::Action::ProjectOpened);
        } catch (...) {
            m_stateMachine->calculateState(StateMachine::Action::OpeningProjectFailed);
        }
        break;
    case StateMachine::State::Save:
        m_editorService->save();
        m_stateMachine->calculateState(StateMachine::Action::ProjectSaved);
        break;
    case StateMachine::State::ShowRecentFilesDialog:
        m_applicationService->requestRecentFilesDialog();
        break;
    case StateMachine::State::ShowUnsavedChangesDialog:
        m_applicationService->requestUnsavedChangesDialog();
        break;
    case StateMachine::State::ShowOpenDialog:
        m_applicationService->requestOpenDialog();
        break;
    case StateMachine::State::ShowSaveAsDialog:
        m_applicationService->requestSaveAsDialog();
        break;
    default:
        break;
    }
}

Application::~Application() = default;

} // namespace noteahead
