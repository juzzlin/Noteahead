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

#ifndef MULTIBAND_COMPRESSOR_TEST_HPP
#define MULTIBAND_COMPRESSOR_TEST_HPP

#include <QObject>

namespace noteahead {

class MultibandCompressorTest : public QObject
{
    Q_OBJECT

private slots:
    void test_crossover_twoBands_shouldSumToUnityMagnitude();
    void test_crossover_threeBands_shouldSumToUnityMagnitude();
    void test_crossover_bandSplit_shouldSeparateByFrequency();

    void test_gain_quietSignal_shouldPassThroughAtUnityMagnitude();
    void test_gain_loudBand_shouldCompressOnlyThatBand();
    void test_gain_ratio_shouldSettleToExpectedReduction();
    void test_makeup_shouldLiftOnlyItsOwnBand();

    void test_bypass_loudBand_shouldNotApplyGain();
    void test_solo_shouldPassOnlySoloedBands();

    void test_sideChain_loudSource_shouldCompressMatchingBand();
    void test_sideChainSourceDeviceIndex_unset_shouldBeEmpty();

    void test_crossoverFrequencies_inverted_shouldKeepBandOrder();
    void test_reset_afterCompressing_shouldReturnToUnity();
};

} // namespace noteahead

#endif // MULTIBAND_COMPRESSOR_TEST_HPP
