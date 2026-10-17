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

#ifndef LFO_MODULATION_TEST_HPP
#define LFO_MODULATION_TEST_HPP

#include <QObject>

namespace noteahead {

class LfoModulationTest : public QObject
{
    Q_OBJECT

private slots:
    void test_engine_lfoOnSynthCutoff_shouldModulateIt();
    void test_engine_anyBufferSize_shouldModulateTheSame();
    void test_engine_effectTarget_shouldFollow();
    void test_engine_removingTheLfo_shouldReleaseItsTarget();
    void test_engine_targetChange_shouldReleaseTheOldTarget();
    void test_engine_stop_shouldKeepModulating();
};

} // namespace noteahead

#endif // LFO_MODULATION_TEST_HPP
