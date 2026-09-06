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

#ifndef FM_SYNTH_TEST_HPP
#define FM_SYNTH_TEST_HPP

#include <QObject>

namespace noteahead {

class FmSynthTest : public QObject
{
    Q_OBJECT

private slots:
    void test_name_shouldReturnCorrectName();
    void test_defaultValues_shouldBeCorrect();
    void test_parameterSetting_shouldUpdateValues();
    void test_operatorParameters_shouldBeIndependentPerOperator();

    void test_algorithms_everyOne_shouldOnlyModulateHigherOperators();
    void test_algorithms_everyOne_shouldHaveACarrier();
    void test_algorithms_everyOne_shouldMakeOperatorOneACarrier();
    void test_algorithms_shouldAllBeDistinct();
    void test_algorithmNames_shouldNameEveryAlgorithm();

    void test_ratioForSetting_shouldFollowTheDxGrid();

    void test_defaultPatch_shouldSoundAPlainSine();
    void test_modulatorLevel_shouldBrightenTheOutput();
    void test_modulatorLevel_shouldNotShiftThePitch();
    void test_modIndexModulation_atRest_shouldLeaveTheTimbreAlone();
    void test_modEg_shouldReachEveryTarget();
    void test_feedback_shouldBrightenTheOutput();

    void test_absSineCarrier_shouldNotOffsetTheOutput();
    void test_algorithm_additive_shouldNotBeLouderThanSerial();

    void test_keyScale_shouldQuietenTheModulatorOnHighNotes();
    void test_velocitySensitivity_shouldQuietenTheModulatorOnSoftNotes();
    void test_velocitySensitivity_zero_shouldLeaveTheModulatorAlone();

    void test_noteOn_shouldProduceAudio();
    void test_noteOff_shouldEventuallySilenceTheVoice();
    void test_ampRelease_shouldGovernTheTail_whateverTheOperatorRelease();
    void test_ampRelease_short_shouldCutALongOperatorRelease();
    void test_noteOff_shouldNotChangeTheTimbre();
    void test_allNotesOff_shouldReleaseEveryVoice();
    void test_polyphony_shouldPlayEveryVoice();
    void test_pitchBend_shouldChangeTheOutput();
    void test_voiceMode_unison_shouldDetuneTheStack();
};

} // namespace noteahead

#endif // FM_SYNTH_TEST_HPP
