// This file is part of Cacophony.
// Copyright (C) 2024 Jussi Lind <jussi.lind@iki.fi>
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

#include "application_service.hpp"

#include "../common/constants.hpp"
#include "../contrib/SimpleLogger/src/simple_logger.hpp"
#include "editor_service.hpp"
#include "state_machine.hpp"

namespace cacophony {

static const auto TAG = "ApplicationService";

ApplicationService::ApplicationService() = default;

QString ApplicationService::applicationName() const
{
    return Constants::applicationName().c_str();
}

QString ApplicationService::applicationVersion() const
{
    return Constants::applicationVersion().c_str();
}

void ApplicationService::requestNewProject()
{
    juzzlin::L(TAG).info() << "'New file' requested";
    m_stateMachine->calculateState(StateMachine::Action::NewProjectRequested);
}

void ApplicationService::requestOpenProject()
{
    juzzlin::L(TAG).info() << "'Open file' requested";
    m_stateMachine->calculateState(StateMachine::Action::OpenProjectRequested);
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

void ApplicationService::cancelSaveProjectAs()
{
    m_stateMachine->calculateState(StateMachine::Action::ProjectSaveAsCanceled);
}

void ApplicationService::saveProjectAs(QUrl fileName)
{
    m_editorService->saveAs(fileName.toLocalFile());
    m_stateMachine->calculateState(StateMachine::Action::ProjectSavedAs);
}

void ApplicationService::requestSaveAsDialog()
{
    juzzlin::L(TAG).info() << "Save as dialog requested";
    emit saveAsDialogRequested();
}

void ApplicationService::setStateMachine(StateMachineS stateMachine)
{
    m_stateMachine = stateMachine;
}

void ApplicationService::setEditorService(EditorServiceS editorService)
{
    m_editorService = editorService;
}

} // namespace cacophony
