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

#include "example_song_test.hpp"

#include "../../application/service/automation_service.hpp"
#include "../../application/service/editor_service.hpp"
#include "../../application/service/property_service.hpp"
#include "../../application/service/selection_service.hpp"
#include "../../application/service/settings_service.hpp"
#include "../../common/constants.hpp"
#include "../../infra/data_service.hpp"

#include <QFile>
#include <QTest>

namespace noteahead {

namespace {

EditorService makeEditorService()
{
    return EditorService { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(),
                           std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
}

} // namespace

void ExampleSongTest::test_exampleSong_shouldBeEmbeddedInTheBinary()
{
    QVERIFY2(QFile::exists(Constants::exampleSongPath()),
             "The example song is not in the binary. Check the qt_add_resources() block in src/CMakeLists.txt.");
}

void ExampleSongTest::test_exampleSong_shouldLoadWithTheCurrentFormat()
{
    // The point of this test: the example is a file committed once and then left alone, so it is
    // the first thing to rot silently as the project format moves on.
    auto editorService = makeEditorService();

    editorService.loadExample();

    QVERIFY(editorService.currentLineCount() > 0);
    QVERIFY(editorService.trackCount() > 0);
}

void ExampleSongTest::test_exampleSong_shouldStayUnnamedSoThatSaveCannotOverwriteIt()
{
    auto editorService = makeEditorService();

    editorService.loadExample();

    // QFile::exists() is true for a ":/" path, so a named example would send Save straight into a
    // read-only resource. Unnamed and unmodified, Save is disabled and Save As is the only way out.
    QVERIFY(editorService.currentFileName().isEmpty());
    QVERIFY(!editorService.isModified());
    QVERIFY(!editorService.canBeSaved());
}

void ExampleSongTest::test_exampleSong_shouldCarryMetadata()
{
    auto editorService = makeEditorService();

    editorService.loadExample();

    // It is the shipped demonstration of the metadata feature, and the window title shows it.
    QVERIFY(!editorService.songMetadataTitle().isEmpty());
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::ExampleSongTest)
