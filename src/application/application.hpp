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

#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include "state_machine.hpp"

#include <memory>

#include <QObject>

class QGuiApplication;
class QQmlApplicationEngine;

namespace noteahead {

class ApplicationService;
class Config;
class EditorService;
class MidiService;
class MixerService;
class PlayerService;
class RecentFilesManager;
class RecentFilesModel;
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

    std::shared_ptr<ApplicationService> m_applicationService;

    std::shared_ptr<Config> m_config;

    std::shared_ptr<EditorService> m_editorService;

    std::shared_ptr<MidiService> m_midiService;

    std::shared_ptr<MixerService> m_mixerService;

    std::shared_ptr<PlayerService> m_playerService;

    std::shared_ptr<StateMachine> m_stateMachine;

    std::shared_ptr<RecentFilesManager> m_recentFilesManager;

    std::unique_ptr<RecentFilesModel> m_recentFilesModel;

    std::unique_ptr<TrackSettingsModel> m_trackSettingsModel;

    std::unique_ptr<QQmlApplicationEngine> m_engine;
};

} // namespace noteahead

#endif // APPLICATION_HPP
