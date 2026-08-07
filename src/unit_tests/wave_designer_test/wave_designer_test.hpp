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

#ifndef WAVE_DESIGNER_TEST_HPP
#define WAVE_DESIGNER_TEST_HPP

#include <QObject>

namespace noteahead {

class WaveDesignerTest : public QObject
{
    Q_OBJECT

private slots:
    void test_centred_shouldPassSignalThrough();
    void test_attack_positive_shouldEmphasiseTheLeadingEdge();
    void test_attack_negative_shouldTameTheLeadingEdge();
    void test_sustain_positive_shouldHoldTheTailUp();
    void test_sustain_negative_shouldShortenTheTail();
    void test_steadyTone_shouldNotBeShaped();
    void test_shaping_shouldBeLevelIndependent();
    void test_gain_shouldScaleOutput();
    void test_mix_zero_shouldPassSignalThrough();
    void test_reset_shouldClearFollowers();
};

} // namespace noteahead

#endif // WAVE_DESIGNER_TEST_HPP
