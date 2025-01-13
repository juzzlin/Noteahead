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

#ifndef APPLICATION_SERVICE_HPP
#define APPLICATION_SERVICE_HPP

#include <QObject>
#include <QUrl>

namespace cacophony {

class EditorService;
class StateMachine;

class ApplicationService : public QObject
{
    Q_OBJECT

public:
    ApplicationService();

    Q_INVOKABLE QString applicationName() const;

    Q_INVOKABLE QString applicationVersion() const;

    Q_INVOKABLE QString copyright() const;

    Q_INVOKABLE QString license() const;

    Q_INVOKABLE void acceptUnsavedChangesDialog();

    Q_INVOKABLE void discardUnsavedChangesDialog();

    Q_INVOKABLE void rejectUnsavedChangesDialog();

    Q_INVOKABLE void requestNewProject();

    Q_INVOKABLE void requestOpenProject();

    Q_INVOKABLE void requestSaveProject();

    Q_INVOKABLE void requestSaveProjectAs();

    Q_INVOKABLE void requestQuit();

    Q_INVOKABLE void cancelOpenProject();

    Q_INVOKABLE void openProject(QUrl url);

    Q_INVOKABLE void cancelSaveProjectAs();

    Q_INVOKABLE void saveProjectAs(QUrl url);

    void requestUnsavedChangesDialog();

    void requestOpenDialog();

    void requestSaveAsDialog();

    using StateMachineS = std::shared_ptr<StateMachine>;
    void setStateMachine(StateMachineS stateMachine);

    using EditorServiceS = std::shared_ptr<EditorService>;
    void setEditorService(EditorServiceS editorService);

signals:
    void unsavedChangesDialogRequested();

    void openDialogRequested();

    void quitRequested();

    void saveAsDialogRequested();

    void statusTextRequested(QString message);

private:
    StateMachineS m_stateMachine;

    EditorServiceS m_editorService;
};

} // namespace cacophony

#endif // APPLICATION_SERVICE_HPP
