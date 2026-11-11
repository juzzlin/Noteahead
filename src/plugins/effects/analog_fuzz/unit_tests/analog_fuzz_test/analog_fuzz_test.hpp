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

#ifndef ANALOG_FUZZ_TEST_HPP
#define ANALOG_FUZZ_TEST_HPP

#include <QObject>

namespace noteahead {

class AnalogFuzzTest : public QObject
{
    Q_OBJECT

private slots:
    void test_oversampling_shouldNotChangeTheLevel();
    void test_mixZero_shouldPassSignalThrough();
    void test_drive_higher_shouldSaturateMore();
    void test_moderateDrive_shouldAudiblyDistort();
    void test_lowDrive_shouldPassTheLevelThrough();
    void test_drive_shouldNotRunAwayInLevel();
    void test_fuzz_harderKnee_shouldGenerateMoreHarmonics();
    void test_cutoff_shouldRemoveWhatIsAboveIt();
    void test_resonance_shouldLiftTheCorner();
    void test_resonance_underDrive_shouldGiveWay();
    void test_bias_offCentre_shouldIncreaseEvenHarmonics();
    void test_output_shouldNotDriftToDc();
    void test_gain_shouldScaleOutput();
    void test_oversampling_shouldSuppressAliasing();
};

} // namespace noteahead

#endif // ANALOG_FUZZ_TEST_HPP
