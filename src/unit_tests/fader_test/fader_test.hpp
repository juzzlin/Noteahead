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

#ifndef FADER_TEST_HPP
#define FADER_TEST_HPP

#include <QObject>

namespace noteahead {

class FaderTest : public QObject
{
    Q_OBJECT

private slots:
    void test_mapFader_unityPosition_shouldGiveUnityGain();
    void test_mapFader_fullThrow_shouldGiveMaximumBoost();
    void test_mapFader_belowUnity_shouldKeepLinearAmplitudeTaper();
    void test_mapFader_zero_shouldBeSilent();
    void test_mapFader_shouldRoundTripThroughUnmap();

    void test_legacyVolume_fullScale_shouldLoadAsUnityGain();
    void test_legacyVolume_halfScale_shouldPreserveGain();
    void test_legacyVolume_zero_shouldPreserveSilence();
    void test_legacyVolume_samplerPad_shouldPreserveGain();
    void test_legacyVolume_absent_shouldDefaultToUnityGain();

    void test_fader_boostedPosition_shouldRoundTripThroughXml();
    void test_midiCc7_fullValue_shouldLandOnUnity();
    void test_midiCc7_extendedValue_shouldReachTheBoostRange();
    void test_midiCcController_fader_shouldDeclareTheExtendedRange();
};

} // namespace noteahead

#endif // FADER_TEST_HPP
