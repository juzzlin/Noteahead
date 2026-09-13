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

#ifndef FM_SYNTH_CONTROLLER_TEST_HPP
#define FM_SYNTH_CONTROLLER_TEST_HPP

#include <QObject>

namespace noteahead {

class FmSynthControllerTest : public QObject
{
    Q_OBJECT

private slots:
    void test_properties_shouldUpdateDevice();
    void test_properties_shouldEmitSignals();
    void test_everyContinuousProperty_shouldRoundTripThroughTheDevice();
    void test_everyContinuousProperty_shouldEmitItsOwnSignal();
    void test_deviceChange_shouldRefreshProperties();
    void test_reset_shouldRestoreDefaultValues();

    void test_presetNames_shouldBeNumberedLikeTheSynths();
    void test_operators_shouldExposeOneControllerPerOperator();
    void test_operatorProperties_shouldUpdateDevice();
    void test_operatorProperties_shouldAddressTheRightOperator();
    void test_operators_deviceChange_shouldFollowTheNewDevice();
    void test_operatorRatioText_shouldReadAsTheRatio();

    void test_operatorRouting_shouldFollowTheAlgorithm();
    void test_operatorRouting_shouldOnlyNameHigherOperators();
    void test_operatorRouting_everyOperator_shouldReachTheOutput();
    void test_feedbackOperator_shouldBeTheLastOne();

    void test_voiceModes_shouldMatchThePersistedOrder();
    void test_voiceMode_everyOfferedMode_shouldReachTheDevice();
    void test_modTarget_everyOfferedTarget_shouldReachTheDevice();
    void test_lfoTarget_everyOfferedTarget_shouldReachTheDevice();
    void test_algorithmNames_everyOfferedAlgorithm_shouldReachTheDevice();
    void test_ratioNames_shouldCoverEveryRatioSetting();
    void test_operatorWaveformNames_shouldCoverEveryWaveform();
};

} // namespace noteahead

#endif // FM_SYNTH_CONTROLLER_TEST_HPP
