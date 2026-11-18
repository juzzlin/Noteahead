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

#include "../../application/models/note_column_line_container_helper.hpp"
#include "../../application/models/note_column_model.hpp"
#include "../../application/service/automation_service.hpp"
#include "../../application/service/editor_service.hpp"
#include "../../application/service/property_service.hpp"
#include "../../application/service/selection_service.hpp"
#include "../../application/service/settings_service.hpp"
#include "../../application/service/util_service.hpp"
#include "../../domain/tracker/line.hpp"
#include "../../domain/tracker/note_data.hpp"
#include "../../infra/data_service.hpp"

#include <QSettings>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

#include <algorithm>

namespace noteahead {

namespace {

// Keeps the settings written by this test process out of the user scope shared by all
// processes. Without it, concurrent runs of this binary (parallel CI workspaces) race on the
// same settings file.
QTemporaryDir & settingsDirectory()
{
    static QTemporaryDir directory;
    return directory;
}

struct TestContext
{
    std::shared_ptr<AutomationService> automationService;
    std::shared_ptr<SelectionService> selectionService;
    std::shared_ptr<SettingsService> settingsService;
    std::shared_ptr<EditorService> editorService;
    std::shared_ptr<UtilService> utilService;
    std::shared_ptr<NoteColumnLineContainerHelper> helper;
    NoteColumnModel model;
};

TestContext createContext(size_t pattern = 0)
{
    const auto automationService = std::make_shared<AutomationService>(std::make_shared<PropertyService>());
    const auto selectionService = std::make_shared<SelectionService>();
    const auto settingsService = std::make_shared<SettingsService>();
    const auto editorService = std::make_shared<EditorService>(selectionService, settingsService, std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>());
    const auto utilService = std::make_shared<UtilService>();
    const auto helper = std::make_shared<NoteColumnLineContainerHelper>(automationService, editorService, selectionService, settingsService, utilService);
    return TestContext {
        automationService,
        selectionService,
        settingsService,
        editorService,
        utilService,
        helper,
        NoteColumnModel { { pattern, 0, 0 }, editorService, helper, settingsService }
    };
}

} // namespace

void NoteColumnModelTest::initTestCase()
{
    QCoreApplication::setOrganizationName("NoteaheadTest");
    QCoreApplication::setApplicationName("NoteColumnModelTest");

    QVERIFY(settingsDirectory().isValid());
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settingsDirectory().path());
}

void NoteColumnModelTest::cleanupTestCase()
{
    QSettings settings {};
    settings.clear();
}

void NoteColumnModelTest::test_rowCount_shouldReturnCorrectValue()
{
    auto context = createContext();

    NoteColumnModel::LineList lines;
    for (int i = 0; i < 10; ++i) {
        lines.push_back(std::make_shared<Line>(static_cast<size_t>(i)));
    }
    context.model.setColumnData(lines);

    QCOMPARE(context.model.rowCount(), 10 + context.settingsService->visibleLines());
}

void NoteColumnModelTest::test_data_shouldReturnCorrectValues()
{
    auto context = createContext();

    auto line = std::make_shared<Line>(0);
    NoteData noteData {};
    noteData.setAsNoteOn(60, 100); // C-4
    line->setNoteData(noteData);

    NoteColumnModel::LineList lines { line };
    context.model.setColumnData(lines);

    const auto idx = context.model.index(context.editorService->positionBarLine(), 0);
    QCOMPARE(context.model.data(idx, static_cast<int>(NoteColumnModel::DataRole::Note)).toString(), QString { "C-5" });
    QCOMPARE(context.model.data(idx, static_cast<int>(NoteColumnModel::DataRole::Velocity)).toString(), QString { "100" });
}

void NoteColumnModelTest::test_data_LineRole_shouldReturnCorrectValue()
{
    auto context = createContext();

    auto line = std::make_shared<Line>(0);
    NoteData noteData {};
    noteData.setAsNoteOn(60, 100); // C-5
    noteData.setDelay(12);
    line->setNoteData(noteData);

    NoteColumnModel::LineList lines { line, std::make_shared<Line>(1) };
    context.model.setColumnData(lines);

    const int barLine = static_cast<int>(context.editorService->positionBarLine());

    // Test filled line
    const auto idx0 = context.model.index(barLine, 0);
    QCOMPARE(context.model.data(idx0, static_cast<int>(NoteColumnModel::DataRole::Line)).toString(), QString { "C-5 100 12 ---" });

    // Test empty line
    const auto idx1 = context.model.index(barLine + 1, 0);
    QCOMPARE(context.model.data(idx1, static_cast<int>(NoteColumnModel::DataRole::Line)).toString(), QString { "--- --- -- ---" });
}

void NoteColumnModelTest::test_data_GhostRow_shouldReturnNeighborLines()
{
    auto context = createContext();

    const int barLine = static_cast<int>(context.editorService->positionBarLine());
    QVERIFY(barLine >= 1); // The top offset area requires at least one virtual row above line 0

    // Two lines in the current column
    context.model.setColumnData({ std::make_shared<Line>(0), std::make_shared<Line>(1) });

    // Previous neighbor: two lines, the last one directly abutting the current line 0
    auto previousTail = std::make_shared<Line>(1);
    NoteData previousNote {};
    previousNote.setAsNoteOn(62, 100); // D-5
    previousTail->setNoteData(previousNote);

    // Next neighbor: a single head line directly abutting the current last line
    auto nextHead = std::make_shared<Line>(0);
    NoteData nextNote {};
    nextNote.setAsNoteOn(64, 100); // E-5
    nextHead->setNoteData(nextNote);

    context.model.setGhostData({ std::make_shared<Line>(0), previousTail }, { nextHead });

    // The row directly above line 0 shows the tail of the previous neighbor as a ghost
    const auto topGhost = context.model.index(barLine - 1, 0);
    QCOMPARE(context.model.data(topGhost, static_cast<int>(NoteColumnModel::DataRole::IsGhostRow)).toBool(), true);
    QCOMPARE(context.model.data(topGhost, static_cast<int>(NoteColumnModel::DataRole::IsVirtualRow)).toBool(), true);
    QCOMPARE(context.model.data(topGhost, static_cast<int>(NoteColumnModel::DataRole::Note)).toString(), QString { "D-5" });

    // The row directly below the last line shows the head of the next neighbor as a ghost
    const auto bottomGhost = context.model.index(barLine + 2, 0);
    QCOMPARE(context.model.data(bottomGhost, static_cast<int>(NoteColumnModel::DataRole::IsGhostRow)).toBool(), true);
    QCOMPARE(context.model.data(bottomGhost, static_cast<int>(NoteColumnModel::DataRole::Note)).toString(), QString { "E-5" });

    // Beyond the neighbor's lines the offset area stays a plain (black) virtual row
    const auto beyondGhost = context.model.index(barLine + 3, 0);
    QCOMPARE(context.model.data(beyondGhost, static_cast<int>(NoteColumnModel::DataRole::IsGhostRow)).toBool(), false);
    QCOMPARE(context.model.data(beyondGhost, static_cast<int>(NoteColumnModel::DataRole::IsVirtualRow)).toBool(), true);
    QCOMPARE(context.model.data(beyondGhost, static_cast<int>(NoteColumnModel::DataRole::Line)).toString(), QString { "" });
}

void NoteColumnModelTest::test_updateNoteDataAtPosition_shouldEmitDataChangedWithCorrectRoles()
{
    auto context = createContext();

    NoteColumnModel::LineList lines { std::make_shared<Line>(0) };
    context.model.setColumnData(lines);

    QSignalSpy spy { &context.model, &NoteColumnModel::dataChanged };
    context.model.updateNoteDataAtPosition(0);

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
    auto context = createContext();

    NoteColumnModel::LineList lines { std::make_shared<Line>(0) };
    context.model.setColumnData(lines);

    QSignalSpy spy { &context.model, &NoteColumnModel::dataChanged };
    context.model.setLineFocused(0, 1);

    QCOMPARE(spy.count(), 1);
    const auto idx = context.model.index(context.editorService->positionBarLine(), 0);
    QCOMPARE(context.model.data(idx, static_cast<int>(NoteColumnModel::DataRole::IsFocused)).toBool(), true);
    QCOMPARE(context.model.data(idx, static_cast<int>(NoteColumnModel::DataRole::LineColumn)).toInt(), 1);
}

void NoteColumnModelTest::test_updateIndexHighlights_shouldEmitDataChangedWithCorrectRange()
{
    auto context = createContext();

    const int lineCount = 10;
    NoteColumnModel::LineList lines;
    for (int i = 0; i < lineCount; ++i) {
        lines.push_back(std::make_shared<Line>(static_cast<size_t>(i)));
    }
    context.model.setColumnData(lines);

    QSignalSpy spy { &context.model, &NoteColumnModel::dataChanged };
    context.model.updateIndexHighlights();

    QCOMPARE(spy.count(), 1);
    const auto arguments = spy.takeFirst();
    const auto topLeft = arguments.at(0).value<QModelIndex>();
    const auto bottomRight = arguments.at(1).value<QModelIndex>();

    const int barLine = static_cast<int>(context.editorService->positionBarLine());
    QCOMPARE(topLeft.row(), barLine);
    QCOMPARE(bottomRight.row(), barLine + lineCount - 1);
}

void NoteColumnModelTest::test_automationCurves_singleLineAutomation_shouldSetOneValue()
{
    auto context = createContext();

    const int lineCount = 10;
    NoteColumnModel::LineList lines;
    for (int i = 0; i < lineCount; ++i) {
        lines.push_back(std::make_shared<Line>(static_cast<size_t>(i)));
    }
    context.model.setColumnData(lines);

    // An automation that starts and ends on the same line still has to reach the renderer: it is the
    // only value it will ever get, and the renderer draws it as a mark rather than as a polyline.
    const int automationLine = 4;
    context.automationService->addMidiCcAutomation(0, 0, 0, 64, automationLine, automationLine, 0, 127, {}, true, 8, 0);

    const int startRow = 0, endRow = 20;
    const auto curves = context.model.automationCurves(startRow, endRow);

    QCOMPARE(curves.size(), size_t { 1 });
    const auto & values = curves.at(0).values;
    QCOMPARE(std::ranges::count_if(values, [](auto && value) { return value.has_value(); }), 1);

    // Row space, not line space: the model shifts the lines down by the position bar
    const auto barLine = static_cast<int>(context.editorService->positionBarLine());
    QVERIFY(values.at(static_cast<size_t>(automationLine + barLine)).has_value());
}

void NoteColumnModelTest::test_automationCurves_ghostRows_shouldDrawTheNeighborPatterns()
{
    // The offset areas peek at the play order neighbors, and an automation belongs to a pattern as
    // much as its notes do. They used to show the notes and drop the curves, because the curves were
    // clamped to the pattern being edited.
    auto context = createContext(1);

    const auto barLine = static_cast<int>(context.editorService->positionBarLine());
    QVERIFY(barLine >= 2);

    const int lineCount = 4;
    const int neighborLineCount = 4;
    NoteColumnModel::LineList lines;
    NoteColumnModel::LineList neighborLines;
    for (int i = 0; i < lineCount; i++) {
        lines.push_back(std::make_shared<Line>(static_cast<size_t>(i)));
    }
    for (int i = 0; i < neighborLineCount; i++) {
        neighborLines.push_back(std::make_shared<Line>(static_cast<size_t>(i)));
    }
    context.model.setColumnData(lines);
    context.model.setGhostData(neighborLines, neighborLines, 0, 2);

    // One automation on each pattern: the last line of the one above, the first of the one below.
    context.automationService->addMidiCcAutomation(0, 0, 0, 64, neighborLineCount - 1, neighborLineCount - 1, 0, 127, {}, true, 8, 0);
    context.automationService->addMidiCcAutomation(2, 0, 0, 64, 0, 0, 0, 127, {}, true, 8, 0);

    const int startRow = 0;
    const int endRow = barLine + lineCount + 4;
    const auto curves = context.model.automationCurves(startRow, endRow);

    QCOMPARE(curves.size(), size_t { 2 });
    QVERIFY(std::ranges::all_of(curves, [](auto && curve) { return curve.isGhost; }));

    // The row above line 0 is the neighbor's last line, and the row below the last line its first
    QVERIFY(curves.at(0).values.at(static_cast<size_t>(barLine - 1)).has_value());
    QVERIFY(curves.at(1).values.at(static_cast<size_t>(barLine + lineCount)).has_value());
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::NoteColumnModelTest)
