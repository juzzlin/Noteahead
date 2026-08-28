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

#ifndef FORMANT_VOICE_TEST_HPP
#define FORMANT_VOICE_TEST_HPP

#include <QObject>

namespace noteahead {

class FormantVoiceTest : public QObject
{
    Q_OBJECT

private slots:
    void test_phonemeTable_everyPhoneme_shouldHaveAscendingFormants();
    void test_phonemeTable_lookup_shouldFindArpabetNamesOnly();

    void test_formantVoice_heldVowel_shouldPeakNearItsFirstFormant();
    void test_formantVoice_frontAndBackVowels_shouldDifferInFormantBands();
    void test_formantVoice_diphthong_shouldMoveItsSecondFormant();
    void test_formantVoice_formantShift_shouldMoveTheWholeVowelSpace();
    void test_formantVoice_sourceRolloff_shouldSoftenTheVoice();

    void test_formantVoice_voicelessPlosive_shouldBeSilentThroughTheClosure();
    void test_formantVoice_voicedPlosive_shouldBuzzThroughTheClosure();
    void test_formantVoice_unvoicedStop_shouldAspirateIntoWhatFollows();
    void test_formantVoice_aspiration_shouldBeShapedByTheFollowingVowel();
    void test_formantVoice_stopAfterAFricative_shouldNotAspirate();
    void test_formantVoice_aspiration_shouldNotStretchWithThePhoneme();
    void test_formantVoice_voicedStop_shouldNotAspirate();
    void test_formantVoice_phraseFinalStop_shouldNotBeReleased_data();
    void test_formantVoice_phraseFinalStop_shouldNotBeReleased();
    void test_formantVoice_fricative_shouldBeBroadbandAndUnpitched();
    void test_phonemeTable_noiseBands_shouldBeBroad();
    void test_formantVoice_sibilant_shouldBeAShelfNotTwoPeaks();
    void test_formantVoice_vowel_shouldHaveEnergyAboveTheThirdFormant();
    void test_formantVoice_sibilant_shouldNotTowerOverTheVowels_data();
    void test_formantVoice_sibilant_shouldNotTowerOverTheVowels();

    void test_formantVoice_vowelBoundary_shouldNotStep();
    void test_formantVoice_fricativeBoundary_shouldNotStep_data();
    void test_formantVoice_fricativeBoundary_shouldNotStep();
    void test_formantVoice_fricativeBoundary_shouldNotStepAtAnyGlideTime();
    void test_formantVoice_plosiveRelease_shouldNotStep();
    void test_formantVoice_consonantLevels_shouldMatchSpeech_data();
    void test_formantVoice_consonantLevels_shouldMatchSpeech();
    void test_formantVoice_voicedFricatives_shouldHaveAVoiceBar_data();
    void test_formantVoice_voicedFricatives_shouldHaveAVoiceBar();
    void test_formantVoice_vowelIntensity_shouldFollowOpenness_data();
    void test_formantVoice_vowelIntensity_shouldFollowOpenness();
    void test_formantVoice_heldVowel_shouldReachUsableLevel();
    void test_formantVoice_afterReset_shouldBeSilent();

    void probe();
};

} // namespace noteahead

#endif // FORMANT_VOICE_TEST_HPP
