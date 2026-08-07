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

#ifndef WAVEGUIDE_STRING_TEST_HPP
#define WAVEGUIDE_STRING_TEST_HPP

#include <QObject>

namespace noteahead {

class WaveguideStringTest : public QObject
{
    Q_OBJECT

private slots:
    void test_tuning_shouldMatchNoteFrequency_acrossMidiRange();
    void test_tuning_shouldMatchNoteFrequency_atHighSampleRate();
    void test_tuning_shouldMatchNoteFrequency_whenBrightnessVaries();
    void test_tuning_shouldMatchNoteFrequency_whenInharmonicityApplied();
    void test_tuning_shouldFollowDetune_atSubSemitoneResolution();
    void test_tuning_shouldRecover_whenSampleRateChangesAfterTrigger();
};

} // namespace noteahead

#endif // WAVEGUIDE_STRING_TEST_HPP
