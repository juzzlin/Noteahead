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

#include "state_machine.hpp"

#include <memory>

#include <QObject>

class QGuiApplication;
class QQmlApplicationEngine;

namespace cacophony {

class ApplicationService;
class Config;
class EditorService;
class MidiService;
class PlayerService;
class TrackSettingsModel;
class UiLogger;

class Application : public QObject
{
    Q_OBJECT

public:
    Application(int & argc, char ** argv);

    ~Application() override;

    int run();

private:
    void applyState(StateMachine::State state);

    void connectServices();

    void handleCommandLineArguments(int & argc, char ** argv);

    void initialize();

    void initializeApplicationEngine();

    void setContextProperties();

    std::unique_ptr<UiLogger> m_uiLogger;

    std::unique_ptr<QGuiApplication> m_application;

    std::unique_ptr<ApplicationService> m_applicationService;

    std::shared_ptr<EditorService> m_editorService;

    std::shared_ptr<MidiService> m_midiService;

    std::shared_ptr<PlayerService> m_playerService;

    std::shared_ptr<StateMachine> m_stateMachine;

    std::unique_ptr<TrackSettingsModel> m_trackSettingsModel;

    std::unique_ptr<Config> m_config;

    std::unique_ptr<QQmlApplicationEngine> m_engine;
};

} // namespace cacophony

#endif // APPLICATION_HPP
