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

#ifndef TEXT_TO_PHONEMES_TEST_HPP
#define TEXT_TO_PHONEMES_TEST_HPP

#include <QObject>

namespace noteahead {

class TextToPhonemesTest : public QObject
{
    Q_OBJECT

private slots:
    void test_textToPhonemes_ordinaryWords_shouldReadCorrectly_data();
    void test_textToPhonemes_ordinaryWords_shouldReadCorrectly();

    void test_textToPhonemes_context_shouldChangeHowALetterIsRead_data();
    void test_textToPhonemes_context_shouldChangeHowALetterIsRead();

    void test_textToPhonemes_functionWords_shouldBeReducedAndShort_data();
    void test_textToPhonemes_functionWords_shouldBeReducedAndShort();
    void test_textToPhonemes_phraseFinalSyllable_shouldBeLengthened();
    void test_textToPhonemes_phraseFinalLengthening_shouldFollowPunctuation();
    void test_textToPhonemes_curlyApostrophe_shouldNotSplitTheWord();
    void test_textToPhonemes_stressMark_shouldChooseTheSyllable_data();
    void test_textToPhonemes_stressMark_shouldChooseTheSyllable();
    void test_textToPhonemes_stressMark_shouldNotEatContractions_data();
    void test_textToPhonemes_stressMark_shouldNotEatContractions();

    void test_textToPhonemes_affricates_shouldExpandToAStopAndAFricative();

    void test_textToPhonemes_escape_shouldBypassTheRules();
    void test_textToPhonemes_escape_shouldAcceptLowercaseAndOddSpacing();
    void test_textToPhonemes_escape_shouldIgnoreUnknownNames();
    void test_textToPhonemes_unterminatedEscape_shouldConsumeTheRest();

    void test_textToPhonemes_words_shouldBeMarkedAtTheirFirstPhoneme();
    void test_textToPhonemes_syllables_shouldFollowMaximalOnset_data();
    void test_textToPhonemes_syllables_shouldFollowMaximalOnset();
    void test_textToPhonemes_consonantOnlyPhrase_shouldStillHaveASyllable();

    void test_textToPhonemes_punctuation_shouldPause();
    void test_textToPhonemes_plainSpace_shouldNotPause();

    void test_textToPhonemes_emptyOrUnreadable_shouldProduceNothing_data();
    void test_textToPhonemes_emptyOrUnreadable_shouldProduceNothing();

    void test_stressRules_shouldAgreeWithMeasuredEnglish();
    void test_stressRules_shouldFollowTheSuffix_data();
    void test_stressRules_shouldFollowTheSuffix();
    void test_textToPhonemes_stress_shouldLengthenTheStrongSyllable();
    void test_textToPhonemes_singleSyllableWord_shouldNotBeScaled();
    void test_textToPhonemes_unaspiratedStop_shouldBeShort_data();
    void test_textToPhonemes_unaspiratedStop_shouldBeShort();

    void test_ruleTable_everyRule_shouldNamePhonemesTheVoiceKnows();
};

} // namespace noteahead

#endif // TEXT_TO_PHONEMES_TEST_HPP
