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

#ifndef STATE_MACHINE_HPP
#define STATE_MACHINE_HPP

#include <QObject>

#include <memory>

namespace noteahead {

class ApplicationService;
class EditorService;

//! State machine mainly for application logic involving user actions (show dialog, load file, etc ... )
class StateMachine : public QObject
{
    Q_OBJECT

public:
    using ApplicationServiceS = std::shared_ptr<ApplicationService>;
    using EditorServiceS = std::shared_ptr<EditorService>;
    StateMachine(ApplicationServiceS applicationService, EditorServiceS editorService, QObject * parent = nullptr);

    enum class State
    {
        Edit,
        Exit,
        Init,
        InitializeNewProject,
        OpenRecent,
        Save,
        ShowRecentFilesDialog,
        ShowUnsavedChangesDialog,
        ShowOpenDialog,
        ShowSaveAsDialog,
        ShowSaveAsTemplateDialog,
        TryCloseWindow
    };

    enum class Action
    {
        ApplicationInitialized,
        NewProjectInitialized,
        NewProjectRequested,
        UnsavedChangesDialogAccepted,
        UnsavedChangesDialogCanceled,
        UnsavedChangesDialogDiscarded,
        OpenProjectRequested,
        OpeningProjectCanceled,
        OpeningProjectFailed,
        ProjectOpened,
        SavingProjectAsCanceled,
        SavingProjectAsFailed,
        ProjectSaveFailed,
        ProjectSaved,
        ProjectSavedAs,
        QuitSelected,
        RecentFileDialogCanceled,
        RecentFileSelected,
        SaveProjectAsRequested,
        SaveProjectAsTemplateRequested,
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

    static QString stateToString(StateMachine::State state);

signals:

    void stateChanged(StateMachine::State state);

private:
    State m_state = State::Init;

    QuitType m_quitType = QuitType::None;

    ApplicationServiceS m_applicationService;

    EditorServiceS m_editorService;
};

} // namespace noteahead

#endif // STATE_MACHINE_HPP
