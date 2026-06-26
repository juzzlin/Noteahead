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

#include "../infra/midi/export/midi_exporter.hpp"
#include "../infra/midi/import/midi_importer.hpp"
#include "state_machine.hpp"

#include <memory>
#include <optional>

#include <QObject>

class QGuiApplication;
class QQmlApplicationEngine;
class QTimer;

namespace juzzlin {
class Argengine;
}

namespace noteahead {

class ApplicationService;
class AudioEngine;
class AudioService;
class AudioSettingsModel;
class AutomationService;
class ColumnSettingsModel;
class DataService;
class DeviceRackController;
class DeviceService;
class BassSynthController;
class DrumSynthController;
class PianoSynthController;
class EditorService;
class EffectRackController;
class EventSelectionModel;
class Instrument;
class JackService;
class KeyboardService;
class KnobController;
class MidiCcAutomationsModel;
class MidiExporter;
class MidiImporter;
class MidiService;
class MidiSettingsModel;
class MixerService;
class NoteColumnLineContainerHelper;
class NoteColumnModelHandler;
class PitchBendAutomationsModel;
class PlayerService;
class PropertyService;
class RecentFilesManager;
class RecentFilesModel;
class RenderService;
class SamplerController;
class SelectionService;
class SettingsService;
class SideChainService;
class SynthController;
class ThemeService;
class TrackSettingsModel;
class UiLogger;
class UtilService;
class WavetableSynthController;

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
    void applyAudioRecording(bool isPlaying, quint64 startTick);
    QString buildAudioFileName() const;
    void applyInstrument(size_t trackIndex, const Instrument & instrument);
    void applyMidiCcSettings(const Instrument & instrument);
    void applyMidiController();
    void applyState(StateMachine::State state);

    void connectServices();
    void connectApplicationService();
    void connectDeviceService();
    void connectAudioService();
    void connectAutomationService();
    void connectEditorService();
    void connectJackService();
    void connectMidiCcAutomationsModel();
    void connectMidiService();
    void connectMidiSettingsModel();
    void connectMixerService();
    void connectRenderService();
    void connectPlayerService();
    void connectStateMachine();
    void connectEventSelectionModel();
    void connectPitchBendAutomationsModel();
    void connectTrackSettingsModel();
    void connectColumnSettingsModel();

    void handleCommandLineArguments(int & argc, char ** argv);

    int initialize();
    void initializeApplicationEngine();
    int initializeTracker();

    void registerTypes();
    void setContextProperties();

    void requestInstruments(QStringList midiPorts);
    void stopAllNotes() const;

    void exportToMidi(QString fileName, quint64 startPosition, quint64 endPosition, MidiExportOptions options);
    void importFromMidi(QString fileName, MidiImportMode importMode, int patternLength, bool quantizeNoteOn, bool quantizeNoteOff, bool connectMidiPorts);

    std::unique_ptr<UiLogger> m_uiLogger;

    std::unique_ptr<QGuiApplication> m_application;
    std::shared_ptr<ApplicationService> m_applicationService;

    std::shared_ptr<SettingsService> m_settingsService;
    std::shared_ptr<ThemeService> m_themeService;

    std::shared_ptr<SelectionService> m_selectionService;

    std::shared_ptr<UtilService> m_utilService;
    std::shared_ptr<PropertyService> m_propertyService;
    std::shared_ptr<AutomationService> m_automationService;
    std::shared_ptr<DataService> m_dataService;

    std::shared_ptr<EditorService> m_editorService;

    std::shared_ptr<AudioEngine> m_audioEngine;
    std::shared_ptr<DeviceService> m_deviceService;
    std::shared_ptr<SamplerController> m_samplerController;
    std::shared_ptr<SynthController> m_synthController;
    std::shared_ptr<WavetableSynthController> m_wavetableSynthController;
    std::shared_ptr<BassSynthController> m_bassSynthController;
    std::shared_ptr<DrumSynthController> m_drumSynthController;
    std::shared_ptr<PianoSynthController> m_pianoSynthController;
    std::shared_ptr<EffectRackController> m_effectRackController;
    std::shared_ptr<DeviceRackController> m_deviceRackController;
    std::shared_ptr<KnobController> m_knobController;

    std::shared_ptr<JackService> m_jackService;

    std::shared_ptr<AudioService> m_audioService;

    std::shared_ptr<EventSelectionModel> m_eventSelectionModel;

    std::shared_ptr<MidiService> m_midiService;
    std::shared_ptr<MixerService> m_mixerService;
    std::shared_ptr<SideChainService> m_sideChainService;
    std::shared_ptr<PlayerService> m_playerService;
    std::shared_ptr<KeyboardService> m_keyboardService;

    std::shared_ptr<MidiExporter> m_midiExporter;
    std::shared_ptr<MidiImporter> m_midiImporter;

    std::shared_ptr<StateMachine> m_stateMachine;

    std::shared_ptr<RecentFilesManager> m_recentFilesManager;
    std::unique_ptr<RecentFilesModel> m_recentFilesModel;

    std::shared_ptr<RenderService> m_renderService;

    std::unique_ptr<MidiCcAutomationsModel> m_midiCcAutomationsModel;
    std::unique_ptr<PitchBendAutomationsModel> m_pitchBendAutomationsModel;
    std::unique_ptr<ColumnSettingsModel> m_columnSettingsModel;
    std::unique_ptr<TrackSettingsModel> m_trackSettingsModel;
    std::unique_ptr<MidiSettingsModel> m_midiSettingsModel;
    std::unique_ptr<AudioSettingsModel> m_audioSettingsModel;

    std::shared_ptr<NoteColumnLineContainerHelper> m_noteColumnLineContainerHelper;
    std::unique_ptr<NoteColumnModelHandler> m_noteColumnModelHandler;

    std::unique_ptr<QQmlApplicationEngine> m_engine;

    std::unique_ptr<QTimer> m_instrumentTimer;

    std::shared_ptr<Instrument> m_livePositionNoteInstrument;
    std::optional<uint8_t> m_livePositionNote;
};

} // namespace noteahead

#endif // APPLICATION_HPP
