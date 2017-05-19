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
#include "../infra/video/video_generator.hpp"
#include "common/utils.hpp"
#include "config.hpp"
#include "models/event_selection_model.hpp"
#include "models/midi_cc_automations_model.hpp"
#include "models/pitch_bend_automations_model.hpp"
#include "models/recent_files_model.hpp"
#include "models/track_settings_model.hpp"
#include "recent_files_manager.hpp"
#include "service/application_service.hpp"
#include "service/automation_service.hpp"
#include "service/editor_service.hpp"
#include "service/midi_service.hpp"
#include "service/mixer_service.hpp"
#include "service/player_service.hpp"
#include "service/selection_service.hpp"
#include "service/util_service.hpp"
#include "state_machine.hpp"
#include "ui_logger.hpp"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

Q_DECLARE_METATYPE(noteahead::InstrumentRequest)
Q_DECLARE_METATYPE(noteahead::Position)

namespace noteahead {

static const auto TAG = "Application";

Application::Application(int & argc, char ** argv)
  : m_uiLogger { std::make_unique<UiLogger>() }
  , m_application { std::make_unique<QGuiApplication>(argc, argv) }
  , m_applicationService { std::make_unique<ApplicationService>() }
  , m_automationService { std::make_unique<AutomationService>() }
  , m_config { std::make_unique<Config>() }
  , m_selectionService { std::make_unique<SelectionService>() }
  , m_editorService { std::make_unique<EditorService>(m_selectionService) }
  , m_eventSelectionModel { std::make_unique<EventSelectionModel>() }
  , m_midiService { std::make_unique<MidiService>() }
  , m_mixerService { std::make_unique<MixerService>() }
  , m_playerService { std::make_unique<PlayerService>(m_midiService, m_mixerService, m_automationService, m_config) }
  , m_stateMachine { std::make_unique<StateMachine>(m_applicationService, m_editorService) }
  , m_recentFilesManager { std::make_unique<RecentFilesManager>() }
  , m_recentFilesModel { std::make_unique<RecentFilesModel>() }
  , m_midiCcAutomationsModel { std::make_unique<MidiCcAutomationsModel>() }
  , m_pitchBendAutomationsModel { std::make_unique<PitchBendAutomationsModel>() }
  , m_trackSettingsModel { std::make_unique<TrackSettingsModel>() }
  , m_utilService { std::make_unique<UtilService>() }
  , m_engine { std::make_unique<QQmlApplicationEngine>() }
{
    registerTypes();

    handleCommandLineArguments(argc, argv); // Handle command-line arguments at initialization

    m_applicationService->setRecentFilesManager(m_recentFilesManager);
    m_recentFilesModel->setRecentFiles(m_recentFilesManager->recentFiles());
    connect(m_recentFilesManager.get(), &RecentFilesManager::recentFilesChanged, m_recentFilesModel.get(), &RecentFilesModel::setRecentFiles);

    m_applicationService->setStateMachine(m_stateMachine);
    m_applicationService->setEditorService(m_editorService);
    m_applicationService->setPlayerService(m_playerService);
}

void Application::registerTypes()
{
    const int majorVersion = 1;
    const int minorVersion = 0;

    qRegisterMetaType<noteahead::Position>("Position");
    qRegisterMetaType<noteahead::InstrumentRequest>("InstrumentRequest");

    qmlRegisterType<ApplicationService>("Noteahead", majorVersion, minorVersion, "ApplicationService");
    qmlRegisterType<AutomationService>("Noteahead", majorVersion, minorVersion, "AutomationService");
    qmlRegisterType<Config>("Noteahead", majorVersion, minorVersion, "Config");
    qmlRegisterType<EditorService>("Noteahead", majorVersion, minorVersion, "EditorService");
    qmlRegisterType<EditorService>("Noteahead", majorVersion, minorVersion, "EventSelectionModel");
    qmlRegisterType<MidiCcAutomationsModel>("Noteahead", majorVersion, minorVersion, "MidiCcAutomationsModel");
    qmlRegisterType<MidiService>("Noteahead", majorVersion, minorVersion, "MidiService");
    qmlRegisterType<MixerService>("Noteahead", majorVersion, minorVersion, "MixerService");
    qmlRegisterType<PitchBendAutomationsModel>("Noteahead", majorVersion, minorVersion, "PitchBendAutomationsModel");
    qmlRegisterType<RecentFilesModel>("Noteahead", majorVersion, minorVersion, "RecentFilesModel");
    qmlRegisterType<SelectionService>("Noteahead", majorVersion, minorVersion, "SelectionService");
    qmlRegisterType<TrackSettingsModel>("Noteahead", majorVersion, minorVersion, "TrackSettingsModel");
    qmlRegisterType<TrackSettingsModel>("Noteahead", majorVersion, minorVersion, "UtilService");
    qmlRegisterType<UiLogger>("Noteahead", majorVersion, minorVersion, "UiLogger");

    qmlRegisterSingletonType(QUrl(QML_ROOT_DIR + QString { "/Constants.qml" }), "Noteahead", majorVersion, minorVersion, "Constants");
    qmlRegisterSingletonType(QUrl(QML_ROOT_DIR + QString { "/UiService.qml" }), "Noteahead", majorVersion, minorVersion, "UiService");
}

void Application::setContextProperties()
{
    m_engine->rootContext()->setContextProperty("applicationService", m_applicationService.get());
    m_engine->rootContext()->setContextProperty("automationService", m_automationService.get());
    m_engine->rootContext()->setContextProperty("config", m_config.get());
    m_engine->rootContext()->setContextProperty("editorService", m_editorService.get());
    m_engine->rootContext()->setContextProperty("eventSelectionModel", m_eventSelectionModel.get());
    m_engine->rootContext()->setContextProperty("midiCcAutomationsModel", m_midiCcAutomationsModel.get());
    m_engine->rootContext()->setContextProperty("midiService", m_midiService.get());
    m_engine->rootContext()->setContextProperty("mixerService", m_mixerService.get());
    m_engine->rootContext()->setContextProperty("pitchBendAutomationsModel", m_pitchBendAutomationsModel.get());
    m_engine->rootContext()->setContextProperty("playerService", m_playerService.get());
    m_engine->rootContext()->setContextProperty("recentFilesModel", m_recentFilesModel.get());
    m_engine->rootContext()->setContextProperty("selectionService", m_selectionService.get());
    m_engine->rootContext()->setContextProperty("trackSettingsModel", m_trackSettingsModel.get());
    m_engine->rootContext()->setContextProperty("uiLogger", m_uiLogger.get());
    m_engine->rootContext()->setContextProperty("utilService", m_utilService.get());
}

void Application::addVideoOptions(juzzlin::Argengine & ae)
{
    ae.addOption({ "--video-ffmpeg" }, [this](const std::string & value) { m_videoConfig.ffmpegPath = value; }, false, "Custom path for ffmpeg executable.");

    ae.addOption({ "--video-audio" }, [this](const std::string & value) { m_videoConfig.audioPath = value; }, false, "The audio file to create a video from.");
    ae.addOption({ "--video-audio-codec" }, [this](const std::string & value) { m_videoConfig.audioCodec = value; }, false, "The audio codec given to ffmpeg.");
    ae.addOption({ "--video-video-codec" }, [this](const std::string & value) { m_videoConfig.videoCodec = value; }, false, "The video codec given to ffmpeg.");
    ae.addOption({ "--video-song" }, [this](const std::string & value) {
        m_videoGeneratorEnabled = true;
        m_videoConfig.songPath = value; }, false, "The .nahd song to create a video from.");

    ae.addOption({ "--video-image" }, [this](const std::string & value) { m_videoConfig.imagePath = value; }, false, "An optional background image for the video.");
    ae.addOption({ "--video-image-opacity" }, [this](const std::string & value) { m_videoConfig.imageOpacity = Utils::parseDecimal(value).value_or(0); }, false, "Opacity of the background image.");
    ae.addOption({ "--video-image-zoom-speed" }, [this](const std::string & value) { m_videoConfig.imageZoomSpeed = Utils::parseDecimal(value).value_or(0); }, false, "Zoom speed for the background image, e.g. 0.0001. The default is zero.");
    ae.addOption({ "--video-logo" }, [this](const std::string & value) { m_videoConfig.logoPath = value; }, false, "An optional logo for the video.");
    ae.addOption({ "--video-logo-pos" }, [this](const std::string & value) {
        const auto valueString = QString::fromStdString(value);
        if (const auto splitString = valueString.split(","); splitString.size() == 2) {
            m_videoConfig.logoX = std::stoi(splitString.at(0).toStdString());
            m_videoConfig.logoY = std::stoi(splitString.at(1).toStdString());
        } else {
            throw std::runtime_error { std::string { "Invalid syntax for size: " } + value};
        } }, false, "Position of the logo image. The default is 0,0.");
    ae.addOption({ "--video-logo-fade-factor" }, [this](const std::string & value) { m_videoConfig.logoFadeFactor = Utils::parseDecimal(value).value_or(1.0); }, false, "Fade out factor for the logo, e.g. 0.99. The default is 1.0.");

    ae.addOption({ "--video-track-opacity" }, [this](const std::string & value) { m_videoConfig.trackOpacity = Utils::parseDecimal(value).value_or(0); }, false, "Opacity of the track lanes.");

    ae.addOption({ "--video-flash-track" }, [this](const std::string & value) { m_videoConfig.flashTrackName = value; }, false, "Source track name for a flash effect.");
    ae.addOption({ "--video-flash-column" }, [this](const std::string & value) { m_videoConfig.flashColumnName = value; }, false, "Source column name for a flash effect.");

    ae.addOption({ "--video-fps" }, [this](const std::string & value) { m_videoConfig.fps = std::stoul(value); }, false, "FPS of the generated video. The default is 60 fps.");
    ae.addOption({ "--video-size" }, [this](const std::string & value) {
        const auto valueString = QString::fromStdString(value);
        if (const auto splitString = valueString.split("x"); splitString.size() == 2) {
            m_videoConfig.width = std::stoi(splitString.at(0).toStdString());
            m_videoConfig.height = std::stoi(splitString.at(1).toStdString());
        } else {
            throw std::runtime_error { std::string { "Invalid syntax for size: " } + value};
        } }, false, "Size of the generated video. The default is 1920x1080.");

    ae.addOption({ "--video-output-dir" }, [this](const std::string & value) { m_videoConfig.outputDir = value + "-" + std::to_string(QDateTime::currentSecsSinceEpoch()); }, false, "The output directory for the generated video frames. Will be created if doesn't exist.");

    ae.addOption({ "--video-start-pos" }, [this](const std::string & value) { m_videoConfig.startPosition = std::stoul(value); }, false, "The song play order position to start the video generation at. The default is 0.");
    ae.addOption({ "--video-length" }, [this](const std::string & value) { m_videoConfig.length = std::chrono::milliseconds { std::stoul(value) }; }, false, "The length of the video in milliseconds. The default is the whole song.");
    ae.addOption({ "--video-lead-in-time" }, [this](const std::string & value) { m_videoConfig.leadInTime = std::chrono::milliseconds { std::stoul(value) }; }, false, "The lead-in time for the video to match audio in milliseconds. The default is 0 ms.");
    ae.addOption({ "--video-lead-out-time" }, [this](const std::string & value) { m_videoConfig.leadOutTime = std::chrono::milliseconds { std::stoul(value) }; }, false, "The lead-out time for the video to match audio in milliseconds. The default is 0 ms.");

    ae.addOption({ "--video-scrolling-text" }, [this](const std::string & value) { m_videoConfig.scrollingText = value; }, false, "An optional text to scroll during the video.");
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

    ae.addOption({ "--window-size" }, [this](const std::string & value) {
        const auto valueString = QString::fromStdString(value);
        if (const auto splitString = valueString.split("x"); splitString.size() == 2) {
            m_config->setWindowSize({std::stoi(splitString.at(0).toStdString()), std::stoi(splitString.at(1).toStdString()) });
        } else {
            throw std::runtime_error { std::string { "Invalid syntax for size: " } + value};
        } }, false, "Force the window size, e.g. '1920x1080'.");

    addVideoOptions(ae);

    ae.parse();
}

void Application::connectServices()
{
    connectApplicationService();

    connectAutomationService();
    connectEditorService();
    connectMidiService();
    connectMixerService();
    connectPlayerService();

    connectStateMachine();

    connectEventSelectionModel();
    connectMidiCcAutomationsModel();
    connectPitchBendAutomationsModel();
    connectTrackSettingsModel();
}

void Application::connectApplicationService()
{
    connect(m_applicationService.get(), &ApplicationService::applyAllTrackSettingsRequested, this, &Application::applyAllInstruments);
    connect(m_applicationService.get(), &ApplicationService::liveNoteOnRequested, m_midiService.get(), &MidiService::playNote);
    connect(m_applicationService.get(), &ApplicationService::liveNoteOffRequested, m_midiService.get(), &MidiService::stopNote);
}

void Application::connectAutomationService()
{
    connect(m_automationService.get(), &AutomationService::lineDataChanged, this, [this]() {
        m_editorService->setIsModified(true);
    });
}

void Application::connectMidiCcAutomationsModel()
{
    connect(m_midiCcAutomationsModel.get(), &MidiCcAutomationsModel::midiCcAutomationsRequested, this, [this]() {
        m_midiCcAutomationsModel->setMidiCcAutomations(m_automationService->midiCcAutomations());
    });
    connect(m_midiCcAutomationsModel.get(), &MidiCcAutomationsModel::midiCcAutomationChanged, this, [this](auto && item) {
        m_automationService->updateMidiCcAutomation(item);
        m_editorService->setIsModified(true);
    });
    connect(m_midiCcAutomationsModel.get(), &MidiCcAutomationsModel::midiCcAutomationDeleted, this, [this](auto && item) {
        m_automationService->deleteMidiCcAutomation(item);
        m_editorService->setIsModified(true);
    });
}

void Application::connectPitchBendAutomationsModel()
{
    connect(m_pitchBendAutomationsModel.get(), &PitchBendAutomationsModel::pitchBendAutomationsRequested, this, [this]() {
        m_pitchBendAutomationsModel->setPitchBendAutomations(m_automationService->pitchBendAutomations());
    });
    connect(m_pitchBendAutomationsModel.get(), &PitchBendAutomationsModel::pitchBendAutomationChanged, this, [this](auto && item) {
        m_automationService->updatePitchBendAutomation(item);
        m_editorService->setIsModified(true);
    });
    connect(m_pitchBendAutomationsModel.get(), &PitchBendAutomationsModel::pitchBendAutomationDeleted, this, [this](auto && item) {
        m_automationService->deletePitchBendAutomation(item);
        m_editorService->setIsModified(true);
    });
}

void Application::connectEditorService()
{
    connect(m_editorService.get(), &EditorService::aboutToChangeSong, m_mixerService.get(), &MixerService::clear);
    connect(m_editorService.get(), &EditorService::aboutToInitialize, m_mixerService.get(), &MixerService::clear);
    connect(m_editorService.get(), &EditorService::instrumentRequested, m_midiService.get(), &MidiService::handleInstrumentRequest);

    connect(m_editorService.get(), &EditorService::automationDeserializationRequested, m_automationService.get(), &AutomationService::deserializeFromXml);
    connect(m_editorService.get(), &EditorService::automationSerializationRequested, m_automationService.get(), &AutomationService::serializeToXml);
    connect(m_editorService.get(), &EditorService::mixerDeserializationRequested, m_mixerService.get(), &MixerService::deserializeFromXml);
    connect(m_editorService.get(), &EditorService::mixerSerializationRequested, m_mixerService.get(), &MixerService::serializeToXml);

    connect(m_editorService.get(), &EditorService::songPositionChanged, m_playerService.get(), &PlayerService::setSongPosition);
}

void Application::connectMidiService()
{
    connect(m_midiService.get(), &MidiService::availableMidiPortsChanged, m_trackSettingsModel.get(), &TrackSettingsModel::setAvailableMidiPorts);
    connect(m_midiService.get(), &MidiService::midiPortsAppeared, this, &Application::requestInstruments);
    connect(m_midiService.get(), &MidiService::statusTextRequested, m_applicationService.get(), &ApplicationService::statusTextRequested);
}

void Application::connectMixerService()
{
    connect(m_mixerService.get(), &MixerService::configurationChanged, this, [this]() {
        m_editorService->setIsModified(true);
    });
    connect(m_mixerService.get(), &MixerService::columnCountOfTrackRequested, this, [this](size_t trackIndex) {
        m_mixerService->setColumnCount(trackIndex, m_editorService->columnCount(trackIndex));
    });
    connect(m_mixerService.get(), &MixerService::trackIndicesRequested, this, [this] {
        m_mixerService->setTrackIndices(m_editorService->trackIndices());
    });
}

void Application::connectStateMachine()
{
    connect(m_stateMachine.get(), &StateMachine::stateChanged, this, &Application::applyState);
}

void Application::connectPlayerService()
{
    connect(m_playerService.get(), &PlayerService::songRequested, this, [this] {
        m_playerService->setSong(m_editorService->song());
    });
    connect(m_playerService.get(), &PlayerService::tickUpdated, m_editorService.get(), &EditorService::requestPositionByTick);

    connect(m_playerService.get(), &PlayerService::isPlayingChanged, this, [this]() {
        m_midiService->setIsPlaying(m_playerService->isPlaying());
    });
}

void Application::connectEventSelectionModel()
{
    connect(m_eventSelectionModel.get(), &EventSelectionModel::dataRequested, this, [this]() {
        if (const auto instrumentSettings = m_editorService->instrumentSettingsAtCurrentPosition(); instrumentSettings) {
            m_eventSelectionModel->fromInstrumentSettings(*instrumentSettings);
        } else {
            m_eventSelectionModel->reset();
        }
    });

    connect(m_eventSelectionModel.get(), &EventSelectionModel::saveRequested, this, [this]() {
        m_editorService->setInstrumentSettingsAtCurrentPosition(m_eventSelectionModel->toInstrumentSettings());
    });
}

void Application::connectTrackSettingsModel()
{
    connect(m_trackSettingsModel.get(), &TrackSettingsModel::instrumentDataRequested, this, [this]() {
        if (const auto instrument = m_editorService->instrument(m_trackSettingsModel->trackIndex()); instrument) {
            m_trackSettingsModel->setInstrumentData(*instrument);
        } else {
            m_trackSettingsModel->reset();
        }
    });

    connect(m_trackSettingsModel.get(), &TrackSettingsModel::applyAllRequested, this, [this]() {
        const InstrumentRequest instrumentRequest { InstrumentRequest::Type::ApplyAll, *m_trackSettingsModel->toInstrument() };
        juzzlin::L(TAG).info() << "Applying ALL: " << instrumentRequest.instrument().toString().toStdString();
        m_midiService->handleInstrumentRequest(instrumentRequest);
    });

    connect(m_trackSettingsModel.get(), &TrackSettingsModel::applyMidiCcRequested, this, [this]() {
        const InstrumentRequest instrumentRequest { InstrumentRequest::Type::ApplyMidiCc, *m_trackSettingsModel->toInstrument() };
        juzzlin::L(TAG).info() << "Applying MIDI CC: " << instrumentRequest.instrument().toString().toStdString();
        m_midiService->handleInstrumentRequest(instrumentRequest);
    });

    connect(m_trackSettingsModel.get(), &TrackSettingsModel::applyPatchRequested, this, [this]() {
        const InstrumentRequest instrumentRequest { InstrumentRequest::Type::ApplyPatch, *m_trackSettingsModel->toInstrument() };
        juzzlin::L(TAG).info() << "Applying PATCH: " << instrumentRequest.instrument().toString().toStdString();
        m_midiService->handleInstrumentRequest(instrumentRequest);
    });

    connect(m_trackSettingsModel.get(), &TrackSettingsModel::saveRequested, this, [this]() {
        m_editorService->setInstrument(m_trackSettingsModel->trackIndex(), m_trackSettingsModel->toInstrument());
    });

    connect(m_trackSettingsModel.get(), &TrackSettingsModel::testSoundRequested, this, [this](uint8_t velocity) {
        m_midiService->playAndStopMiddleC(m_trackSettingsModel->portName(), m_trackSettingsModel->channel(), velocity);
    });
}

int Application::initializeTracker()
{
    juzzlin::L(TAG).info() << "Initializing tracker";

    initializeApplicationEngine();
    connectServices();
    m_stateMachine->calculateState(StateMachine::Action::ApplicationInitialized);

    return m_application->exec();
}

int Application::runVideoGenerator()
{
    juzzlin::L(TAG).info() << "Running video generator";
    VideoGenerator videoGenerator { m_mixerService };
    m_editorService->load(QString::fromStdString(m_videoConfig.songPath));
    m_videoConfig.image = !m_videoConfig.imagePath.empty() ? QImage { QString::fromStdString(m_videoConfig.imagePath) } : QImage {};
    m_videoConfig.logo = !m_videoConfig.logoPath.empty() ? QImage { QString::fromStdString(m_videoConfig.logoPath) } : QImage {};
    videoGenerator.run(m_editorService->song(), m_videoConfig);

    return EXIT_SUCCESS;
}

int Application::initialize()
{
    if (m_videoGeneratorEnabled) {
        return runVideoGenerator();
    } else {
        return initializeTracker();
    }
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

void Application::requestInstruments(QStringList midiPorts)
{
    for (auto && midiPort : midiPorts) {
        for (auto && trackIndex : m_editorService->trackIndices()) {
            if (const auto instrument = m_editorService->instrument(trackIndex); instrument) {
                if (Utils::portNameMatchScore(instrument->device().portName.toStdString(), midiPort.toStdString()) > 0.75) {
                    applyInstrument(trackIndex, *instrument);
                }
            }
        }
    }
}

int Application::run()
{
    return initialize();
}

void Application::applyInstrument(size_t trackIndex, const Instrument & instrument)
{
    const InstrumentRequest instrumentRequest { InstrumentRequest::Type::ApplyAll, instrument };
    juzzlin::L(TAG).info() << "Applying instrument on track " << trackIndex << ": " << instrumentRequest.instrument().toString().toStdString();
    m_midiService->handleInstrumentRequest(instrumentRequest);
}

void Application::applyMidiCcSettings(const Instrument & instrument)
{
    const InstrumentRequest instrumentRequest { InstrumentRequest::Type::ApplyMidiCc, instrument };
    juzzlin::L(TAG).info() << "Applying MIDI CC settings: " << instrumentRequest.instrument().toString().toStdString();
    m_midiService->handleInstrumentRequest(instrumentRequest);
}

void Application::applyAllInstruments()
{
    juzzlin::L(TAG).info() << "Applying all instruments";

    for (auto trackIndex : m_editorService->trackIndices()) {
        if (const auto instrument = m_editorService->instrument(trackIndex); instrument) {
            applyInstrument(trackIndex, *instrument);
        }
    }

    m_instrumentTimer = std::make_unique<QTimer>();
    m_instrumentTimer->setSingleShot(true);
    m_instrumentTimer->setInterval(1000);
    connect(m_instrumentTimer.get(), &QTimer::timeout, this, &Application::applyAllMidiCcSettings);
    m_instrumentTimer->start();
}

void Application::applyAllMidiCcSettings()
{
    for (auto trackIndex : m_editorService->trackIndices()) {
        if (const auto instrument = m_editorService->instrument(trackIndex); instrument) {
            applyMidiCcSettings(*instrument);
        }
    }
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
        m_automationService->clear();
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
    case StateMachine::State::ShowSaveAsTemplateDialog:
        m_applicationService->requestSaveAsTemplateDialog();
        break;
    default:
        break;
    }
}

Application::~Application() = default;

} // namespace noteahead
