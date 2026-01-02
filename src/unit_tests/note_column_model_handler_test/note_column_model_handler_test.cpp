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

#include "note_column_model_handler_test.hpp"

#include "../../application/models/note_column_model_handler.hpp"
#include "../../application/service/automation_service.hpp"
#include "../../application/service/editor_service.hpp"
#include "../../application/service/selection_service.hpp"
#include "../../application/service/settings_service.hpp"

namespace noteahead {

void NoteColumnModelHandlerTest::initTestCase()
{
    QCoreApplication::setOrganizationName("NoteaheadTest");
    QCoreApplication::setApplicationName("NoteColumnModelHandlerTest");
}

void NoteColumnModelHandlerTest::cleanupTestCase()
{
    QSettings settings {};
    settings.clear();
}

void NoteColumnModelHandlerTest::test_columnModel_shouldCreateAndReturnModel()
{
    const auto automationService { std::make_shared<AutomationService>() };
    const auto selectionService { std::make_shared<SelectionService>() };
    const auto editorService { std::make_shared<EditorService>(selectionService) };
    const auto settingsService { std::make_shared<SettingsService>() };

    NoteColumnModelHandler handler { editorService, selectionService, automationService, settingsService };

    auto model { handler.columnModel(0, 0, 0) };
    QVERIFY(model);

    auto sameModel { handler.columnModel(0, 0, 0) };
    QCOMPARE(model, sameModel);
}

void NoteColumnModelHandlerTest::test_clear_shouldClearModels()
{
    const auto automationService { std::make_shared<AutomationService>() };
    const auto selectionService { std::make_shared<SelectionService>() };
    const auto editorService { std::make_shared<EditorService>(selectionService) };
    const auto settingsService { std::make_shared<SettingsService>() };

    NoteColumnModelHandler handler { editorService, selectionService, automationService, settingsService };

    handler.columnModel(0, 0, 0);
    handler.clear();

    // Since handler is cleared, it should create a new model for the same address
    auto newModel { handler.columnModel(0, 0, 0) };
    QVERIFY(newModel);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::NoteColumnModelHandlerTest)
