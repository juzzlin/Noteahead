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

#include "application_service.hpp"
#include "editor_service.hpp"

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
        m_quitType = QuitType::New;
        if (m_editorService->isModified()) {
            m_state = State::ShowUnsavedChangesDialog;
        } else {
            m_state = State::InitializeNewProject;
        }
        break;

    case Action::ApplicationInitialized:
        if (!m_applicationService->recentFiles().isEmpty()) {
            m_state = State::ShowRecentFilesDialog;
        } else {
            m_state = State::InitializeNewProject;
        }
        break;

    case Action::UnsavedChangesDialogDiscarded:
    case Action::ProjectSaved:
    case Action::ProjectSavedAs:
        switch (m_quitType) {
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

    case Action::OpenProjectRequested:
        m_quitType = QuitType::Open;
        if (m_editorService->isModified()) {
            m_state = State::ShowUnsavedChangesDialog;
        } else {
            m_state = State::ShowOpenDialog;
        }
        break;

    case Action::QuitSelected:
        m_quitType = QuitType::Close;
        if (m_editorService->isModified()) {
            m_state = State::ShowUnsavedChangesDialog;
        } else {
            m_state = State::Exit;
        }
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

    case Action::RecentFileDialogCanceled:
        m_state = State::InitializeNewProject;
        break;

    case Action::RecentFileSelected:
        m_quitType = QuitType::OpenRecent;
        if (m_editorService->isModified()) {
            m_state = State::ShowUnsavedChangesDialog;
        } else {
            m_state = State::OpenRecent;
        }
        break;
    }

    emit stateChanged(m_state);
}

QString StateMachine::stateToString(State state)
{
    static const std::map<State, QString> stateMap = {
        { State::Edit, "Edit" },
        { State::Exit, "Exit" },
        { State::Init, "Init" },
        { State::InitializeNewProject, "InitializeNewProject" },
        { State::OpenRecent, "OpenRecent" },
        { State::Save, "Save" },
        { State::ShowRecentFilesDialog, "ShowRecentFilesDialog" },
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
