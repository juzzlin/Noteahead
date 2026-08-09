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

#ifndef BASS_GRINDER_TEST_HPP
#define BASS_GRINDER_TEST_HPP

#include <QObject>

namespace noteahead {

class BassGrinderTest : public QObject
{
    Q_OBJECT

private slots:
    void test_mix_zero_shouldPassSignalThrough();
    void test_blend_zero_shouldLeaveSignalClean();
    void test_split_high_shouldLeaveLowBandUndistorted();
    void test_split_low_shouldDistortTheWholeBand();
    void test_drive_higher_shouldSaturateMore();
    void test_clipper_shouldGenerateEvenHarmonics();
    void test_output_shouldNotDriftToDc();
    void test_color_on_shouldScoopMids();
    void test_bassGain_shouldBoostLowEnd();
    void test_highGain_shouldBoostTopEnd();
    void test_midFreq_shouldMoveTheBell();
    void test_gain_shouldScaleOutput();
    void test_oversampling_shouldSuppressAliasing();
};

} // namespace noteahead

#endif // BASS_GRINDER_TEST_HPP
