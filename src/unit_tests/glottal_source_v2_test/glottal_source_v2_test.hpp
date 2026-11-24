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

#ifndef GLOTTAL_SOURCE_V2_TEST_HPP
#define GLOTTAL_SOURCE_V2_TEST_HPP

#include <QObject>

namespace noteahead {

class GlottalSourceV2Test : public QObject
{
    Q_OBJECT

private slots:
    void test_pulse_shouldBeNonNegativeAndContinuous();
    void test_pulse_shouldRepeatAtFundamental();
    void test_openQuotient_shouldChangeHarmonicBalance();
    void test_returnPhase_shouldControlSpectralTilt();
    void test_highPitch_shouldAliasFarLessThanRosenberg();
    void test_openness_shouldHaveUnitMeanAcrossOpenQuotients();
    void test_subharmonicAndPerturbation_shouldProduceGrowlAndVariation();
    void test_loudnessNormalization_shouldStaySteadyAcrossShapes();
    void test_parameterChanges_shouldNotClickMidPeriod();
    void test_reset_shouldBeDeterministic();
};

} // namespace noteahead

#endif // GLOTTAL_SOURCE_V2_TEST_HPP
