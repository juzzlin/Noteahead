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

#include "speech_test.hpp"

#include "../../domain/devices/speech_device.hpp"
#include "../../domain/devices/synth_device.hpp"
#include "../../domain/dsp/fft.hpp"
#include "../../domain/dsp/speech/speech_sequencer.hpp"
#include "../../domain/dsp/speech/text_to_phonemes.hpp"

#include <QFile>
#include <QTest>

#include <algorithm>
#include <cmath>
#include <map>
#include <string>
#include <vector>

namespace noteahead {

namespace {

constexpr double SampleRate = 48000.0;

SpeechSequencer makeSequencer(const std::string & phrase)
{
    SpeechSequencer sequencer;
    sequencer.setSampleRate(SampleRate);
    sequencer.setBpm(120.0);
    sequencer.setPhonemes(textToPhonemes(phrase));
    return sequencer;
}

//! Runs an utterance to its end, returning how many frames each phoneme occupied, in order.
std::vector<size_t> phonemeFrames(SpeechSequencer & sequencer)
{
    std::vector<size_t> frames;
    const PhonemeSpec * previous = nullptr;
    size_t count = 0;

    // Generous ceiling: a stuck sequencer must fail the assertions rather than hang the suite.
    for (size_t frame = 0; frame < static_cast<size_t>(SampleRate * 60); frame++) {
        const auto * spec = sequencer.phoneme();
        if (!spec) {
            break;
        }
        if (spec != previous && previous) {
            frames.push_back(count);
            count = 0;
        }
        previous = spec;
        count++;
        if (!sequencer.advance()) {
            frames.push_back(count);
            break;
        }
    }
    return frames;
}

size_t totalFrames(SpeechSequencer & sequencer)
{
    const auto frames = phonemeFrames(sequencer);
    size_t total = 0;
    for (auto && f : frames) {
        total += f;
    }
    return total;
}

//! The phoneme names an utterance spoke, in order.
std::vector<std::string> spokenNames(SpeechSequencer & sequencer)
{
    std::vector<std::string> names;
    const PhonemeSpec * previous = nullptr;
    for (size_t frame = 0; frame < static_cast<size_t>(SampleRate * 60); frame++) {
        const auto * spec = sequencer.phoneme();
        if (!spec) {
            break;
        }
        if (spec != previous) {
            names.emplace_back(spec->name);
            previous = spec;
        }
        if (!sequencer.advance()) {
            break;
        }
    }
    return names;
}

//! Fundamental by autocorrelation with a parabolic fit on the peak.
//!
//! Not limited to whole samples, unlike the plain version above: at 261 Hz one sample of lag is
//! already six cents, which is more than the whole tolerance a tuning test is worth having.
double preciseFundamental(const std::vector<double> & buffer)
{
    std::vector<double> mono;
    for (size_t i = 0; i < buffer.size(); i += 2) {
        mono.push_back(buffer[i]);
    }

    const auto minLag = static_cast<size_t>(SampleRate / 1000.0);
    const auto maxLag = static_cast<size_t>(SampleRate / 50.0);
    if (mono.size() < maxLag * 2) {
        return 0.0;
    }

    std::vector<double> correlation(maxLag + 2, 0.0);
    for (size_t lag = minLag; lag <= maxLag + 1; lag++) {
        double sum = 0.0;
        for (size_t i = 0; i + lag < mono.size(); i++) {
            sum += mono[i] * mono[i + lag];
        }
        correlation[lag] = sum;
    }

    size_t best = minLag;
    for (size_t lag = minLag + 1; lag <= maxLag; lag++) {
        if (correlation[lag] > correlation[best]) {
            best = lag;
        }
    }

    const double a = correlation[best - 1];
    const double b = correlation[best];
    const double c = correlation[best + 1];
    const double divisor = a - 2.0 * b + c;
    const double offset = divisor != 0.0 ? 0.5 * (a - c) / divisor : 0.0;
    return SampleRate / (static_cast<double>(best) + offset);
}

//! Frequency of the strongest spectral component between two bounds, from an interleaved buffer.
double spectralPeak(const std::vector<double> & buffer, double lowHz, double highHz)
{
    constexpr int size = 16384;
    std::vector<double> re(size, 0.0);
    std::vector<double> im(size, 0.0);
    for (int i = 0; i < size && static_cast<size_t>(i) * 2 < buffer.size(); i++) {
        const double window = 0.5 - 0.5 * std::cos(2.0 * M_PI * i / (size - 1));
        re[static_cast<size_t>(i)] = buffer[static_cast<size_t>(i) * 2] * window;
    }
    Fft::forward(re.data(), im.data(), size);

    const auto low = static_cast<size_t>(std::lround(lowHz * size / SampleRate));
    const auto high = static_cast<size_t>(std::lround(highHz * size / SampleRate));
    size_t peak = low;
    for (size_t i = low; i <= high && i < static_cast<size_t>(size / 2); i++) {
        if (std::hypot(re[i], im[i]) > std::hypot(re[peak], im[peak])) {
            peak = i;
        }
    }
    return static_cast<double>(peak) * SampleRate / size;
}

std::vector<double> renderDevice(Device & device, uint32_t frames, double bpm = 120.0)
{
    std::vector<double> buffer(frames * 2, 0.0);
    AudioContext context { std::span<double>(buffer.data(), buffer.size()), frames, static_cast<uint32_t>(SampleRate), bpm, {}, 1, false };
    device.processAudio(context);
    return buffer;
}

double peakAmplitude(const std::vector<double> & buffer)
{
    double peak = 0.0;
    for (auto && sample : buffer) {
        peak = std::max(peak, std::abs(sample));
    }
    return peak;
}

//! Fundamental of the left channel, by autocorrelation over the range a speaking voice covers.
//!
//! Counting zero crossings does not work here: the formants are far louder than the fundamental, so
//! a crossing count returns whichever resonance dominates and reads the same for every note. The
//! fundamental is a periodicity rather than a peak, which is what autocorrelation finds.
double fundamentalFrequency(const std::vector<double> & buffer)
{
    std::vector<double> mono;
    for (size_t i = 0; i < buffer.size(); i += 2) {
        mono.push_back(buffer[i]);
    }

    const auto minLag = static_cast<size_t>(SampleRate / 500.0);
    const auto maxLag = static_cast<size_t>(SampleRate / 60.0);
    if (mono.size() < maxLag * 2) {
        return 0.0;
    }

    double best = 0.0;
    size_t bestLag = 0;
    for (size_t lag = minLag; lag <= maxLag; lag++) {
        double sum = 0.0;
        for (size_t i = 0; i + lag < mono.size(); i++) {
            sum += mono[i] * mono[i + lag];
        }
        if (sum > best) {
            best = sum;
            bestLag = lag;
        }
    }
    return bestLag ? SampleRate / static_cast<double>(bestLag) : 0.0;
}

} // namespace

void SpeechTest::test_sequencer_freeMode_shouldUseNaturalDurations()
{
    auto sequencer = makeSequencer("hello");
    sequencer.setSyncMode(SpeechSequencer::SyncMode::Free);
    sequencer.setRate(1.0);
    sequencer.trigger();

    const auto phonemes = textToPhonemes("hello");
    const auto frames = phonemeFrames(sequencer);
    QCOMPARE(frames.size(), phonemes.size());

    // Natural duration is the phoneme's own length as the prosody scales it: a phrase-final syllable
    // runs long and a function word runs short, and the sequencer has to honour both.
    for (size_t i = 0; i < frames.size(); i++) {
        const auto expected = static_cast<size_t>(phonemes[i].spec->nominalMs * phonemes[i].lengthScale * SampleRate / 1000.0);
        QVERIFY2(std::abs(static_cast<long>(frames[i]) - static_cast<long>(expected)) <= 2, phonemes[i].spec->name.data());
    }
}

void SpeechTest::test_sequencer_freeMode_rate_shouldScaleTheWholePhrase()
{
    auto slow = makeSequencer("hello world");
    slow.setSyncMode(SpeechSequencer::SyncMode::Free);
    slow.setRate(1.0);
    slow.trigger();
    const auto slowFrames = totalFrames(slow);

    auto fast = makeSequencer("hello world");
    fast.setSyncMode(SpeechSequencer::SyncMode::Free);
    fast.setRate(2.0);
    fast.trigger();
    const auto fastFrames = totalFrames(fast);

    QVERIFY(fastFrames < slowFrames);
    QVERIFY(std::abs(static_cast<double>(slowFrames) / static_cast<double>(fastFrames) - 2.0) < 0.05);
}

void SpeechTest::test_sequencer_fitMode_shouldSpanTheGivenLength_data()
{
    QTest::addColumn<double>("bpm");
    QTest::addColumn<double>("beats");

    QTest::newRow("120 bpm, one bar") << 120.0 << 4.0;
    QTest::newRow("120 bpm, two beats") << 120.0 << 2.0;
    QTest::newRow("90 bpm, one bar") << 90.0 << 4.0;
    QTest::newRow("174 bpm, two bars") << 174.0 << 8.0;
}

void SpeechTest::test_sequencer_fitMode_shouldSpanTheGivenLength()
{
    QFETCH(double, bpm);
    QFETCH(double, beats);

    auto sequencer = makeSequencer("this is the speech device speaking");
    sequencer.setBpm(bpm);
    sequencer.setSyncMode(SpeechSequencer::SyncMode::Fit);
    sequencer.setLengthBeats(beats);
    sequencer.trigger();

    // The whole point of Fit: whatever the tempo, the phrase ends on the beat it was told to.
    const auto expected = beats * 60.0 / bpm * SampleRate;
    const auto actual = static_cast<double>(totalFrames(sequencer));
    QVERIFY2(std::abs(actual - expected) / expected < 0.02, qPrintable(QString::number(actual) + " vs " + QString::number(expected)));
}

void SpeechTest::test_sequencer_fitMode_shouldKeepTheRelativeRhythm()
{
    auto sequencer = makeSequencer("hello world");
    sequencer.setSyncMode(SpeechSequencer::SyncMode::Fit);
    sequencer.setLengthBeats(4.0);
    sequencer.trigger();

    const auto phonemes = textToPhonemes("hello world");
    const auto frames = phonemeFrames(sequencer);
    QCOMPARE(frames.size(), phonemes.size());

    // Scaled together rather than flattened: a phoneme that is naturally twice as long as another
    // stays twice as long. Making them all equal would land on the beat too, and sound nothing like
    // speech.
    for (size_t i = 1; i < frames.size(); i++) {
        const double nominal = phonemes[i].spec->nominalMs * phonemes[i].lengthScale;
        const double previous = phonemes[i - 1].spec->nominalMs * phonemes[i - 1].lengthScale;
        const double actualRatio = static_cast<double>(frames[i]) / static_cast<double>(frames[i - 1]);
        QVERIFY2(std::abs(actualRatio - nominal / previous) < 0.05, phonemes[i].spec->name.data());
    }
}

void SpeechTest::test_sequencer_gridMode_shouldGiveEachSyllableOneDivision()
{
    auto sequencer = makeSequencer("hello world");
    sequencer.setBpm(120.0);
    sequencer.setSyncMode(SpeechSequencer::SyncMode::Grid);
    sequencer.setDivisionBeats(0.5);
    sequencer.trigger();

    const auto events = textToPhonemes("hello world");
    const auto syllables = static_cast<double>(std::ranges::count(events, true, &PhonemeEvent::syllableStart));
    QVERIFY(syllables > 0.0);

    const auto expected = syllables * 0.5 * 60.0 / 120.0 * SampleRate;
    const auto actual = static_cast<double>(totalFrames(sequencer));
    QVERIFY2(std::abs(actual - expected) / expected < 0.02, qPrintable(QString::number(actual) + " vs " + QString::number(expected)));
}

void SpeechTest::test_sequencer_gridMode_shouldStretchTheVowelNotTheConsonants()
{
    const auto measure = [](double division) {
        auto sequencer = makeSequencer("/hh eh l ow/");
        sequencer.setBpm(120.0);
        sequencer.setSyncMode(SpeechSequencer::SyncMode::Grid);
        sequencer.setDivisionBeats(division);
        sequencer.trigger();
        std::map<std::string, size_t> byName;
        const auto frames = phonemeFrames(sequencer);
        const auto names = std::vector<std::string> { "HH", "EH", "L", "OW" };
        for (size_t i = 0; i < frames.size() && i < names.size(); i++) {
            byName[names[i]] = frames[i];
        }
        return byName;
    };

    const auto tight = measure(0.5);
    const auto loose = measure(1.5);

    // Consonants are near enough constant in real speech while vowels are what stretch. Sharing a
    // syllable's slot out proportionally instead makes every consonant stretch with the tempo, and
    // it stops sounding like speech.
    QCOMPARE(tight.at("HH"), loose.at("HH"));
    QCOMPARE(tight.at("L"), loose.at("L"));
    QVERIFY(loose.at("EH") > tight.at("EH") * 2);
    QVERIFY(loose.at("OW") > tight.at("OW") * 2);
}

void SpeechTest::test_sequencer_anyMode_shouldNeverProduceAnInaudiblePhoneme()
{
    // A tempo fast enough to squeeze the phrase below the length of a formant transition must slur
    // it, not turn it into clicks.
    for (auto && mode : { SpeechSequencer::SyncMode::Fit, SpeechSequencer::SyncMode::Grid }) {
        auto sequencer = makeSequencer("this is the speech device speaking");
        sequencer.setBpm(300.0);
        sequencer.setSyncMode(mode);
        sequencer.setLengthBeats(0.25);
        sequencer.setDivisionBeats(0.0625);
        sequencer.trigger();

        for (auto && frames : phonemeFrames(sequencer)) {
            QVERIFY(frames >= static_cast<size_t>(0.010 * SampleRate));
        }
    }
}

void SpeechTest::test_sequencer_phraseMode_shouldSpeakEverythingFromTheStart()
{
    auto sequencer = makeSequencer("hello world");
    sequencer.setTriggerMode(SpeechSequencer::TriggerMode::Phrase);
    sequencer.trigger();

    const auto spoken = spokenNames(sequencer);
    QCOMPARE(spoken.size(), textToPhonemes("hello world").size());
    QCOMPARE(spoken.front(), std::string { "HH" });
    QCOMPARE(spoken.back(), std::string { "D" });

    // And it starts over rather than continuing.
    sequencer.trigger();
    QCOMPARE(spokenNames(sequencer).front(), std::string { "HH" });
}

void SpeechTest::test_sequencer_stepMode_shouldAdvanceOneSyllablePerTrigger()
{
    auto sequencer = makeSequencer("hello world");
    sequencer.setTriggerMode(SpeechSequencer::TriggerMode::Step);
    sequencer.setSyncMode(SpeechSequencer::SyncMode::Grid);

    // "hello world" is HH EH | L OW | W ER L D, so three notes speak it.
    QCOMPARE(sequencer.syllableCount(), size_t { 3 });

    sequencer.trigger();
    QCOMPARE(spokenNames(sequencer), (std::vector<std::string> { "HH", "EH" }));
    sequencer.trigger();
    QCOMPARE(spokenNames(sequencer), (std::vector<std::string> { "L", "OW" }));
    sequencer.trigger();
    QCOMPARE(spokenNames(sequencer), (std::vector<std::string> { "W", "ER", "L", "D" }));
}

void SpeechTest::test_sequencer_stepMode_shouldWrapAtTheEndOfThePhrase()
{
    auto sequencer = makeSequencer("hello world");
    sequencer.setTriggerMode(SpeechSequencer::TriggerMode::Step);
    sequencer.setSyncMode(SpeechSequencer::SyncMode::Grid);

    for (size_t i = 0; i < sequencer.syllableCount(); i++) {
        sequencer.trigger();
        spokenNames(sequencer);
    }

    // Back to the beginning, so a pattern longer than the phrase keeps saying it.
    sequencer.trigger();
    QCOMPARE(spokenNames(sequencer), (std::vector<std::string> { "HH", "EH" }));
}

void SpeechTest::test_sequencer_stepMode_heldNote_shouldSustainTheFinalVowel()
{
    auto sequencer = makeSequencer("hello world");
    sequencer.setTriggerMode(SpeechSequencer::TriggerMode::Step);
    sequencer.setSyncMode(SpeechSequencer::SyncMode::Free);
    sequencer.trigger();

    // A syllable on a note lasts as long as the note, so its vowel holds while the note is down.
    const auto held = static_cast<size_t>(SampleRate * 2);
    for (size_t frame = 0; frame < held; frame++) {
        QVERIFY(sequencer.advance());
    }
    QVERIFY(sequencer.isActive());
    QCOMPARE(std::string { sequencer.phoneme()->name }, std::string { "EH" });

    sequencer.release();
    size_t remaining = 0;
    while (sequencer.advance() && remaining < static_cast<size_t>(SampleRate)) {
        remaining++;
    }
    QVERIFY(!sequencer.isActive());
}

void SpeechTest::test_sequencer_emptyPhrase_shouldNeverBecomeActive()
{
    auto sequencer = makeSequencer("");
    sequencer.trigger();
    QVERIFY(!sequencer.isActive());
    QCOMPARE(sequencer.phoneme(), nullptr);
    QVERIFY(!sequencer.advance());
}

void SpeechTest::test_device_noteOn_shouldProduceAudio()
{
    SpeechDevice device { "Speech" };
    QVERIFY(!device.hasActiveAudio());

    device.processMidiNoteOn(60, 100);
    const auto rendered = renderDevice(device, 8192);
    QVERIFY(peakAmplitude(rendered) > 0.001);
    QVERIFY(device.hasActiveAudio());
}

void SpeechTest::test_device_loudness_shouldMatchTheRestOfTheRack()
{
    const auto loudness = [](Device & device, size_t maxFrames) {
        std::vector<double> mono;
        while (mono.size() < maxFrames && (device.hasActiveAudio() || mono.empty())) {
            const auto block = renderDevice(device, 1024);
            for (size_t i = 0; i < block.size(); i += 2) {
                mono.push_back(block[i]);
            }
        }
        double sum = 0.0;
        for (auto && sample : mono) {
            sum += sample * sample;
        }
        return std::sqrt(sum / std::max<size_t>(1, mono.size()));
    };

    SpeechDevice speech { "Speech" };
    speech.setPhrase("I will destroy the whole world");
    speech.processMidiNoteOn(60, 100);
    const double spoken = loudness(speech, static_cast<size_t>(SampleRate * 12));

    SynthDevice synth { "Synth" };
    synth.processMidiNoteOn(60, 100);
    const double played = loudness(synth, static_cast<size_t>(SampleRate * 2));

    // Against loudness, not peaks. Matched peak for peak the device came out 10 dB quieter than the
    // rest of the rack in practice, because a held note has a crest factor near 5 dB and a spoken
    // phrase near 13 -- speech is mostly the quieter things between the vowels. A fader is set by
    // what a thing sounds like, so this is the comparison that decides whether it sits in a mix.
    const double difference = 20.0 * std::log10(spoken / played);
    QVERIFY2(std::abs(difference) < 6.0, qPrintable(QString::number(difference, 'f', 1) + " dB against a synth"));
}

void SpeechTest::test_device_velocitySensitivity_shouldScaleTheLevel_data()
{
    QTest::addColumn<double>("sensitivity");
    QTest::addColumn<double>("expected");

    // 1 - sensitivity + sensitivity * velocity, the formula the other devices use. Soft is velocity
    // 32 against loud at 127, so the ratio the level should fall by is that of the two velocities
    // put through it.
    const double soft = 32.0 / 127.0;
    QTest::newRow("off") << 0.0 << 1.0;
    QTest::newRow("half") << 0.5 << (1.0 - 0.5 + 0.5 * soft);
    QTest::newRow("full") << 1.0 << soft;
}

void SpeechTest::test_device_velocitySensitivity_shouldScaleTheLevel()
{
    QFETCH(double, sensitivity);
    QFETCH(double, expected);

    const auto peakAt = [sensitivity](uint8_t velocity) {
        SpeechDevice device { "Speech" };
        device.setPhrase("/aa/");
        device.setVelocitySensitivity(static_cast<float>(sensitivity));
        device.setTriggerMode(static_cast<int>(SpeechSequencer::TriggerMode::Step));
        device.setSyncMode(static_cast<int>(SpeechSequencer::SyncMode::Free));
        device.processMidiNoteOn(60, velocity);
        renderDevice(device, 8192);
        return peakAmplitude(renderDevice(device, 16384));
    };

    const double ratio = peakAt(32) / peakAt(127);
    QVERIFY2(std::abs(ratio - expected) < 0.03, qPrintable(QString::number(ratio, 'f', 3) + " against " + QString::number(expected, 'f', 3)));
}

void SpeechTest::test_device_velocitySensitivity_shouldDefaultToHalf()
{
    SpeechDevice device { "Speech" };
    QVERIFY(std::abs(device.velocitySensitivity() - 0.5f) < 0.001f);
}

void SpeechTest::test_device_noteOn_shouldFollowTheNotePitch()
{
    const auto pitchOf = [](uint8_t note) {
        SpeechDevice device { "Speech" };
        device.setPhrase("/aa/");
        device.setIntonation(0.0f);
        device.setVibratoDepth(0.0f);
        // Step mode holds its final vowel for as long as the note is down, which is the only way to
        // get a steady tone long enough to measure a pitch from.
        device.setTriggerMode(static_cast<int>(SpeechSequencer::TriggerMode::Step));
        device.setSyncMode(static_cast<int>(SpeechSequencer::SyncMode::Free));
        device.processMidiNoteOn(note, 100);
        renderDevice(device, 4096); // Let the level glide arrive before measuring.
        return fundamentalFrequency(renderDevice(device, 32768));
    };

    // Two octaves apart, so nothing but the note itself could account for the difference.
    const auto low = pitchOf(36);
    const auto high = pitchOf(60);
    QVERIFY2(std::abs(low - 65.4) < 3.0, qPrintable(QString::number(low)));
    QVERIFY2(std::abs(high - 261.6) < 8.0, qPrintable(QString::number(high)));
}

void SpeechTest::test_device_stressedSyllable_shouldTakeAPitchAccent()
{
    const auto pitchOfPhrase = [](float intonation, size_t syllable) {
        SpeechDevice device { "Speech" };
        // "de-STROY": the second syllable is the stressed one.
        device.setPhrase("destroy");
        device.setIntonation(intonation);
        device.setVibratoDepth(0.0f);
        device.setRate(0.0f); // Slowest, so each syllable is long enough to measure a pitch from.
        device.processMidiNoteOn(60, 100);

        std::vector<double> rendered;
        while (device.hasActiveAudio() && rendered.size() < static_cast<size_t>(SampleRate * 8)) {
            const auto block = renderDevice(device, 2048);
            rendered.insert(rendered.end(), block.begin(), block.end());
        }
        // Frames, not samples: the buffer is interleaved stereo.
        const size_t frames = rendered.size() / 2;
        const size_t from = syllable == 0 ? frames / 8 : frames * 5 / 8;
        return fundamentalFrequency(std::vector<double>(rendered.begin() + static_cast<long>(from * 2),
                                                        rendered.begin() + static_cast<long>(std::min(frames, from + frames / 4) * 2)));
    };

    // A pitch accent is the other half of what marks stress, the first half being length. With the
    // intonation control at zero there is no contour at all, so the two syllables must agree.
    const double weakFlat = pitchOfPhrase(0.0f, 0);
    const double strongFlat = pitchOfPhrase(0.0f, 1);
    QVERIFY(weakFlat > 0.0 && strongFlat > 0.0);
    QVERIFY2(std::abs(strongFlat - weakFlat) < weakFlat * 0.03, qPrintable(QString::number(weakFlat) + " vs " + QString::number(strongFlat)));

    const double weak = pitchOfPhrase(1.0f, 0);
    const double strong = pitchOfPhrase(1.0f, 1);
    QVERIFY2(strong > weak, qPrintable(QString::number(weak) + " -> " + QString::number(strong)));
}

void SpeechTest::test_device_tuning_shouldBeExact_data()
{
    QTest::addColumn<int>("note");

    // Three octaves of the range a voice is played over.
    for (auto && note : { 36, 43, 48, 55, 60, 67, 72 }) {
        QTest::newRow(QString::number(note).toUtf8()) << note;
    }
}

void SpeechTest::test_device_tuning_shouldBeExact()
{
    QFETCH(int, note);

    SpeechDevice device { "Speech" };
    device.setPhrase("/aa/");
    // Intonation and vibrato both move the pitch on purpose -- a phrase falls as it goes and a
    // stressed syllable is lifted -- so tuning is what is left when neither is applied.
    device.setIntonation(0.0f);
    device.setVibratoDepth(0.0f);
    device.setTriggerMode(static_cast<int>(SpeechSequencer::TriggerMode::Step));
    device.setSyncMode(static_cast<int>(SpeechSequencer::SyncMode::Free));
    device.processMidiNoteOn(static_cast<uint8_t>(note), 100);

    renderDevice(device, 8192); // Let the level arrive before measuring.
    const double measured = preciseFundamental(renderDevice(device, 65536));
    const double expected = 440.0 * std::pow(2.0, (note - 69) / 12.0);
    const double cents = 1200.0 * std::log2(measured / expected);

    // A cent is already far below anything audible; the measurement itself is good to a few
    // hundredths, so this is as tight as the test can honestly be.
    QVERIFY2(std::abs(cents) < 1.0, qPrintable(QString::number(cents, 'f', 3) + " cents at " + QString::number(measured, 'f', 2) + " Hz"));
}

void SpeechTest::test_device_voiceType_shouldRaiseTheFormants()
{
    const auto firstFormant = [](int type) {
        SpeechDevice device { "Speech" };
        device.setPhrase("/aa/");
        device.setVoiceType(type);
        device.setIntonation(0.0f);
        device.setVibratoDepth(0.0f);
        device.setTriggerMode(static_cast<int>(SpeechSequencer::TriggerMode::Step));
        device.setSyncMode(static_cast<int>(SpeechSequencer::SyncMode::Free));
        device.processMidiNoteOn(48, 100);
        renderDevice(device, 8192);
        return spectralPeak(renderDevice(device, 65536), 300.0, 1400.0);
    };

    // A woman's tract is about a sixth shorter, which lifts every formant by about that much. The
    // note is held the same on purpose: this is the part of the difference that is not pitch.
    const double male = firstFormant(0);
    const double female = firstFormant(1);
    QVERIFY2(female > male * 1.08, qPrintable(QString::number(male, 'f', 0) + " -> " + QString::number(female, 'f', 0) + " Hz"));
}

void SpeechTest::test_device_formantShift_shouldBeNeutralAtHalfTravel()
{
    const auto firstFormant = [](float shift, int type) {
        SpeechDevice device { "Speech" };
        device.setPhrase("/aa/");
        device.setVoiceType(type);
        device.setFormantShift(shift);
        device.setIntonation(0.0f);
        device.setVibratoDepth(0.0f);
        device.setTriggerMode(static_cast<int>(SpeechSequencer::TriggerMode::Step));
        device.setSyncMode(static_cast<int>(SpeechSequencer::SyncMode::Free));
        device.processMidiNoteOn(48, 100);
        renderDevice(device, 8192);
        return spectralPeak(renderDevice(device, 65536), 300.0, 1400.0);
    };

    // Half travel is no trim: the voice type alone decides the voice unless the user says otherwise.
    // /a/ has its first formant at 730 Hz, and the male type is the table's own voice.
    QVERIFY2(std::abs(firstFormant(0.5f, 0) - 730.0) < 80.0, qPrintable(QString::number(firstFormant(0.5f, 0), 'f', 0)));
    QVERIFY(firstFormant(1.0f, 0) > firstFormant(0.5f, 0));
    QVERIFY(firstFormant(0.0f, 0) < firstFormant(0.5f, 0));
}

void SpeechTest::test_device_phrase_shouldCompileOnAssignment()
{
    SpeechDevice device { "Speech" };
    QCOMPARE(device.phrase(), SpeechDevice::defaultPhrase());
    QVERIFY(!device.phrasePhonemes().empty());

    device.setPhrase("noteahead");
    QCOMPARE(device.phrase(), std::string { "noteahead" });
    QCOMPARE(device.phrasePhonemes(), std::string { "N OW T IY HH EH D" });
    QVERIFY(device.syllableCount() > 0);
}

void SpeechTest::test_device_emptyPhrase_shouldStaySilent()
{
    SpeechDevice device { "Speech" };
    device.setPhrase("");
    device.processMidiNoteOn(60, 100);
    QCOMPARE(peakAmplitude(renderDevice(device, 8192)), 0.0);
}

void SpeechTest::test_device_afterThePhrase_shouldGoInactive()
{
    SpeechDevice device { "Speech" };
    device.setPhrase("/aa/");
    device.processMidiNoteOn(60, 100);

    // A phrase is an event and runs to its end, so the device has to release the slot by itself
    // rather than waiting for a note off that may never come.
    for (int block = 0; block < 200 && device.hasActiveAudio(); block++) {
        renderDevice(device, 4096);
    }
    QVERIFY(!device.hasActiveAudio());
}

void SpeechTest::test_device_endOfPhrase_shouldNotStep()
{
    SpeechDevice device { "Speech" };
    device.setPhrase("/aa/");
    device.processMidiNoteOn(60, 100);

    std::vector<double> left;
    for (int block = 0; block < 60; block++) {
        const auto rendered = renderDevice(device, 4096);
        for (size_t i = 0; i < rendered.size(); i += 2) {
            left.push_back(rendered[i]);
        }
    }

    // The phrase running out must be a fade, not a step to zero, which would click.
    //
    // Measured against what the signal does while it is still speaking, rather than against a fixed
    // number: a sawtooth-excited bank steps once per period by its nature, and the question is only
    // whether the ending steps harder than that.
    const auto stepIn = [&](size_t from, size_t to) {
        double peak = 0.0;
        for (size_t i = std::max<size_t>(from, 1); i < std::min(to, left.size()); i++) {
            peak = std::max(peak, std::abs(left[i] - left[i - 1]));
        }
        return peak;
    };

    size_t lastSounding = left.size();
    while (lastSounding > 1 && std::abs(left[lastSounding - 1]) < 1.0e-5) {
        lastSounding--;
    }

    const double ending = stepIn(lastSounding > 4096 ? lastSounding - 4096 : 0, lastSounding + 4096);
    const double speaking = stepIn(4096, 12288);
    QVERIFY2(ending < speaking * 1.5, qPrintable(QString::number(ending) + " vs " + QString::number(speaking)));
}

void SpeechTest::test_device_fitMode_shouldFollowTheContextTempo()
{
    const auto framesAtBpm = [](double bpm) {
        SpeechDevice device { "Speech" };
        device.setPhrase("hello world");
        device.setSyncMode(static_cast<int>(SpeechSequencer::SyncMode::Fit));
        device.setSyncLength(16); // One bar of sixteenths.
        device.processMidiNoteOn(60, 100);

        size_t frames = 0;
        while (device.hasActiveAudio() && frames < static_cast<size_t>(SampleRate * 30)) {
            renderDevice(device, 512, bpm);
            frames += 512;
        }
        return frames;
    };

    // The tempo is read from the audio context per block rather than cached, so an offline render
    // follows the same tempo playback does. Halving it must double the phrase.
    const auto fast = framesAtBpm(160.0);
    const auto slow = framesAtBpm(80.0);
    QVERIFY2(std::abs(static_cast<double>(slow) / static_cast<double>(fast) - 2.0) < 0.15,
             qPrintable(QString::number(fast) + " -> " + QString::number(slow)));
}

void SpeechTest::test_device_midiCc_shouldReachTheParameters_data()
{
    QTest::addColumn<int>("controller");
    QTest::addColumn<QString>("name");

    // The four every device answers to, on the numbers the rack uses throughout.
    QTest::newRow("CC 7 fader") << 7 << "fader";
    QTest::newRow("CC 10 pan") << 10 << "pan";
    QTest::newRow("CC 74 LPF") << 74 << "lpfCutoff";
    QTest::newRow("CC 81 HPF") << 81 << "hpfCutoff";
}

void SpeechTest::test_device_midiCc_shouldReachTheParameters()
{
    QFETCH(int, controller);
    QFETCH(QString, name);

    SpeechDevice device { "Speech" };

    // Advertised, or nothing in the application offers it to the user.
    const auto available = device.availableMidiCcControllers();
    QVERIFY2(std::ranges::any_of(available, [controller](const MidiCcController & c) {
                 return c.number == controller;
             }),
             qPrintable(QString::number(controller)));

    const auto valueOf = [&] {
        const auto parameter = device.parameter(name.toStdString());
        return parameter ? parameter->get().value() : -1.0f;
    };

    device.processMidiCc(static_cast<uint8_t>(controller), 0, 0);
    const float low = valueOf();
    device.processMidiCc(static_cast<uint8_t>(controller), 127, 0);
    const float high = valueOf();

    QVERIFY2(high > low, qPrintable(name + ": " + QString::number(low) + " -> " + QString::number(high)));
}

void SpeechTest::test_device_midiCc_shouldNotAuthorTheProject()
{
    SpeechDevice device { "Speech" };
    const auto authored = device.parameter("lpfCutoff")->get().authoredValue();

    device.processMidiCc(74, 0, 0);

    // A CC writes the live layer only. Otherwise playing a song back would rewrite the patch it is
    // playing, and saving afterwards would keep whatever the automation last sent.
    QCOMPARE(device.parameter("lpfCutoff")->get().authoredValue(), authored);
    QVERIFY(device.parameter("lpfCutoff")->get().value() < authored);
}

void SpeechTest::test_device_midiCc_reset_shouldRestoreTheAuthoredValues()
{
    SpeechDevice device { "Speech" };
    const auto authored = device.parameter("hpfCutoff")->get().authoredValue();

    device.processMidiCc(81, 127, 0);
    QVERIFY(device.parameter("hpfCutoff")->get().value() > authored);

    // CC 121, Reset All Controllers: what the transport sends when it stops.
    device.processMidiCc(121, 0, 0);
    QCOMPARE(device.parameter("hpfCutoff")->get().value(), authored);
}

void SpeechTest::test_device_allNotesOff_shouldStopSpeaking()
{
    SpeechDevice device { "Speech" };
    device.processMidiNoteOn(60, 100);
    renderDevice(device, 4096);
    QVERIFY(device.hasActiveAudio());

    device.processMidiAllNotesOff();
    for (int block = 0; block < 20 && device.hasActiveAudio(); block++) {
        renderDevice(device, 4096);
    }
    QVERIFY(!device.hasActiveAudio());
}

void SpeechTest::test_device_reset_shouldRestoreTheDefaultPhrase()
{
    SpeechDevice device { "Speech" };
    device.setPhrase("something else");
    device.setIntonation(0.9f);
    device.reset();

    QCOMPARE(device.phrase(), SpeechDevice::defaultPhrase());
    QVERIFY(std::abs(device.intonation() - 0.4f) < 0.001f);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::SpeechTest)
