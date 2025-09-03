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

#include "application_service.hpp"

#include "../../common/constants.hpp"
#include "../../contrib/SimpleLogger/src/simple_logger.hpp"
#include "../../domain/instrument.hpp"
#include "../../domain/midi_note_data.hpp"
#include "../note_converter.hpp"
#include "../state_machine.hpp"
#include "editor_service.hpp"
#include "player_service.hpp"
#include "recent_files_manager.hpp"

namespace noteahead {

static const auto TAG = "ApplicationService";

using namespace std::chrono_literals;

ApplicationService::ApplicationService()
{
}

QString ApplicationService::applicationName() const
{
    return Constants::applicationName();
}

QString ApplicationService::applicationVersion() const
{
    return Constants::applicationVersion();
}

QString ApplicationService::copyright() const
{
    return Constants::copyright();
}

QString ApplicationService::license() const
{
    return Constants::license();
}

QString ApplicationService::fileFormatExtension() const
{
    return Constants::fileFormatExtension();
}

QString ApplicationService::webSiteUrl() const
{
    return Constants::webSiteUrl();
}

void ApplicationService::acceptUnsavedChangesDialog()
{
    juzzlin::L(TAG).info() << "Unsaved changes accepted";
    m_stateMachine->calculateState(StateMachine::Action::UnsavedChangesDialogAccepted);
}

void ApplicationService::applyAllTrackSettings()
{
    juzzlin::L(TAG).info() << "Apply all track settings";
    emit applyAllTrackSettingsRequested();
}

void ApplicationService::discardUnsavedChangesDialog()
{
    juzzlin::L(TAG).info() << "Unsaved changes discarded";
    m_stateMachine->calculateState(StateMachine::Action::UnsavedChangesDialogDiscarded);
}

void ApplicationService::rejectUnsavedChangesDialog()
{
    juzzlin::L(TAG).info() << "Unsaved changes rejected";
    m_stateMachine->calculateState(StateMachine::Action::UnsavedChangesDialogCanceled);
}

void ApplicationService::requestNewProject()
{
    juzzlin::L(TAG).info() << "'New file' requested";
    m_stateMachine->calculateState(StateMachine::Action::NewProjectRequested);
}

void ApplicationService::requestRecentFilesDialog()
{
    // No need to route through state machine as this takes place only on startup
    juzzlin::L(TAG).info() << "Recent files requested";
    emit recentFilesDialogRequested();
}

void ApplicationService::requestOpenProject()
{
    juzzlin::L(TAG).info() << "'Open file' requested";
    m_stateMachine->calculateState(StateMachine::Action::OpenProjectRequested);
}

void ApplicationService::requestQuit()
{
    juzzlin::L(TAG).info() << "Quit requested";
    m_stateMachine->calculateState(StateMachine::Action::QuitSelected);
}

void ApplicationService::requestSaveProject()
{
    juzzlin::L(TAG).info() << "'Save file' requested";
    m_stateMachine->calculateState(StateMachine::Action::SaveProjectRequested);
}

void ApplicationService::requestSaveProjectAs()
{
    juzzlin::L(TAG).info() << "'Save file as' requested";
    m_stateMachine->calculateState(StateMachine::Action::SaveProjectAsRequested);
}

void ApplicationService::requestSaveProjectAsTemplate()
{
    juzzlin::L(TAG).info() << "'Save file as template' requested";
    m_stateMachine->calculateState(StateMachine::Action::SaveProjectAsTemplateRequested);
}

void ApplicationService::requestLiveNoteOn(quint8 key, quint8 octave, quint8 velocity)
{
    if (const auto instrument = m_editorService->instrument(m_editorService->position().track); instrument) {
        if (const auto midiNote = NoteConverter::keyAndOctaveToMidiNote(key, octave); midiNote.has_value()) {
            juzzlin::L(TAG).debug() << "Live note ON " << midiNote->first << " requested on instrument " << instrument->toString().toStdString();
            emit liveNoteOnRequested(instrument, { midiNote->second, velocity });
        }
    } else {
        juzzlin::L(TAG).info() << "No instrument set on track!";
    }
}

void ApplicationService::requestInstrumentReset()
{
    juzzlin::L(TAG).info() << "Instrument reset requested";
    emit applyAllTrackSettingsRequested();
}

void ApplicationService::requestAllNotesOff()
{
    juzzlin::L(TAG).info() << "All notes off requested";
    emit allNotesOffRequested();
}

void ApplicationService::requestLiveNoteOff(quint8 key, quint8 octave)
{
    if (const auto instrument = m_editorService->instrument(m_editorService->position().track); instrument) {
        if (const auto midiNote = NoteConverter::keyAndOctaveToMidiNote(key, octave); midiNote.has_value()) {
            juzzlin::L(TAG).debug() << "Live note OFF " << midiNote->first << " requested on instrument " << instrument->toString().toStdString();
            emit liveNoteOffRequested(instrument, { midiNote->second, 0 });
        }
    } else {
        juzzlin::L(TAG).info() << "No instrument set on track!";
    }
}

void ApplicationService::cancelOpenProject()
{
    m_stateMachine->calculateState(StateMachine::Action::OpeningProjectCanceled);
}

void ApplicationService::openProject(QUrl url)
{
    try {
        m_editorService->load(url.toLocalFile());
        m_recentFilesManager->addRecentFile(url.toLocalFile());
        m_stateMachine->calculateState(StateMachine::Action::ProjectOpened);
    } catch (std::exception & e) {
        const auto message = QString { "Failed to load project: %1 " }.arg(e.what());
        juzzlin::L(TAG).error() << message.toStdString();
        emit statusTextRequested(message);
    }
}

void ApplicationService::openRecentProject(QString filePath)
{
    m_recentFilesManager->setSelectedFile(filePath);

    m_stateMachine->calculateState(StateMachine::Action::RecentFileSelected);
}

void ApplicationService::cancelRecentFileDialog()
{
    m_stateMachine->calculateState(StateMachine::Action::RecentFileDialogCanceled);
}

void ApplicationService::cancelSaveProjectAs()
{
    m_stateMachine->calculateState(StateMachine::Action::SavingProjectAsCanceled);
}

void ApplicationService::saveProjectAs(QUrl url)
{
    try {
        auto fileName = url.toLocalFile();
        if (!fileName.endsWith(Constants::fileFormatExtension())) {
            fileName += Constants::fileFormatExtension();
        }
        m_editorService->saveAs(fileName);
        m_recentFilesManager->addRecentFile(fileName);
        m_stateMachine->calculateState(StateMachine::Action::ProjectSavedAs);
    } catch (std::exception & e) {
        const auto message = QString { "Failed to save project: %1 " }.arg(e.what());
        juzzlin::L(TAG).error() << message.toStdString();
        emit statusTextRequested(message);
    }
}

void ApplicationService::saveProjectAsTemplate(QUrl url)
{
    try {
        auto fileName = url.toLocalFile();
        if (!fileName.endsWith(Constants::fileFormatExtension())) {
            fileName += Constants::fileFormatExtension();
        }
        m_editorService->saveAsTemplate(fileName);
        m_recentFilesManager->addRecentFile(fileName);
    } catch (std::exception & e) {
        const auto message = QString { "Failed to save project: %1 " }.arg(e.what());
        juzzlin::L(TAG).error() << message.toStdString();
        emit statusTextRequested(message);
    }
}

QStringList ApplicationService::recentFiles() const
{
    return m_recentFilesManager->recentFiles();
}

void ApplicationService::requestUnsavedChangesDialog()
{
    juzzlin::L(TAG).info() << "Not saved dialog requested";
    emit unsavedChangesDialogRequested();
}

void ApplicationService::requestOpenDialog()
{
    juzzlin::L(TAG).info() << "Open dialog requested";
    emit openDialogRequested();
}

void ApplicationService::requestSaveAsDialog()
{
    juzzlin::L(TAG).info() << "Save as dialog requested";
    emit saveAsDialogRequested();
}

void ApplicationService::requestSaveAsTemplateDialog()
{
    juzzlin::L(TAG).info() << "Save as template dialog requested";
    emit saveAsTemplateDialogRequested();
}

void ApplicationService::setRecentFilesManager(RecentFilesManagerS recentFilesManager)
{
    m_recentFilesManager = recentFilesManager;
}

void ApplicationService::setStateMachine(StateMachineS stateMachine)
{
    m_stateMachine = stateMachine;
}

void ApplicationService::setEditorService(EditorServiceS editorService)
{
    m_editorService = editorService;
}

void ApplicationService::setPlayerService(PlayerServiceS playerService)
{
    m_playerService = playerService;
}

bool ApplicationService::editMode() const
{
    return m_editMode;
}

void ApplicationService::setEditMode(bool editMode)
{
    m_editMode = editMode;
}

ApplicationService::~ApplicationService() = default;

} // namespace noteahead
