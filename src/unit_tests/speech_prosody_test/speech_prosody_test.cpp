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

#include "speech_prosody_test.hpp"

#include "../../domain/dsp/speech/speech_prosody.hpp"
#include "../../domain/dsp/speech/speech_sequencer_v2.hpp"
#include "../../domain/dsp/speech/text_to_phonemes.hpp"

#include <QTest>

#include <cmath>
#include <vector>

namespace noteahead {

namespace {

std::vector<size_t> pauseIndices(const PhonemeEventList & events)
{
    std::vector<size_t> indices;
    for (size_t i = 0; i < events.size(); i++) {
        if (events[i].spec->type == PhonemeType::Silence) {
            indices.push_back(i);
        }
    }
    return indices;
}

size_t lastVowelBefore(const PhonemeEventList & events, size_t limit)
{
    for (size_t i = std::min(limit, events.size()); i > 0; i--) {
        if (events[i - 1].spec->type == PhonemeType::Vowel) {
            return i - 1;
        }
    }
    return events.size();
}

} // namespace

void SpeechProsodyTest::test_punctuation_shouldSetBoundaryTonesAndPauseLengths()
{
    auto events = textToPhonemes("Hello, world. How are you? Look!");
    const auto prosody = SpeechProsody::analyze("Hello, world. How are you? Look!", events);
    const auto pauses = pauseIndices(events);
    QCOMPARE(pauses.size(), size_t { 4 });

    QCOMPARE(prosody[pauses[0]].boundaryTone, BoundaryTone::Continuation);
    QVERIFY(events[pauses[0]].lengthScale > 1.4 && events[pauses[0]].lengthScale < 1.7);

    QCOMPARE(prosody[pauses[1]].boundaryTone, BoundaryTone::Statement);
    QVERIFY(events[pauses[1]].lengthScale > 2.9);

    QCOMPARE(prosody[pauses[2]].boundaryTone, BoundaryTone::Question);
    QVERIFY(events[pauses[2]].lengthScale > 2.9);

    QCOMPARE(prosody[pauses[3]].boundaryTone, BoundaryTone::Exclamation);
    QVERIFY(events[pauses[3]].lengthScale > 2.9);

    // Consecutive dots in ellipses do not compound into three 280 ms pauses
    auto ellipsisEvents = textToPhonemes("Wait... what.");
    SpeechProsody::analyze("Wait... what.", ellipsisEvents);
    const auto ellipsisPauses = pauseIndices(ellipsisEvents);
    QCOMPARE(ellipsisPauses.size(), size_t { 4 });
    QVERIFY(ellipsisEvents[ellipsisPauses[0]].lengthScale > 2.9);
    QCOMPARE(ellipsisEvents[ellipsisPauses[1]].lengthScale, 1.0);
    QCOMPARE(ellipsisEvents[ellipsisPauses[2]].lengthScale, 1.0);
}

void SpeechProsodyTest::test_pitchContour_shouldDistinguishQuestionCommaStatement()
{
    const auto boundaryPitchOf = [](std::string_view phrase, size_t pauseNumber) {
        auto events = textToPhonemes(phrase);
        const auto prosody = SpeechProsody::analyze(phrase, events);
        const auto pauses = pauseIndices(events);
        const size_t limit = pauseNumber < pauses.size() ? pauses[pauseNumber] : events.size();
        const size_t v = lastVowelBefore(events, limit);
        Q_ASSERT(v < events.size());
        return SpeechProsody::pitchOffsetSemitones(prosody[v], *events[v].spec, 0.90, 0.15, 1.0);
    };

    const double question = boundaryPitchOf("You are ready?", 0);
    const double comma = boundaryPitchOf("You are ready, now.", 0);
    const double statement = boundaryPitchOf("You are ready.", 0);

    QVERIFY2(question > comma, qPrintable(QString { "q=%1 vs c=%2" }.arg(question).arg(comma)));
    QVERIFY2(comma > statement, qPrintable(QString { "c=%1 vs s=%2" }.arg(comma).arg(statement)));
    QVERIFY2(question > statement + 2.5, qPrintable(QString { "q=%1 vs s=%2" }.arg(question).arg(statement)));
}

void SpeechProsodyTest::test_downstep_shouldLowerSuccessiveAccentsAndResetAfterComma()
{
    auto events = textToPhonemes("Happy purple tiger, shiny dragon.");
    const auto prosody = SpeechProsody::analyze("Happy purple tiger, shiny dragon.", events);

    std::vector<double> firstClause;
    std::vector<double> secondClause;
    bool seenComma = false;
    for (size_t i = 0; i < events.size(); i++) {
        if (events[i].spec->type == PhonemeType::Silence) {
            seenComma = true;
            continue;
        }
        if (prosody[i].accentSemitones > 0.0) {
            if (!seenComma) {
                firstClause.push_back(prosody[i].accentSemitones);
            } else {
                secondClause.push_back(prosody[i].accentSemitones);
            }
        }
    }

    QVERIFY(firstClause.size() >= 3);
    QVERIFY(firstClause[0] > firstClause[1]);
    QVERIFY(firstClause[1] > firstClause[2]);
    QVERIFY(!secondClause.empty());
    QVERIFY(std::abs(secondClause[0] - firstClause[0]) < 1.0e-6);
}

void SpeechProsodyTest::test_vowelAccent_shouldPeakNearFortyPercent()
{
    auto events = textToPhonemes("tiger.");
    const auto prosody = SpeechProsody::analyze("tiger.", events);

    size_t vowelIdx = events.size();
    for (size_t i = 0; i < events.size(); i++) {
        if (events[i].spec->type == PhonemeType::Vowel && prosody[i].accentSemitones > 0.0) {
            vowelIdx = i;
            break;
        }
    }
    QVERIFY(vowelIdx < events.size());

    const auto eval = [&](double p) {
        return SpeechProsody::pitchOffsetSemitones(prosody[vowelIdx], *events[vowelIdx].spec, p, 0.20, 1.0);
    };
    QVERIFY(eval(0.40) > eval(0.05) + 0.8);
    QVERIFY(eval(0.40) > eval(0.95) + 0.8);
}

void SpeechProsodyTest::test_microprosody_shouldLiftAfterUnvoicedAndDipAfterVoiced()
{
    auto unvoicedEvents = textToPhonemes("pa.");
    const auto unvoicedProsody = SpeechProsody::analyze("pa.", unvoicedEvents);
    auto voicedEvents = textToPhonemes("ba.");
    const auto voicedProsody = SpeechProsody::analyze("ba.", voicedEvents);

    size_t unvoicedVowel = unvoicedEvents.size();
    for (size_t i = 0; i < unvoicedEvents.size(); i++) {
        if (unvoicedEvents[i].spec->type == PhonemeType::Vowel) {
            unvoicedVowel = i;
            break;
        }
    }
    size_t voicedVowel = voicedEvents.size();
    for (size_t i = 0; i < voicedEvents.size(); i++) {
        if (voicedEvents[i].spec->type == PhonemeType::Vowel) {
            voicedVowel = i;
            break;
        }
    }

    QVERIFY(unvoicedVowel < unvoicedEvents.size() && voicedVowel < voicedEvents.size());
    QVERIFY(unvoicedProsody[unvoicedVowel].microprosodySemitones > 0.8);
    QVERIFY(voicedProsody[voicedVowel].microprosodySemitones < -0.4);

    const double onsetP = SpeechProsody::pitchOffsetSemitones(
      unvoicedProsody[unvoicedVowel], *unvoicedEvents[unvoicedVowel].spec, 0.0, 0.002, 1.0);
    const double decayedP = SpeechProsody::pitchOffsetSemitones(
      unvoicedProsody[unvoicedVowel], *unvoicedEvents[unvoicedVowel].spec, 0.0, 0.090, 1.0);
    QVERIFY(onsetP > decayedP + 0.7);
}

void SpeechProsodyTest::test_sequencerV2_stepModeDiphthong_shouldHoldNucleusAndReleaseOffglide()
{
    SpeechSequencerV2 sequencer {};
    sequencer.setSampleRate(48000.0);
    sequencer.setTriggerMode(SpeechSequencerV2::TriggerMode::Step);
    sequencer.setSyncMode(SpeechSequencerV2::SyncMode::Free);
    sequencer.setPhonemes(textToPhonemes("/AY/"));

    sequencer.trigger();
    for (size_t i = 0; i < 24000; i++) {
        QVERIFY(sequencer.advance());
    }
    QVERIFY(sequencer.isActive());
    QVERIFY(sequencer.isSustaining());
    QVERIFY(std::abs(sequencer.progress() - 0.25) < 0.02);

    sequencer.release();
    QVERIFY(!sequencer.isSustaining());
    for (size_t i = 0; i < 1200; i++) {
        QVERIFY(sequencer.advance());
    }
    QVERIFY(sequencer.progress() > 0.30);

    for (size_t i = 0; i < 24000; i++) {
        sequencer.advance();
    }
    QVERIFY(!sequencer.isActive());
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::SpeechProsodyTest)
