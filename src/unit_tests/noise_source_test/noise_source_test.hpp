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

#ifndef NOISE_SOURCE_TEST_HPP
#define NOISE_SOURCE_TEST_HPP

#include <QObject>

namespace noteahead {

class NoiseSourceTest : public QObject
{
    Q_OBJECT

private slots:
    void test_noiseSource_spectralSlope_shouldMatchColour_data();
    void test_noiseSource_spectralSlope_shouldMatchColour();
    void test_noiseSource_pinkSlope_highSampleRate_shouldMatchColour();
    void test_noiseSource_level_acrossColours_shouldStayConstant();
    void test_noiseSource_reset_shouldRepeatSequence();
    void test_noiseSource_seed_different_shouldDecorrelate();
    void test_noiseSource_holdRate_shouldHoldValues();
    void test_noiseSource_holdRate_none_shouldPassEverySample();

    void test_crackleSource_density_shouldMatchImpulseCount();
    void test_crackleSource_density_zero_shouldBeSilent();
};

} // namespace noteahead

#endif // NOISE_SOURCE_TEST_HPP
