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

#include "note_column_renderer_test.hpp"

#include "../../application/models/note_column_line_container_helper.hpp"
#include "../../application/models/note_column_model.hpp"
#include "../../application/service/automation_service.hpp"
#include "../../application/service/editor_service.hpp"
#include "../../application/service/property_service.hpp"
#include "../../application/service/selection_service.hpp"
#include "../../application/service/settings_service.hpp"
#include "../../application/service/util_service.hpp"
#include "../../common/constants.hpp"
#include "../../domain/tracker/line.hpp"
#include "../../infra/data_service.hpp"
#include "../../view/qml/Editor/note_column_renderer.hpp"

#include <QImage>
#include <QPainter>
#include <QTest>

#include <set>

namespace noteahead {

using Values = std::vector<std::optional<double>>;
using Runs = std::vector<std::pair<size_t, size_t>>;

namespace {

constexpr int imageWidth = 80;
constexpr int imageHeight = 320;
constexpr int visibleLines = 32;
constexpr int rowHeight = imageHeight / visibleLines;

//! Rows carrying a pixel of the curve colour, after painting one automation over the given lines.
//!
//! Painted into an image rather than asserted through the geometry, because the defect this guards
//! against was a run of one point producing a perfectly valid but entirely invisible polyline.
std::set<int> paintedCurveRows(quint64 line0, quint64 line1)
{
    const auto automationService { std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };
    const auto selectionService { std::make_shared<SelectionService>() };
    const auto settingsService { std::make_shared<SettingsService>() };
    const auto editorService { std::make_shared<EditorService>(selectionService, settingsService, std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>()) };
    const auto utilService { std::make_shared<UtilService>() };
    const auto helper { std::make_shared<NoteColumnLineContainerHelper>(automationService, editorService, selectionService, settingsService, utilService) };

    NoteColumnModel model { { 0, 0, 0 }, editorService, helper, settingsService };
    NoteColumnModel::LineList lines;
    for (int i = 0; i < 10; i++) {
        lines.push_back(std::make_shared<Line>(static_cast<size_t>(i)));
    }
    model.setColumnData(lines);

    // Flat at mid level, so the trace sits well inside the column and away from the margins
    automationService->addMidiCcAutomation(0, 0, 0, 64, line0, line1, 64, 64, {}, true, 8, 0);

    NoteColumnRenderer renderer;
    renderer.setWidth(imageWidth);
    renderer.setHeight(imageHeight);
    renderer.setVisibleLines(visibleLines);
    renderer.setModel(&model);
    renderer.setAutomationDisplayMode(static_cast<int>(Constants::AutomationDisplayMode::Curve));
    renderer.setAutomationCurveColors(QVariantList { QColor { "#00ff00" } });

    QImage image { imageWidth, imageHeight, QImage::Format_ARGB32 };
    image.fill(Qt::black);
    QPainter painter { &image };
    renderer.paint(&painter);
    painter.end();

    // The note text is drawn in greys, so only a pixel that is green and nothing else is the curve
    std::set<int> rows;
    for (int y = 0; y < image.height(); y++) {
        for (int x = 0; x < image.width(); x++) {
            const auto pixel = image.pixel(x, y);
            if (qGreen(pixel) > 60 && qRed(pixel) < 60 && qBlue(pixel) < 60) {
                rows.insert(y / rowHeight);
            }
        }
    }
    return rows;
}

//! Row the given line lands on: the model shifts the lines down by the position bar.
int rowForLine(int line)
{
    const EditorService editorService { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(),
                                        std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    return line + static_cast<int>(editorService.positionBarLine());
}

} // namespace

void NoteColumnRendererTest::test_valueRuns_allUnset_shouldFindNothing()
{
    QVERIFY(NoteColumnRenderer::valueRuns(Values(4)).empty());
}

void NoteColumnRendererTest::test_valueRuns_allSet_shouldFindOneRun()
{
    const Values values { 0.0, 0.25, 0.5, 1.0 };

    const auto runs = NoteColumnRenderer::valueRuns(values);

    QCOMPARE(runs, (Runs { { 0, 3 } }));
}

void NoteColumnRendererTest::test_valueRuns_singleValue_shouldFindRunOfOne()
{
    // What a single-line automation collapses to. It has no segment to draw, so the renderer has to
    // tell it apart from a longer run instead of handing it to drawPolyline, which would paint nothing.
    const Values values { std::nullopt, 0.5, std::nullopt };

    const auto runs = NoteColumnRenderer::valueRuns(values);

    QCOMPARE(runs, (Runs { { 1, 1 } }));
}

void NoteColumnRendererTest::test_valueRuns_singleValueAtStart_shouldFindRunOfOne()
{
    // A longer automation scrolled so that only its last row is on screen clips to this
    const Values values { 0.5, std::nullopt, std::nullopt };

    const auto runs = NoteColumnRenderer::valueRuns(values);

    QCOMPARE(runs, (Runs { { 0, 0 } }));
}

void NoteColumnRendererTest::test_valueRuns_singleValueAtEnd_shouldFindRunOfOne()
{
    const Values values { std::nullopt, std::nullopt, 0.5 };

    const auto runs = NoteColumnRenderer::valueRuns(values);

    QCOMPARE(runs, (Runs { { 2, 2 } }));
}

void NoteColumnRendererTest::test_valueRuns_gap_shouldSplitIntoRuns()
{
    // The gap has to break the run, so a stretch the automation does not reach stays a gap on screen
    const Values values { 0.0, 0.5, std::nullopt, 1.0, 0.75 };

    const auto runs = NoteColumnRenderer::valueRuns(values);

    QCOMPARE(runs, (Runs { { 0, 1 }, { 3, 4 } }));
}

void NoteColumnRendererTest::test_valueRuns_empty_shouldFindNothing()
{
    QVERIFY(NoteColumnRenderer::valueRuns({}).empty());
}

void NoteColumnRendererTest::test_paint_singleLineAutomation_shouldDrawMarkOnItsRow()
{
    const auto rows = paintedCurveRows(4, 4);

    // An automation on one line used to paint nothing: one point makes no polyline segment
    QCOMPARE(rows, (std::set<int> { rowForLine(4) }));
}

void NoteColumnRendererTest::test_paint_multiLineAutomation_shouldDrawTrace()
{
    const auto rows = paintedCurveRows(4, 7);

    QCOMPARE(*rows.begin(), rowForLine(4));
    QCOMPARE(*rows.rbegin(), rowForLine(7));
}

} // namespace noteahead

QTEST_MAIN(noteahead::NoteColumnRendererTest)
