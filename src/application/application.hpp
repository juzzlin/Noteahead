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

#include "../infra/video/video_config.hpp"
#include "state_machine.hpp"

#include <memory>

#include <QObject>

class QGuiApplication;
class QQmlApplicationEngine;
class QTimer;

namespace juzzlin {
class Argengine;
}

namespace noteahead {

class ApplicationService;
class AudioService;
class AutomationService;
class EditorService;
class EventSelectionModel;
class Instrument;
class MidiCcAutomationsModel;
class MidiExporter;
class MidiService;
class MidiSettingsModel;
class MixerService;
class NoteColumnLineContainerHelper;
class NoteColumnModelHandler;
class PitchBendAutomationsModel;
class PlayerService;
class RecentFilesManager;
class RecentFilesModel;
class SelectionService;
class SettingsService;
class TrackSettingsModel;
class UiLogger;
class UtilService;

class Application : public QObject
{
    Q_OBJECT

public:
    Application(int & argc, char ** argv);
    ~Application() override;

    int run();

private:
    void applyAllInstruments();
    void applyAllMidiCcSettings();
    void applyAudioRecording(bool isPlaying);
    QString buildAudioFileName() const;
    void applyInstrument(size_t trackIndex, const Instrument & instrument);
    void applyMidiCcSettings(const Instrument & instrument);
    void applyMidiController();
    void applyState(StateMachine::State state);

    void connectServices();
    void connectApplicationService();
    void connectAutomationService();
    void connectEditorService();
    void connectMidiCcAutomationsModel();
    void connectMidiService();
    void connectMidiSettingsModel();
    void connectMixerService();
    void connectPlayerService();
    void connectStateMachine();
    void connectEventSelectionModel();
    void connectPitchBendAutomationsModel();
    void connectTrackSettingsModel();

    void addVideoOptions(juzzlin::Argengine & ae);
    void handleCommandLineArguments(int & argc, char ** argv);

    int initialize();
    void initializeApplicationEngine();
    int initializeTracker();
    int runVideoGenerator();

    void registerTypes();
    void setContextProperties();

    void requestInstruments(QStringList midiPorts);
    void stopAllNotes() const;

    void exportToMidi(QString fileName);

    std::unique_ptr<UiLogger> m_uiLogger;

    std::unique_ptr<QGuiApplication> m_application;
    std::shared_ptr<ApplicationService> m_applicationService;

    std::unique_ptr<AudioService> m_audioService;

    std::shared_ptr<AutomationService> m_automationService;

    std::shared_ptr<SettingsService> m_settingsService;

    std::shared_ptr<SelectionService> m_selectionService;
    std::shared_ptr<EditorService> m_editorService;

    std::shared_ptr<EventSelectionModel> m_eventSelectionModel;

    std::shared_ptr<MidiExporter> m_midiExporter;
    std::shared_ptr<MidiService> m_midiService;
    std::shared_ptr<MixerService> m_mixerService;
    std::shared_ptr<PlayerService> m_playerService;

    std::shared_ptr<StateMachine> m_stateMachine;

    std::shared_ptr<RecentFilesManager> m_recentFilesManager;
    std::unique_ptr<RecentFilesModel> m_recentFilesModel;

    std::unique_ptr<MidiCcAutomationsModel> m_midiCcAutomationsModel;
    std::unique_ptr<PitchBendAutomationsModel> m_pitchBendAutomationsModel;
    std::unique_ptr<TrackSettingsModel> m_trackSettingsModel;
    std::unique_ptr<MidiSettingsModel> m_midiSettingsModel;

    std::shared_ptr<UtilService> m_utilService;

    std::shared_ptr<NoteColumnLineContainerHelper> m_noteColumnLineContainerHelper;
    std::unique_ptr<NoteColumnModelHandler> m_noteColumnModelHandler;

    std::unique_ptr<QQmlApplicationEngine> m_engine;

    bool m_videoGeneratorEnabled = false;

    VideoConfig m_videoConfig;

    std::unique_ptr<QTimer> m_instrumentTimer;
};

} // namespace noteahead

#endif // APPLICATION_HPP
