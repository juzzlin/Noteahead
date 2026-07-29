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

#ifndef DIVIDE_DOWN_GENERATOR_TEST_HPP
#define DIVIDE_DOWN_GENERATOR_TEST_HPP

#include <QObject>

namespace noteahead {

class DivideDownGeneratorTest : public QObject
{
    Q_OBJECT

private slots:
    void test_saw_octaveTaps_shouldStayPhaseLocked();
    void test_saw_octaveTaps_shouldDoubleTheFrequency();
    void test_saw_note_shouldMatchEqualTemperament();
    void test_saw_defaultSampleRate_shouldStillAdvance();
    void test_saw_aboveNyquist_shouldBeSilent();
    void test_saw_highOctave_shouldStayBandLimited();
    void test_saw_shouldStayBounded();
    void test_reset_shouldRestartAllPhases();
};

} // namespace noteahead

#endif // DIVIDE_DOWN_GENERATOR_TEST_HPP
