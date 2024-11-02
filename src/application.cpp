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
#include "midi_service.hpp" // Include the MidiService header

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <cstring>
#include <iostream>

namespace cacophony {

Application::Application(int & argc, char ** argv)
  : m_application(std::make_unique<QGuiApplication>(argc, argv))
  , m_engine(std::make_unique<QQmlApplicationEngine>())
  , m_config(std::make_unique<Config>())
  , m_midiService(std::make_unique<MidiService>()) // Initialize MidiService
{
    qmlRegisterType<Config>("Cacophony", 1, 0, "Config");

    handleCommandLineArguments(); // Handle command-line arguments at initialization
}

void Application::handleCommandLineArguments()
{
    // Parse command-line arguments for --list-devices
    for (int i = 1; i < m_application->arguments().size(); ++i) {
        if (strcmp(m_application->arguments().at(i).toStdString().c_str(), "--list-devices") == 0) {
            m_listDevices = true;
            break;
        }
    }

    // If --list-devices is set, list devices and exit
    if (m_listDevices) {
        std::cout << "Listing available MIDI devices:\n";
        m_midiService->listDevices();
        m_application->exit(); // Exit after listing devices
    }
}

int Application::run()
{
    // If --list-devices was set, we should exit, so skip loading QML
    if (m_listDevices) {
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
