// This file is part of Cacophony.
// Copyright (C) 2025 Jussi Lind <jussi.lind@iki.fi>
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

#include "state_machine.hpp"

#include "editor_service.hpp"
#include "simple_logger.hpp"

namespace cacophony {

static const auto TAG = "StateMachine";

StateMachine::StateMachine(EditorServiceS editorService, QObject * parent)
  : QObject { parent }
  , m_editorService { editorService }
{
}

void StateMachine::calculateState(StateMachine::Action action)
{
    switch (action) {

    case Action::NewProjectRequested:
        m_quitType = QuitType::New;
        if (m_editorService->isModified()) {
            m_state = State::ShowNotSavedDialog;
        } else {
            m_state = State::InitializeNewProject;
        }
        break;

    case Action::MainWindowInitialized:
        m_state = State::InitializeNewProject;
        break;

    case Action::NotSavedDialogDiscarded:
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

    case Action::ProjectOpened:
    case Action::ProjectSaveFailed:
    case Action::SavingProjectAsCanceled:
    case Action::SavingProjectAsFailed:
    case Action::NewProjectInitialized:
    case Action::NotSavedDialogCanceled:
    case Action::OpeningProjectCanceled:
    case Action::OpeningProjectFailed:
        m_quitType = QuitType::None;
        m_state = State::Edit;
        break;

    case Action::OpenProjectRequested:
        m_quitType = QuitType::Open;
        if (m_editorService->isModified()) {
            m_state = State::ShowNotSavedDialog;
        } else {
            m_state = State::ShowOpenDialog;
        }
        break;

    case Action::QuitSelected:
        m_quitType = QuitType::Close;
        if (m_editorService->isModified()) {
            m_state = State::ShowNotSavedDialog;
        } else {
            m_state = State::Exit;
        }
        break;

    case Action::NotSavedDialogAccepted:
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

    case Action::RecentFileSelected:
        m_quitType = QuitType::OpenRecent;
        if (m_editorService->isModified()) {
            m_state = State::ShowNotSavedDialog;
        } else {
            m_state = State::OpenRecent;
        }
        break;

    default:
        juzzlin::L(TAG).warning() << "Action " << static_cast<int>(action) << " not handled!";
    }

    emit stateChanged(m_state);
}

} // namespace cacophony
