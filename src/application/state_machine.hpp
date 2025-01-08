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

#ifndef STATE_MACHINE_HPP
#define STATE_MACHINE_HPP

#include <QObject>

#include <memory>

namespace cacophony {

class EditorService;

//! State machine mainly for application logic involving user actions (show dialog, load file, etc ... )
class StateMachine : public QObject
{
    Q_OBJECT

public:
    using EditorServiceS = std::shared_ptr<EditorService>;
    StateMachine(EditorServiceS editorService, QObject * parent = nullptr);

    enum class State
    {
        Edit,
        Exit,
        Init,
        InitializeNewProject,
        OpenRecent,
        Save,
        ShowNotSavedDialog,
        ShowOpenDialog,
        ShowSaveAsDialog,
        TryCloseWindow
    };

    enum class Action
    {
        MainWindowInitialized,
        NewProjectInitialized,
        NewProjectRequested,
        NotSavedDialogAccepted,
        NotSavedDialogCanceled,
        NotSavedDialogDiscarded,
        OpenProjectRequested,
        OpeningProjectCanceled,
        OpeningProjectFailed,
        ProjectOpened,
        ProjectSaveAsCanceled,
        ProjectSaveAsFailed,
        ProjectSaveFailed,
        ProjectSaved,
        ProjectSavedAs,
        QuitSelected,
        RecentFileSelected,
        SaveProjectAsRequested,
        SaveProjectRequested
    };

    enum class QuitType
    {
        None,
        New,
        Open,
        OpenRecent,
        Close
    };

    void calculateState(StateMachine::Action action);

signals:

    void stateChanged(StateMachine::State state);

private:
    State m_state = State::Init;

    QuitType m_quitType = QuitType::None;

    EditorServiceS m_editorService;
};

} // namespace cacophony

#endif // STATE_MACHINE_HPP
