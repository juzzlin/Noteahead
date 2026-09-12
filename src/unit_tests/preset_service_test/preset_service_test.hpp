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

#ifndef PRESET_SERVICE_TEST_HPP
#define PRESET_SERVICE_TEST_HPP

#include <QObject>

namespace noteahead {

class PresetServiceTest : public QObject
{
    Q_OBJECT

private slots:
    void test_saveUserPreset_shouldRoundTripParameters();
    void test_saveUserPreset_shouldNotifyOfTheChange();
    void test_saveUserPreset_emptyName_shouldSaveNothing();
    void test_saveUserPreset_sameName_shouldReplaceTheStoredPreset();
    void test_saveUserPreset_nameWithPathCharacters_shouldKeepTheNameAsGiven();
    void test_userPresetNames_shouldBeSortedCaseInsensitively();
    void test_userPresetNames_otherType_shouldNotBeListed();
    void test_applyUserPreset_shouldDefaultWhatThePresetDoesNotName();
    void test_applyUserPreset_otherType_shouldChangeNothing();
    void test_applyUserPreset_missing_shouldFailWithoutThrowing();
    void test_applyUserPreset_garbageFile_shouldFailWithoutThrowing();
    void test_deleteUserPreset_shouldRemoveItFromTheListing();
    void test_deleteUserPreset_missing_shouldFail();
};

} // namespace noteahead

#endif // PRESET_SERVICE_TEST_HPP
