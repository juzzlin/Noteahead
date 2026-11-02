// This file is part of Noteahead.
// Copyright (C) 2025 Jussi Lind <jussi.lind@iki.fi>
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

#include "state_machine.hpp"

#include "service/application_service.hpp"
#include "service/editor_service.hpp"

#include <utility>

namespace noteahead {

StateMachine::StateMachine(ApplicationServiceS applicationService, EditorServiceS editorService, QObject * parent)
  : QObject { parent }
  , m_applicationService { applicationService }
  , m_editorService { editorService }
{
}

void StateMachine::calculateState(StateMachine::Action action)
{
    switch (action) {

    case Action::NewProjectRequested:
        leaveProject(QuitType::New, State::InitializeNewProject);
        break;

    case Action::ApplicationInitialized:
        if (!m_applicationService->initialFilePath().isEmpty()) {
            const auto filePath = m_applicationService->initialFilePath();
            if (m_applicationService->isMidiFile(filePath)) {
                m_applicationService->addRecentFile(filePath);
                m_state = State::ShowMidiImportDialog;
            } else {
                m_applicationService->openRecentProject(filePath);
                return; // openRecentProject calls calculateState recursively
            }
        } else if (!m_applicationService->recentFiles().isEmpty()) {
            m_state = State::ShowRecentFilesDialog;
        } else {
            m_state = State::InitializeNewProject;
        }
        break;

    case Action::MidiImportRequested:
        m_state = State::ShowMidiImportDialog;
        break;

    case Action::UnsavedChangesDialogDiscarded:
    case Action::ProjectSaved:
    case Action::ProjectSavedAs:
        // Taken as it is acted on: left behind, it would make the next ordinary save start a new
        // project or reopen a file all over again, throwing away everything done since
        switch (std::exchange(m_quitType, QuitType::None)) {
        case QuitType::Close:
            m_state = State::Exit;
            break;
        case QuitType::New:
            m_state = State::InitializeNewProject;
            break;
        case QuitType::Open:
            m_state = State::ShowOpenDialog;
            break;
        case QuitType::OpenRecent:
            m_state = State::OpenRecent;
            break;
        case QuitType::OpenExample:
            m_state = State::OpenExample;
            break;
        case QuitType::OpenTemplate:
            m_state = State::OpenTemplate;
            break;
        default:
            m_state = State::Edit;
            break;
        }
        break;

    case Action::NewProjectInitialized:
    case Action::OpeningProjectCanceled:
    case Action::OpeningProjectFailed:
    case Action::ProjectOpened:
    case Action::ProjectSaveFailed:
    case Action::SavingProjectAsCanceled:
    case Action::SavingProjectAsFailed:
    case Action::UnsavedChangesDialogCanceled:
        m_quitType = QuitType::None;
        m_state = State::Edit;
        break;

    case Action::OpenExampleRequested:
        leaveProject(QuitType::OpenExample, State::OpenExample);
        break;

    case Action::OpenTemplateRequested:
        leaveProject(QuitType::OpenTemplate, State::OpenTemplate);
        break;

    case Action::OpenProjectRequested:
        leaveProject(QuitType::Open, State::ShowOpenDialog);
        break;

    case Action::QuitSelected:
        leaveProject(QuitType::Close, State::Exit);
        break;

    case Action::UnsavedChangesDialogAccepted:
    case Action::SaveProjectRequested:
        if (m_editorService->canBeSaved()) {
            m_state = State::Save;
        } else {
            m_state = State::ShowSaveAsDialog;
        }
        break;

    case Action::SaveProjectAsRequested:
        m_state = State::ShowSaveAsDialog;
        break;

    case Action::SaveProjectAsTemplateRequested:
        m_state = State::ShowSaveAsTemplateDialog;
        break;

    case Action::RecentFileDialogCanceled:
        // The initial project should be already loaded
        m_state = State::Edit;
        break;

    case Action::RecentFileSelected:
        leaveProject(QuitType::OpenRecent, State::OpenRecent);
        break;
    }

    emit stateChanged(m_state);
}

void StateMachine::leaveProject(QuitType quitType, State state)
{
    // There is something to come back to after a save only while the unsaved changes dialog is up
    if (m_editorService->isModified()) {
        m_quitType = quitType;
        m_state = State::ShowUnsavedChangesDialog;
    } else {
        m_quitType = QuitType::None;
        m_state = state;
    }
}

QString StateMachine::stateToString(State state)
{
    static const std::map<State, QString> stateMap = {
        { State::Edit, "Edit" },
        { State::Exit, "Exit" },
        { State::Init, "Init" },
        { State::InitializeNewProject, "InitializeNewProject" },
        { State::OpenExample, "OpenExample" },
        { State::OpenRecent, "OpenRecent" },
        { State::OpenTemplate, "OpenTemplate" },
        { State::Save, "Save" },
        { State::ShowRecentFilesDialog, "ShowRecentFilesDialog" },
        { State::ShowMidiImportDialog, "ShowMidiImportDialog" },
        { State::ShowUnsavedChangesDialog, "ShowUnsavedChangesDialog" },
        { State::ShowOpenDialog, "ShowOpenDialog" },
        { State::ShowSaveAsDialog, "ShowSaveAsDialog" },
        { State::TryCloseWindow, "TryCloseWindow" }
    };
    if (const auto it = stateMap.find(state); it != stateMap.end()) {
        return it->second;
    } else {
        return QString::number(static_cast<int>(state));
    }
}

} // namespace noteahead
