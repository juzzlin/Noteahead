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

#ifndef RESONANCE_TAMER_TEST_HPP
#define RESONANCE_TAMER_TEST_HPP

#include <QObject>

namespace noteahead {

class ResonanceTamerTest : public QObject
{
    Q_OBJECT

private slots:
    void test_processBlock_sineOverNoise_shouldCutTheSineByDepth();
    void test_processBlock_sineOverNoise_shouldLeaveTheNoiseAlone();
    void test_processBlock_pinkNoise_shouldBeLeftAlone();
    void test_processBlock_sineOutsideRange_shouldBeLeftAlone();
    void test_processBlock_zeroDepth_shouldBeTransparent();
    void test_processBlock_sweptSine_shouldFollowIt();
    void test_processBlock_stereoMode_shouldCutOnlyTheRingingSide();
    void test_process_solo_shouldBeWhatIsTakenOut();
    void test_latencyFrames_shouldBeZero();
    void test_factoryPresets_shouldAllProduceFiniteOutput();
};

} // namespace noteahead

#endif // RESONANCE_TAMER_TEST_HPP
