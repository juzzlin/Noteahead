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

#include "note_column_model_test.hpp"

#include "../../application/models/note_column_model.hpp"
#include "../../application/models/note_column_line_container_helper.hpp"
#include "../../application/service/automation_service.hpp"
#include "../../application/service/editor_service.hpp"
#include "../../application/service/selection_service.hpp"
#include "../../application/service/util_service.hpp"
#include "../../application/service/settings_service.hpp"
#include "../../domain/line.hpp"
#include "../../domain/note_data.hpp"

#include <QSettings>
#include <QSignalSpy>
#include <QTest>

namespace noteahead {

void NoteColumnModelTest::initTestCase()
{
    QCoreApplication::setOrganizationName("NoteaheadTest");
    QCoreApplication::setApplicationName("NoteColumnModelTest");
}

void NoteColumnModelTest::cleanupTestCase()
{
    QSettings settings {};
    settings.clear();
}

void NoteColumnModelTest::test_rowCount_shouldReturnCorrectValue()
{
    const auto automationService { std::make_shared<AutomationService>() };
    const auto selectionService { std::make_shared<SelectionService>() };
    const auto settingsService { std::make_shared<SettingsService>() };
    const auto editorService { std::make_shared<EditorService>(selectionService, settingsService) };
    const auto utilService { std::make_shared<UtilService>() };
    const auto helper { std::make_shared<NoteColumnLineContainerHelper>(automationService, editorService, selectionService, utilService) };

    NoteColumnModel model { { 0, 0, 0 }, editorService, helper, settingsService };

    NoteColumnModel::LineList lines;
    for (int i = 0; i < 10; ++i) {
        lines.push_back(std::make_shared<Line>(static_cast<size_t>(i)));
    }
    model.setColumnData(lines);

    QCOMPARE(model.rowCount(), 10 + settingsService->visibleLines());
}

void NoteColumnModelTest::test_data_shouldReturnCorrectValues()
{
    const auto automationService { std::make_shared<AutomationService>() };
    const auto selectionService { std::make_shared<SelectionService>() };
    const auto settingsService { std::make_shared<SettingsService>() };
    const auto editorService { std::make_shared<EditorService>(selectionService, settingsService) };
    const auto utilService { std::make_shared<UtilService>() };
    const auto helper { std::make_shared<NoteColumnLineContainerHelper>(automationService, editorService, selectionService, utilService) };

    NoteColumnModel model { { 0, 0, 0 }, editorService, helper, settingsService };

    auto line { std::make_shared<Line>(0) };
    NoteData noteData {};
    noteData.setAsNoteOn(60, 100); // C-4
    line->setNoteData(noteData);

    NoteColumnModel::LineList lines { line };
    model.setColumnData(lines);

    const auto idx = model.index(editorService->positionBarLine(), 0);
    QCOMPARE(model.data(idx, static_cast<int>(NoteColumnModel::DataRole::Note)).toString(), QString { "C-5" });
    QCOMPARE(model.data(idx, static_cast<int>(NoteColumnModel::DataRole::Velocity)).toString(), QString { "100" });
}

void NoteColumnModelTest::test_data_LineRole_shouldReturnCorrectValue()
{
    const auto automationService { std::make_shared<AutomationService>() };
    const auto selectionService { std::make_shared<SelectionService>() };
    const auto settingsService { std::make_shared<SettingsService>() };
    const auto editorService { std::make_shared<EditorService>(selectionService, settingsService) };
    const auto utilService { std::make_shared<UtilService>() };
    const auto helper { std::make_shared<NoteColumnLineContainerHelper>(automationService, editorService, selectionService, utilService) };

    NoteColumnModel model { { 0, 0, 0 }, editorService, helper, settingsService };

    auto line { std::make_shared<Line>(0) };
    NoteData noteData {};
    noteData.setAsNoteOn(60, 100); // C-5
    noteData.setDelay(12);
    line->setNoteData(noteData);

    NoteColumnModel::LineList lines { line, std::make_shared<Line>(1) };
    model.setColumnData(lines);

    const int barLine = static_cast<int>(editorService->positionBarLine());
    
    // Test filled line
    const auto idx0 = model.index(barLine, 0);
    QCOMPARE(model.data(idx0, static_cast<int>(NoteColumnModel::DataRole::Line)).toString(), QString { "C-5 100 12" });

    // Test empty line
    const auto idx1 = model.index(barLine + 1, 0);
    QCOMPARE(model.data(idx1, static_cast<int>(NoteColumnModel::DataRole::Line)).toString(), QString { "--- --- --" });
}

void NoteColumnModelTest::test_updateNoteDataAtPosition_shouldEmitDataChangedWithCorrectRoles()
{
    const auto automationService { std::make_shared<AutomationService>() };
    const auto selectionService { std::make_shared<SelectionService>() };
    const auto settingsService { std::make_shared<SettingsService>() };
    const auto editorService { std::make_shared<EditorService>(selectionService, settingsService) };
    const auto utilService { std::make_shared<UtilService>() };
    const auto helper { std::make_shared<NoteColumnLineContainerHelper>(automationService, editorService, selectionService, utilService) };

    NoteColumnModel model { { 0, 0, 0 }, editorService, helper, settingsService };

    NoteColumnModel::LineList lines { std::make_shared<Line>(0) };
    model.setColumnData(lines);

    QSignalSpy spy { &model, &NoteColumnModel::dataChanged };
    model.updateNoteDataAtPosition(0);

    QCOMPARE(spy.count(), 1);
    const auto arguments = spy.takeFirst();
    const auto roles = arguments.at(2).value<QList<int>>();

    QVERIFY(roles.contains(static_cast<int>(NoteColumnModel::DataRole::Note)));
    QVERIFY(roles.contains(static_cast<int>(NoteColumnModel::DataRole::Velocity)));
    QVERIFY(roles.contains(static_cast<int>(NoteColumnModel::DataRole::Delay)));
    QVERIFY(roles.contains(static_cast<int>(NoteColumnModel::DataRole::Line)));
}

void NoteColumnModelTest::test_setLineFocused_shouldUpdateData()
{
    const auto automationService { std::make_shared<AutomationService>() };
    const auto selectionService { std::make_shared<SelectionService>() };
    const auto settingsService { std::make_shared<SettingsService>() };
    const auto editorService { std::make_shared<EditorService>(selectionService, settingsService) };
    const auto utilService { std::make_shared<UtilService>() };
    const auto helper { std::make_shared<NoteColumnLineContainerHelper>(automationService, editorService, selectionService, utilService) };

    NoteColumnModel model { { 0, 0, 0 }, editorService, helper, settingsService };

    NoteColumnModel::LineList lines { std::make_shared<Line>(0) };
    model.setColumnData(lines);

    QSignalSpy spy { &model, &NoteColumnModel::dataChanged };
    model.setLineFocused(0, 1);

    QCOMPARE(spy.count(), 1);
    const auto idx = model.index(editorService->positionBarLine(), 0);
    QCOMPARE(model.data(idx, static_cast<int>(NoteColumnModel::DataRole::IsFocused)).toBool(), true);
    QCOMPARE(model.data(idx, static_cast<int>(NoteColumnModel::DataRole::LineColumn)).toInt(), 1);
}

void NoteColumnModelTest::test_updateIndexHighlights_shouldEmitDataChangedWithCorrectRange()
{
    const auto automationService { std::make_shared<AutomationService>() };
    const auto selectionService { std::make_shared<SelectionService>() };
    const auto settingsService { std::make_shared<SettingsService>() };
    const auto editorService { std::make_shared<EditorService>(selectionService, settingsService) };
    const auto utilService { std::make_shared<UtilService>() };
    const auto helper { std::make_shared<NoteColumnLineContainerHelper>(automationService, editorService, selectionService, utilService) };

    NoteColumnModel model { { 0, 0, 0 }, editorService, helper, settingsService };

    const int lineCount = 10;
    NoteColumnModel::LineList lines;
    for (int i = 0; i < lineCount; ++i) {
        lines.push_back(std::make_shared<Line>(static_cast<size_t>(i)));
    }
    model.setColumnData(lines);

    QSignalSpy spy { &model, &NoteColumnModel::dataChanged };
    model.updateIndexHighlights();

    QCOMPARE(spy.count(), 1);
    const auto arguments = spy.takeFirst();
    const auto topLeft = arguments.at(0).value<QModelIndex>();
    const auto bottomRight = arguments.at(1).value<QModelIndex>();

    const int barLine = static_cast<int>(editorService->positionBarLine());
    QCOMPARE(topLeft.row(), barLine);
    QCOMPARE(bottomRight.row(), barLine + lineCount - 1);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::NoteColumnModelTest)
