// This file is part of Cacophony.
// Copyright (C) 2020 Jussi Lind <jussi.lind@iki.fi>
//
// Cacophony is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// Cacophony is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Cacophony. If not, see <http://www.gnu.org/licenses/>.

#include "application.hpp"

#include "../contrib/Argengine/src/argengine.hpp"
#include "../contrib/SimpleLogger/src/simple_logger.hpp"
#include "../infra/midi_service_rt_midi.hpp" // Include the MidiService header
#include "application_service.hpp"
#include "config.hpp"
#include "editor_service.hpp"
#include "player_service.hpp"
#include "state_machine.hpp"
#include "ui_logger.hpp"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include <iostream>
#include <thread>

namespace cacophony {

static const auto TAG = "Application";

Application::Application(int & argc, char ** argv)
  : m_uiLogger(std::make_unique<UiLogger>())
  , m_application(std::make_unique<QGuiApplication>(argc, argv))
  , m_applicationService(std::make_unique<ApplicationService>())
  , m_editorService(std::make_unique<EditorService>())
  , m_playerService(std::make_unique<PlayerService>())
  , m_stateMachine(std::make_unique<StateMachine>(m_editorService))
  , m_config(std::make_unique<Config>())
  , m_engine(std::make_unique<QQmlApplicationEngine>())
  , m_midiService(std::make_unique<MidiServiceRtMidi>())
{    
    qmlRegisterType<UiLogger>("Cacophony", 1, 0, "UiLogger");
    qmlRegisterType<ApplicationService>("Cacophony", 1, 0, "ApplicationService");
    qmlRegisterType<Config>("Cacophony", 1, 0, "Config");
    qmlRegisterType<EditorService>("Cacophony", 1, 0, "EditorService");

    qmlRegisterSingletonType(QUrl(QML_ROOT_DIR + QString { "/Constants.qml" }), "Cacophony", 1, 0, "Constants");
    qmlRegisterSingletonType(QUrl(QML_ROOT_DIR + QString { "/UiService.qml" }), "Cacophony", 1, 0, "UiService");

    handleCommandLineArguments(argc, argv); // Handle command-line arguments at initialization

    m_applicationService->setStateMachine(m_stateMachine);
    m_applicationService->setEditorService(m_editorService);
    m_applicationService->setMidiService(m_midiService);
    m_applicationService->setPlayerService(m_playerService);
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

    ae.addOption({ "--list-devices" }, [this] {
        listDevices();
    });

    ae.addOption({ "--test-device" }, [this](std::string argument) {
        m_testDeviceIndex = std::stoi(argument);
    });

    ae.addOption({ "--test-channel" }, [this](std::string argument) {
        m_testDeviceChannel = std::stoi(argument);
    });

    ae.parse();

    // Handle --test-device option
    if (m_testDeviceIndex.has_value() && m_testDeviceChannel.has_value()) {
        testDevice();
    }
}

void Application::listDevices()
{
    m_midiService->updateAvailableDevices();

    for (auto && midiDevice : m_midiService->listDevices()) {
        std::cout << midiDevice->toString() << std::endl;
    }
    m_application->exit(); // Exit after listing devices
}

void Application::testDevice()
{
    m_midiService->updateAvailableDevices();

    if (const auto device = m_midiService->deviceByPortIndex(*m_testDeviceIndex); device) {
        if (m_midiService->openDevice(device)) {
            std::cout << "Playing middle C on device index " << *m_testDeviceIndex << " on channel " << *m_testDeviceChannel << std::endl;
            m_midiService->sendNoteOn(device, *m_testDeviceChannel, 60, 100); // Middle C
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            m_midiService->sendNoteOff(device, *m_testDeviceChannel, 60, 100); // Stop Middle C
        } else {
            std::cerr << "Failed to open MIDI device at index " << *m_testDeviceIndex << std::endl;
        }
    } else {
        std::cerr << "No MIDI device at index " << *m_testDeviceIndex << std::endl;
    }

    m_application->exit(); // Exit after testing device
}

void Application::setContextProperties()
{
    m_engine->rootContext()->setContextProperty("applicationService", m_applicationService.get());
    m_engine->rootContext()->setContextProperty("config", m_config.get());
    m_engine->rootContext()->setContextProperty("editorService", m_editorService.get());
    m_engine->rootContext()->setContextProperty("playerService", m_playerService.get());
    m_engine->rootContext()->setContextProperty("uiLogger", m_uiLogger.get());
}

void Application::connectServices()
{
    connect(m_playerService.get(), &PlayerService::songRequested, this, [this]{
        m_playerService->setSong(m_editorService->song());
    });
    connect(m_playerService.get(), &PlayerService::tickUpdated, m_editorService.get(), &EditorService::requestPositionByTick);

    connect(m_stateMachine.get(), &StateMachine::stateChanged, this, &Application::applyState);
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
    // If --list-devices was set, we should exit, so skip loading QML
    if (m_listDevices || m_testDeviceIndex.has_value()) {
        return EXIT_SUCCESS;
    }

    initialize();

    return m_application->exec();
}

void Application::applyState(StateMachine::State state)
{
    juzzlin::L(TAG).info() << "Applying state: " << StateMachine::stateToString(state).toStdString();

    switch (state) {
    case StateMachine::State::Exit:
        m_application->exit(EXIT_SUCCESS);
        break;
    case StateMachine::State::InitializeNewProject:
        m_editorService->initialize();
        break;
    case StateMachine::State::Save:
        m_editorService->save();
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

} // namespace cacophony
