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

#include "../../application/models/note_column_model.hpp"
#include "../../application/models/note_column_model_handler.hpp"
#include "../../application/service/automation_service.hpp"
#include "../../application/service/editor_service.hpp"
#include "../../application/service/property_service.hpp"
#include "../../application/service/selection_service.hpp"
#include "../../application/service/settings_service.hpp"
#include "../../infra/data_service.hpp"

#include <QSettings>
#include <QTemporaryDir>
#include <QTest>

namespace noteahead {

namespace {

// Keeps the settings written by this test process out of the user scope shared by all
// processes. Without it, concurrent runs of this binary (parallel CI workspaces) race on the
// same settings file and observe each other's pattern peek writes.
QTemporaryDir & settingsDirectory()
{
    static QTemporaryDir directory;
    return directory;
}

// Builds a three-position play order (positions 0, 1, 2 mapped to patterns 0, 1, 2) and drops a
// note on the tail of pattern 0 and the head of pattern 2, so the peeked neighbors are identifiable.
void setupThreePatternSong(EditorService & editorService)
{
    editorService.setPatternAtSongPosition(0, 0);
    editorService.setPatternAtSongPosition(1, 1);
    editorService.setPatternAtSongPosition(2, 2);

    // The top offset area peeks the tail of the previous pattern
    editorService.requestPosition(0, 0, 0, static_cast<qint64>(editorService.lineCount(0)) - 1, 0);
    editorService.requestNoteOnAtCurrentPosition(1, 4, 100);

    // The bottom offset area peeks the head of the next pattern
    editorService.requestPosition(2, 0, 0, 0, 0);
    editorService.requestNoteOnAtCurrentPosition(3, 4, 100);
}

int ghostRole()
{
    return static_cast<int>(NoteColumnModel::DataRole::IsGhostRow);
}

int virtualRole()
{
    return static_cast<int>(NoteColumnModel::DataRole::IsVirtualRow);
}

int noteRole()
{
    return static_cast<int>(NoteColumnModel::DataRole::Note);
}

int lineRole()
{
    return static_cast<int>(NoteColumnModel::DataRole::Line);
}

} // namespace

void NoteColumnModelHandlerTest::initTestCase()
{
    QCoreApplication::setOrganizationName("NoteaheadTest");
    QCoreApplication::setApplicationName("NoteColumnModelHandlerTest");

    QVERIFY(settingsDirectory().isValid());
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settingsDirectory().path());
}

void NoteColumnModelHandlerTest::cleanupTestCase()
{
    QSettings settings {};
    settings.clear();
}

void NoteColumnModelHandlerTest::test_columnModel_shouldCreateAndReturnModel()
{
    const auto automationService { std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };
    const auto selectionService { std::make_shared<SelectionService>() };
    const auto settingsService { std::make_shared<SettingsService>() };
    const auto editorService { std::make_shared<EditorService>(selectionService, settingsService, std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>()) };

    NoteColumnModelHandler handler { editorService, selectionService, automationService, settingsService };

    auto model { handler.columnModel(0, 0, 0) };
    QVERIFY(model);

    auto sameModel { handler.columnModel(0, 0, 0) };
    QCOMPARE(model, sameModel);
}

void NoteColumnModelHandlerTest::test_clear_shouldClearModels()
{
    const auto automationService { std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };
    const auto selectionService { std::make_shared<SelectionService>() };
    const auto settingsService { std::make_shared<SettingsService>() };
    const auto editorService { std::make_shared<EditorService>(selectionService, settingsService, std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>()) };

    NoteColumnModelHandler handler { editorService, selectionService, automationService, settingsService };

    handler.columnModel(0, 0, 0);
    handler.clear();

    // Since handler is cleared, it should create a new model for the same address
    auto newModel { handler.columnModel(0, 0, 0) };
    QVERIFY(newModel);
}

void NoteColumnModelHandlerTest::test_updatePattern_shouldPreserveFocus()
{
    const auto automationService { std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };
    const auto selectionService { std::make_shared<SelectionService>() };
    const auto settingsService { std::make_shared<SettingsService>() };
    const auto editorService { std::make_shared<EditorService>(selectionService, settingsService, std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>()) };

    NoteColumnModelHandler handler { editorService, selectionService, automationService, settingsService };

    auto model { handler.columnModel(0, 0, 0) };

    // Set position
    editorService->requestPosition(0, 0, 0, 10, 0);

    // Check if focused. Note: NoteColumnModel adds positionBarLine() to the row index.
    const int row = 10 + static_cast<int>(editorService->positionBarLine());
    QVERIFY(model->data(model->index(row, 0), static_cast<int>(NoteColumnModel::DataRole::IsFocused)).toBool());

    // Update pattern - this used to clear the focus state in the model
    handler.updatePattern(0);

    // Verify focus is preserved
    QVERIFY(model->data(model->index(row, 0), static_cast<int>(NoteColumnModel::DataRole::IsFocused)).toBool());
}

void NoteColumnModelHandlerTest::test_patternPeek_middlePosition_shouldPreviewPlayOrderNeighbors()
{
    const auto automationService { std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };
    const auto selectionService { std::make_shared<SelectionService>() };
    const auto settingsService { std::make_shared<SettingsService>() };
    const auto editorService { std::make_shared<EditorService>(selectionService, settingsService, std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>()) };

    settingsService->setPatternPeekEnabled(true);

    NoteColumnModelHandler handler { editorService, selectionService, automationService, settingsService };
    setupThreePatternSong(*editorService);

    editorService->setSongPosition(1); // Current pattern is 1; play order neighbors are patterns 0 and 2

    auto model { handler.columnModel(1, 0, 0) };
    QVERIFY(model);

    const int barLine = static_cast<int>(editorService->positionBarLine());
    QVERIFY(barLine >= 1);
    const int previousLineCount = static_cast<int>(editorService->lineCount(0));
    const int currentLineCount = static_cast<int>(editorService->lineCount(1));

    // The row directly above line 0 shows the tail of the previous neighbor (pattern 0)
    const auto topGhost = model->index(barLine - 1, 0);
    QVERIFY(model->data(topGhost, ghostRole()).toBool());
    QVERIFY(model->data(topGhost, noteRole()).toString() != editorService->noDataString());
    QCOMPARE(model->data(topGhost, noteRole()).toString(), editorService->displayNoteAtPosition(0, 0, 0, static_cast<quint64>(previousLineCount - 1)));

    // The row directly below the last line shows the head of the next neighbor (pattern 2)
    const auto bottomGhost = model->index(barLine + currentLineCount, 0);
    QVERIFY(model->data(bottomGhost, ghostRole()).toBool());
    QVERIFY(model->data(bottomGhost, noteRole()).toString() != editorService->noDataString());
    QCOMPARE(model->data(bottomGhost, noteRole()).toString(), editorService->displayNoteAtPosition(2, 0, 0, 0));
}

void NoteColumnModelHandlerTest::test_patternPeek_songBoundaries_shouldStayBlack()
{
    const auto automationService { std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };
    const auto selectionService { std::make_shared<SelectionService>() };
    const auto settingsService { std::make_shared<SettingsService>() };
    const auto editorService { std::make_shared<EditorService>(selectionService, settingsService, std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>()) };

    settingsService->setPatternPeekEnabled(true);

    NoteColumnModelHandler handler { editorService, selectionService, automationService, settingsService };
    setupThreePatternSong(*editorService);

    const int barLine = static_cast<int>(editorService->positionBarLine());
    QVERIFY(barLine >= 1);

    // First position: there is no previous neighbor, so the top offset area stays black
    editorService->setSongPosition(0);
    auto firstModel { handler.columnModel(0, 0, 0) };
    const auto topRow = firstModel->index(barLine - 1, 0);
    QVERIFY(!firstModel->data(topRow, ghostRole()).toBool());
    QVERIFY(firstModel->data(topRow, virtualRole()).toBool());
    QCOMPARE(firstModel->data(topRow, lineRole()).toString(), QString { "" });
    // ...but the next neighbor (pattern 1) still peeks at the bottom
    const auto firstBottomRow = firstModel->index(barLine + static_cast<int>(editorService->lineCount(0)), 0);
    QVERIFY(firstModel->data(firstBottomRow, ghostRole()).toBool());

    // Last position: there is no next neighbor, so the bottom offset area stays black
    editorService->setSongPosition(2);
    auto lastModel { handler.columnModel(2, 0, 0) };
    const auto lastBottomRow = lastModel->index(barLine + static_cast<int>(editorService->lineCount(2)), 0);
    QVERIFY(!lastModel->data(lastBottomRow, ghostRole()).toBool());
    QVERIFY(lastModel->data(lastBottomRow, virtualRole()).toBool());
    QCOMPARE(lastModel->data(lastBottomRow, lineRole()).toString(), QString { "" });
}

void NoteColumnModelHandlerTest::test_patternPeek_disabled_shouldStayBlack()
{
    const auto automationService { std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };
    const auto selectionService { std::make_shared<SelectionService>() };
    const auto settingsService { std::make_shared<SettingsService>() };
    const auto editorService { std::make_shared<EditorService>(selectionService, settingsService, std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>()) };

    settingsService->setPatternPeekEnabled(false);

    NoteColumnModelHandler handler { editorService, selectionService, automationService, settingsService };
    setupThreePatternSong(*editorService);

    editorService->setSongPosition(1);

    auto model { handler.columnModel(1, 0, 0) };
    const int barLine = static_cast<int>(editorService->positionBarLine());
    QVERIFY(barLine >= 1);
    const auto topGhost = model->index(barLine - 1, 0);

    // Disabled: the offset area stays black even between neighbors
    QVERIFY(!model->data(topGhost, ghostRole()).toBool());
    QCOMPARE(model->data(topGhost, lineRole()).toString(), QString { "" });

    // Enabling it live repaints the peek without any navigation
    settingsService->setPatternPeekEnabled(true);
    QVERIFY(model->data(topGhost, ghostRole()).toBool());
    QVERIFY(model->data(topGhost, noteRole()).toString() != editorService->noDataString());
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::NoteColumnModelHandlerTest)
