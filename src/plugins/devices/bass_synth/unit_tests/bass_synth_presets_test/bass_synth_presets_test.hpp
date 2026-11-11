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

#ifndef BASS_SYNTH_PRESETS_TEST_HPP
#define BASS_SYNTH_PRESETS_TEST_HPP

#include <QObject>

namespace noteahead {

class BassSynthPresetsTest : public QObject
{
    Q_OBJECT

private slots:
    void test_presets_shouldAllBeNamed();
    void test_presets_afterInit_shouldBeAlphabetical();
    void test_presets_everyOne_shouldNameOnlyRealParameters();
    void test_presets_everyOne_shouldSound();
    void test_presets_everyOne_shouldStayWithinFullScale();
    void test_presets_everyOne_shouldStayWithinAUsableLevelRange();

    void test_loadPreset_shouldReplaceTheWholePanel();
    void test_loadPreset_outOfRange_shouldChangeNothing();
    void test_programChange_shouldNotTouchTheAuthoredPatch();
};

} // namespace noteahead

#endif // BASS_SYNTH_PRESETS_TEST_HPP
