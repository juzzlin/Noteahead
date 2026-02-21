// This file is part of Noteahead.
// Copyright (C) 2024 Jussi Lind <jussi.lind@iki.fi>
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

#ifndef APPLICATION_SERVICE_HPP
#define APPLICATION_SERVICE_HPP

#include <QObject>
#include <QTimer>
#include <QUrl>
#include <QVariantMap>

#include <memory>

namespace noteahead {

class EditorService;
class Instrument;
class MidiNoteData;
class PlayerService;
class RecentFilesManager;
class StateMachine;

class ApplicationService : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool editMode READ editMode WRITE setEditMode NOTIFY editModeChanged)

public:
    ApplicationService();

    ~ApplicationService() override;

    Q_INVOKABLE QString applicationName() const;
    Q_INVOKABLE QString applicationVersion() const;
    Q_INVOKABLE QString copyright() const;
    Q_INVOKABLE QString fileFormatExtension() const;
    Q_INVOKABLE QString midiFileExtension() const;
    Q_INVOKABLE QString license() const;
    Q_INVOKABLE QString webSiteUrl() const;

    Q_INVOKABLE QStringList recentFiles() const;
    Q_INVOKABLE QString lastImportDirectory() const;
    Q_INVOKABLE void setLastImportDirectory(QString directory);

    Q_INVOKABLE void acceptUnsavedChangesDialog();

    Q_INVOKABLE void applyAllTrackSettings();

    Q_INVOKABLE void cancelOpenProject();
    Q_INVOKABLE void cancelRecentFileDialog();
    Q_INVOKABLE void cancelSaveProjectAs();

    Q_INVOKABLE void discardUnsavedChangesDialog();

    Q_INVOKABLE void openProject(QUrl url);
    Q_INVOKABLE void openRecentProject(QString filePath);

    Q_INVOKABLE void rejectUnsavedChangesDialog();

    Q_INVOKABLE void requestAllNotesOff();
    Q_INVOKABLE virtual void requestLiveNoteOff(quint8 key, quint8 octave);
    Q_INVOKABLE virtual void requestLiveNoteOn(quint8 key, quint8 octave, quint8 velocity);
    Q_INVOKABLE virtual void requestLiveNoteOnAtCurrentPosition();

    Q_INVOKABLE void requestInstrumentReset();

    Q_INVOKABLE void requestNewProject();
    Q_INVOKABLE void requestOpenProject();
    Q_INVOKABLE void requestQuit();

    Q_INVOKABLE void requestRecentFilesDialog();
    Q_INVOKABLE void requestSaveProject();
    Q_INVOKABLE void requestSaveProjectAs();
    Q_INVOKABLE void requestSaveProjectAsTemplate();
    Q_INVOKABLE void saveProjectAs(QUrl url);
    Q_INVOKABLE void saveProjectAsTemplate(QUrl url);

    Q_INVOKABLE void requestMidiExportDialog();
    Q_INVOKABLE void exportMidiFile(QUrl url, quint64 startPosition, quint64 endPosition);

    Q_INVOKABLE void requestMidiImportDialog();
    Q_INVOKABLE void importMidiFile(QUrl url, int importMode, int patternLength, bool quantizeNoteOn, bool quantizeNoteOff);

    Q_INVOKABLE virtual bool editMode() const;
    Q_INVOKABLE virtual void setEditMode(bool editMode);
    Q_INVOKABLE virtual void toggleEditMode();

    void requestUnsavedChangesDialog();
    Q_INVOKABLE void requestOpenDialog();
    Q_INVOKABLE void requestSaveAsDialog();
    void requestSaveAsTemplateDialog();

    void requestAlertDialog(QString text);
    void requestStatusText(QString text);

    using RecentFilesManagerS = std::shared_ptr<RecentFilesManager>;
    void setRecentFilesManager(RecentFilesManagerS recentFilesManager);

    using StateMachineS = std::shared_ptr<StateMachine>;
    void setStateMachine(StateMachineS stateMachine);

    using EditorServiceS = std::shared_ptr<EditorService>;
    void setEditorService(EditorServiceS editorService);

    using PlayerServiceS = std::shared_ptr<PlayerService>;
    void setPlayerService(PlayerServiceS playerService);

    using InstrumentS = std::shared_ptr<Instrument>;

    using MidiNoteDataCR = const MidiNoteData &;

signals:
    void editModeChanged(bool editMode);

    void applyAllTrackSettingsRequested();

    void allNotesOffRequested();
    void liveNoteOnAtCurrentPositionRequested(InstrumentS instrument);
    void liveNoteOnRequested(InstrumentS instrument, MidiNoteDataCR data);
    void liveNoteOffRequested(InstrumentS instrument, MidiNoteDataCR data);

    void unsavedChangesDialogRequested();
    void openDialogRequested();

    void quitRequested();

    void recentFilesDialogRequested();

    void saveAsDialogRequested();
    void saveAsTemplateDialogRequested();
    void midiExportDialogRequested();
    void midiExportRequested(QString fileName, quint64 startPosition, quint64 endPosition);

    void midiImportDialogRequested();
    void midiImportRequested(QString fileName, int importMode, int patternLength, bool quantizeNoteOn, bool quantizeNoteOff);

    void alertDialogRequested(QString message);
    void statusTextRequested(QString message);

private:
    RecentFilesManagerS m_recentFilesManager;

    StateMachineS m_stateMachine;

    EditorServiceS m_editorService;

    PlayerServiceS m_playerService;

    bool m_editMode = false;
};

} // namespace noteahead

#endif // APPLICATION_SERVICE_HPP
