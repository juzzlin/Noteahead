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

#include "text_to_phonemes_test.hpp"

#include "../../domain/dsp/speech/text_to_phonemes.hpp"

#include <QTest>

#include <algorithm>
#include <iterator>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>

namespace noteahead {

namespace {

QString spoken(const std::string & text)
{
    return QString::fromStdString(phonemeNames(textToPhonemes(text)));
}

//! The phonemes with a '-' before each syllable start and a '|' before each word start, so a test
//! can state where the divisions fall without counting indices.
QString divided(const std::string & text)
{
    QString marks;
    for (auto && event : textToPhonemes(text)) {
        if (event.wordStart) {
            marks += "|";
        } else if (event.syllableStart) {
            marks += "-";
        } else if (!marks.isEmpty()) {
            marks += " ";
        }
        marks += QString::fromStdString(std::string { event.spec->name });
    }
    return marks;
}

} // namespace

void TextToPhonemesTest::test_textToPhonemes_ordinaryWords_shouldReadCorrectly_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<QString>("phonemes");

    // A rule set of this size reads ordinary words and misses some proper nouns and loanwords. These
    // are the ordinary ones; where it does miss, the /.../ escape is the fix, per word.
    QTest::newRow("hello world") << "hello world" << "HH EH L OW  W ER L D";
    QTest::newRow("noteahead") << "noteahead" << "N OW T IY HH EH D";
    QTest::newRow("street") << "street" << "S T R IY T";
    QTest::newRow("night") << "night" << "N AY T";
    QTest::newRow("thought") << "thought" << "TH AO T";
    QTest::newRow("through") << "through" << "TH R UW";
    QTest::newRow("please") << "please" << "P L IY Z";
    QTest::newRow("voice") << "voice" << "V OY S";
    QTest::newRow("music") << "music" << "M Y UW Z IH K";
    QTest::newRow("nation") << "nation" << "N EY SH AH N";
    QTest::newRow("tracker") << "tracker" << "T R AE K ER";
    QTest::newRow("one two three") << "one two three" << "W AH N  T UW  TH R IY";
    QTest::newRow("talk to me") << "talk to me" << "T AO K  T AX  M IY";

    // Each of these needed a rule of its own, and each names the trap it was written for.
    QTest::newRow("extra: final A is a schwa") << "extra" << "EH K S T R AH";
    QTest::newRow("singing: NG before a vowel keeps no G") << "singing" << "S IH NG IH NG";
    QTest::newRow("laughter: AUGH is not always AO") << "laughter" << "L AE F T ER";
    QTest::newRow("rhythm: H is silent after initial R") << "rhythm" << "R IH TH M";
    QTest::newRow("beautiful: EAU is one vowel") << "beautiful" << "B Y UW T IH F UH L";
    QTest::newRow("children: not child plus a suffix") << "children" << "T SH IH L D R EH N";
}

void TextToPhonemesTest::test_textToPhonemes_ordinaryWords_shouldReadCorrectly()
{
    QFETCH(QString, text);
    QFETCH(QString, phonemes);
    QCOMPARE(spoken(text.toStdString()), phonemes);
}

void TextToPhonemesTest::test_textToPhonemes_context_shouldChangeHowALetterIsRead_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<QString>("phonemes");

    // The whole reason this is a rule set and not a lookup table: the same letter is read differently
    // according to what surrounds it. If these pairs ever agree, the context matching has stopped
    // working and every word is being read by its fallback rule.
    QTest::newRow("C before a back vowel") << "cat" << "K AE T";
    QTest::newRow("C before a front vowel") << "city" << "S IH T IY";
    QTest::newRow("G before a back vowel") << "got" << "G AA T";
    QTest::newRow("G before a front vowel") << "gem" << "D ZH EH M";
    QTest::newRow("TH unvoiced") << "thin" << "TH IH N";
    QTest::newRow("TH voiced in a function word") << "the" << "DH AX";
    QTest::newRow("plural S after a voiceless stop") << "cats" << "K AE T S";
    QTest::newRow("plural S after a voiced stop") << "dogs" << "D AA G Z";
}

void TextToPhonemesTest::test_textToPhonemes_context_shouldChangeHowALetterIsRead()
{
    QFETCH(QString, text);
    QFETCH(QString, phonemes);
    QCOMPARE(spoken(text.toStdString()), phonemes);
}

void TextToPhonemesTest::test_textToPhonemes_stressMark_shouldChooseTheSyllable_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<int>("syllable");

    // The mark stands in front of the syllable it names, which is the notation a dictionary uses.
    // The rules read "America" as stressed on its first syllable, which is wrong, and this is how
    // that gets settled without a pronunciation dictionary.
    QTest::newRow("first") << "'America" << 0;
    QTest::newRow("second") << "A'merica" << 1;
    QTest::newRow("third") << "Ame'rica" << 2;
    QTest::newRow("marking what the rules already chose") << "de'stroy" << 1;
}

void TextToPhonemesTest::test_textToPhonemes_stressMark_shouldChooseTheSyllable()
{
    QFETCH(QString, text);
    QFETCH(int, syllable);

    const auto events = textToPhonemes(text.toStdString());
    QVERIFY(!events.empty());

    size_t index = 0;
    bool started = false;
    std::optional<size_t> stressedSyllable;
    for (auto && event : events) {
        if (event.syllableStart) {
            if (started) {
                index++;
            }
            started = true;
        }
        if (event.stressed && !stressedSyllable.has_value()) {
            stressedSyllable = index;
        }
    }

    QVERIFY(stressedSyllable.has_value());
    QCOMPARE(*stressedSyllable, static_cast<size_t>(syllable));
}

void TextToPhonemesTest::test_textToPhonemes_stressMark_shouldNotEatContractions_data()
{
    QTest::addColumn<QString>("contraction");
    QTest::addColumn<QString>("expanded");

    // A contraction ends the word right after its apostrophe, in one of a closed set of endings, and
    // a stress mark is followed by a syllable. That is the whole of what tells them apart.
    QTest::newRow("don't") << "don't" << "dont";
    QTest::newRow("it's") << "it's" << "its";
    QTest::newRow("we'll") << "we'll" << "well";
    QTest::newRow("they've") << "they've" << "theyve";
    QTest::newRow("I'm") << "I'm" << "im";
}

void TextToPhonemesTest::test_textToPhonemes_stressMark_shouldNotEatContractions()
{
    QFETCH(QString, contraction);
    QFETCH(QString, expanded);

    // Read as one word, with its apostrophe kept, rather than as a stress mark splitting it.
    const auto events = textToPhonemes(contraction.toStdString());
    QCOMPARE(std::ranges::count(events, true, &PhonemeEvent::wordStart), 1);
    QCOMPARE(phonemeNames(events), phonemeNames(textToPhonemes(expanded.toStdString())));
}

void TextToPhonemesTest::test_textToPhonemes_functionWords_shouldBeReducedAndShort_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<QString>("phonemes");

    // English reduces the words that carry no content, and reading them at full value is what makes
    // every word in a sentence weigh the same. "The" is not /thee/ in running speech, it is /thuh/.
    QTest::newRow("the") << "the" << "DH AX";
    QTest::newRow("of") << "of" << "AX V";
    QTest::newRow("to") << "to" << "T AX";
    QTest::newRow("and") << "and" << "AX N D";
    QTest::newRow("a") << "a" << "AX";
    QTest::newRow("from") << "from" << "F R AX M";
}

void TextToPhonemesTest::test_textToPhonemes_functionWords_shouldBeReducedAndShort()
{
    QFETCH(QString, text);
    QFETCH(QString, phonemes);

    const auto events = textToPhonemes(text.toStdString());
    QCOMPARE(QString::fromStdString(phonemeNames(events)), phonemes);

    // Weak as well as reduced. Measured mid-phrase, where nothing else is scaling anything, and on
    // the vowel, because that is the only thing prosody stretches -- stretching a consonant would
    // mean stretching a stop's silence.
    const auto midPhrase = textToPhonemes(("call " + text + " now").toStdString());
    bool sawWeakVowel = false;
    size_t word = 0;
    for (auto && event : midPhrase) {
        if (event.wordStart) {
            word++;
        }
        if (word == 2 && event.spec->type == PhonemeType::Vowel) {
            QVERIFY2(event.lengthScale < 1.0, qPrintable(QString::number(event.lengthScale)));
            sawWeakVowel = true;
        }
    }
    QVERIFY(sawWeakVowel);
}

void TextToPhonemesTest::test_textToPhonemes_phraseFinalSyllable_shouldBeLengthened()
{
    const auto events = textToPhonemes("feel the beat");
    QVERIFY(!events.empty());

    // The last syllable before a pause runs long, which is what makes a phrase sound finished rather
    // than cut off. On its vowel: a phrase ends on a long vowel, not on a long closure.
    const auto lastVowel = std::ranges::find_if(std::ranges::reverse_view(events), [](const PhonemeEvent & event) {
        return event.spec->type == PhonemeType::Vowel;
    });
    QVERIFY(lastVowel != events.rend());
    QVERIFY2(lastVowel->lengthScale > 1.2, qPrintable(QString::number(lastVowel->lengthScale)));

    // Only the last one: the syllables before it are left alone.
    const auto first = std::ranges::find(events, std::string_view { "IY" }, [](const PhonemeEvent & e) {
        return e.spec->name;
    });
    QVERIFY(first != events.end());
    QCOMPARE(first->lengthScale, 1.0);
}

void TextToPhonemesTest::test_textToPhonemes_phraseFinalLengthening_shouldFollowPunctuation()
{
    // A phrase is what falls between the pauses, not the whole text, so a comma gives the words
    // before it an ending of their own.
    //
    // Measured against the same word said mid-phrase rather than against a fixed number: "it" is a
    // function word and is shortened first, so its lengthened form is still shorter than a content
    // word's natural length.
    const auto midPhrase = textToPhonemes("feel it now");
    const auto twoPhrases = textToPhonemes("feel it, feel it");

    const auto scaleOfIt = [](const PhonemeEventList & events, size_t occurrence) {
        size_t seen = 0;
        for (size_t i = 0; i + 1 < events.size(); i++) {
            if (events[i].spec->name == "IH" && events[i + 1].spec->name == "T") {
                if (seen++ == occurrence) {
                    return events[i].lengthScale;
                }
            }
        }
        return 0.0;
    };

    QVERIFY(scaleOfIt(twoPhrases, 0) > scaleOfIt(midPhrase, 0));
    QVERIFY(scaleOfIt(twoPhrases, 1) > scaleOfIt(midPhrase, 0));
}

void TextToPhonemesTest::test_textToPhonemes_curlyApostrophe_shouldNotSplitTheWord()
{
    // Typed text carries the curly apostrophe as often as the straight one. Left unhandled it does
    // not merely get ignored -- it ends the word, and "don't" is read as "don" then "t".
    const auto straight = textToPhonemes("don't");
    const auto curly = textToPhonemes("don\xE2\x80\x99t");
    QCOMPARE(phonemeNames(curly), phonemeNames(straight));
    QCOMPARE(std::ranges::count(curly, true, &PhonemeEvent::wordStart), 1);
}

void TextToPhonemesTest::test_textToPhonemes_affricates_shouldExpandToAStopAndAFricative()
{
    // The synthesizer has no affricate, because an affricate is a stop released into a fricative and
    // the voice renders that correctly from the two phonemes. The rules may still say CH and JH.
    QCOMPARE(spoken("speech"), QString { "S P IY T SH" });
    QCOMPARE(spoken("jump"), QString { "D ZH AH M P" });
}

void TextToPhonemesTest::test_textToPhonemes_escape_shouldBypassTheRules()
{
    // "world" happens to come out right; the point is that the escaped form is not being read by the
    // rules at all, so a word the rules get wrong can be spelled by hand.
    QCOMPARE(spoken("hello /w er l d/"), QString { "HH EH L OW  W ER L D" });
    QCOMPARE(spoken("/hh ax l ow/"), QString { "HH AX L OW" });
    QCOMPARE(spoken("/z z z/"), QString { "Z Z Z" });
}

void TextToPhonemesTest::test_textToPhonemes_escape_shouldAcceptLowercaseAndOddSpacing()
{
    QCOMPARE(spoken("/AA/"), QString { "AA" });
    QCOMPARE(spoken("/aa/"), QString { "AA" });
    QCOMPARE(spoken("/  aa   iy  /"), QString { "AA IY" });
}

void TextToPhonemesTest::test_textToPhonemes_escape_shouldIgnoreUnknownNames()
{
    // A misspelled phoneme is dropped rather than substituted. Substituting would put a sound in the
    // phrase that the user did not ask for and cannot see.
    QCOMPARE(spoken("/aa qq iy/"), QString { "AA IY" });
}

void TextToPhonemesTest::test_textToPhonemes_unterminatedEscape_shouldConsumeTheRest()
{
    QCOMPARE(spoken("/aa iy"), QString { "AA IY" });
    QVERIFY(textToPhonemes("/").empty());
}

void TextToPhonemesTest::test_textToPhonemes_words_shouldBeMarkedAtTheirFirstPhoneme()
{
    const auto events = textToPhonemes("hello world");
    QVERIFY(!events.empty());

    size_t words = 0;
    for (size_t i = 0; i < events.size(); i++) {
        if (events[i].wordStart) {
            words++;
        }
    }
    QCOMPARE(words, size_t { 2 });
    QVERIFY(events.front().wordStart);
    QCOMPARE(divided("hello world"), QString { "|HH EH-L OW|W ER L D" });
}

void TextToPhonemesTest::test_textToPhonemes_syllables_shouldFollowMaximalOnset_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<QString>("divided");

    // The consonants before a vowel belong to that vowel's syllable, capped at two so a long run
    // does not leave the syllable before it bare.
    QTest::newRow("hello") << "hello" << "|HH EH-L OW";
    QTest::newRow("extra") << "extra" << "|EH K S-T R AH";
    QTest::newRow("computer") << "computer" << "|K AA M-P Y UW-T ER";
    // No preceding syllable to leave anything to, so the whole onset is taken and "street" is one
    // syllable rather than two.
    QTest::newRow("street") << "street" << "|S T R IY T";
}

void TextToPhonemesTest::test_textToPhonemes_syllables_shouldFollowMaximalOnset()
{
    QFETCH(QString, text);
    QFETCH(QString, divided);
    QCOMPARE(noteahead::divided(text.toStdString()), divided);
}

void TextToPhonemesTest::test_textToPhonemes_consonantOnlyPhrase_shouldStillHaveASyllable()
{
    // Step mode advances by syllables, so a phrase with no vowel in it must still be divisible or
    // there is nothing for a note to trigger.
    const auto events = textToPhonemes("/s s s/");
    QVERIFY(!events.empty());
    QVERIFY(events.front().syllableStart);
}

void TextToPhonemesTest::test_textToPhonemes_punctuation_shouldPause()
{
    const auto events = textToPhonemes("hello, world");
    const auto silences = std::ranges::count_if(events, [](const PhonemeEvent & event) {
        return event.spec->type == PhonemeType::Silence;
    });
    QCOMPARE(silences, 1);
}

void TextToPhonemesTest::test_textToPhonemes_plainSpace_shouldNotPause()
{
    // Speech does not stop between every word, and in Step mode a silence between them would eat a
    // note. A pause is something the user asks for with punctuation.
    const auto events = textToPhonemes("hello world");
    QVERIFY(std::ranges::none_of(events, [](const PhonemeEvent & event) {
        return event.spec->type == PhonemeType::Silence;
    }));
}

void TextToPhonemesTest::test_textToPhonemes_emptyOrUnreadable_shouldProduceNothing_data()
{
    QTest::addColumn<QString>("text");

    QTest::newRow("empty") << "";
    QTest::newRow("spaces") << "   ";
    QTest::newRow("digits") << "12345";
    QTest::newRow("symbols") << "@#$%^&*()";
}

void TextToPhonemesTest::test_textToPhonemes_emptyOrUnreadable_shouldProduceNothing()
{
    QFETCH(QString, text);
    // Must terminate rather than stall on a character no rule covers, which is the failure mode of a
    // matcher that can consume nothing and still call it a match.
    QVERIFY(textToPhonemes(text.toStdString()).empty());
}

void TextToPhonemesTest::test_textToPhonemes_unaspiratedStop_shouldBeShort_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<int>("index");
    QTest::addColumn<bool>("shortened");

    // An unvoiced stop is long only because it has to hold a burst and the aspiration after it.
    QTest::newRow("in a cluster after /s/") << "destroy" << 3 << true;
    QTest::newRow("at the end of a phrase") << "stop" << 3 << true;
    // Between vowels it is aspirated, so it keeps its length.
    QTest::newRow("before a vowel") << "later" << 2 << false;
}

void TextToPhonemesTest::test_textToPhonemes_unaspiratedStop_shouldBeShort()
{
    QFETCH(QString, text);
    QFETCH(int, index);
    QFETCH(bool, shortened);

    const auto events = textToPhonemes(text.toStdString());
    QVERIFY(static_cast<size_t>(index) < events.size());
    QCOMPARE(events[static_cast<size_t>(index)].spec->type, PhonemeType::Plosive);

    // All that length would otherwise become closure, which is silence -- and a four millisecond
    // burst after a tenth of a second of nothing is a click, whatever the waveform does through it.
    if (shortened) {
        QVERIFY2(events[static_cast<size_t>(index)].lengthScale < 0.8, qPrintable(QString::number(events[static_cast<size_t>(index)].lengthScale)));
    } else {
        QVERIFY2(events[static_cast<size_t>(index)].lengthScale > 0.9, qPrintable(QString::number(events[static_cast<size_t>(index)].lengthScale)));
    }
}

void TextToPhonemesTest::test_stressRules_shouldAgreeWithMeasuredEnglish()
{
    struct Case
    {
        std::string_view word;
        size_t stressed;
        size_t syllables;
    };

    // 400 dictionary words with the syllable carrying primary stress, taken from espeak. Baked in
    // rather than generated, so the suite does not depend on espeak being installed.
    //
    // The threshold is a floor on accuracy, not a list of right answers: English stress is irregular
    // and a rule set of this size will always get a fair number wrong. What matters is that it stays
    // well clear of the 66% that always choosing the first syllable scores, which is what it
    // replaced -- and that a change to the rules cannot quietly make it worse.
    static constexpr Case Cases[] {
        { "dividends", 0, 3 },
        { "restarted", 1, 3 },
        { "timers", 0, 2 },
        { "scarcity", 0, 3 },
        { "golfing", 0, 2 },
        { "nothings", 0, 2 },
        { "recuperate", 1, 4 },
        { "solaced", 0, 2 },
        { "redress", 1, 2 },
        { "settlers", 0, 2 },
        { "daydreaming", 0, 3 },
        { "healthcare", 0, 2 },
        { "inquisitions", 2, 4 },
        { "syndicate", 0, 3 },
        { "generators", 0, 4 },
        { "omen", 0, 2 },
        { "fruitful", 0, 2 },
        { "aspics", 0, 2 },
        { "exclusivity", 2, 5 },
        { "angioplasty", 0, 5 },
        { "resembles", 1, 3 },
        { "fragility", 1, 4 },
        { "kilohertzes", 0, 4 },
        { "coercing", 1, 3 },
        { "broadly", 0, 2 },
        { "staccatos", 1, 3 },
        { "remorsefully", 1, 4 },
        { "cherry", 0, 2 },
        { "mortice", 0, 2 },
        { "churchyards", 0, 2 },
        { "gingivitis", 2, 4 },
        { "cheaper", 0, 2 },
        { "bonging", 0, 2 },
        { "solider", 0, 3 },
        { "coagulants", 1, 4 },
        { "contextual", 1, 3 },
        { "puddled", 0, 2 },
        { "simplifies", 0, 3 },
        { "epigram", 0, 3 },
        { "tattering", 0, 3 },
        { "underhanded", 2, 4 },
        { "trusteeships", 0, 3 },
        { "passé", 1, 2 },
        { "fruitfulness", 0, 3 },
        { "children", 0, 2 },
        { "perplexed", 1, 2 },
        { "condones", 1, 2 },
        { "displeases", 1, 3 },
        { "adjust", 1, 2 },
        { "wallpapered", 0, 3 },
        { "stalling", 0, 2 },
        { "faucet", 0, 2 },
        { "perkiness", 0, 3 },
        { "applying", 1, 2 },
        { "pelvic", 0, 2 },
        { "whackers", 0, 2 },
        { "defaulter", 1, 3 },
        { "energies", 0, 3 },
        { "colons", 0, 2 },
        { "unmercifully", 1, 5 },
        { "rationalists", 0, 4 },
        { "horticulture", 0, 4 },
        { "gratuitously", 1, 4 },
        { "honors", 0, 2 },
        { "evaluating", 1, 5 },
        { "unreal", 1, 2 },
        { "autism", 0, 3 },
        { "clemency", 0, 3 },
        { "immaculately", 1, 5 },
        { "stomping", 0, 2 },
        { "apart", 1, 2 },
        { "unexpected", 2, 4 },
        { "merchandizes", 0, 4 },
        { "dourly", 0, 2 },
        { "countdowns", 0, 2 },
        { "refuels", 1, 2 },
        { "pockets", 0, 2 },
        { "wrangler", 0, 2 },
        { "directed", 1, 3 },
        { "respired", 1, 2 },
        { "preserving", 1, 3 },
        { "followings", 0, 2 },
        { "forefathers", 0, 3 },
        { "levitated", 0, 4 },
        { "woodenness", 0, 3 },
        { "downloaded", 0, 3 },
        { "hardships", 0, 2 },
        { "writings", 0, 2 },
        { "bizarrely", 1, 3 },
        { "reminiscence", 2, 4 },
        { "slinking", 0, 2 },
        { "forewent", 0, 2 },
        { "audited", 0, 3 },
        { "parser", 0, 2 },
        { "nacre", 0, 2 },
        { "presently", 0, 3 },
        { "lapses", 0, 2 },
        { "replay", 0, 2 },
        { "starlit", 0, 2 },
        { "nonplus", 1, 2 },
        { "screenplay", 0, 2 },
        { "hydrant", 0, 2 },
        { "whimpers", 0, 2 },
        { "noncombatant", 1, 4 },
        { "hustings", 0, 2 },
        { "barefoot", 0, 2 },
        { "realize", 0, 2 },
        { "trifler", 0, 2 },
        { "collocate", 0, 3 },
        { "dispersed", 1, 2 },
        { "voted", 0, 2 },
        { "coupons", 0, 2 },
        { "methodology", 2, 5 },
        { "hesitancy", 0, 4 },
        { "menially", 0, 3 },
        { "dazzling", 0, 2 },
        { "collation", 1, 3 },
        { "numeric", 1, 3 },
        { "oppression", 1, 3 },
        { "parading", 1, 3 },
        { "sambas", 0, 2 },
        { "expunging", 1, 3 },
        { "couriers", 0, 2 },
        { "everglades", 0, 3 },
        { "rushing", 0, 2 },
        { "rededicate", 1, 4 },
        { "airmen", 0, 2 },
        { "crusty", 0, 2 },
        { "raccoon", 1, 2 },
        { "coffer", 0, 2 },
        { "oxide", 0, 2 },
        { "intricately", 0, 4 },
        { "posting", 0, 2 },
        { "sunnier", 0, 2 },
        { "parsecs", 0, 2 },
        { "forgiven", 1, 3 },
        { "catbirds", 0, 2 },
        { "safaried", 0, 3 },
        { "arrowhead", 0, 3 },
        { "sureness", 0, 2 },
        { "milligram", 0, 3 },
        { "pontiff", 0, 2 },
        { "reviling", 1, 3 },
        { "glowingly", 0, 2 },
        { "mossier", 0, 2 },
        { "retain", 1, 2 },
        { "uncork", 1, 2 },
        { "marabous", 1, 3 },
        { "vacillated", 0, 4 },
        { "callable", 0, 3 },
        { "severest", 1, 3 },
        { "mingled", 0, 2 },
        { "ingenuity", 2, 4 },
        { "merchandized", 0, 3 },
        { "midpoint", 0, 2 },
        { "diverge", 1, 2 },
        { "governments", 0, 3 },
        { "canopy", 0, 3 },
        { "investors", 1, 3 },
        { "confute", 1, 2 },
        { "hollows", 0, 2 },
        { "sissiest", 0, 2 },
        { "satellites", 0, 3 },
        { "babysits", 0, 3 },
        { "bogies", 0, 2 },
        { "blistering", 0, 3 },
        { "invincibly", 1, 4 },
        { "crescent", 0, 2 },
        { "cognizance", 0, 3 },
        { "perigee", 0, 3 },
        { "overprice", 2, 3 },
        { "seasickness", 0, 3 },
        { "griddles", 0, 2 },
        { "articulates", 1, 4 },
        { "rattletraps", 0, 3 },
        { "accounted", 1, 3 },
        { "twitches", 0, 2 },
        { "serialized", 0, 3 },
        { "unilateral", 2, 5 },
        { "grubbier", 0, 2 },
        { "encores", 0, 2 },
        { "bypassed", 0, 2 },
        { "shunting", 0, 2 },
        { "zombis", 0, 2 },
        { "pursuers", 1, 2 },
        { "imposition", 2, 4 },
        { "diarists", 0, 2 },
        { "hypocrite", 0, 3 },
        { "emphatically", 1, 4 },
        { "curtain", 0, 2 },
        { "iodized", 0, 2 },
        { "predetermine", 2, 4 },
        { "commentate", 0, 3 },
        { "remarries", 1, 3 },
        { "freewill", 0, 2 },
        { "wieners", 0, 2 },
        { "disputing", 1, 3 },
        { "liaises", 1, 3 },
        { "elbowroom", 0, 3 },
        { "decaffeinate", 1, 4 },
        { "raggedier", 0, 3 },
        { "retrogress", 2, 3 },
        { "momentum", 1, 3 },
        { "memberships", 0, 3 },
        { "elfin", 0, 2 },
        { "outrigger", 1, 3 },
        { "mediocrity", 2, 5 },
        { "promoting", 1, 3 },
        { "backbite", 0, 2 },
        { "toxins", 0, 2 },
        { "lonelier", 0, 2 },
        { "invulnerably", 1, 5 },
        { "commenced", 1, 2 },
        { "aftermaths", 0, 3 },
        { "clothesline", 0, 2 },
        { "illicit", 1, 3 },
        { "paintwork", 0, 2 },
        { "karma", 0, 2 },
        { "rediscover", 2, 4 },
        { "parrot", 0, 2 },
        { "create", 1, 2 },
        { "barrios", 0, 3 },
        { "glistens", 0, 2 },
        { "wintergreen", 0, 3 },
        { "deference", 0, 2 },
        { "irritations", 2, 4 },
        { "wisecracked", 0, 2 },
        { "alines", 1, 2 },
        { "chummiest", 0, 2 },
        { "naivety", 1, 4 },
        { "bestirs", 1, 2 },
        { "transducers", 1, 3 },
        { "deter", 1, 2 },
        { "woman", 0, 2 },
        { "swaddling", 0, 2 },
        { "affectionate", 1, 4 },
        { "lavishing", 0, 3 },
        { "obese", 1, 2 },
        { "natty", 0, 2 },
        { "preposterous", 1, 4 },
        { "secures", 1, 2 },
        { "coronets", 0, 3 },
        { "livening", 0, 2 },
        { "celestial", 1, 3 },
        { "pimpernels", 0, 3 },
        { "gingko", 0, 2 },
        { "neonatal", 0, 3 },
        { "complex", 0, 2 },
        { "outreaches", 1, 3 },
        { "retiring", 1, 3 },
        { "differ", 0, 2 },
        { "debilitate", 1, 4 },
        { "resettle", 1, 3 },
        { "voiding", 0, 2 },
        { "littoral", 0, 3 },
        { "angleworm", 0, 3 },
        { "clambered", 0, 2 },
        { "bounden", 0, 2 },
        { "ensnared", 1, 2 },
        { "efforts", 0, 2 },
        { "nattily", 0, 3 },
        { "flotations", 1, 3 },
        { "tollgates", 0, 2 },
        { "torching", 0, 2 },
        { "siroccos", 1, 3 },
        { "boondoggles", 0, 3 },
        { "circulatory", 2, 5 },
        { "migrating", 1, 3 },
        { "contrition", 1, 3 },
        { "stalking", 0, 2 },
        { "insignias", 1, 3 },
        { "outstays", 1, 2 },
        { "pickets", 0, 2 },
        { "meridians", 1, 3 },
        { "furies", 0, 2 },
        { "stricken", 0, 2 },
        { "earliest", 0, 2 },
        { "chubbiness", 0, 3 },
        { "frigates", 0, 2 },
        { "carcasses", 0, 3 },
        { "muckrakes", 0, 2 },
        { "retired", 1, 2 },
        { "imaginable", 1, 5 },
        { "evoke", 1, 2 },
        { "minion", 0, 2 },
        { "redefined", 2, 3 },
        { "specifier", 0, 3 },
        { "typecast", 0, 2 },
        { "quoting", 0, 2 },
        { "fragrance", 0, 2 },
        { "jeremiad", 0, 3 },
        { "thatching", 0, 2 },
        { "abnegated", 0, 4 },
        { "tailless", 0, 2 },
        { "singsonging", 0, 3 },
        { "kicker", 0, 2 },
        { "abominable", 1, 5 },
        { "fusillade", 0, 3 },
        { "bushy", 0, 2 },
        { "affronts", 1, 2 },
        { "showcased", 0, 2 },
        { "barraged", 0, 2 },
        { "reformed", 1, 2 },
        { "turtles", 0, 2 },
        { "popinjays", 0, 3 },
        { "peppered", 0, 2 },
        { "upturned", 0, 2 },
        { "levying", 0, 2 },
        { "anecdote", 0, 3 },
        { "joyrode", 0, 2 },
        { "intuited", 1, 3 },
        { "rotating", 1, 3 },
        { "doodlers", 0, 2 },
        { "pomading", 0, 3 },
        { "dramatics", 1, 3 },
        { "motion", 0, 2 },
        { "weakening", 0, 3 },
        { "shallot", 1, 2 },
        { "skinless", 0, 2 },
        { "countries", 0, 2 },
        { "correlating", 0, 4 },
        { "seedier", 0, 2 },
        { "fortify", 0, 3 },
        { "cosign", 0, 2 },
        { "suggestible", 1, 4 },
        { "baseman", 0, 2 },
        { "contradicts", 2, 3 },
        { "generalities", 2, 5 },
        { "birdseed", 0, 2 },
        { "smitten", 0, 2 },
        { "scribble", 0, 2 },
        { "inlays", 0, 2 },
        { "steady", 0, 2 },
        { "fiberboard", 0, 3 },
        { "kebabs", 1, 2 },
        { "anticked", 0, 2 },
        { "progress", 0, 2 },
        { "subdivision", 2, 4 },
        { "mugger", 0, 2 },
        { "prettiest", 0, 2 },
        { "discomfiting", 1, 4 },
        { "muesli", 0, 2 },
        { "arrayed", 1, 2 },
        { "jokers", 0, 2 },
        { "incoherence", 2, 4 },
        { "goddamed", 0, 2 },
        { "subdue", 1, 2 },
        { "pulsing", 0, 2 },
        { "eardrums", 0, 2 },
        { "matriculates", 1, 4 },
        { "placeholder", 0, 3 },
        { "portrayed", 1, 2 },
        { "philanderer", 1, 4 },
        { "conifer", 1, 3 },
        { "sloven", 0, 2 },
        { "hostessed", 0, 2 },
        { "paragraphed", 0, 3 },
        { "raciness", 0, 3 },
        { "annually", 0, 3 },
        { "garote", 1, 2 },
        { "perchance", 1, 2 },
        { "dubbing", 0, 2 },
        { "clearances", 0, 3 },
        { "hatching", 0, 2 },
        { "tabued", 0, 2 },
        { "barrette", 1, 2 },
        { "movements", 0, 2 },
        { "damages", 0, 3 },
        { "prizefight", 0, 2 },
        { "disable", 1, 3 },
        { "sierras", 1, 3 },
        { "treetop", 0, 2 },
        { "blousing", 0, 2 },
        { "gymnasiums", 1, 3 },
        { "facepalm", 0, 2 },
        { "unsent", 1, 2 },
        { "rectangle", 0, 3 },
        { "rambunctious", 1, 3 },
        { "schnauzers", 0, 2 },
        { "volleyballs", 0, 3 },
        { "blindsided", 0, 3 },
        { "euphemisms", 0, 4 },
        { "detachments", 1, 3 },
        { "caroller", 0, 3 },
        { "emanate", 0, 3 },
        { "periwig", 0, 3 },
        { "debentures", 1, 3 },
        { "deduced", 1, 2 },
        { "flimsy", 0, 2 },
        { "testicle", 0, 3 },
        { "unstuck", 1, 2 },
        { "pimientos", 1, 3 },
        { "bonding", 0, 2 },
        { "gamma", 0, 2 },
        { "upholstering", 1, 4 },
        { "nihilism", 0, 3 },
        { "executor", 1, 4 },
        { "superstars", 0, 3 },
        { "organelle", 2, 3 },
        { "baselines", 0, 2 },
    };

    size_t agreed = 0;
    for (auto && testCase : Cases) {
        if (speechStressedSyllable(QString::fromUtf8(testCase.word.data(), static_cast<int>(testCase.word.size())).toUpper().toStdString(), testCase.syllables) == testCase.stressed) {
            agreed++;
        }
    }

    const double accuracy = static_cast<double>(agreed) / std::size(Cases);
    QVERIFY2(accuracy > 0.75, qPrintable(QString::number(accuracy, 'f', 3)));

    // And the baseline it has to beat, computed the same way, so the comparison cannot go stale.
    const auto firstSyllable = std::ranges::count(Cases, size_t { 0 }, &Case::stressed);
    QVERIFY(accuracy > static_cast<double>(firstSyllable) / std::size(Cases) + 0.08);
}

void TextToPhonemesTest::test_stressRules_shouldFollowTheSuffix_data()
{
    QTest::addColumn<QString>("word");
    QTest::addColumn<int>("syllables");
    QTest::addColumn<int>("stressed");

    // The suffix rules, each with a word that shows why it exists.
    QTest::newRow("-tion takes the syllable before it") << "NATION" << 2 << 0;
    QTest::newRow("-ation likewise") << "INFORMATION" << 4 << 2;
    QTest::newRow("-ic likewise") << "ELECTRIC" << 3 << 1;
    QTest::newRow("-ity takes two before") << "ACTIVITY" << 4 << 1;
    QTest::newRow("-eer takes the suffix itself") << "ENGINEER" << 3 << 2;
    QTest::newRow("-ese likewise") << "JAPANESE" << 3 << 2;
    // A neutral suffix hangs off the end and the root keeps the stress.
    QTest::newRow("-ing keeps the root's stress") << "SPEAKING" << 2 << 0;
    QTest::newRow("-ness keeps it too") << "HAPPINESS" << 3 << 0;
    // A prefix carries none, so it pushes the stress onto the root.
    QTest::newRow("un- pushes it onto the root") << "UNDONE" << 2 << 1;
    QTest::newRow("de- likewise") << "DESTROY" << 2 << 1;
    QTest::newRow("a word with none of that takes the first") << "WORLD" << 1 << 0;
    QTest::newRow("and so does a plain two-syllable word") << "HAPPY" << 2 << 0;
}

void TextToPhonemesTest::test_stressRules_shouldFollowTheSuffix()
{
    QFETCH(QString, word);
    QFETCH(int, syllables);
    QFETCH(int, stressed);
    QCOMPARE(speechStressedSyllable(word.toStdString(), static_cast<size_t>(syllables)), static_cast<size_t>(stressed));
}

void TextToPhonemesTest::test_textToPhonemes_stress_shouldLengthenTheStrongSyllable()
{
    const auto events = textToPhonemes("destroy");
    QVERIFY(!events.empty());

    // "de-STROY": the strong syllable runs long and the weak one short. That alternation is the
    // rhythm of the language, and every syllable at the same length is what sounded like a list.
    const auto weak = events.front().lengthScale;
    const auto strong = events.back().lengthScale;
    QVERIFY2(strong > weak, qPrintable(QString::number(weak) + " -> " + QString::number(strong)));
    QVERIFY(!events.front().stressed);
    QVERIFY(events.back().stressed);
}

void TextToPhonemesTest::test_textToPhonemes_singleSyllableWord_shouldNotBeScaled()
{
    // Nothing to alternate with, so it keeps its natural length rather than being marked strong and
    // stretched for no reason. Checked away from the end of the phrase, which lengthens whatever is
    // there for its own reasons.
    const auto events = textToPhonemes("world now then");
    size_t words = 0;
    for (auto && event : events) {
        if (event.wordStart) {
            words++;
        }
        if (words > 2) {
            break;
        }
        QCOMPARE(event.lengthScale, 1.0);
    }
    QVERIFY(words >= 3);
}

void TextToPhonemesTest::test_ruleTable_everyRule_shouldNamePhonemesTheVoiceKnows()
{
    // A typo in the table would otherwise show up only as a word that quietly loses a sound.
    QVERIFY(speechRulesAreWellFormed());
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::TextToPhonemesTest)
