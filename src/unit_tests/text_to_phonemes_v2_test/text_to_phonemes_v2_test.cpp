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

#include "text_to_phonemes_v2_test.hpp"

#include "../../domain/dsp/speech/text_to_phonemes_v2.hpp"

#include <QTest>

#include <algorithm>
#include <string>

namespace noteahead {

namespace {

double totalVowelMs(std::string_view word)
{
    const auto events = textToPhonemesV2(word);
    double sum = 0.0;
    for (const auto & event : events) {
        if (event.spec->type == PhonemeType::Vowel) {
            sum += static_cast<double>(event.spec->nominalMs) * event.lengthScale;
        }
    }
    return sum;
}

bool containsPhoneme(const PhonemeEventList & events, std::string_view name)
{
    return std::ranges::any_of(events, [name](const PhonemeEvent & event) {
        return event.spec->name == name;
    });
}

} // namespace

void TextToPhonemesV2Test::test_lexicon_shouldBeWellFormed()
{
    QVERIFY(speechV2LexiconIsWellFormed());
}

void TextToPhonemesV2Test::test_preFortisClipping_shouldShortenVowelsBeforeVoicelessCodas()
{
    QVERIFY(totalVowelMs("bat") < totalVowelMs("bad") * 0.78);
    QVERIFY(totalVowelMs("seat") < totalVowelMs("seed") * 0.78);
    QVERIFY(totalVowelMs("ice") < totalVowelMs("eyes") * 0.78);
}

void TextToPhonemesV2Test::test_allophone_flap_shouldTriggerBeforeUnstressedVowelOnly()
{
    for (const auto * flappedWord : { "water", "better", "city", "ready" }) {
        QVERIFY2(containsPhoneme(textToPhonemesV2(flappedWord), "DX"), flappedWord);
    }
    for (const auto * unflappedWord : { "today", "attack", "guitar" }) {
        QVERIFY2(!containsPhoneme(textToPhonemesV2(unflappedWord), "DX"), unflappedWord);
    }
}

void TextToPhonemesV2Test::test_allophone_the_shouldAdaptToFollowingVowel()
{
    const auto theApple = phonemeNames(textToPhonemesV2("the apple"));
    const auto theOcean = phonemeNames(textToPhonemesV2("the ocean"));
    const auto theWorld = phonemeNames(textToPhonemesV2("the world"));
    const auto theBass = phonemeNames(textToPhonemesV2("the bass"));

    QVERIFY2(theApple.starts_with("DH IY"), theApple.c_str());
    QVERIFY2(theOcean.starts_with("DH IY"), theOcean.c_str());
    QVERIFY2(theWorld.starts_with("DH AX"), theWorld.c_str());
    QVERIFY2(theBass.starts_with("DH AX"), theBass.c_str());
}

void TextToPhonemesV2Test::test_numbers_shouldExpandNaturally()
{
    const auto tr808 = phonemeNames(textToPhonemesV2("808"));
    QVERIFY2(tr808.find("EY T  OW  EY T") != std::string::npos, tr808.c_str());

    const auto seq = phonemeNames(textToPhonemesV2("1 2 3 4"));
    QVERIFY2(seq.find("W AH N  T UW  TH R IY  F AO R") != std::string::npos, seq.c_str());

    const auto year2026 = phonemeNames(textToPhonemesV2("2026"));
    QVERIFY2(year2026.find("T W EH N T IY") != std::string::npos && year2026.find("S IH K S") != std::string::npos,
             year2026.c_str());

    const auto fortyTwo = phonemeNames(textToPhonemesV2("42"));
    QVERIFY2(fortyTwo.find("T UW") != std::string::npos, fortyTwo.c_str());

    const auto v2 = phonemeNames(textToPhonemesV2("v2"));
    QVERIFY2(v2.find("T UW") != std::string::npos, v2.c_str());
}

void TextToPhonemesV2Test::test_contractions_shouldPronounceNaturally()
{
    QCOMPARE(phonemeNames(textToPhonemesV2("I'm")), std::string { "AY M" });
    QCOMPARE(phonemeNames(textToPhonemesV2("we'll")), std::string { "W IY L" });
    QCOMPARE(phonemeNames(textToPhonemesV2("won't")), std::string { "W OW N T" });
    QCOMPARE(phonemeNames(textToPhonemesV2("can't")), std::string { "K AE N T" });
    QCOMPARE(phonemeNames(textToPhonemesV2("you're")), std::string { "Y UW R" });
    QCOMPARE(phonemeNames(textToPhonemesV2("they're")), std::string { "DH EH R" });
    QCOMPARE(phonemeNames(textToPhonemesV2("don't")), std::string { "D OW N T" });
}

void TextToPhonemesV2Test::test_exceptionLexicon_shouldFixIrregularWords()
{
    QCOMPARE(phonemeNames(textToPhonemesV2("love")), std::string { "L AH V" });
    QCOMPARE(phonemeNames(textToPhonemesV2("heart")), std::string { "HH AA R T" });
    QCOMPARE(phonemeNames(textToPhonemesV2("rhythm")), std::string { "R IH DH AX M" });
    QCOMPARE(phonemeNames(textToPhonemesV2("machine")), std::string { "M AX SH IY N" });
    QCOMPARE(phonemeNames(textToPhonemesV2("bass")), std::string { "B EY S" });
    QCOMPARE(phonemeNames(textToPhonemesV2("ocean")), std::string { "OW SH AX N" });
    QCOMPARE(phonemeNames(textToPhonemesV2("listen")), std::string { "L IH S AX N" });
    QCOMPARE(phonemeNames(textToPhonemesV2("work")), std::string { "W ER K" });

    const auto america = textToPhonemesV2("A'merica");
    const bool stressedEh = std::ranges::any_of(america, [](const PhonemeEvent & event) {
        return event.stressed && event.spec->name == "EH";
    });
    QVERIFY(stressedEh);

    QCOMPARE(phonemeNames(textToPhonemesV2("/w er l d/")), std::string { "W ER L D" });
}

void TextToPhonemesV2Test::test_affricates_shouldShortenClosureInsideWord()
{
    const auto firstTMillis = [](std::string_view word) {
        for (const auto & event : textToPhonemesV2(word)) {
            if (event.spec->name == "T") {
                return static_cast<double>(event.spec->nominalMs) * event.lengthScale;
            }
        }
        return 0.0;
    };

    const double affricateT = firstTMillis("change");
    const double standaloneT = firstTMillis("top");
    QVERIFY2(affricateT > 0.0 && affricateT < standaloneT * 0.55,
             qPrintable(QString { "affricate=%1 ms vs standalone=%2 ms" }.arg(affricateT).arg(standaloneT)));
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::TextToPhonemesV2Test)
