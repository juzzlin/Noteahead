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

#include "formant_voice_test.hpp"

#include "../../domain/dsp/fft.hpp"
#include "../../domain/dsp/speech/formant_voice.hpp"
#include "../../domain/dsp/speech/phoneme.hpp"
#include "../../domain/dsp/speech/text_to_phonemes.hpp"

#include <QDataStream>
#include <QFile>
#include <QTest>

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <string>
#include <utility>
#include <vector>

namespace noteahead {

namespace {

constexpr double SampleRate = 48000.0;
//! A male speaking fundamental. Low enough that the harmonics sample the spectral envelope densely
//! enough for a formant peak to land on one of them.
constexpr double Fundamental = 110.0;

constexpr int FftSize = 16384;
//! Frames dropped before analysis, so the level glide and the formant glide have both arrived.
constexpr size_t SettleFrames = 6000;

using Utterance = std::vector<std::pair<std::string, size_t>>;

FormantVoice makeVoice()
{
    FormantVoice voice;
    voice.setSampleRate(SampleRate);
    voice.setFrequency(Fundamental);
    return voice;
}

std::vector<double> render(FormantVoice & voice, const Utterance & utterance)
{
    std::vector<double> output;
    for (size_t index = 0; index < utterance.size(); index++) {
        const auto & [name, frames] = utterance[index];
        const auto * spec = speechPhoneme(name);
        Q_ASSERT(spec);
        // The next phoneme is handed over as the device hands it over, so that an unvoiced stop can
        // aspirate into what follows it here too.
        const auto * next = index + 1 < utterance.size() ? speechPhoneme(utterance[index + 1].first) : nullptr;
        voice.setPhoneme(*spec, next, static_cast<double>(frames) / SampleRate);
        for (size_t frame = 0; frame < frames; frame++) {
            voice.setPhonemeProgress(static_cast<double>(frame) / static_cast<double>(frames));
            output.push_back(voice.nextSample());
        }
    }
    return output;
}

//! Magnitude spectrum of FftSize frames taken from @p offset, Hann windowed.
std::vector<double> spectrum(const std::vector<double> & signal, size_t offset)
{
    std::vector<double> re(FftSize, 0.0);
    std::vector<double> im(FftSize, 0.0);
    for (int i = 0; i < FftSize; i++) {
        const size_t index = offset + static_cast<size_t>(i);
        if (index >= signal.size()) {
            break;
        }
        const double window = 0.5 - 0.5 * std::cos(2.0 * M_PI * i / (FftSize - 1));
        re[static_cast<size_t>(i)] = signal[index] * window;
    }
    Fft::forward(re.data(), im.data(), FftSize);

    std::vector<double> magnitudes(FftSize / 2, 0.0);
    for (size_t i = 0; i < magnitudes.size(); i++) {
        magnitudes[i] = std::hypot(re[i], im[i]);
    }
    return magnitudes;
}

int binOf(double hz)
{
    return static_cast<int>(std::lround(hz * FftSize / SampleRate));
}

double binFrequency(size_t bin)
{
    return static_cast<double>(bin) * SampleRate / FftSize;
}

//! Frequency of the loudest bin between @p lowHz and @p highHz.
double peakFrequency(const std::vector<double> & magnitudes, double lowHz, double highHz)
{
    const auto low = static_cast<size_t>(std::max(1, binOf(lowHz)));
    const auto high = std::min(magnitudes.size() - 1, static_cast<size_t>(binOf(highHz)));
    size_t peak = low;
    for (size_t i = low; i <= high; i++) {
        if (magnitudes[i] > magnitudes[peak]) {
            peak = i;
        }
    }
    return binFrequency(peak);
}

double bandEnergy(const std::vector<double> & magnitudes, double lowHz, double highHz)
{
    const auto low = static_cast<size_t>(std::max(1, binOf(lowHz)));
    const auto high = std::min(magnitudes.size() - 1, static_cast<size_t>(binOf(highHz)));
    double sum = 0.0;
    for (size_t i = low; i <= high; i++) {
        sum += magnitudes[i] * magnitudes[i];
    }
    return sum;
}

double rms(const std::vector<double> & signal, size_t from, size_t to)
{
    const auto last = std::min(to, signal.size());
    if (from >= last) {
        return 0.0;
    }
    double sumSquares = 0.0;
    for (size_t i = from; i < last; i++) {
        sumSquares += signal[i] * signal[i];
    }
    return std::sqrt(sumSquares / static_cast<double>(last - from));
}

//! RMS of one phoneme, spoken between two vowels, over the part of it that makes a sound.
//!
//! A stop spends most of its length in closure, so measuring the whole phoneme reports how long it
//! is rather than how loud. Only the part from the release onwards is measured, which is the burst
//! and the aspiration after it -- the part a listener actually hears.
double phonemeLevel(const std::string & phoneme)
{
    const auto * spec = speechPhoneme(phoneme);
    const auto frames = static_cast<size_t>(SampleRate * spec->nominalMs / 1000.0);
    auto voice = makeVoice();
    // Between two vowels, which is the context these are heard in: it decides what the bank carries
    // when the phoneme starts, and it lets an unvoiced stop aspirate into what follows.
    const auto rendered = render(voice, { { "AA", frames }, { phoneme, frames }, { "AA", frames } });

    if (spec->type == PhonemeType::Plosive) {
        // The burst, not the whole release. What follows it is breath, which is meant to be quiet
        // and brief; measuring across it reports how restrained the aspiration is rather than how
        // loud the stop is, and a stop is heard as its transient.
        const auto release = 2 * frames - static_cast<size_t>(SampleRate * (spec->voicing > 0.0 ? 0.012 : 0.040));
        return rms(rendered, release, release + static_cast<size_t>(SampleRate * 0.008));
    }
    return rms(rendered, frames + frames / 10, 2 * frames);
}

//! The largest step the waveform takes between adjacent samples in a window. What a click is.
double peakStep(const std::vector<double> & signal, size_t from, size_t to)
{
    double peak = 0.0;
    for (size_t i = std::max<size_t>(from, 1); i < std::min(to, signal.size()); i++) {
        peak = std::max(peak, std::abs(signal[i] - signal[i - 1]));
    }
    return peak;
}

double peakAmplitude(const std::vector<double> & signal)
{
    double peak = 0.0;
    for (auto && sample : signal) {
        peak = std::max(peak, std::abs(sample));
    }
    return peak;
}

//! Largest jump in the second difference of @p signal, divided by its RMS. Borrowed from
//! CascadedSvfTest: a step in the output is what a listener hears as a click, and it shows up here
//! as a spike far above what a continuously moving signal produces.
double peakSecondDifferenceOverRms(const std::vector<double> & signal, size_t from, size_t to)
{
    const double level = rms(signal, from, to);
    if (level <= 1.0e-12) {
        return 0.0;
    }
    double peak = 0.0;
    for (size_t i = from + 1; i + 1 < std::min(to, signal.size()); i++) {
        peak = std::max(peak, std::abs(signal[i + 1] - 2.0 * signal[i] + signal[i - 1]));
    }
    return peak / level;
}

} // namespace

void FormantVoiceTest::test_phonemeTable_everyPhoneme_shouldHaveAscendingFormants()
{
    QVERIFY(!speechPhonemes().empty());

    for (auto && spec : speechPhonemes()) {
        const auto name = QString::fromStdString(std::string { spec.name });
        QVERIFY2(spec.nominalMs > 0, qPrintable(name));
        QVERIFY2(spec.voicing >= 0.0 && spec.voicing <= 1.0, qPrintable(name));
        QVERIFY2(spec.amplitude >= 0.0, qPrintable(name));

        for (size_t i = 0; i < spec.formants.size(); i++) {
            QVERIFY2(spec.formants[i].frequency > 0.0, qPrintable(name));
            QVERIFY2(spec.formants[i].bandwidth > 0.0, qPrintable(name));
            QVERIFY2(spec.formants[i].amplitude >= 0.0, qPrintable(name));
            if (i > 0) {
                // A bank that can only add positive resonances has no way to spell a formant that
                // sits below the one before it, so an out-of-order entry is a typo, not a voice.
                QVERIFY2(spec.formants[i].frequency > spec.formants[i - 1].frequency, qPrintable(name));
            }
        }

        if (spec.glideTo.has_value()) {
            for (size_t i = 0; i < spec.glideTo->size(); i++) {
                QVERIFY2(spec.glideTo->at(i).frequency > 0.0, qPrintable(name));
                QVERIFY2(spec.glideTo->at(i).bandwidth > 0.0, qPrintable(name));
            }
        }
    }
}

void FormantVoiceTest::test_phonemeTable_lookup_shouldFindArpabetNamesOnly()
{
    QVERIFY(speechPhoneme("AA"));
    QVERIFY(speechPhoneme("SH"));
    QVERIFY(speechPhoneme("_"));
    QCOMPARE(speechPhoneme("aa"), nullptr);
    QCOMPARE(speechPhoneme("QQ"), nullptr);
    QCOMPARE(speechPhoneme(""), nullptr);

    QCOMPARE(speechSilence().type, PhonemeType::Silence);
    QCOMPARE(speechSilence().amplitude, 0.0);
}

void FormantVoiceTest::test_formantVoice_heldVowel_shouldPeakNearItsFirstFormant()
{
    auto voice = makeVoice();
    const auto rendered = render(voice, { { "AA", SettleFrames + FftSize } });
    const auto magnitudes = spectrum(rendered, SettleFrames);

    // /ɑ/ has F1 at 730 Hz. The source only samples the envelope at multiples of the fundamental,
    // so the peak lands on the harmonic nearest it rather than on the formant itself.
    const double peak = peakFrequency(magnitudes, 200.0, 1000.0);
    QVERIFY2(std::abs(peak - 730.0) < Fundamental, qPrintable(QString::number(peak)));
}

void FormantVoiceTest::test_formantVoice_frontAndBackVowels_shouldDifferInFormantBands()
{
    auto frontVoice = makeVoice();
    const auto front = spectrum(render(frontVoice, { { "IY", SettleFrames + FftSize } }), SettleFrames);
    auto backVoice = makeVoice();
    const auto back = spectrum(render(backVoice, { { "AA", SettleFrames + FftSize } }), SettleFrames);

    // /i/ puts F1 down at 270 Hz and F2 up at 2290; /ɑ/ does the opposite, F1 730 and F2 1090. Those
    // two coordinates are the whole of what tells one vowel from another, so if they do not separate
    // here, no vowel in the table is telling a listener anything.
    //
    // The bands are chosen around each vowel's own F1 rather than around a shared one, because /ɑ/'s
    // F3 at 2440 lands inside any band wide enough to catch /i/'s F2 and hides the difference.
    QVERIFY(bandEnergy(front, 200.0, 500.0) > bandEnergy(back, 200.0, 500.0) * 4.0);
    QVERIFY(bandEnergy(back, 600.0, 1300.0) > bandEnergy(front, 600.0, 1300.0) * 4.0);

    // And F2 itself: /i/ has a peak up where /ɑ/ has only the skirt of F1.
    const double frontSecond = peakFrequency(front, 1500.0, 3000.0);
    QVERIFY2(std::abs(frontSecond - 2290.0) < Fundamental, qPrintable(QString::number(frontSecond)));
}

void FormantVoiceTest::test_formantVoice_diphthong_shouldMoveItsSecondFormant()
{
    auto voice = makeVoice();
    // /aɪ/ starts at /ɑ/, F2 1090, and ends at /i/, F2 2290. A listener identifies it by that
    // movement, so a diphthong frozen at either end is a different phoneme.
    const size_t frames = SettleFrames + 3 * FftSize;
    const auto rendered = render(voice, { { "AY", frames } });

    const auto early = spectrum(rendered, SettleFrames);
    const auto late = spectrum(rendered, frames - FftSize);

    const double earlyRatio = bandEnergy(early, 2000.0, 2600.0) / bandEnergy(early, 900.0, 1300.0);
    const double lateRatio = bandEnergy(late, 2000.0, 2600.0) / bandEnergy(late, 900.0, 1300.0);
    QVERIFY(lateRatio > earlyRatio * 4.0);
}

void FormantVoiceTest::test_formantVoice_formantShift_shouldMoveTheWholeVowelSpace()
{
    auto neutralVoice = makeVoice();
    const auto neutral = spectrum(render(neutralVoice, { { "AA", SettleFrames + FftSize } }), SettleFrames);

    auto shiftedVoice = makeVoice();
    shiftedVoice.setFormantShift(1.25);
    const auto shifted = spectrum(render(shiftedVoice, { { "AA", SettleFrames + FftSize } }), SettleFrames);

    const double neutralPeak = peakFrequency(neutral, 200.0, 1200.0);
    const double shiftedPeak = peakFrequency(shifted, 200.0, 1400.0);
    QVERIFY2(shiftedPeak > neutralPeak, qPrintable(QString::number(neutralPeak) + " -> " + QString::number(shiftedPeak)));
    QVERIFY(std::abs(shiftedPeak - neutralPeak * 1.25) < 2.0 * Fundamental);
}

void FormantVoiceTest::test_formantVoice_sourceRolloff_shouldSoftenTheVoice()
{
    const auto highAgainstLow = [](double rolloff) {
        auto voice = makeVoice();
        voice.setSourceRolloff(rolloff);
        const auto magnitudes = spectrum(render(voice, { { "AA", SettleFrames + FftSize } }), SettleFrames);
        return bandEnergy(magnitudes, 2000.0, 6000.0) / bandEnergy(magnitudes, 200.0, 1000.0);
    };

    // Half of what separates a woman's voice from a man's, the other half being the shorter tract:
    // the source falls away faster, and that is why it reads as softer rather than merely higher.
    //
    // Measured here rather than on the device, where the two cannot be told apart: the female
    // setting also lifts the formants, which puts energy back into the very band the steeper source
    // takes it out of.
    //
    // It is a filter of its own rather than a change to the glottal tilt, because the tilt and lip
    // radiation together are a one-pole high pass: flat above its corner, so moving that corner
    // moved this ratio by under 3%.
    QVERIFY2(highAgainstLow(3500.0) < highAgainstLow(0.0) * 0.75,
             qPrintable(QString::number(highAgainstLow(0.0), 'e', 2) + " -> " + QString::number(highAgainstLow(3500.0), 'e', 2)));
}

void FormantVoiceTest::test_formantVoice_voicelessPlosive_shouldBeSilentThroughTheClosure()
{
    auto voice = makeVoice();
    const size_t frames = static_cast<size_t>(SampleRate * 0.2);
    // Followed by a vowel, because a stop with nothing after it is not released at all.
    const auto rendered = render(voice, { { "P", frames }, { "AA", frames } });

    // What follows an aspirated stop's release takes a fixed 40 ms, so the release sits that far
    // from the end of the phoneme however long the phoneme is.
    const auto release = frames - static_cast<size_t>(SampleRate * 0.040);

    // A stop that is not silent before its burst does not read as a stop. Measured from a quarter of
    // the way in rather than from the start: entering a stop, the voice glides down from whatever
    // preceded it, and that fall is the closing gesture rather than a failure to close.
    const double closure = rms(rendered, frames / 4, release - static_cast<size_t>(SampleRate * 0.01));
    const double burst = rms(rendered, release, release + static_cast<size_t>(SampleRate * 0.003));
    QVERIFY2(burst > closure * 50.0, qPrintable(QString::number(burst, 'e', 2) + " vs closure " + QString::number(closure, 'e', 2)));
}

void FormantVoiceTest::test_formantVoice_voicedPlosive_shouldBuzzThroughTheClosure()
{
    const size_t frames = static_cast<size_t>(SampleRate * 0.2);

    auto voicelessVoice = makeVoice();
    const auto voiceless = render(voicelessVoice, { { "P", frames }, { "AA", frames } });
    auto voicedVoice = makeVoice();
    const auto voiced = render(voicedVoice, { { "B", frames }, { "AA", frames } });

    // B and P share a burst spectrum. The voice bar heard through the closure is the whole of the
    // difference between them, so if it is not there the two are the same phoneme.
    // Inside the closure of both: the unvoiced stop releases at 45% and the voiced one at 65%.
    const double voicelessClosure = rms(voiceless, static_cast<size_t>(frames * 0.25), static_cast<size_t>(frames * 0.42));
    const double voicedClosure = rms(voiced, static_cast<size_t>(frames * 0.25), static_cast<size_t>(frames * 0.42));
    QVERIFY(voicedClosure > voicelessClosure * 10.0);
}

void FormantVoiceTest::test_formantVoice_unvoicedStop_shouldAspirateIntoWhatFollows()
{
    const auto frames = static_cast<size_t>(SampleRate * 0.2);

    auto alone = makeVoice();
    const auto isolated = render(alone, { { "T", frames }, { "_", frames } });
    auto beforeVowel = makeVoice();
    const auto aspirated = render(beforeVowel, { { "T", frames }, { "AA", frames } });

    // English puts 40 to 80 ms of breath between an unvoiced stop and the voice after it. Without it
    // a /t/ is a burst and nothing else, which is a click -- and a click through resonant bands is a
    // hihat, which is what this sounded like.
    const auto after = static_cast<size_t>(frames * 0.6);
    const double withVowel = rms(aspirated, after, frames);
    const double withNothing = rms(isolated, after, frames);
    QVERIFY2(withVowel > withNothing * 100.0, qPrintable(QString::number(withVowel, 'e', 2) + " vs " + QString::number(withNothing, 'e', 2)));
}

void FormantVoiceTest::test_formantVoice_aspiration_shouldBeShapedByTheFollowingVowel()
{
    const auto frames = static_cast<size_t>(SampleRate * 0.2);

    const auto aspirationSpectrum = [&](const std::string & vowel) {
        auto voice = makeVoice();
        const auto rendered = render(voice, { { "T", frames }, { vowel, frames } });
        // Inside the aspiration, after the burst and before the vowel starts.
        return spectrum(rendered, static_cast<size_t>(frames * 0.55));
    };

    // The breath passes through a tract that has already taken up the position for the coming vowel,
    // so the aspiration is coloured by that vowel and leads into it. That colouring is the difference
    // between /t/ sounding like a consonant and sounding like percussion.
    const auto beforeFront = aspirationSpectrum("IY");
    const auto beforeBack = aspirationSpectrum("AA");

    // Compared as shape rather than as level: a vowel's intrinsic intensity depends on how open it
    // is, so /i/ is legitimately quieter than /a/ and an absolute comparison would only measure that.
    const auto tilt = [](const std::vector<double> & m) {
        return bandEnergy(m, 2000.0, 2600.0) / std::max(bandEnergy(m, 600.0, 1300.0), 1.0e-12);
    };
    QVERIFY2(tilt(beforeFront) > tilt(beforeBack) * 4.0,
             qPrintable(QString::number(tilt(beforeFront), 'e', 2) + " vs " + QString::number(tilt(beforeBack), 'e', 2)));
}

void FormantVoiceTest::test_formantVoice_stopAfterAFricative_shouldNotAspirate()
{
    const auto frames = static_cast<size_t>(SampleRate * 0.115);

    const auto breath = [&](const Utterance & utterance) {
        auto voice = makeVoice();
        const auto rendered = render(voice, utterance);
        const auto stopEnds = utterance.size() == 3 ? 2 * frames : frames;
        // From 35 ms before the stop ends to 15 ms before it: aspiration fills this, and an
        // unaspirated stop is still closed here because its burst is only 12 ms from the end.
        return rms(rendered, stopEnds - static_cast<size_t>(SampleRate * 0.035), stopEnds - static_cast<size_t>(SampleRate * 0.015));
    };

    // "stop" against "top": a stop after /s/ is unaspirated, and that is not a nicety. Aspirating it
    // puts the breath immediately after the fricative's own noise, and the two run together into one
    // long hiss -- which is what the /str/ of "destroy" sounded like.
    const double afterFricative = breath({ { "S", frames }, { "T", frames }, { "AA", frames } });
    const double afterVowel = breath({ { "T", frames }, { "AA", frames } });
    QVERIFY2(afterFricative < afterVowel * 0.25,
             qPrintable(QString::number(afterFricative, 'e', 2) + " vs " + QString::number(afterVowel, 'e', 2)));
}

void FormantVoiceTest::test_formantVoice_aspiration_shouldNotStretchWithThePhoneme()
{
    // Aspiration lasts what it lasts in speech, whatever the syllable has been stretched to. When it
    // was a share of the phoneme instead, stress and phrase-final lengthening turned a /t/ into
    // 120 ms of breath, and a word like "destroy" was more frication than voice.
    const auto measure = [](double seconds) {
        const auto frames = static_cast<size_t>(SampleRate * seconds);
        auto voice = makeVoice();
        // Preceded by a vowel so the closure is a real one, reached by closing rather than by the
        // level never having risen in the first place.
        const auto rendered = render(voice, { { "AA", frames }, { "T", frames }, { "AA", frames } });
        const auto at = [&](double from, double to) {
            return rms(rendered, 2 * frames - static_cast<size_t>(SampleRate * from), 2 * frames - static_cast<size_t>(SampleRate * to));
        };
        // The breath, and the stretch of closure just before it.
        // Starting after the burst has decayed, so this measures the breath alone: how much of the
        // burst falls inside the window otherwise shifts with rounding and swamps the comparison.
        // The ten milliseconds just before the release, which is deep closure for every duration.
        return std::pair { at(0.030, 0.005), at(0.055, 0.045) };
    };

    double loudest = 0.0;
    double quietest = std::numeric_limits<double>::max();
    for (auto && seconds : { 0.115, 0.220, 0.400 }) {
        const auto [breath, closure] = measure(seconds);
        // Whatever the phoneme's length, the stretch just before the breath is closure.
        QVERIFY2(breath > closure * 10.0, qPrintable(QString::number(seconds) + ": " + QString::number(breath, 'e', 2) + " vs " + QString::number(closure, 'e', 2)));
        loudest = std::max(loudest, breath);
        quietest = std::min(quietest, breath);
    }

    // And it is the same breath each time, not a proportional share of a longer phoneme.
    QVERIFY2(loudest < quietest * 1.3, qPrintable(QString::number(quietest, 'e', 2) + " to " + QString::number(loudest, 'e', 2)));
}

void FormantVoiceTest::test_formantVoice_voicedStop_shouldNotAspirate()
{
    const auto frames = static_cast<size_t>(SampleRate * 0.2);

    // Measured from just after each stop's own release to the end of it: for an unvoiced stop that
    // span is the aspiration and is loud, and for a voiced one there is barely any span at all --
    // voicing starts at the release, which is what a voice onset time near zero means. Aspirating a
    // voiced stop would turn /b/ into /p/.
    const auto breathAfterRelease = [&](const std::string & stop, double releaseSeconds) {
        auto voice = makeVoice();
        const auto rendered = render(voice, { { stop, frames }, { "AA", frames } });
        const auto from = frames - static_cast<size_t>(SampleRate * releaseSeconds) + static_cast<size_t>(SampleRate * 0.008);
        return rms(rendered, from, frames);
    };

    const double voiced = breathAfterRelease("B", 0.012);
    const double unvoiced = breathAfterRelease("P", 0.040);
    QVERIFY2(voiced < unvoiced * 0.5, qPrintable(QString::number(voiced, 'e', 2) + " vs " + QString::number(unvoiced, 'e', 2)));
}

void FormantVoiceTest::test_formantVoice_phraseFinalStop_shouldNotBeReleased_data()
{
    QTest::addColumn<QString>("stop");

    QTest::newRow("D") << "D";
    QTest::newRow("T") << "T";
    QTest::newRow("K") << "K";
    QTest::newRow("B") << "B";
}

void FormantVoiceTest::test_formantVoice_phraseFinalStop_shouldNotBeReleased()
{
    QFETCH(QString, stop);
    const auto frames = static_cast<size_t>(SampleRate * 0.2);
    const auto name = stop.toStdString();

    auto ending = makeVoice();
    const auto atEnd = render(ending, { { "AA", frames }, { name, frames } });
    auto beforeVowel = makeVoice();
    const auto released = render(beforeVowel, { { "AA", frames }, { name, frames }, { "AA", frames } });

    // An English speaker ending a word on a stop forms the closure and stops there. Releasing one
    // into silence leaves a bare burst with nothing after it, which is a click -- and a click through
    // resonant bands is a cymbal, which is what "world" ended with.
    // Measured as a peak rather than as energy: a voiced stop's closure buzzes whether it is released
    // or not, and that voice bar is meant to be there. What must not be there is the burst, which is a
    // transient and shows up in the peak.
    const auto window = [&](const std::vector<double> & signal) {
        return peakAmplitude(std::vector<double>(signal.begin() + static_cast<long>(frames + frames / 2),
                                                 signal.begin() + static_cast<long>(2 * frames)));
    };
    QVERIFY2(window(atEnd) < window(released) * 0.25,
             qPrintable(QString::number(window(atEnd), 'e', 2) + " vs " + QString::number(window(released), 'e', 2)));
}

void FormantVoiceTest::test_formantVoice_fricative_shouldBeBroadbandAndUnpitched()
{
    auto voice = makeVoice();
    const auto magnitudes = spectrum(render(voice, { { "S", SettleFrames + FftSize } }), SettleFrames);

    // /s/ is noise around 5.5 kHz, not a tone: nearly all of its energy sits above 3 kHz, and there
    // must be no harmonic of the fundamental left down where the vowels live.
    const double high = bandEnergy(magnitudes, 3000.0, 9000.0);
    const double low = bandEnergy(magnitudes, 100.0, 1500.0);
    QVERIFY(high > low * 20.0);
}

void FormantVoiceTest::test_phonemeTable_noiseBands_shouldBeBroad()
{
    // The property that decides whether noise reads as a rush of air or as a tuned tone. At a Q of
    // five to eight the bands stand apart as peaks: /s/ came out thin and piercing, and the stop
    // bursts -- noise through a resonant band with an exponential decay -- were a hihat, because
    // that is exactly how a hihat is made.
    //
    // Only the bands that shape noise. A voiced fricative's lowest band is its voice bar, a real
    // resonance of the tract rather than a noise shelf, and it is meant to be narrow.
    for (auto && spec : speechPhonemes()) {
        if (spec.type != PhonemeType::Fricative && spec.type != PhonemeType::Plosive) {
            continue;
        }
        const auto name = QString::fromStdString(std::string { spec.name });
        for (auto && formant : spec.formants) {
            if (formant.frequency < 1000.0) {
                continue;
            }
            const double q = formant.frequency / formant.bandwidth;
            QVERIFY2(q <= 2.5, qPrintable(name + " Q " + QString::number(q, 'f', 1) + " at " + QString::number(formant.frequency)));
        }
    }
}

void FormantVoiceTest::test_formantVoice_sibilant_shouldBeAShelfNotTwoPeaks()
{
    auto voice = makeVoice();
    const auto magnitudes = spectrum(render(voice, { { "S", SettleFrames + FftSize } }), SettleFrames);

    // A real /s/ is a shelf running from about 3.5 kHz up, not two spikes with a hole beneath them.
    // The octave below the peak carried 21 dB less than the peak when it was two spikes, which is
    // what made it whistle rather than hiss.
    const double low = bandEnergy(magnitudes, 2500.0, 4000.0);
    const double peak = bandEnergy(magnitudes, 4000.0, 6000.0);
    const double top = bandEnergy(magnitudes, 6000.0, 8000.0);

    QVERIFY2(low > peak * 0.02, qPrintable(QString::number(10.0 * std::log10(low / peak), 'f', 1) + " dB under the peak"));
    QVERIFY(top > peak * 0.1);
    // And it stays a sibilant: nearly nothing where the vowels live.
    QVERIFY(bandEnergy(magnitudes, 200.0, 1200.0) < peak * 0.01);
}

void FormantVoiceTest::test_formantVoice_vowel_shouldHaveEnergyAboveTheThirdFormant()
{
    auto voice = makeVoice();
    const auto magnitudes = spectrum(render(voice, { { "AA", SettleFrames + FftSize } }), SettleFrames);

    // A real voice has a fourth and fifth formant, and without them everything above 3 kHz is
    // silent. That silence is what made the sibilants read as piercing: /s/ was not loud, it was the
    // only thing in the phrase with any high end at all.
    const double body = bandEnergy(magnitudes, 200.0, 3000.0);
    const double high = bandEnergy(magnitudes, 3000.0, 6000.0);
    QVERIFY2(high > body * 1.0e-3, qPrintable(QString::number(10.0 * std::log10(high / body), 'f', 1) + " dB under the body"));
}

void FormantVoiceTest::test_formantVoice_sibilant_shouldNotTowerOverTheVowels_data()
{
    QTest::addColumn<QString>("sibilant");
    QTest::addColumn<QString>("vowel");

    QTest::newRow("S over AA") << "S" << "AA";
    QTest::newRow("S over EH") << "S" << "EH";
    QTest::newRow("SH over AA") << "SH" << "AA";
}

void FormantVoiceTest::test_formantVoice_sibilant_shouldNotTowerOverTheVowels()
{
    QFETCH(QString, sibilant);
    QFETCH(QString, vowel);

    const auto bandOf = [](const std::string & name) {
        auto voice = makeVoice();
        const auto magnitudes = spectrum(render(voice, { { name, SettleFrames + FftSize } }), SettleFrames);
        return bandEnergy(magnitudes, 3000.0, 6000.0);
    };

    // In the band the ear is most sensitive in. A sibilant belongs above the vowels here -- that is
    // what makes it a sibilant -- but not by the 13 to 18 dB it managed when the voice had nothing
    // above its third formant.
    const double excess = 10.0 * std::log10(bandOf(sibilant.toStdString()) / bandOf(vowel.toStdString()));
    QVERIFY2(excess < 8.0, qPrintable(QString::number(excess, 'f', 1) + " dB over the vowel"));
}

void FormantVoiceTest::test_formantVoice_vowelBoundary_shouldNotStep()
{
    auto voice = makeVoice();
    const size_t frames = static_cast<size_t>(SampleRate * 0.15);
    const auto rendered = render(voice, { { "AA", frames }, { "IY", frames } });

    // The glide has to carry the formants across the boundary. A bank that jumped to the new
    // targets would step the output here, which is a click.
    QVERIFY(peakSecondDifferenceOverRms(rendered, frames - 2000, frames + 2000) < 1.0);
}

void FormantVoiceTest::test_formantVoice_fricativeBoundary_shouldNotStep_data()
{
    QTest::addColumn<QString>("first");
    QTest::addColumn<QString>("second");

    // A fricative's formants are the centres of its noise bands rather than a shape the tract was
    // ever in, so the bank cannot be moved between one and a vowel by either gliding or snapping
    // without a seam. These are the pairs that made "I can say things" pop.
    QTest::newRow("S to EY") << "S" << "EY";
    QTest::newRow("TH to IH") << "TH" << "IH";
    QTest::newRow("AE to Z") << "AE" << "Z";
    QTest::newRow("IH to SH") << "IH" << "SH";
    QTest::newRow("SH to UW") << "SH" << "UW";
}

void FormantVoiceTest::test_formantVoice_fricativeBoundary_shouldNotStep()
{
    QFETCH(QString, first);
    QFETCH(QString, second);

    auto voice = makeVoice();
    const auto frames = static_cast<size_t>(SampleRate * 0.15);
    const auto rendered = render(voice, { { first.toStdString(), frames }, { second.toStdString(), frames } });

    // The largest step the waveform takes near the boundary, against the largest it takes while
    // holding either phoneme steady. A sawtooth-excited bank steps once per period by its nature, so
    // the question is not whether the boundary steps but whether it steps harder than the signal
    // does anyway.
    //
    // Deliberately not measured as a second difference over the window RMS: a fricative can sit 20 dB
    // below the vowel beside it, and that alone makes the ratio blow up on a boundary that is in fact
    // perfectly continuous.
    const double atBoundary = peakStep(rendered, frames - 200, frames + 200);
    const double steady = std::max(peakStep(rendered, frames / 4, frames / 2),
                                   peakStep(rendered, frames + frames / 2, 2 * frames - 100));
    QVERIFY2(atBoundary < steady * 1.5, qPrintable(QString::number(atBoundary) + " vs " + QString::number(steady)));
}

void FormantVoiceTest::test_formantVoice_fricativeBoundary_shouldNotStepAtAnyGlideTime()
{
    // The seam is crossfaded rather than swept, so unlike a glide its cost does not grow with the
    // glide setting. Before that, the longest glide stepped nearly three times as hard as the
    // shortest, which is how the artifact was found.
    const auto boundaryStep = [](double glide) {
        auto voice = makeVoice();
        voice.setGlideTime(glide);
        const auto frames = static_cast<size_t>(SampleRate * 0.15);
        const auto rendered = render(voice, { { "S", frames }, { "EY", frames } });
        return peakStep(rendered, frames - 200, frames + 200);
    };

    const double shortest = boundaryStep(0.005);
    const double longest = boundaryStep(0.075);
    QVERIFY2(longest < shortest * 1.5, qPrintable(QString::number(shortest) + " -> " + QString::number(longest)));
}

void FormantVoiceTest::test_formantVoice_plosiveRelease_shouldNotStep()
{
    auto voice = makeVoice();
    const auto frames = static_cast<size_t>(SampleRate * 0.2);
    const auto rendered = render(voice, { { "K", frames }, { "AA", frames } });

    // A stop release is an abrupt event and has to stay one, but abrupt is not the same as
    // instantaneous: a burst that goes from silence to full in a single sample is a click.
    const auto release = static_cast<size_t>(frames * 0.45);
    double peakStep = 0.0;
    for (size_t i = release + 1; i < release + 2000; i++) {
        peakStep = std::max(peakStep, std::abs(rendered[i] - rendered[i - 1]));
    }
    QVERIFY2(peakStep < peakAmplitude(rendered) * 0.35, qPrintable(QString::number(peakStep)));
}

void FormantVoiceTest::test_formantVoice_consonantLevels_shouldMatchSpeech_data()
{
    QTest::addColumn<QString>("name");
    QTest::addColumn<double>("minDb");
    QTest::addColumn<double>("maxDb");

    // Levels relative to an open vowel, which is how speech levels are quoted, with the ranges taken
    // from what speech measurement puts them at. This is not a preference: a consonant 25 dB under
    // the vowel beside it is not quiet, it is missing, and the phrase loses the word. "Feel the beat"
    // came out as "eel uh eet" with /f/ at -39 dB and /d/ at -28.
    //
    // Wide bands, because the point is to catch a systematic slide rather than to freeze a voicing.
    const auto expect = [](const char * name, double minDb, double maxDb) {
        QTest::newRow(name) << QString { name } << minDb << maxDb;
    };

    expect("M", -12.0, -5.0);
    expect("N", -12.0, -5.0);
    expect("L", -7.0, -1.0);
    expect("R", -7.0, -1.0);
    expect("W", -7.0, -1.0);

    // The sibilants are the loudest consonants in the language and carry a lot of the message. The
    // range speech allows them is wide -- ten to twenty dB under the vowel beside them, depending on
    // speaker and position -- and within it the exact level is a voicing decision made by ear.
    expect("S", -19.0, -9.0);
    expect("SH", -18.0, -8.0);
    expect("Z", -18.0, -9.0);
    expect("ZH", -18.0, -9.0);

    // The voiced fricatives sit above their unvoiced partners, because they have a voice bar and the
    // partners do not.
    expect("V", -19.0, -10.0);
    expect("DH", -20.0, -11.0);

    // /f/ and /th/ really are this weak; they are the quietest sounds in English.
    expect("F", -25.0, -15.0);
    expect("TH", -27.0, -17.0);
    expect("HH", -26.0, -16.0);

    // A stop spends most of its length silent, so its level over the whole phoneme reads low.
    expect("P", -27.0, -16.0);
    expect("T", -25.0, -14.0);
    expect("K", -26.0, -15.0);
}

void FormantVoiceTest::test_formantVoice_consonantLevels_shouldMatchSpeech()
{
    QFETCH(QString, name);
    QFETCH(double, minDb);
    QFETCH(double, maxDb);

    const double db = 20.0 * std::log10(std::max(phonemeLevel(name.toStdString()), 1.0e-9) / phonemeLevel("AA"));
    QVERIFY2(db > minDb && db < maxDb, qPrintable(QString::number(db, 'f', 1) + " dB"));
}

void FormantVoiceTest::test_formantVoice_voicedFricatives_shouldHaveAVoiceBar_data()
{
    QTest::addColumn<QString>("voiced");
    QTest::addColumn<QString>("unvoiced");

    QTest::newRow("DH over TH") << "DH" << "TH";
    QTest::newRow("V over F") << "V" << "F";
    QTest::newRow("Z over S") << "Z" << "S";
}

void FormantVoiceTest::test_formantVoice_voicedFricatives_shouldHaveAVoiceBar()
{
    QFETCH(QString, voiced);
    QFETCH(QString, unvoiced);

    const auto lowBandEnergy = [](const std::string & phoneme) {
        auto voice = makeVoice();
        const auto rendered = render(voice, { { "AA", SettleFrames }, { phoneme, SettleFrames + FftSize } });
        const auto magnitudes = spectrum(rendered, SettleFrames * 2);
        return bandEnergy(magnitudes, 150.0, 450.0);
    };

    // The glottal buzz heard through the constriction, and most of what makes a voiced fricative
    // audible at all. Its unvoiced partner has none, so the difference is the bar itself.
    QVERIFY(lowBandEnergy(voiced.toStdString()) > lowBandEnergy(unvoiced.toStdString()) * 10.0);
}

void FormantVoiceTest::test_formantVoice_vowelIntensity_shouldFollowOpenness_data()
{
    QTest::addColumn<QString>("name");
    QTest::addColumn<double>("minDb");
    QTest::addColumn<double>("maxDb");

    // A vowel's intrinsic intensity follows how open it is, and /a/ is the loudest sound in the
    // language. Given equal amplitudes the bank produced the opposite ordering -- a close vowel's
    // first formant sits low where the glottal source is strongest, so /i/ and /u/ came out 5 dB
    // *above* /a/ and jumped out of every phrase they appeared in.
    const auto expect = [](const char * name, double minDb, double maxDb) {
        QTest::newRow(name) << QString { name } << minDb << maxDb;
    };

    expect("AA", -1.0, 1.0);
    expect("AO", -3.0, 0.5);
    expect("AE", -3.0, 0.5);
    expect("EH", -5.0, -0.5);
    expect("AH", -4.0, 0.0);
    expect("IH", -6.0, -1.0);
    expect("UH", -6.0, -1.0);
    expect("IY", -9.0, -2.5);
    expect("UW", -9.0, -2.5);
    // A schwa is a reduced vowel and is weaker than any of them.
    expect("AX", -12.0, -3.0);
}

void FormantVoiceTest::test_formantVoice_vowelIntensity_shouldFollowOpenness()
{
    QFETCH(QString, name);
    QFETCH(double, minDb);
    QFETCH(double, maxDb);

    const auto levelOf = [](const std::string & vowel) {
        auto voice = makeVoice();
        const auto rendered = render(voice, { { vowel, SettleFrames + FftSize } });
        return rms(rendered, SettleFrames, rendered.size());
    };

    const double db = 20.0 * std::log10(levelOf(name.toStdString()) / levelOf("AA"));
    QVERIFY2(db > minDb && db < maxDb, qPrintable(QString::number(db, 'f', 1) + " dB"));
}

void FormantVoiceTest::test_formantVoice_heldVowel_shouldReachUsableLevel()
{
    auto voice = makeVoice();
    const auto rendered = render(voice, { { "AA", SettleFrames + FftSize } });

    // A coarse sanity check only. The device's real calibration is against the loudness of a whole
    // phrase, which needs a sequencer and so is asserted in the device's own tests: a vowel is the
    // loudest thing the voice makes, and a phrase is mostly the quieter things between vowels.
    const double peak = peakAmplitude(rendered);
    QVERIFY2(peak > 0.05, qPrintable(QString::number(peak)));
    QVERIFY2(peak < 0.4, qPrintable(QString::number(peak)));
}

void FormantVoiceTest::test_formantVoice_afterReset_shouldBeSilent()
{
    auto voice = makeVoice();
    render(voice, { { "AA", 4000 } });
    voice.reset();

    std::vector<double> output;
    for (size_t frame = 0; frame < 1000; frame++) {
        output.push_back(voice.nextSample());
    }
    QCOMPARE(peakAmplitude(output), 0.0);
}

namespace {

void writeWav(const QString & path, const std::vector<double> & signal)
{
    QFile file { path };
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Could not open" << path;
        return;
    }
    const uint32_t rate = static_cast<uint32_t>(SampleRate);
    const uint32_t dataBytes = static_cast<uint32_t>(signal.size() * 2);
    QDataStream out { &file };
    out.setByteOrder(QDataStream::LittleEndian);
    file.write("RIFF");
    out << static_cast<uint32_t>(36 + dataBytes);
    file.write("WAVEfmt ");
    out << static_cast<uint32_t>(16) << static_cast<uint16_t>(1) << static_cast<uint16_t>(1);
    out << rate << rate * 2 << static_cast<uint16_t>(2) << static_cast<uint16_t>(16);
    file.write("data");
    out << dataBytes;
    for (auto && sample : signal) {
        out << static_cast<int16_t>(std::lround(std::clamp(sample, -1.0, 1.0) * 32000.0));
    }
}

} // namespace

//! Throwaway measurement slot: prints the levels and band energies the table was tuned against, and
//! with NOTEAHEAD_PROBE_DIR set, renders a phrase to a WAV there so the result can be listened to.
//! Silent and nearly free otherwise. Goes when the voice stops being tuned.
void FormantVoiceTest::probe()
{
    const QString directory = qEnvironmentVariable("NOTEAHEAD_PROBE_DIR");
    if (directory.isEmpty()) {
        QSKIP("Set NOTEAHEAD_PROBE_DIR to render a phrase to listen to.");
    }

    auto voice = makeVoice();
    voice.setGlideTime(0.03);
    std::vector<double> phrase;
    // Written as text, so this exercises the letter-to-sound rules and the voice together, which is
    // the only way to hear whether the two agree about what a word is made of.
    for (auto && text : { "Hello world.", "Feel the beat!", "I will destroy the whole world.",
                          "'Make A'merica 'Great 'Again!", "Take two, tick tock, top ten.", "Noteahead, a tracker for Linux.",
                          "I can say things.", "This is the speech device speaking.",
                          "Both are the same underlying issue, my fricative and burst bands are far too resonant.",
                          "/iy ih eh ae aa ao uh uw ah er/", "/p aa t aa k aa/", "/b aa d aa g aa/",
                          "/s aa sh aa f aa th aa/", "/m aa n aa ng aa l aa r aa/", "/ay ey oy aw ow/" }) {
        Utterance utterance;
        for (auto && event : textToPhonemes(text)) {
            const std::string name { event.spec->name };
            utterance.emplace_back(name, static_cast<size_t>(SampleRate * event.spec->nominalMs * event.lengthScale / 1000.0));
        }
        utterance.emplace_back("_", static_cast<size_t>(SampleRate * 0.35));
        const auto rendered = render(voice, utterance);
        phrase.insert(phrase.end(), rendered.begin(), rendered.end());
    }

    // Normalised for listening only. The device's own level is asserted separately, and at the
    // rack's -26 dBFS convention a probe file is too quiet to judge by ear.
    const double peak = peakAmplitude(phrase);
    for (auto & sample : phrase) {
        sample *= 0.9 / peak;
    }
    const auto path = directory + "/speech_probe.wav";
    writeWav(path, phrase);
    qDebug() << "Wrote" << path << phrase.size() / SampleRate << "s, device peak" << peak;
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::FormantVoiceTest)
