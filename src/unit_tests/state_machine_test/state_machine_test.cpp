// This file is part of Noteahead.
// Copyright (C) 2026 Jussi Lind <jussi.lind@iki.fi>
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

#include "state_machine_test.hpp"

#include "../../application/service/application_service.hpp"
#include "../../application/service/automation_service.hpp"
#include "../../application/service/editor_service.hpp"
#include "../../application/service/property_service.hpp"
#include "../../application/service/selection_service.hpp"
#include "../../application/service/settings_service.hpp"
#include "../../application/state_machine.hpp"
#include "../../infra/data_service.hpp"

#include <QTest>

#include <memory>

namespace noteahead {

namespace {

//! A state machine on real services, remembering the last state it went to.
struct Fixture
{
    std::shared_ptr<EditorService> editorService { std::make_shared<EditorService>(std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>()) };
    StateMachine stateMachine { std::make_shared<ApplicationService>(), editorService };
    StateMachine::State state { StateMachine::State::Init };

    Fixture()
    {
        QObject::connect(&stateMachine, &StateMachine::stateChanged, [this](StateMachine::State newState) { state = newState; });
    }

    //! Some editing, then Save As carried through to the end, as the File menu does it
    void editAndSaveAs()
    {
        editorService->setIsModified(true);
        stateMachine.calculateState(StateMachine::Action::SaveProjectAsRequested);
        QCOMPARE(state, StateMachine::State::ShowSaveAsDialog);
        stateMachine.calculateState(StateMachine::Action::ProjectSavedAs);
    }
};

} // namespace

void StateMachineTest::test_projectSavedAs_afterNewProject_shouldStayInEdit()
{
    // What cleared the pattern: the new project's request was still pending when the save
    // finished, so the save started another new project. Nothing reports back here on purpose:
    // whatever the application does in between, a later save must not repeat the request.
    Fixture fixture;
    fixture.stateMachine.calculateState(StateMachine::Action::NewProjectRequested);
    QCOMPARE(fixture.state, StateMachine::State::InitializeNewProject);

    fixture.editAndSaveAs();

    QCOMPARE(fixture.state, StateMachine::State::Edit);
}

void StateMachineTest::test_projectSavedAs_afterOpenRecent_shouldStayInEdit()
{
    // Reopening the file from disk would throw away everything done since it was opened
    Fixture fixture;
    fixture.stateMachine.calculateState(StateMachine::Action::RecentFileSelected);
    QCOMPARE(fixture.state, StateMachine::State::OpenRecent);

    fixture.editAndSaveAs();

    QCOMPARE(fixture.state, StateMachine::State::Edit);
}

void StateMachineTest::test_projectSaved_afterQuitWithoutChanges_shouldStayInEdit()
{
    Fixture fixture;
    fixture.stateMachine.calculateState(StateMachine::Action::QuitSelected);
    QCOMPARE(fixture.state, StateMachine::State::Exit);

    fixture.stateMachine.calculateState(StateMachine::Action::ProjectSaved);

    QCOMPARE(fixture.state, StateMachine::State::Edit);
}

void StateMachineTest::test_projectSavedAs_fromUnsavedChangesDialog_shouldContinueOnce()
{
    // The one case the pending request exists for: New with unsaved changes, saved first
    Fixture fixture;
    fixture.editorService->setIsModified(true);
    fixture.stateMachine.calculateState(StateMachine::Action::NewProjectRequested);
    QCOMPARE(fixture.state, StateMachine::State::ShowUnsavedChangesDialog);
    fixture.stateMachine.calculateState(StateMachine::Action::UnsavedChangesDialogAccepted);
    QCOMPARE(fixture.state, StateMachine::State::ShowSaveAsDialog);

    fixture.stateMachine.calculateState(StateMachine::Action::ProjectSavedAs);
    QCOMPARE(fixture.state, StateMachine::State::InitializeNewProject);

    fixture.editAndSaveAs();
    QCOMPARE(fixture.state, StateMachine::State::Edit);
}

void StateMachineTest::test_unsavedChangesDiscarded_shouldContinueOnce()
{
    Fixture fixture;
    fixture.editorService->setIsModified(true);
    fixture.stateMachine.calculateState(StateMachine::Action::OpenExampleRequested);
    QCOMPARE(fixture.state, StateMachine::State::ShowUnsavedChangesDialog);

    fixture.stateMachine.calculateState(StateMachine::Action::UnsavedChangesDialogDiscarded);
    QCOMPARE(fixture.state, StateMachine::State::OpenExample);

    fixture.editAndSaveAs();
    QCOMPARE(fixture.state, StateMachine::State::Edit);
}

void StateMachineTest::test_newProjectRequested_withoutChanges_shouldReturnToEditWhenInitialized()
{
    Fixture fixture;
    fixture.stateMachine.calculateState(StateMachine::Action::NewProjectRequested);

    fixture.stateMachine.calculateState(StateMachine::Action::NewProjectInitialized);

    QCOMPARE(fixture.state, StateMachine::State::Edit);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::StateMachineTest)
