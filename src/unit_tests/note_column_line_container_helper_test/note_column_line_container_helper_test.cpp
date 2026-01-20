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

#include "note_column_line_container_helper_test.hpp"

#include "../../application/models/note_column_line_container_helper.hpp"
#include "../../application/service/automation_service.hpp"
#include "../../application/service/editor_service.hpp"
#include "../../application/service/selection_service.hpp"
#include "../../application/service/settings_service.hpp"
#include "../../application/service/util_service.hpp"
#include "../../domain/instrument_settings.hpp"

namespace noteahead {

void NoteColumnLineContainerHelperTest::test_lineColorAndBorderWidth_selected_shouldReturnSelectionColor()
{
    const auto automationService { std::make_shared<AutomationService>() };
    const auto selectionService { std::make_shared<SelectionService>() };
    const auto settingsService { std::make_shared<SettingsService>() };
    const auto editorService { std::make_shared<EditorService>(selectionService, settingsService) };
    const auto utilService { std::make_shared<UtilService>() };

    const NoteColumnLineContainerHelper helper { automationService, editorService, selectionService, utilService };

    selectionService->requestSelectionStart(0, 0, 0, 0);
    selectionService->requestSelectionEnd(0, 0, 0, 0);

    const auto result = helper.lineColorAndBorderWidth(0, 0, 0, 0);
    QCOMPARE(result.size(), 2);
    QCOMPARE(result.at(1).toInt(), 0); // Border width for selection
}

void NoteColumnLineContainerHelperTest::test_lineColorAndBorderWidth_hasInstrumentSettings_shouldReturnInstrumentColor()
{
    const auto automationService { std::make_shared<AutomationService>() };
    const auto selectionService { std::make_shared<SelectionService>() };
    const auto settingsService { std::make_shared<SettingsService>() };
    const auto editorService { std::make_shared<EditorService>(selectionService, settingsService) };
    const auto utilService { std::make_shared<UtilService>() };

    const NoteColumnLineContainerHelper helper { automationService, editorService, selectionService, utilService };

    editorService->requestPosition(0, 0, 0, 0, 0);
    editorService->setInstrumentSettingsAtCurrentPosition(std::make_shared<InstrumentSettings>());

    const auto result = helper.lineColorAndBorderWidth(0, 0, 0, 0);
    QCOMPARE(result.size(), 2);
    QCOMPARE(result.at(1).toInt(), 1); // Border width for instrument
    // We could check color too, but it depends on blendColors which we're not mocking.
}

void NoteColumnLineContainerHelperTest::test_lineColorAndBorderWidth_hasAutomations_shouldReturnAutomationColor()
{
    const auto automationService { std::make_shared<AutomationService>() };
    const auto selectionService { std::make_shared<SelectionService>() };
    const auto settingsService { std::make_shared<SettingsService>() };
    const auto editorService { std::make_shared<EditorService>(selectionService, settingsService) };
    const auto utilService { std::make_shared<UtilService>() };

    const NoteColumnLineContainerHelper helper { automationService, editorService, selectionService, utilService };

    automationService->addMidiCcAutomation(0, 0, 0, 7, 0, 4, 0, 100, "Test", true, 8, 0);

    const auto result = helper.lineColorAndBorderWidth(0, 0, 0, 0);
    QCOMPARE(result.size(), 2);
    QCOMPARE(result.at(1).toInt(), 1);
}

void NoteColumnLineContainerHelperTest::test_lineColorAndBorderWidth_default_shouldReturnDefaultColor()
{
    const auto automationService { std::make_shared<AutomationService>() };
    const auto selectionService { std::make_shared<SelectionService>() };
    const auto settingsService { std::make_shared<SettingsService>() };
    const auto editorService { std::make_shared<EditorService>(selectionService, settingsService) };
    const auto utilService { std::make_shared<UtilService>() };

    const NoteColumnLineContainerHelper helper { automationService, editorService, selectionService, utilService };

    const auto result = helper.lineColorAndBorderWidth(0, 0, 0, 0);
    QCOMPARE(result.size(), 2);
    QCOMPARE(result.at(1).toInt(), 1);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::NoteColumnLineContainerHelperTest)
