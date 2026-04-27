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
#include "../infra/midi/export/midi_exporter.hpp"
#include "../infra/video/video_generator.hpp"
#include "common/utils.hpp"
#include "domain/column_settings.hpp"
#include "domain/midi_note_data.hpp"
#include "models/audio_settings_model.hpp"
#include "models/column_settings_model.hpp"
#include "models/event_selection_model.hpp"
#include "models/midi_cc_automations_model.hpp"
#include "models/midi_settings_model.hpp"
#include "models/note_column_line_container_helper.hpp"
#include "models/note_column_model_handler.hpp"
#include "models/pitch_bend_automations_model.hpp"
#include "models/recent_files_model.hpp"
#include "models/sampler/sampler_pad_model.hpp"
#include "models/track_settings_model.hpp"
#include "note_converter.hpp"
#include "service/application_service.hpp"
#include "service/audio_service.hpp"
#include "service/automation_service.hpp"
#include "service/device_service.hpp"
#include "service/editor_service.hpp"
#include "service/jack_service.hpp"
#include "service/keyboard_service.hpp"
#include "service/midi_service.hpp"
#include "service/mixer_service.hpp"
#include "service/player_service.hpp"
#include "service/property_service.hpp"
#include "service/recent_files_manager.hpp"
#include "service/sampler_controller.hpp"
#include "service/selection_service.hpp"
#include "service/settings_service.hpp"
#include "service/side_chain_service.hpp"
#include "service/theme_service.hpp"
#include "service/util_service.hpp"
#include "domain/devices/sampler_device.hpp"
#include "infra/audio/audio_engine.hpp"
#include "infra/audio/backend/audio_file_reader.hpp"
#include "infra/audio/backend/sndfile_reader.hpp"
#include "state_machine.hpp"
#include "ui_logger.hpp"
#include "view/qml/Editor/line_number_renderer.hpp"
#include "view/qml/Editor/note_column_renderer.hpp"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QFileInfo>

#include <iostream>
#include <iomanip>
#include <regex>

Q_DECLARE_METATYPE(noteahead::InstrumentRequest)
Q_DECLARE_METATYPE(noteahead::Position)

namespace noteahead {

static const auto TAG = "Application";

Application::Application(int & argc, char ** argv)
  : m_uiLogger { std::make_unique<UiLogger>() }
  , m_application { std::make_unique<QGuiApplication>(argc, argv) }
  , m_applicationService { std::make_shared<ApplicationService>() }
  , m_settingsService { std::make_shared<SettingsService>() }
  , m_themeService { std::make_shared<ThemeService>() }
  , m_selectionService { std::make_shared<SelectionService>() }
  , m_utilService { std::make_shared<UtilService>() }
  , m_propertyService { std::make_shared<PropertyService>() }
  , m_automationService { std::make_shared<AutomationService>(m_propertyService) }
  , m_editorService { std::make_shared<EditorService>(m_selectionService, m_settingsService, m_automationService) }
  , m_audioEngine { std::make_shared<AudioEngine>() }
  , m_deviceService { std::make_shared<DeviceService>(m_audioEngine) }
  , m_samplerController { std::make_shared<SamplerController>(std::make_shared<SamplerDevice>()) }
  , m_jackService { std::make_shared<JackService>(m_settingsService, m_audioEngine) }
  , m_audioService { std::make_shared<AudioService>(m_settingsService, m_jackService, m_audioEngine) }
  , m_eventSelectionModel { std::make_shared<EventSelectionModel>() }
  , m_midiService { std::make_shared<MidiService>(m_deviceService) }
  , m_mixerService { std::make_shared<MixerService>() }
  , m_sideChainService { std::make_shared<SideChainService>() }
  , m_playerService { std::make_shared<PlayerService>(m_midiService, m_mixerService, m_automationService, m_settingsService, m_sideChainService, m_jackService) }
  , m_keyboardService { std::make_shared<KeyboardService>(m_applicationService, m_editorService, m_playerService, m_selectionService, m_settingsService) }
  , m_midiExporter { std::make_shared<MidiExporter>(m_automationService, m_mixerService, m_sideChainService) }
  , m_midiImporter { std::make_shared<MidiImporter>() }
  , m_stateMachine { std::make_shared<StateMachine>(m_applicationService, m_editorService) }
  , m_recentFilesManager { std::make_shared<RecentFilesManager>() }
  , m_recentFilesModel { std::make_unique<RecentFilesModel>() }
  , m_midiCcAutomationsModel { std::make_unique<MidiCcAutomationsModel>() }
  , m_pitchBendAutomationsModel { std::make_unique<PitchBendAutomationsModel>() }
  , m_columnSettingsModel { std::make_unique<ColumnSettingsModel>() }
  , m_trackSettingsModel { std::make_unique<TrackSettingsModel>() }
  , m_midiSettingsModel { std::make_unique<MidiSettingsModel>(m_settingsService) }
  , m_audioSettingsModel { std::make_unique<AudioSettingsModel>(m_audioService, m_settingsService) }
  , m_noteColumnLineContainerHelper { std::make_shared<NoteColumnLineContainerHelper>(
      m_automationService, m_editorService, m_selectionService, m_utilService) }
  , m_noteColumnModelHandler { std::make_unique<NoteColumnModelHandler>(m_editorService, m_selectionService, m_automationService, m_settingsService) }
  , m_engine { std::make_unique<QQmlApplicationEngine>() }
{
    m_deviceService->registerDevice(m_samplerController->sampler());
    m_editorService->setMixerService(m_mixerService);

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
    qRegisterMetaType<noteahead::MidiAddress>("MidiAddress");
    qRegisterMetaType<const noteahead::MidiAddress &>("MidiAddressCR");
    qRegisterMetaType<noteahead::MidiNoteData>("MidiNoteData");
    qRegisterMetaType<const noteahead::MidiNoteData &>("MidiNoteDataCR");

    qmlRegisterType<ApplicationService>("Noteahead", majorVersion, minorVersion, "ApplicationService");
    qmlRegisterType<AudioSettingsModel>("Noteahead", majorVersion, minorVersion, "AudioSettingsModel");
    qmlRegisterType<AutomationService>("Noteahead", majorVersion, minorVersion, "AutomationService");
    qmlRegisterType<ColumnSettingsModel>("Noteahead", majorVersion, minorVersion, "ColumnSettingsModel");
    qmlRegisterType<EditorService>("Noteahead", majorVersion, minorVersion, "EditorService");
    qmlRegisterType<EventSelectionModel>("Noteahead", majorVersion, minorVersion, "EventSelectionModel");
    qmlRegisterType<KeyboardService>("Noteahead", majorVersion, minorVersion, "KeyboardService");
    qmlRegisterType<LineNumberRenderer>("Noteahead", majorVersion, minorVersion, "LineNumberRenderer");
    qmlRegisterType<MidiCcAutomationsModel>("Noteahead", majorVersion, minorVersion, "MidiCcAutomationsModel");
    qmlRegisterType<MidiCcSelectionModel>("Noteahead", majorVersion, minorVersion, "MidiCcSelectionModel");
    qmlRegisterType<MidiService>("Noteahead", majorVersion, minorVersion, "MidiService");
    qmlRegisterType<MidiSettingsModel>("Noteahead", majorVersion, minorVersion, "MidiSettingsModel");
    qmlRegisterType<MixerService>("Noteahead", majorVersion, minorVersion, "MixerService");
    qmlRegisterType<NoteColumnLineContainerHelper>("Noteahead", majorVersion, minorVersion, "NoteColumnLineContainerHelper");
    qmlRegisterType<NoteColumnRenderer>("Noteahead", majorVersion, minorVersion, "NoteColumnRenderer");
    qmlRegisterType<PitchBendAutomationsModel>("Noteahead", majorVersion, minorVersion, "PitchBendAutomationsModel");
    qmlRegisterType<PropertyService>("Noteahead", majorVersion, minorVersion, "PropertyService");
    qmlRegisterType<RecentFilesModel>("Noteahead", majorVersion, minorVersion, "RecentFilesModel");
    qmlRegisterType<SamplerPadModel>("Noteahead", majorVersion, minorVersion, "SamplerPadModel");
    qmlRegisterType<SamplerController>("Noteahead", majorVersion, minorVersion, "SamplerController");
    qmlRegisterType<SelectionService>("Noteahead", majorVersion, minorVersion, "SelectionService");
    qmlRegisterType<SettingsService>("Noteahead", majorVersion, minorVersion, "SettingsService");
    qmlRegisterType<SideChainService>("Noteahead", majorVersion, minorVersion, "SideChainService");
    qmlRegisterType<TrackSettingsModel>("Noteahead", majorVersion, minorVersion, "TrackSettingsModel");
    qmlRegisterType<UiLogger>("Noteahead", majorVersion, minorVersion, "UiLogger");
    qmlRegisterType<UtilService>("Noteahead", majorVersion, minorVersion, "UtilService");

    qmlRegisterSingletonType(QUrl(QML_ROOT_DIR + QString { "/Constants.qml" }), "Noteahead", majorVersion, minorVersion, "Constants");
    qmlRegisterSingletonType(QUrl(QML_ROOT_DIR + QString { "/UiService.qml" }), "Noteahead", majorVersion, minorVersion, "UiService");
}

void Application::setContextProperties()
{
    m_engine->rootContext()->setContextProperty("applicationService", m_applicationService.get());
    m_engine->rootContext()->setContextProperty("audioService", m_audioService.get());
    m_engine->rootContext()->setContextProperty("audioSettingsModel", m_audioSettingsModel.get());
    m_engine->rootContext()->setContextProperty("automationService", m_automationService.get());
    m_engine->rootContext()->setContextProperty("columnSettingsModel", m_columnSettingsModel.get());
    m_engine->rootContext()->setContextProperty("deviceService", m_deviceService.get());
    m_engine->rootContext()->setContextProperty("samplerController", m_samplerController.get());
    m_engine->rootContext()->setContextProperty("editorService", m_editorService.get());
    m_engine->rootContext()->setContextProperty("eventSelectionModel", m_eventSelectionModel.get());
    m_engine->rootContext()->setContextProperty("midiCcAutomationsModel", m_midiCcAutomationsModel.get());
    m_engine->rootContext()->setContextProperty("keyboardService", m_keyboardService.get());
    m_engine->rootContext()->setContextProperty("midiService", m_midiService.get());
    m_engine->rootContext()->setContextProperty("midiSettingsModel", m_midiSettingsModel.get());
    m_engine->rootContext()->setContextProperty("mixerService", m_mixerService.get());
    m_engine->rootContext()->setContextProperty("noteColumnLineContainerHelper", m_noteColumnLineContainerHelper.get());
    m_engine->rootContext()->setContextProperty("noteColumnModelHandler", m_noteColumnModelHandler.get());
    m_engine->rootContext()->setContextProperty("pitchBendAutomationsModel", m_pitchBendAutomationsModel.get());
    m_engine->rootContext()->setContextProperty("playerService", m_playerService.get());
    m_engine->rootContext()->setContextProperty("propertyService", m_propertyService.get());
    m_engine->rootContext()->setContextProperty("recentFilesModel", m_recentFilesModel.get());
    m_engine->rootContext()->setContextProperty("selectionService", m_selectionService.get());
    m_engine->rootContext()->setContextProperty("settingsService", m_settingsService.get());
    m_engine->rootContext()->setContextProperty("themeService", m_themeService.get());
    m_engine->rootContext()->setContextProperty("sideChainService", m_sideChainService.get());
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
    ae.addOption({ "--video-image-opacity" }, [this](const std::string & value) { m_videoConfig.imageOpacity = Utils::Misc::parseDecimal(value).value_or(0); }, false, "Opacity of the background image.");
    ae.addOption({ "--video-image-zoom-speed" }, [this](const std::string & value) { m_videoConfig.imageZoomSpeed = Utils::Misc::parseDecimal(value).value_or(0); }, false, "Zoom speed for the background image, e.g. 0.0001. The default is zero.");
    ae.addOption({ "--video-image-rotation-speed" }, [this](const std::string & value) { m_videoConfig.imageRotationSpeed = Utils::Misc::parseDecimal(value).value_or(0); }, false, "Rotation speed for the background image, e.g. 0.0001. The default is zero.");
    ae.addOption({ "--video-logo" }, [this](const std::string & value) { m_videoConfig.logoPath = value; }, false, "An optional logo for the video.");
    ae.addOption({ "--video-logo-pos" }, [this](const std::string & value) {
        const auto valueString = QString::fromStdString(value);
        if (const auto splitString = valueString.split(","); splitString.size() == 2) {
            m_videoConfig.logoX = std::stoi(splitString.at(0).toStdString());
            m_videoConfig.logoY = std::stoi(splitString.at(1).toStdString());
        } else {
            throw std::runtime_error { std::string { "Invalid syntax for size: " } + value};
        } }, false, "Position of the logo image. The default is 0,0.");
    ae.addOption({ "--video-logo-fade-factor" }, [this](const std::string & value) { m_videoConfig.logoFadeFactor = Utils::Misc::parseDecimal(value).value_or(1.0); }, false, "Fade out factor for the logo, e.g. 0.99. The default is 1.0.");

    ae.addOption({ "--video-track-opacity" }, [this](const std::string & value) { m_videoConfig.trackOpacity = Utils::Misc::parseDecimal(value).value_or(0); }, false, "Opacity of the track lanes.");

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
    ae.addOption({ "--video-type" }, [this](const std::string & value) { m_videoConfig.type = (value == "bars" ? VideoConfig::Type::Bars : VideoConfig::Type::Default); }, false, "Animation type: [default, bars]");
}

void Application::handleCommandLineArguments(int & argc, char ** argv)
{
    juzzlin::Argengine ae { argc, argv };

    ae.addOption({ "--debug" }, [] {
        juzzlin::SimpleLogger::setLoggingLevel(juzzlin::SimpleLogger::Level::Debug);
        juzzlin::SimpleLogger::setBatchInterval(0ms);
    });

    ae.addOption({ "--trace" }, [] {
        juzzlin::SimpleLogger::setLoggingLevel(juzzlin::SimpleLogger::Level::Trace);
        juzzlin::SimpleLogger::setBatchInterval(0ms);
    });

    ae.addOption({ "-v", "--version" }, [] {
        std::cout << "Noteahead version " << VERSION << std::endl;
        std::exit(EXIT_SUCCESS); }, false, "Print the version and exit.");

    ae.addOption({ "--window-size" }, [this](const std::string & value) {
        const auto valueString = QString::fromStdString(value);
        if (const auto splitString = valueString.split("x"); splitString.size() == 2) {
            m_settingsService->setWindowSize({ std::stoi(splitString.at(0).toStdString()), std::stoi(splitString.at(1).toStdString()) });
        } else {
            throw std::runtime_error { std::string { "Invalid syntax for size: " } + value};
        } }, false, "Force the window size, e.g. '1920x1080'.");

    ae.setPositionalArgumentCallback([this](const std::vector<std::string> & args) {
        if (!args.empty()) {
            const QString path = QString::fromStdString(args.front());
            m_applicationService->setInitialFilePath(QFileInfo { path }.absoluteFilePath());
        }
    });

    addVideoOptions(ae);

    ae.parse();
}

void Application::connectServices()
{
    connectApplicationService();
    connectDeviceService();

    connectAudioService();
    connectAutomationService();
    connectEditorService();
    connectJackService();
    connectMidiService();
    connectMixerService();
    connectPlayerService();

    connectStateMachine();

    connectEventSelectionModel();
    connectMidiCcAutomationsModel();
    connectPitchBendAutomationsModel();
    connectTrackSettingsModel();
    connectMidiSettingsModel();
    connectColumnSettingsModel();
}

void Application::connectDeviceService()
{
    connect(m_editorService.get(), &EditorService::devicesSerializationRequested, m_deviceService.get(), &DeviceService::serializeToXml);
    connect(m_editorService.get(), &EditorService::devicesDeserializationRequested, m_deviceService.get(), &DeviceService::deserializeFromXml);
    connect(m_editorService.get(), &EditorService::projectPathChanged, m_deviceService.get(), &DeviceService::setProjectPath);

    connect(m_deviceService.get(), &DeviceService::dataChanged, this, [this]() {
        m_editorService->setIsModified(true);
    });
}

void Application::connectAudioService()
{
    connect(m_audioService.get(), &AudioService::errorOccurred, m_applicationService.get(), &ApplicationService::requestAlertDialog);
    connect(m_audioService.get(), &AudioService::latestRecordingFileNameChanged, this, [this]() {
        m_editorService->setIsModified(true);
    });
}

void Application::connectJackService()
{
    connect(m_jackService.get(), &JackService::errorOccurred, m_applicationService.get(), &ApplicationService::requestAlertDialog);
    connect(m_jackService.get(), &JackService::rewindRequested, this, [this]() {
        if (m_playerService->isPlaying()) {
            m_playerService->stop();
            m_editorService->requestPositionByTick(0);
            m_playerService->play();
        } else {
            m_editorService->requestPositionByTick(0);
        }
    });
    m_jackService->initialize();
}

void Application::connectApplicationService()
{
    connect(m_applicationService.get(), &ApplicationService::applyAllTrackSettingsRequested, this, &Application::applyAllInstruments);
    connect(m_applicationService.get(), &ApplicationService::liveNoteOnAtCurrentPositionRequested, [this](std::shared_ptr<Instrument> instrument) {
        if (const auto noteData = m_editorService->song()->noteDataAtPosition(m_editorService->position()); noteData && noteData->note().has_value() && instrument) {
            m_midiService->playNote(instrument, { *noteData->note(), noteData->velocity() });
        }
    });
    connect(m_applicationService.get(), &ApplicationService::liveNoteOnRequested, m_midiService.get(), &MidiService::playNote);
    connect(m_applicationService.get(), &ApplicationService::liveNoteOffRequested, m_midiService.get(), &MidiService::stopNote);
    connect(m_applicationService.get(), &ApplicationService::allNotesOffRequested, this, &Application::stopAllNotes);

    connect(m_applicationService.get(), &ApplicationService::midiExportRequested, this, &Application::exportToMidi);
    connect(m_applicationService.get(), &ApplicationService::midiImportRequested, this, &Application::importFromMidi);
}

void Application::exportToMidi(QString fileName, quint64 startPosition, quint64 endPosition, MidiExportOptions options)
{
    try {
        m_midiExporter->exportTo(fileName.toStdString(), m_editorService->song(), startPosition, endPosition, options);
        const auto message = QString { "Exported the project to '%1' " }.arg(fileName);
        m_applicationService->requestStatusText(message);
    } catch (std::exception & e) {
        const auto message = QString { "Failed to export as MIDI: %1 " }.arg(e.what());
        juzzlin::L(TAG).error() << message.toStdString();
        m_applicationService->requestStatusText(message);
    }
}
void Application::importFromMidi(QString fileName, MidiImportMode importMode, int patternLength, bool quantizeNoteOn, bool quantizeNoteOff, bool connectMidiPorts)
{
    try {
        const auto midiData = m_midiImporter->parseMidiFile(fileName.toStdString());
        if (importMode == MidiImportMode::Overwrite) {
            m_mixerService->clear();
            m_automationService->clear();
        }
        m_midiImporter->importTo(midiData, m_editorService->song(), importMode, patternLength, quantizeNoteOn, quantizeNoteOff, connectMidiPorts ? m_midiService : nullptr);
        const auto message = QString { "Imported MIDI file '%1' " }.arg(fileName);
        m_applicationService->requestStatusText(message);
        m_editorService->setSong(m_editorService->song());
        emit m_editorService->beatsPerMinuteChanged();
        m_editorService->setIsModified(true);
        m_recentFilesManager->addRecentFile(fileName);
    } catch (std::exception & e) {
        const auto message = QString { "Failed to import MIDI: %1 " }.arg(e.what());
        juzzlin::L(TAG).error() << message.toStdString();
        m_applicationService->requestStatusText(message);
    }
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

void Application::connectColumnSettingsModel()
{
    connect(m_columnSettingsModel.get(), &ColumnSettingsModel::saveRequested, this, [this](quint64 trackIndex, quint64 columnIndex, const ColumnSettings & settings) {
        auto sharedSettings = std::make_shared<ColumnSettings>(settings);
        m_editorService->setColumnSettings(trackIndex, columnIndex, sharedSettings);
    });

    connect(m_columnSettingsModel.get(), &ColumnSettingsModel::dataRequested, this, [this]() {
        if (const auto settings = m_editorService->columnSettings(m_columnSettingsModel->trackIndex(), m_columnSettingsModel->columnIndex()); settings) {
            m_columnSettingsModel->setColumnSettings(*settings);
        } else {
            m_columnSettingsModel->reset();
        }
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
    connect(m_editorService.get(), &EditorService::sideChainDeserializationRequested, m_sideChainService.get(), &SideChainService::deserializeFromXml);
    connect(m_editorService.get(), &EditorService::sideChainSerializationRequested, m_sideChainService.get(), &SideChainService::serializeToXml);
    connect(m_editorService.get(), &EditorService::audioRecorderDeserializationRequested, m_audioService.get(), &AudioService::deserializeFromXml);
    connect(m_editorService.get(), &EditorService::audioRecorderSerializationRequested, m_audioService.get(), &AudioService::serializeToXml);
    connect(m_editorService.get(), &EditorService::patternsDeleted, m_automationService.get(), &AutomationService::deletePatterns);
    connect(m_editorService.get(), &EditorService::patternsDeleted, m_noteColumnModelHandler.get(), &NoteColumnModelHandler::deletePatterns);
    connect(m_editorService.get(), &EditorService::trackDeleted, m_sideChainService.get(), &SideChainService::removeSettings);

    connect(m_editorService.get(), &EditorService::songPositionChanged, m_playerService.get(), &PlayerService::setSongPosition);

    connect(m_editorService.get(), &EditorService::positionChanged, this, [this](const auto & newPosition, const auto &) {
        if (const auto settings = m_editorService->columnSettings(newPosition.track, newPosition.column); settings) {
            m_columnSettingsModel->setColumnSettings(*settings);
        } else {
            // Reset or clear the model if no settings are found
        }
    });
}

void Application::connectMidiService()
{
    connect(m_midiService.get(), &MidiService::inputPortsChanged, m_midiSettingsModel.get(), &MidiSettingsModel::setMidiInPorts);
    connect(m_midiService.get(), &MidiService::inputPortsChanged, this, &Application::applyMidiController);
    connect(m_midiService.get(), &MidiService::outputPortsChanged, m_trackSettingsModel.get(), &TrackSettingsModel::setAvailableMidiPorts);
    connect(m_midiService.get(), &MidiService::outputPortsAppeared, this, &Application::requestInstruments);
    connect(m_midiService.get(), &MidiService::statusTextRequested, m_applicationService.get(), &ApplicationService::statusTextRequested);

    connect(m_settingsService.get(), &SettingsService::midiSyncEnabledChanged, this, [this]() {
        m_midiService->setMidiSyncEnabled(m_settingsService->midiSyncEnabled());
    });

    // Play a note via a MIDI controller
    connect(m_midiService.get(), &MidiService::noteOnReceived, this, [this](const auto &, const auto & data) {
        if (const auto instrument = m_editorService->instrument(m_editorService->position().track); instrument) {
            juzzlin::L(TAG).debug() << "Live note ON " << NoteConverter::midiToString(data.note()) << " requested on instrument " << instrument->toString().toStdString();
            m_midiService->playNote(instrument, data);
        } else {
            juzzlin::L(TAG).info() << "No instrument set on track!";
        }
    });

    // Note insertion via a MIDI controller
    connect(m_midiService.get(), &MidiService::noteOnReceived, this, [this](const auto &, const auto & data) {
        if (m_applicationService->editMode() && m_editorService->isAtNoteColumn()) {
            const auto [key, octave] = NoteConverter::midiToKeyAndOctave(data.note());
            if (m_editorService->requestNoteOnAtCurrentPosition(key, octave, m_settingsService->velocity(100))) {
                if (!m_playerService->isPlaying()) {
                    m_editorService->requestScroll(m_settingsService->step(1));
                }
            }
        }
    });

    // Stop a note via a MIDI controller
    connect(m_midiService.get(), &MidiService::noteOffReceived, this, [this](const auto &, const auto & data) {
        if (const auto instrument = m_editorService->instrument(m_editorService->position().track); instrument) {
            juzzlin::L(TAG).debug() << "Live note OFF " << NoteConverter::midiToString(data.note()) << " requested on instrument " << instrument->toString().toStdString();
            m_midiService->stopNote(instrument, data);
        } else {
            juzzlin::L(TAG).info() << "No instrument set on track!";
        }
    });

    // Apply pitch bend via a MIDI controller
    connect(m_midiService.get(), &MidiService::pitchBendReceived, this, [this](const auto &, const auto & value) {
        const auto track = m_editorService->position().track;
        const auto column = m_editorService->position().column;
        if (const auto instrument = m_editorService->instrument(track); instrument) {
            juzzlin::L(TAG).debug() << "Pitch Bend " << value << " requested on instrument " << instrument->toString().toStdString();
            m_midiService->sendPitchBendData(instrument, { track, column, value });
        } else {
            juzzlin::L(TAG).info() << "No instrument set on track!";
        }
    });

    // Apply MIDI CC via a MIDI controller
    connect(m_midiService.get(), &MidiService::controlChangeReceived, this, [this](const auto &, const auto & controller, const auto & value) {
        const auto track = m_editorService->position().track;
        const auto column = m_editorService->position().column;
        if (const auto instrument = m_editorService->instrument(track); instrument) {
            juzzlin::L(TAG).debug() << "MIDI CC value " << std::hex << std::setfill('0') << value << " on controller " << std::hex << std::setfill('0') << controller << " requested on instrument " << instrument->toString().toStdString();
            m_midiService->sendCcData(instrument, { track, column, controller, value });
        } else {
            juzzlin::L(TAG).info() << "No instrument set on track!";
        }
    });

    connect(m_midiService.get(), &MidiService::continueReceived, m_playerService.get(), &PlayerService::play);
    connect(m_midiService.get(), &MidiService::startReceived, m_playerService.get(), &PlayerService::play);
    connect(m_midiService.get(), &MidiService::stopReceived, m_playerService.get(), &PlayerService::stop);

    connect(m_midiService.get(), &MidiService::dataReceived, m_midiSettingsModel.get(), &MidiSettingsModel::setDebugData);
}

void Application::connectMidiSettingsModel()
{
    connect(m_midiSettingsModel.get(), &MidiSettingsModel::controllerPortChanged, m_midiService.get(), &MidiService::setControllerPort);
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
        const auto isPlaying = m_playerService->isPlaying();
        m_midiService->setIsPlaying(isPlaying);
        if (m_settingsService->recordingEnabled()) {
            applyAudioRecording(isPlaying, m_playerService->tick());
        }
    });

    connect(m_jackService.get(), &JackService::playRequested, m_playerService.get(), &PlayerService::play, Qt::QueuedConnection);
    connect(m_jackService.get(), &JackService::stopRequested, m_playerService.get(), &PlayerService::stop, Qt::QueuedConnection);
}

static QString sanitizeFileName(const QString & name)
{
    // Illegal filename characters (Windows safe set + space)
    static const std::regex re(R"([\\\/:*?"<>|. ])");
    return std::regex_replace(name.toStdString(), re, "_").c_str();
}

QString Application::buildAudioFileName() const
{
    if (const auto projectFileName = m_editorService->currentFileName(); !projectFileName.isEmpty()) {
        QStringList activeTrackAndSoloedColumnNames;
        for (auto trackIndex : m_editorService->trackIndices()) {
            if (m_mixerService->shouldTrackPlay(trackIndex)) {
                QStringList soloedColumnNames;
                if (m_mixerService->hasSoloedColumns(trackIndex)) {
                    for (quint64 columnIndex = 0; columnIndex < m_editorService->columnCount(trackIndex); columnIndex++) {
                        if (m_mixerService->isColumnSoloed(trackIndex, columnIndex)) {
                            if (const auto & columnName = m_editorService->columnName(trackIndex, columnIndex); !columnName.isEmpty()) {
                                soloedColumnNames << columnName;
                            }
                        }
                    }
                }

                auto trackName = m_editorService->trackName(trackIndex);
                if (!soloedColumnNames.isEmpty()) {
                    trackName += "_" + soloedColumnNames.join("_");
                }
                activeTrackAndSoloedColumnNames << trackName;
            }
        }
        const auto date = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
        return QString { "%1_%2_%3.wav" }.arg(projectFileName, sanitizeFileName(activeTrackAndSoloedColumnNames.join("_")), date);
    }

    return {};
}

void Application::applyAudioRecording(bool isPlaying, quint64 startTick)
{
    if (isPlaying) {
        if (const auto audioFileName = buildAudioFileName(); !audioFileName.isEmpty()) {
            juzzlin::L(TAG).info() << "Recording audio to " << std::quoted(audioFileName.toStdString());
            m_audioService->startRecording(audioFileName, static_cast<uint32_t>(m_settingsService->audioBufferSize()), startTick);
        } else {
            juzzlin::L(TAG).error() << "Output audio filename is empty!";
        }
    } else {
        m_audioService->stopRecording(startTick);
    }
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
        const auto scaledVelocity = Utils::Midi::scaleVelocityByKey(velocity, 60, m_trackSettingsModel->velocityKeyTrack(), m_trackSettingsModel->velocityKeyTrackOffset());
        m_midiService->playAndStopMiddleC(m_trackSettingsModel->portName(), m_trackSettingsModel->channel(), scaledVelocity);
    });

    connect(m_trackSettingsModel.get(), &TrackSettingsModel::noteOnRequested, this, [this](uint8_t note, uint8_t velocity) {
        if (auto instrument = std::shared_ptr<Instrument> { m_trackSettingsModel->toInstrument() }; instrument) {
            const auto scaledVelocity = Utils::Midi::scaleVelocityByKey(velocity, note, instrument->settings().midiEffects.velocityKeyTrack, instrument->settings().midiEffects.velocityKeyTrackOffset);
            m_midiService->playNote(instrument, { note, scaledVelocity });
        }
    });

    connect(m_trackSettingsModel.get(), &TrackSettingsModel::noteOffRequested, this, [this](uint8_t note) {
        if (auto instrument = std::shared_ptr<Instrument> { m_trackSettingsModel->toInstrument() }; instrument) {
            m_midiService->stopNote(instrument, { note, 0 });
        }
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
    videoGenerator.initialize(m_videoConfig, m_editorService->song());
    videoGenerator.run();

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
        for (auto && [trackIndex, instrument] : m_editorService->instruments()) {
            if (Utils::Midi::portNameMatchScore(instrument->midiAddress().portName().toStdString(), midiPort.toStdString()) > 0.75) {
                applyInstrument(trackIndex, *instrument);
            }
        }
    }
}

void Application::stopAllNotes() const
{
    juzzlin::L(TAG).info() << "Stopping all notes";

    for (auto && [trackIndex, instrument] : m_editorService->instruments()) {
        m_midiService->stopAllNotes(instrument);
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

void Application::applyMidiController()
{
    m_midiService->setMidiSyncEnabled(m_settingsService->midiSyncEnabled());

    if (!m_settingsService->controllerPort().isEmpty()) {
        juzzlin::L(TAG).info() << "Applying MIDI controller: " << m_settingsService->controllerPort().toStdString();
        m_midiService->setControllerPort(m_settingsService->controllerPort());
    }
}

void Application::applyAllInstruments()
{
    juzzlin::L(TAG).info() << "Applying all instruments";

    for (auto && [trackIndex, instrument] : m_editorService->instruments()) {
        applyInstrument(trackIndex, *instrument);
    }

    m_instrumentTimer = std::make_unique<QTimer>();
    m_instrumentTimer->setSingleShot(true);
    m_instrumentTimer->setInterval(1000);
    connect(m_instrumentTimer.get(), &QTimer::timeout, this, &Application::applyAllMidiCcSettings);
    m_instrumentTimer->start();
}

void Application::applyAllMidiCcSettings()
{
    for (auto && [trackIndex, instrument] : m_editorService->instruments()) {
        applyMidiCcSettings(*instrument);
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
            const auto filePath = m_recentFilesManager->selectedFile();
            if (m_applicationService->isMidiFile(filePath)) {
                m_stateMachine->calculateState(StateMachine::Action::MidiImportRequested);
            } else {
                m_editorService->load(filePath);
                m_recentFilesManager->addRecentFile(filePath); // Moves the loaded file to top
                m_stateMachine->calculateState(StateMachine::Action::ProjectOpened);
            }
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
    case StateMachine::State::ShowMidiImportDialog:
        m_applicationService->requestMidiImportDialog();
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

Application::~Application()
{
    juzzlin::SimpleLogger::flush();
}

} // namespace noteahead

