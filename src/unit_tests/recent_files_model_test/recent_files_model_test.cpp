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

#include "recent_files_model_test.hpp"

#include "../../application/models/recent_files_model.hpp"

#include <QSignalSpy>
#include <QTemporaryFile>
#include <QTest>

namespace noteahead {

void RecentFilesModelTest::test_initialState_shouldBeEmpty()
{
    const RecentFilesModel model {};
    QCOMPARE(model.rowCount(), 0);
}

void RecentFilesModelTest::test_setRecentFiles_shouldUpdateRowCount()
{
    RecentFilesModel model {};
    const QStringList files { "file1.nahd", "file2.nahd" };
    model.setRecentFiles(files);
    QCOMPARE(model.rowCount(), 2);
}

void RecentFilesModelTest::test_data_shouldReturnCorrectValues()
{
    RecentFilesModel model {};
    const QStringList files { "non_existent_file.nahd" };
    model.setRecentFiles(files);

    const auto index { model.index(0, 0) };
    QCOMPARE(model.data(index, static_cast<int>(RecentFilesModel::Role::FilePath)).toString(), QString { "non_existent_file.nahd" });
    QCOMPARE(model.data(index, static_cast<int>(RecentFilesModel::Role::Exists)).toBool(), false);
}

void RecentFilesModelTest::test_filter_everyWord_shouldMatchInAnyOrderAndCase()
{
    RecentFilesModel model;
    model.setRecentFiles({ "/music/2026/Massive/Massive.nahd", "/music/2025/Drift/Drift.nahd", "/music/2026/Sketch.nahd" });

    model.setFilter("massive");
    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(model.data(model.index(0), static_cast<int>(RecentFilesModel::Role::FilePath)).toString(), QString { "/music/2026/Massive/Massive.nahd" });

    // Words are matched separately, so a folder and a name can be typed in either order
    model.setFilter("sketch 2026");
    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(model.data(model.index(0), static_cast<int>(RecentFilesModel::Role::FilePath)).toString(), QString { "/music/2026/Sketch.nahd" });

    model.setFilter("2026");
    QCOMPARE(model.rowCount(), 2);

    model.setFilter("nothing");
    QCOMPARE(model.rowCount(), 0);
}

void RecentFilesModelTest::test_filter_empty_shouldListEveryFile()
{
    RecentFilesModel model;
    model.setRecentFiles({ "/a.nahd", "/b.nahd" });
    model.setFilter("a.nahd");
    QCOMPARE(model.rowCount(), 1);

    QSignalSpy spy { &model, &RecentFilesModel::filterChanged };
    model.setFilter("  ");
    QCOMPARE(model.rowCount(), 2);
    model.setFilter("");
    QCOMPARE(model.rowCount(), 2);
    QCOMPARE(spy.count(), 2);
}

void RecentFilesModelTest::test_filter_setRecentFiles_shouldStayApplied()
{
    // The list is replaced whenever a project is opened or saved, the filter in the field must still hold
    RecentFilesModel model;
    model.setFilter("keep");
    model.setRecentFiles({ "/keep.nahd", "/drop.nahd" });
    QCOMPARE(model.rowCount(), 1);
}

void RecentFilesModelTest::test_firstExistingFile_shouldSkipMissingFiles()
{
    QTemporaryFile file;
    QVERIFY(file.open());
    RecentFilesModel model;
    model.setRecentFiles({ "/no/such/file.nahd", file.fileName() });

    QCOMPARE(model.firstExistingFile(), file.fileName());

    model.setFilter("such");
    QCOMPARE(model.firstExistingFile(), QString {});
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::RecentFilesModelTest)
