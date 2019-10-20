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

#ifndef UPSAMPLER_TEST_HPP
#define UPSAMPLER_TEST_HPP

#include <QObject>

namespace noteahead {

class UpsamplerTest : public QObject
{
    Q_OBJECT

private slots:
    void test_process_factorOne_shouldPassThrough();
    void test_process_dcInput_shouldPreserveLevel();
    void test_roundTrip_factorTwo_lowFrequency_shouldReconstruct();
    void test_roundTrip_factorFour_lowFrequency_shouldReconstruct();
    void test_clampOversampleFactor_shouldAllowSupportedValues();
    void test_clampOversampleFactor_shouldFallBackToTwo();
    void test_decimator_factorOne_shouldPassThrough();
    void test_decimator_dcInput_shouldPreserveLevel();
    void test_decimator_audioBand_shouldNotAttenuateWithOversampling();
    void test_decimator_aboveBaseNyquist_shouldRejectAliases();
    void test_noiseGain_shouldKeepInBandNoiseLevelConstant();
};

} // namespace noteahead

#endif // UPSAMPLER_TEST_HPP
