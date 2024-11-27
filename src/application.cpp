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
#include "config.hpp"
#include "midi_service_rt_midi.hpp" // Include the MidiService header

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include <iostream>
#include <thread>

namespace cacophony {

Application::Application(int & argc, char ** argv)
  : m_application(std::make_unique<QGuiApplication>(argc, argv))
  , m_engine(std::make_unique<QQmlApplicationEngine>())
  , m_config(std::make_unique<Config>())
  , m_midiService(std::make_unique<MidiServiceRtMidi>()) // Initialize MidiService
{
    qmlRegisterType<Config>("Cacophony", 1, 0, "Config");

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
    for (auto && midiDevice : m_midiService->listDevices()) {
        std::cout << midiDevice.toString() << std::endl;
    }
    m_application->exit(); // Exit after listing devices
}

void Application::testDevice()
{
    if (m_midiService->openDevice(*m_testDeviceIndex)) {
        std::cout << "Playing middle C on device index " << *m_testDeviceIndex << " on channel " << *m_testDeviceChannel << std::endl;
        m_midiService->sendNoteOn(*m_testDeviceChannel, 60, 100); // Middle C
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        m_midiService->sendNoteOff(*m_testDeviceChannel, 60, 100); // Stop Middle C
    } else {
        std::cerr << "Failed to open MIDI device at index " << *m_testDeviceIndex << "\n";
    }
    m_application->exit(); // Exit after testing device
}

int Application::run()
{
    // If --list-devices was set, we should exit, so skip loading QML
    if (m_listDevices || m_testDeviceIndex.has_value()) {
        return EXIT_SUCCESS;
    }

    m_engine->rootContext()->setContextProperty("config", m_config.get());
    m_engine->load(QML_ENTRY_POINT);
    if (m_engine->rootObjects().isEmpty()) {
        return EXIT_FAILURE;
    }

    return m_application->exec();
}

Application::~Application() = default;

} // namespace cacophony
