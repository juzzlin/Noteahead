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

#ifndef MULTI_ENGINE_TEST_HPP
#define MULTI_ENGINE_TEST_HPP

#include <QObject>

namespace noteahead {

class MultiEngineTest : public QObject
{
    Q_OBJECT

private slots:
    void test_lowType_keyTrackZero_shouldIgnoreNote();
    void test_highType_keyTrackZero_shouldIgnoreNote();
    void test_peakType_keyTrackZero_shouldIgnoreNote();

    void test_lowType_keyTrack_shouldOpenCutoffOnHigherNote();
    void test_highType_keyTrack_shouldCloseCutoffOnHigherNote();
    void test_peakType_keyTrack_shouldShiftBandOnHigherNote();
    void test_decimType_keyTrack_shouldRaiseRateOnHigherNote();

    void test_peakType_keyTrack_shouldTrackOctaveExactly();
    void test_keyTrack_shouldNotExceedNyquist();
};

} // namespace noteahead

#endif // MULTI_ENGINE_TEST_HPP
