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

#ifndef ELECTRIC_BASS_TEST_HPP
#define ELECTRIC_BASS_TEST_HPP

#include <QObject>

namespace noteahead {

class ElectricBassTest : public QObject
{
    Q_OBJECT

private slots:
    void test_string_pluck_shouldSoundAtItsPitchAndDecay();
    void test_string_mute_shouldDieFasterThanHeld();
    void test_string_pitchRatio_shouldBendWithoutAJump();
    void test_device_noteOn_shouldTakeTheLowestFret();
    void test_device_noteOn_repeated_shouldReuseItsString();
    void test_device_noteOn_chord_shouldUseSeparateStrings();
    void test_device_noteOn_overlapping_shouldPlayLegatoOnTheSameString();
    void test_device_noteOff_legato_shouldPullOffToTheHeldNote();
    void test_device_pitchBend_shouldRaiseThePitch();
    void test_presets_every_shouldPeakNearInit();
};

} // namespace noteahead

#endif // ELECTRIC_BASS_TEST_HPP
