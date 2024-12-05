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

#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include <memory>
#include <optional>

class QGuiApplication;
class QQmlApplicationEngine;

namespace cacophony {

class ApplicationService;
class Config;
class Editor;
class MidiService; // Forward declaration of MidiService

class Application
{
public:
    Application(int & argc, char ** argv);

    ~Application();

    int run();

private:
    void handleCommandLineArguments();

    void initialize();

    void initializeApplicationEngine();

    void initializeEditor();

    void listDevices();

    void setContextProperties();

    void testDevice();

    std::unique_ptr<QGuiApplication> m_application;

    std::unique_ptr<QQmlApplicationEngine> m_engine;

    std::unique_ptr<ApplicationService> m_applicationService;

    std::unique_ptr<Config> m_config;

    std::unique_ptr<Editor> m_editor;

    std::unique_ptr<MidiService> m_midiService; // MidiService instance for --list-devices

    bool m_listDevices = false; // Flag for --list-devices

    std::optional<unsigned int> m_testDeviceIndex;

    std::optional<unsigned int> m_testDeviceChannel;
};

} // namespace cacophony

#endif // APPLICATION_HPP
