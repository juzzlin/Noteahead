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

#include "../infra/midi_service_rt_midi.hpp" // Include the MidiService header
#include "application_service.hpp"
#include "config.hpp"
#include "editor_service.hpp"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include <iostream>
#include <thread>

namespace cacophony {

Application::Application(int & argc, char ** argv)
  : m_application(std::make_unique<QGuiApplication>(argc, argv))
  , m_engine(std::make_unique<QQmlApplicationEngine>())
  , m_applicationService(std::make_unique<ApplicationService>())
  , m_config(std::make_unique<Config>())
  , m_editorService(std::make_unique<EditorService>())
  , m_midiService(std::make_unique<MidiServiceRtMidi>()) // Initialize MidiService
{
    qmlRegisterType<Config>("Cacophony", 1, 0, "ApplicationService");
    qmlRegisterType<Config>("Cacophony", 1, 0, "Config");
    qmlRegisterType<Config>("Cacophony", 1, 0, "Editor");

    handleCommandLineArguments(); // Handle command-line arguments at initialization
}

void Application::handleCommandLineArguments()
{
    // Parse command-line arguments for --list-devices and --test-device
    QStringList arguments = m_application->arguments();
    for (int i = 1; i < arguments.size(); ++i) {
        if (arguments.at(i) == "--list-devices") {
            m_listDevices = true;
        } else if (arguments.at(i) == "--test-device" && i + 1 < arguments.size()) {
            m_testDeviceIndex = arguments.at(i + 1).toInt();
            ++i; // Skip the index argument
        } else if (arguments.at(i) == "--test-channel" && i + 1 < arguments.size()) {
            m_testDeviceChannel = arguments.at(i + 1).toInt();
            ++i; // Skip the index argument
        }
    }

    // Handle --list-devices option
    if (m_listDevices) {
        listDevices();
    }

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
    m_engine->rootContext()->setContextProperty("editor", m_editorService.get());
}

void Application::initialize()
{
    initializeApplicationEngine();

    initializeEditorService();
}

void Application::initializeApplicationEngine()
{
    setContextProperties();

    m_engine->load(QML_ENTRY_POINT);
    if (m_engine->rootObjects().isEmpty()) {
        throw std::runtime_error("Failed to initialize QML application engine!");
    }
}

void Application::initializeEditorService()
{
    m_editorService->initialize();
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

Application::~Application() = default;

} // namespace cacophony
