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

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::RecentFilesModelTest)
