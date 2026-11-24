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

#ifndef FORMANT_VOICE_V2_TEST_HPP
#define FORMANT_VOICE_V2_TEST_HPP

#include <QObject>

namespace noteahead {

class FormantVoiceV2Test : public QObject
{
    Q_OBJECT

private slots:
    void test_heldVowel_shouldPeakAtFormantsAndReflectOpenness();
    void test_nasal_shouldHaveMurmurAndAntiresonanceNotch();
    void test_coarticulation_cvTransitions_shouldDifferByPlace();
    void test_coarticulation_vcClosing_shouldBendTowardStopLocus();
    void test_coarticulation_velarPinch_shouldTrackAdjacentVowel();
    void test_glottalH_shouldTakeFollowingVowelShape();
    void test_voicedFricatives_shouldHaveCleanVoiceBarAndFrication();
    void test_consonantLevels_shouldMatchSpeech_data();
    void test_consonantLevels_shouldMatchSpeech();
    void test_plosives_closureAspirationAndPhraseFinal();
    void test_boundaries_shouldNotClick_data();
    void test_boundaries_shouldNotClick();
    void test_plosiveBoundaries_shouldNotClick_data();
    void test_plosiveBoundaries_shouldNotClick();
    void test_geminateStop_shouldNotReleaseTwice();
    void test_monsterAndWhisper_shouldStayFreeOfClicks();
    void test_highPitch_shouldPreserveFundamentalThroughF1Floor();
    void test_returnPhase_shouldControlHighFrequencyTilt();
    void test_voicingOnset_shouldBeSofterThanSteadyState();
    void test_phraseFinal_shouldTaperAndSoftenAtTail();
    void test_diphthong_shouldHoldNucleusThenGlideSmoothly();
    void test_allophones_darkLAndFlap();
    void test_unstressedVowel_shouldCentralizeTowardSchwa();
    void test_breathiness_shouldAddHighFrequencyAirWithoutLowRumble();
    void test_afterReset_shouldBeSilent();
};

} // namespace noteahead

#endif // FORMANT_VOICE_V2_TEST_HPP
