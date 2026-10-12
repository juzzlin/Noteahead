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

#ifndef DEVICE_OCTAVE_TEST_HPP
#define DEVICE_OCTAVE_TEST_HPP

#include <QObject>

namespace noteahead {

class DeviceOctaveTest : public QObject
{
    Q_OBJECT

private slots:
    void test_processMidiNoteOn_octave_shouldShiftByTwelveSemitones();
    void test_processMidiNoteOff_octaveMovedWhileHeld_shouldReleaseWhatItStarted();
    void test_processMidiNoteOff_withoutNoteOn_shouldUseTheCurrentOctave();
    void test_processMidiNoteOn_shiftedOutOfMidi_shouldDropTheNoteAndItsOff();
    void test_setOctave_outOfRange_shouldClampAndSignalOnce();
    void test_setOctave_unsupportedDevice_shouldStayAtZero();
    void test_supportsOctave_shouldLeaveOutPadsAndDrums();
    void test_serializeToXml_shouldKeepTheOctaveAndOmitZero();
    void test_loadPreset_shouldLeaveTheOctaveAlone();
    void test_controller_octave_shouldReachTheDevice();
};

} // namespace noteahead

#endif // DEVICE_OCTAVE_TEST_HPP
