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

#include "formant_voice_v2_test.hpp"

#include "../../domain/dsp/fft.hpp"
#include "../../domain/dsp/speech/formant_voice_v2.hpp"
#include "../../domain/dsp/speech/phoneme.hpp"
#include "../../domain/dsp/speech/speech_prosody.hpp"
#include "../../domain/dsp/speech/speech_sequencer_v2.hpp"
#include "../../domain/dsp/speech/text_to_phonemes_v2.hpp"

#include <QTest>

#include <algorithm>
#include <cmath>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace noteahead {

namespace {

constexpr double SampleRate { 48000.0 };
constexpr double Fundamental { 110.0 };
constexpr int FftSize { 16384 };
constexpr size_t SettleFrames { 6000 };

using Utterance = std::vector<std::pair<std::string, size_t>>;

FormantVoiceV2 makeVoice(double frequency = Fundamental)
{
    FormantVoiceV2 voice;
    voice.setSampleRate(SampleRate);
    voice.setFrequency(frequency);
    return voice;
}

std::vector<double> render(FormantVoiceV2 & voice, const Utterance & utterance)
{
    std::vector<double> output;
    for (size_t index = 0; index < utterance.size(); index++) {
        const auto & [name, frames] = utterance[index];
        const auto * spec = speechPhoneme(name);
        Q_ASSERT(spec);
        const auto * next = index + 1 < utterance.size() ? speechPhoneme(utterance[index + 1].first) : nullptr;
        voice.setPhoneme(*spec, next, static_cast<double>(frames) / SampleRate);
        for (size_t frame = 0; frame < frames; frame++) {
            voice.setPhonemeProgress(static_cast<double>(frame) / static_cast<double>(frames));
            output.push_back(voice.nextSample());
        }
    }
    return output;
}

std::vector<double> spectrum(const std::vector<double> & signal, size_t offset, int fftSize = FftSize)
{
    std::vector<double> re(fftSize, 0.0);
    std::vector<double> im(fftSize, 0.0);
    for (int i = 0; i < fftSize; i++) {
        const size_t index = offset + static_cast<size_t>(i);
        if (index >= signal.size()) {
            break;
        }
        const double window = 0.5 - 0.5 * std::cos(2.0 * M_PI * i / (fftSize - 1));
        re[static_cast<size_t>(i)] = signal[index] * window;
    }
    Fft::forward(re.data(), im.data(), fftSize);

    std::vector<double> magnitudes(fftSize / 2, 0.0);
    for (size_t i = 0; i < magnitudes.size(); i++) {
        magnitudes[i] = std::hypot(re[i], im[i]);
    }
    return magnitudes;
}

double bandEnergy(const std::vector<double> & magnitudes, double lowHz, double highHz, int fftSize = FftSize)
{
    const auto low = static_cast<size_t>(std::max<long>(1, std::lround(lowHz * fftSize / SampleRate)));
    const auto high = std::min(magnitudes.size() - 1, static_cast<size_t>(std::lround(highHz * fftSize / SampleRate)));
    double sum = 0.0;
    for (size_t i = low; i <= high; i++) {
        sum += magnitudes[i] * magnitudes[i];
    }
    return sum;
}

double peakFrequency(const std::vector<double> & magnitudes, double lowHz, double highHz, int fftSize = FftSize)
{
    const auto low = static_cast<size_t>(std::max<long>(1, std::lround(lowHz * fftSize / SampleRate)));
    const auto high = std::min(magnitudes.size() - 1, static_cast<size_t>(std::lround(highHz * fftSize / SampleRate)));
    size_t peak = low;
    for (size_t i = low; i <= high; i++) {
        if (magnitudes[i] > magnitudes[peak]) {
            peak = i;
        }
    }
    return static_cast<double>(peak) * SampleRate / fftSize;
}

double rms(const std::vector<double> & signal, size_t from, size_t to)
{
    const auto last = std::min(to, signal.size());
    if (from >= last) {
        return 0.0;
    }
    double sum = 0.0;
    for (size_t i = from; i < last; i++) {
        sum += signal[i] * signal[i];
    }
    return std::sqrt(sum / static_cast<double>(last - from));
}

double peakAmplitude(const std::vector<double> & signal)
{
    double peak = 0.0;
    for (const double sample : signal) {
        peak = std::max(peak, std::abs(sample));
    }
    return peak;
}

double peakStep(const std::vector<double> & signal, size_t from, size_t to)
{
    double peak = 0.0;
    for (size_t i = std::max<size_t>(from, 1); i < std::min(to, signal.size()); i++) {
        peak = std::max(peak, std::abs(signal[i] - signal[i - 1]));
    }
    return peak;
}

double phonemeLevel(const std::string & phoneme)
{
    const auto * spec = speechPhoneme(phoneme);
    const auto frames = static_cast<size_t>(SampleRate * spec->nominalMs / 1000.0);
    auto voice = makeVoice();
    const auto rendered = render(voice, { { "AA", frames }, { phoneme, frames }, { "AA", frames } });
    if (spec->type == PhonemeType::Plosive) {
        const auto release = 2 * frames - static_cast<size_t>(SampleRate * (spec->voicing > 0.0 ? 0.012 : 0.040));
        return rms(rendered, release, release + static_cast<size_t>(SampleRate * 0.008));
    }
    return rms(rendered, frames + frames / 10, 2 * frames);
}

double centroidInBand(const std::vector<double> & magnitudes, double lowHz, double highHz, int fftSize)
{
    const auto low = static_cast<size_t>(std::max<long>(1, std::lround(lowHz * fftSize / SampleRate)));
    const auto high = std::min(magnitudes.size() - 1, static_cast<size_t>(std::lround(highHz * fftSize / SampleRate)));
    double weighted = 0.0;
    double total = 0.0;
    for (size_t i = low; i <= high; i++) {
        const double mag = magnitudes[i] * magnitudes[i];
        weighted += mag * (static_cast<double>(i) * SampleRate / fftSize);
        total += mag;
    }
    return total > 0.0 ? weighted / total : 0.0;
}

} // namespace

void FormantVoiceV2Test::test_heldVowel_shouldPeakAtFormantsAndReflectOpenness()
{
    const auto vowelData = [](const std::string & name) {
        auto voice = makeVoice();
        const auto rendered = render(voice, { { name, SettleFrames + FftSize } });
        return std::pair { rendered, spectrum(rendered, SettleFrames) };
    };

    const auto [aaAudio, aaSpec] = vowelData("AA");
    const auto [iyAudio, iySpec] = vowelData("IY");
    const auto [uwAudio, uwSpec] = vowelData("UW");

    const double aaF1 = peakFrequency(aaSpec, 300.0, 1000.0);
    QVERIFY2(std::abs(aaF1 - 730.0) <= Fundamental, qPrintable(QString::number(aaF1)));

    const double iyF2 = peakFrequency(iySpec, 1500.0, 3000.0);
    QVERIFY2(std::abs(iyF2 - 2290.0) <= Fundamental, qPrintable(QString::number(iyF2)));

    // Open /ɑ/ is naturally louder than close /i/ and /u/
    const double aaDb = 20.0 * std::log10(rms(aaAudio, SettleFrames, aaAudio.size()));
    const double iyDb = 20.0 * std::log10(rms(iyAudio, SettleFrames, iyAudio.size()));
    const double uwDb = 20.0 * std::log10(rms(uwAudio, SettleFrames, uwAudio.size()));
    QVERIFY2(aaDb - iyDb > 2.0 && aaDb - iyDb < 10.0, qPrintable(QString { "AA-IY=%1 dB" }.arg(aaDb - iyDb)));
    QVERIFY2(aaDb - uwDb > 2.0 && aaDb - uwDb < 10.0, qPrintable(QString { "AA-UW=%1 dB" }.arg(aaDb - uwDb)));

    // Upper formants provide real energy above 3 kHz with 48 kHz higher-pole correction
    const double body = bandEnergy(aaSpec, 200.0, 3000.0);
    const double high = bandEnergy(aaSpec, 3000.0, 6000.0);
    QVERIFY2(high > body * 5.0e-5, qPrintable(QString::number(10.0 * std::log10(high / body), 'f', 1)));

    const double iyBody = bandEnergy(iySpec, 200.0, 3000.0);
    const double iyHigh = bandEnergy(iySpec, 3000.0, 6000.0);
    QVERIFY2(iyHigh > iyBody * 1.0e-3, qPrintable(QString::number(10.0 * std::log10(iyHigh / iyBody), 'f', 1)));

    auto brightVoice = makeVoice();
    brightVoice.setReturnPhase(0.004);
    const auto brightIySpec = spectrum(render(brightVoice, { { "IY", SettleFrames + FftSize } }), SettleFrames);
    const double brightBody = bandEnergy(brightIySpec, 200.0, 3000.0);
    const double brightHigh = bandEnergy(brightIySpec, 3000.0, 6000.0);
    QVERIFY2(brightHigh > brightBody * 2.0e-3, qPrintable(QString::number(10.0 * std::log10(brightHigh / brightBody), 'f', 1)));
}

void FormantVoiceV2Test::test_nasal_shouldHaveMurmurAndAntiresonanceNotch()
{
    const auto specOf = [](const std::string & name) {
        auto voice = makeVoice();
        const auto rendered = render(voice, { { name, SettleFrames + FftSize } });
        return spectrum(rendered, SettleFrames);
    };

    const auto aa = specOf("AA");
    const auto m = specOf("M");
    const auto n = specOf("N");

    // Strong low murmur around 250 Hz in nasals
    QVERIFY(bandEnergy(m, 150.0, 400.0) > bandEnergy(m, 800.0, 1400.0) * 10.0);
    QVERIFY(bandEnergy(n, 150.0, 400.0) > bandEnergy(n, 1400.0, 2000.0) * 10.0);

    // M has an antiresonance notch near 1000 Hz; N pushes that zero up near 1700 Hz
    const double mNotchVsMurmur = bandEnergy(m, 850.0, 1150.0) / bandEnergy(m, 150.0, 400.0);
    const double aaMidVsLow = bandEnergy(aa, 850.0, 1150.0) / bandEnergy(aa, 150.0, 400.0);
    QVERIFY(mNotchVsMurmur < aaMidVsLow * 0.05);

    const double nZeroVsMurmur = bandEnergy(n, 1500.0, 1900.0) / bandEnergy(n, 150.0, 400.0);
    QVERIFY(nZeroVsMurmur < 0.02);
}

void FormantVoiceV2Test::test_coarticulation_cvTransitions_shouldDifferByPlace()
{
    // Compare early vowel F2 centroid following bilabial B vs alveolar D
    const auto onsetF2 = [](const std::string & stop) {
        auto voice = makeVoice();
        const auto preFrames = static_cast<size_t>(SampleRate * 0.08);
        const auto stopFrames = static_cast<size_t>(SampleRate * 0.065);
        const auto vowelFrames = static_cast<size_t>(SampleRate * 0.17);
        const auto rendered = render(voice, { { "AA", preFrames }, { stop, stopFrames }, { "AA", vowelFrames } });
        const size_t onset = preFrames + stopFrames;
        const auto mags = spectrum(rendered, onset, 2048);
        return centroidInBand(mags, 700.0, 2200.0, 2048);
    };

    const double afterB = onsetF2("B");
    const double afterD = onsetF2("D");
    QVERIFY2(afterD > afterB + 150.0, qPrintable(QString { "B=%1 Hz vs D=%2 Hz" }.arg(afterB).arg(afterD)));
}

void FormantVoiceV2Test::test_coarticulation_vcClosing_shouldBendTowardStopLocus()
{
    const auto codaF2 = [](const std::string & stop) {
        auto voice = makeVoice();
        const auto vowelFrames = static_cast<size_t>(SampleRate * 0.18);
        const auto stopFrames = static_cast<size_t>(SampleRate * 0.065);
        const auto rendered = render(voice, { { "AA", vowelFrames }, { stop, stopFrames } });
        const size_t windowStart = vowelFrames - 2048;
        const auto mags = spectrum(rendered, windowStart, 2048);
        return centroidInBand(mags, 700.0, 2200.0, 2048);
    };

    const double beforeP = codaF2("P");
    const double beforeT = codaF2("T");
    QVERIFY2(beforeT > beforeP + 80.0, qPrintable(QString { "before P=%1 Hz vs before T=%2 Hz" }.arg(beforeP).arg(beforeT)));
}

void FormantVoiceV2Test::test_coarticulation_velarPinch_shouldTrackAdjacentVowel()
{
    const auto onsetF2 = [](const std::string & vowel) {
        auto voice = makeVoice();
        const auto stopFrames = static_cast<size_t>(SampleRate * 0.12);
        const auto vowelFrames = static_cast<size_t>(SampleRate * 0.15);
        const auto rendered = render(voice, { { "K", stopFrames }, { vowel, vowelFrames } });
        // Measure during aspiration + earliest onset
        const size_t release = stopFrames - static_cast<size_t>(SampleRate * 0.032);
        const auto mags = spectrum(rendered, release, 2048);
        return centroidInBand(mags, 900.0, 2500.0, 2048);
    };

    const double beforeAa = onsetF2("AA");
    const double beforeIy = onsetF2("IY");
    QVERIFY2(beforeIy > beforeAa + 400.0, qPrintable(QString { "K+AA=%1 Hz vs K+IY=%2 Hz" }.arg(beforeAa).arg(beforeIy)));
}

void FormantVoiceV2Test::test_glottalH_shouldTakeFollowingVowelShape()
{
    const auto ratioOf = [](const std::string & vowel) {
        auto voice = makeVoice();
        const auto frames = static_cast<size_t>(SampleRate * 0.09);
        const auto rendered = render(voice, { { "HH", frames }, { vowel, frames } });
        const auto mags = spectrum(rendered, 1000, 4096);
        return bandEnergy(mags, 1900.0, 2700.0, 4096) / std::max(1.0e-12, bandEnergy(mags, 600.0, 1250.0, 4096));
    };

    const double beforeIy = ratioOf("IY");
    const double beforeAa = ratioOf("AA");
    QVERIFY2(beforeIy > beforeAa * 4.0, qPrintable(QString { "HH+IY=%1 vs HH+AA=%2" }.arg(beforeIy).arg(beforeAa)));
}

void FormantVoiceV2Test::test_voicedFricatives_shouldHaveCleanVoiceBarAndFrication()
{
    const auto lowBand = [](const std::string & phoneme) {
        auto voice = makeVoice();
        const auto rendered = render(voice, { { "AA", SettleFrames }, { phoneme, SettleFrames + FftSize } });
        return bandEnergy(spectrum(rendered, SettleFrames * 2), 150.0, 450.0);
    };

    QVERIFY(lowBand("Z") > lowBand("S") * 10.0);
    QVERIFY(lowBand("V") > lowBand("F") * 10.0);
    QVERIFY(lowBand("DH") > lowBand("TH") * 10.0);

    auto sVoice = makeVoice();
    const auto sSpec = spectrum(render(sVoice, { { "S", SettleFrames + FftSize } }), SettleFrames);
    const double sPeak = bandEnergy(sSpec, 4000.0, 6000.0);
    QVERIFY(bandEnergy(sSpec, 200.0, 1200.0) < sPeak * 0.01);
}

void FormantVoiceV2Test::test_consonantLevels_shouldMatchSpeech_data()
{
    QTest::addColumn<QString>("name");
    QTest::addColumn<double>("minDb");
    QTest::addColumn<double>("maxDb");

    const auto expect = [](const char * name, double minDb, double maxDb) {
        QTest::newRow(name) << QString { name } << minDb << maxDb;
    };

    expect("M", -13.0, -4.0);
    expect("N", -13.0, -4.0);
    expect("L", -8.0, -0.5);
    expect("R", -8.0, -0.5);
    expect("W", -8.0, -0.5);

    expect("S", -20.0, -8.0);
    expect("SH", -19.0, -7.0);
    expect("Z", -19.0, -7.0);
    expect("ZH", -19.0, -7.0);

    expect("V", -20.0, -9.0);
    expect("DH", -21.0, -10.0);

    expect("F", -26.0, -14.0);
    expect("TH", -28.0, -16.0);
    expect("HH", -28.0, -14.0);

    expect("P", -28.0, -14.0);
    expect("T", -26.0, -12.0);
    expect("K", -27.0, -13.0);
}

void FormantVoiceV2Test::test_consonantLevels_shouldMatchSpeech()
{
    QFETCH(QString, name);
    QFETCH(double, minDb);
    QFETCH(double, maxDb);

    const double db = 20.0 * std::log10(std::max(phonemeLevel(name.toStdString()), 1.0e-9) / phonemeLevel("AA"));
    QVERIFY2(db > minDb && db < maxDb, qPrintable(QString::number(db, 'f', 1) + " dB"));
}

void FormantVoiceV2Test::test_plosives_closureAspirationAndPhraseFinal()
{
    const auto frames = static_cast<size_t>(SampleRate * 0.2);

    // Unvoiced closure is silent, voiced closure buzzes
    {
        auto pVoice = makeVoice();
        const auto pAudio = render(pVoice, { { "P", frames }, { "AA", frames } });
        auto bVoice = makeVoice();
        const auto bAudio = render(bVoice, { { "B", frames }, { "AA", frames } });
        const auto release = frames - static_cast<size_t>(SampleRate * 0.040);
        const double pClosure = rms(pAudio, frames / 4, release - static_cast<size_t>(SampleRate * 0.01));
        const double bClosure = rms(bAudio, static_cast<size_t>(frames * 0.25), static_cast<size_t>(frames * 0.42));
        const double pBurst = rms(pAudio, release, release + static_cast<size_t>(SampleRate * 0.003));
        QVERIFY(pBurst > pClosure * 50.0);
        QVERIFY(bClosure > pClosure * 10.0);
    }

    // Stop after fricative is unaspirated
    {
        const auto sFrames = static_cast<size_t>(SampleRate * 0.115);
        auto afterFric = makeVoice();
        const auto afterFricAudio = render(afterFric, { { "S", sFrames }, { "T", sFrames }, { "AA", sFrames } });
        auto afterStop = makeVoice();
        const auto afterVowelAudio = render(afterStop, { { "T", sFrames }, { "AA", sFrames } });
        const double breathAfterFric = rms(afterFricAudio, 2 * sFrames - static_cast<size_t>(SampleRate * 0.035), 2 * sFrames - static_cast<size_t>(SampleRate * 0.015));
        const double breathAlone = rms(afterVowelAudio, sFrames - static_cast<size_t>(SampleRate * 0.035), sFrames - static_cast<size_t>(SampleRate * 0.015));
        QVERIFY(breathAfterFric < breathAlone * 0.25);
    }

    // Phrase-final stop releases softly rather than vanishing
    {
        auto endVoice = makeVoice();
        const auto atEnd = render(endVoice, { { "AA", frames }, { "T", frames } });
        auto relVoice = makeVoice();
        const auto released = render(relVoice, { { "AA", frames }, { "T", frames }, { "AA", frames } });
        const auto tailPeak = [&](const std::vector<double> & s) {
            return peakAmplitude(std::vector<double>(s.begin() + static_cast<long>(frames + frames / 2), s.begin() + static_cast<long>(2 * frames)));
        };
        const double endPeak = tailPeak(atEnd);
        const double relPeak = tailPeak(released);
        QVERIFY2(endPeak > 0.005 && endPeak < relPeak * 0.78,
                 qPrintable(QString { "endPeak=%1 vs relPeak=%2" }.arg(endPeak).arg(relPeak)));
    }
}

void FormantVoiceV2Test::test_boundaries_shouldNotClick_data()
{
    QTest::addColumn<QString>("first");
    QTest::addColumn<QString>("second");

    QTest::newRow("AA to IY") << "AA" << "IY";
    QTest::newRow("S to EY") << "S" << "EY";
    QTest::newRow("EY to S") << "EY" << "S";
    QTest::newRow("S to SH") << "S" << "SH";
    QTest::newRow("AE to Z") << "AE" << "Z";
}

void FormantVoiceV2Test::test_boundaries_shouldNotClick()
{
    QFETCH(QString, first);
    QFETCH(QString, second);

    auto voice = makeVoice();
    const auto frames = static_cast<size_t>(SampleRate * 0.15);
    const auto rendered = render(voice, { { first.toStdString(), frames }, { second.toStdString(), frames } });

    const double atBoundary = peakStep(rendered, frames - 200, frames + 200);
    const double steady = std::max(peakStep(rendered, frames / 4, frames / 2),
                                   peakStep(rendered, frames + frames / 2, 2 * frames - 100));
    QVERIFY2(atBoundary < steady * 1.5, qPrintable(QString { "%1 vs steady %2" }.arg(atBoundary).arg(steady)));
}

void FormantVoiceV2Test::test_plosiveBoundaries_shouldNotClick_data()
{
    QTest::addColumn<QString>("first");
    QTest::addColumn<QString>("second");

    QTest::newRow("T to B") << "T" << "B";
    QTest::newRow("T to S") << "T" << "S";
    QTest::newRow("S to T") << "S" << "T";
    QTest::newRow("T to T") << "T" << "T";
    QTest::newRow("T to _") << "T" << "_";
}

void FormantVoiceV2Test::test_plosiveBoundaries_shouldNotClick()
{
    QFETCH(QString, first);
    QFETCH(QString, second);

    auto voice = makeVoice();
    const auto frames = static_cast<size_t>(SampleRate * 0.15);
    const auto rendered = render(voice, { { first.toStdString(), frames }, { second.toStdString(), frames } });

    const double atBoundary { peakStep(rendered, frames - 4, frames + 4) };
    const double preBoundary { peakStep(rendered, frames - 400, frames - 20) };
    QVERIFY2(atBoundary < std::max(0.002, preBoundary * 1.5),
             qPrintable(QString { "atBoundary=%1 vs preBoundary=%2" }.arg(atBoundary).arg(preBoundary)));
}

void FormantVoiceV2Test::test_geminateStop_shouldNotReleaseTwice()
{
    auto voice = makeVoice();
    const auto frames = static_cast<size_t>(SampleRate * 0.15);
    const auto rendered = render(voice, { { "AA", frames }, { "T", frames }, { "T", frames }, { "AA", frames } });

    const double firstStopTailPeak { peakAmplitude(
      std::vector<double>(rendered.begin() + static_cast<long>(frames + frames / 2),
                          rendered.begin() + static_cast<long>(2 * frames - 16))) };
    QVERIFY2(firstStopTailPeak < 0.002, qPrintable(QString { "firstStopTailPeak=%1" }.arg(firstStopTailPeak)));

    const double secondStopReleasePeak { peakAmplitude(
      std::vector<double>(rendered.begin() + static_cast<long>(2 * frames + frames / 2),
                          rendered.begin() + static_cast<long>(3 * frames))) };
    QVERIFY2(secondStopReleasePeak > 0.005, qPrintable(QString { "secondStopReleasePeak=%1" }.arg(secondStopReleasePeak)));
}

void FormantVoiceV2Test::test_alveolarT_shouldCoarticulateAndStayWarm()
{
    const auto tSpec { speechPhoneme("T") };
    const auto renderStop = [&](const std::string & vowelName, bool stressed) {
        const auto * vowelSpec { speechPhoneme(vowelName) };
        Q_ASSERT(tSpec && vowelSpec);
        const auto stopFrames { static_cast<size_t>(SampleRate * 0.10) };
        const auto vowelFrames { static_cast<size_t>(SampleRate * 0.14) };
        auto voice { makeVoice() };
        voice.setPhonemeStressed(stressed);
        voice.setPhoneme(*tSpec, vowelSpec, static_cast<double>(stopFrames) / SampleRate);
        std::vector<double> audio;
        audio.reserve(stopFrames + vowelFrames);
        for (size_t i { 0 }; i < stopFrames; i++) {
            voice.setPhonemeProgress(static_cast<double>(i) / static_cast<double>(stopFrames));
            audio.push_back(voice.nextSample());
        }
        voice.setPhoneme(*vowelSpec, nullptr, static_cast<double>(vowelFrames) / SampleRate);
        for (size_t i { 0 }; i < vowelFrames; i++) {
            voice.setPhonemeProgress(static_cast<double>(i) / static_cast<double>(vowelFrames));
            audio.push_back(voice.nextSample());
        }
        return audio;
    };

    const auto stopFrames { static_cast<size_t>(SampleRate * 0.10) };
    const auto releaseFrame { stopFrames - static_cast<size_t>(SampleRate * 0.040) };

    const auto tUwAudio { renderStop("UW", true) };
    const auto tIyAudio { renderStop("IY", true) };
    const auto tUwUnstressed { renderStop("UW", false) };

    constexpr int burstFft { 1024 };
    const auto uwSpec { spectrum(tUwAudio, releaseFrame, burstFft) };
    const auto iySpec { spectrum(tIyAudio, releaseFrame, burstFft) };
    const double uwCentroid { centroidInBand(uwSpec, 1200.0, 8000.0, burstFft) };
    const double iyCentroid { centroidInBand(iySpec, 1200.0, 8000.0, burstFft) };
    QVERIFY2(iyCentroid > uwCentroid + 250.0,
             qPrintable(QString { "T+IY=%1 Hz vs T+UW=%2 Hz" }.arg(iyCentroid).arg(uwCentroid)));

    const double iyMidBody { bandEnergy(iySpec, 1800.0, 4800.0, burstFft) };
    const double iyUpperCrack { bandEnergy(iySpec, 5000.0, 10000.0, burstFft) };
    QVERIFY2(iyUpperCrack < iyMidBody,
             qPrintable(QString { "iyUpperCrack=%1 vs iyMidBody=%2" }.arg(iyUpperCrack).arg(iyMidBody)));

    const double iyBurstRms { rms(tIyAudio, releaseFrame, releaseFrame + 480) };
    const double iyAspRms { rms(tIyAudio, releaseFrame + 480, stopFrames) };
    QVERIFY2(iyAspRms >= iyBurstRms * 0.45,
             qPrintable(QString { "iyAspRms=%1 vs iyBurstRms=%2" }.arg(iyAspRms).arg(iyBurstRms)));

    const double uwStressedBurst { rms(tUwAudio, releaseFrame, releaseFrame + 480) };
    const double uwUnstressedBurst { rms(tUwUnstressed, releaseFrame, releaseFrame + 480) };
    QVERIFY2(uwUnstressedBurst < uwStressedBurst * 0.80,
             qPrintable(QString { "unstressed=%1 vs stressed=%2" }.arg(uwUnstressedBurst).arg(uwStressedBurst)));
}

void FormantVoiceV2Test::test_monsterAndWhisper_shouldStayFreeOfClicks()
{
    const std::string phrase { "I want to tear your brain out and EAT IT!" };
    auto events { textToPhonemesV2(phrase) };
    const auto prosody { SpeechProsody::analyze(phrase, events) };

    const auto renderPreset = [&](double formantShift,
                                  double openQuotient,
                                  double breathiness,
                                  double perturbation,
                                  double speedQuotient,
                                  double returnPhase,
                                  double subharmonic,
                                  double intonationScale,
                                  double consonantLevel,
                                  const FormantVoiceV2::VoiceCharacter & character) {
        SpeechSequencerV2 seq;
        seq.setSampleRate(SampleRate);
        seq.setRate(0.35 + 2.65 * 0.25);
        seq.setPhonemes(events);
        seq.trigger();

        auto voice = std::make_unique<FormantVoiceV2>();
        voice->setSampleRate(SampleRate);
        voice->setGlideTime(0.005 + 0.075 * 0.35);
        voice->setFormantShift(formantShift);
        voice->setOpenQuotient(openQuotient);
        voice->setReturnPhase(returnPhase);
        voice->setVoicePerturbation(perturbation);
        voice->setSpeedQuotient(speedQuotient);
        voice->setSubharmonic(subharmonic);
        voice->setBreathiness(0.10 + breathiness);
        voice->setConsonantLevel(consonantLevel);
        voice->setSibilance(0.31);
        voice->setVoiceCharacter(character);
        voice->setPhraseDynamics(0.4 * intonationScale);

        struct Rendered
        {
            std::vector<double> audio;
            std::vector<size_t> indices;
        } out;

        double pitch { -1.0 };
        const double pitchCoeff { 1.0 - std::exp(-1.0 / (0.015 * SampleRate)) };
        while (seq.isActive()) {
            const auto * spec { seq.phoneme() };
            const size_t idx { seq.currentIndex() };
            const double prog { seq.progress() };
            voice->setPhoneme(*spec, seq.nextPhoneme(), seq.phonemeSeconds());
            voice->setPhonemeStressed(seq.isStressed());
            voice->setPhonemeProgress(prog);
            voice->setUtteranceProgress(seq.utteranceProgress());
            const double semi { SpeechProsody::pitchOffsetSemitones(
              prosody[idx], *spec, prog, seq.phonemeElapsedSeconds(), 0.4 * intonationScale) };
            const double asked { std::log2(130.8128 * std::pow(2.0, semi / 12.0)) };
            pitch = pitch < 0.0 ? asked : pitch + (asked - pitch) * pitchCoeff;
            voice->setFrequency(std::exp2(pitch));
            out.audio.push_back(voice->nextSample());
            out.indices.push_back(idx);
            seq.advance();
        }
        return out;
    };

    const auto countHfClicks = [&](const std::vector<double> & audio, const std::vector<size_t> * indices, bool vowelsOnly) {
        struct Hp
        {
            double b0 { 1.0 };
            double b1 { 0.0 };
            double b2 { 0.0 };
            double a1 { 0.0 };
            double a2 { 0.0 };
            double s1 { 0.0 };
            double s2 { 0.0 };

            explicit Hp(double fc)
            {
                const double w0 { 2.0 * M_PI * fc / SampleRate };
                const double cw { std::cos(w0) };
                const double al { std::sin(w0) / (2.0 * 0.7071) };
                const double inv { 1.0 / (1.0 + al) };
                b0 = 0.5 * (1.0 + cw) * inv;
                b1 = -(1.0 + cw) * inv;
                b2 = b0;
                a1 = (-2.0 * cw) * inv;
                a2 = (1.0 - al) * inv;
            }

            double process(double x)
            {
                const double y { b0 * x + s1 };
                s1 = b1 * x - a1 * y + s2;
                s2 = b2 * x - a2 * y;
                return y;
            }
        } hp1 { 4000.0 }, hp2 { 4000.0 };

        std::vector<double> hf(audio.size(), 0.0);
        for (size_t i { 0 }; i < audio.size(); i++) {
            hf[i] = hp2.process(hp1.process(audio[i]));
        }

        constexpr size_t window { 96 };
        std::vector<double> binRms(hf.size() / window, 0.0);
        for (size_t b { 0 }; b < binRms.size(); b++) {
            double sum { 0.0 };
            for (size_t k { 0 }; k < window; k++) {
                const double v { hf[b * window + k] };
                sum += v * v;
            }
            binRms[b] = std::sqrt(sum / static_cast<double>(window));
        }

        int clicks { 0 };
        size_t lastClick { 0 };
        for (size_t b { 20 }; b + 20 < binRms.size(); b++) {
            if (vowelsOnly && indices && events[(*indices)[b * window]].spec->type != PhonemeType::Vowel) {
                continue;
            }
            std::vector<double> bg;
            for (size_t k { b - 20 }; k < b - 4; k++) {
                bg.push_back(binRms[k]);
            }
            for (size_t k { b + 4 }; k < b + 20; k++) {
                bg.push_back(binRms[k]);
            }
            std::sort(bg.begin(), bg.end());
            const double bgMed { bg[bg.size() / 2] };
            if (binRms[b] > 0.0015 && binRms[b] > bgMed * 4.0 && binRms[b] >= binRms[b - 1] && binRms[b] >= binRms[b + 1]) {
                if (lastClick == 0 || b - lastClick > 10) {
                    clicks++;
                    lastClick = b;
                }
            }
        }
        return clicks;
    };

    const auto boundaryStep = [](const std::vector<double> & audio, const std::vector<size_t> & indices, size_t fromIdx, size_t toIdx) {
        for (size_t i { 4 }; i + 4 < indices.size(); i++) {
            if (indices[i - 1] == fromIdx && indices[i] == toIdx) {
                return peakStep(audio, i - 4, i + 4);
            }
        }
        return 0.0;
    };

    const FormantVoiceV2::VoiceCharacter monsterChar { .drive = 1.8, .subOctaveMix = 0.32 };
    const auto monster { renderPreset(0.76, 0.34, 0.06, 0.75, 3.6, 0.010, 0.72, 0.85, 1.0, monsterChar) };
    QVERIFY(std::ranges::all_of(monster.audio, [](double s) { return std::isfinite(s); }));
    QVERIFY(peakAmplitude(monster.audio) < 1.0);
    QVERIFY(boundaryStep(monster.audio, monster.indices, 4, 5) < 0.002);
    QVERIFY(boundaryStep(monster.audio, monster.indices, 25, 26) < 0.002);
    const int monsterVowelClicks { countHfClicks(monster.audio, &monster.indices, true) };
    QVERIFY2(monsterVowelClicks <= 2, qPrintable(QString { "monsterVowelClicks=%1" }.arg(monsterVowelClicks)));

    const FormantVoiceV2::VoiceCharacter whisperChar { .bandwidthScale = 1.25, .whisperMix = 0.94 };
    const auto whisper { renderPreset(1.04, 0.82, 0.20, 0.30, 2.0, 0.060, 0.00, 0.40, 1.0, whisperChar) };
    QVERIFY(std::ranges::all_of(whisper.audio, [](double s) { return std::isfinite(s); }));
    QVERIFY(peakAmplitude(whisper.audio) < 1.0);
    QVERIFY(boundaryStep(whisper.audio, whisper.indices, 4, 5) < 0.002);
    QVERIFY(boundaryStep(whisper.audio, whisper.indices, 25, 26) < 0.002);

    const auto whisperNoCons { renderPreset(1.04, 0.82, 0.20, 0.30, 2.0, 0.060, 0.00, 0.40, 0.0, whisperChar) };
    QCOMPARE(countHfClicks(whisperNoCons.audio, &whisperNoCons.indices, false), 0);
}

void FormantVoiceV2Test::test_highPitch_shouldPreserveFundamentalThroughF1Floor()
{
    auto voice = makeVoice(600.0);
    const auto rendered = render(voice, { { "IY", SettleFrames + FftSize } });
    const auto mags = spectrum(rendered, SettleFrames);
    const double fundamentalEnergy = bandEnergy(mags, 550.0, 650.0);
    const double totalVocal = bandEnergy(mags, 200.0, 4000.0);
    QVERIFY2(fundamentalEnergy > totalVocal * 0.02, qPrintable(QString::number(fundamentalEnergy / totalVocal)));
}

void FormantVoiceV2Test::test_returnPhase_shouldControlHighFrequencyTilt()
{
    const auto upperVsF1 = [](double returnPhase) {
        auto voice = makeVoice();
        voice.setReturnPhase(returnPhase);
        const auto rendered = render(voice, { { "AA", SettleFrames + FftSize } });
        const auto mags = spectrum(rendered, SettleFrames);
        return bandEnergy(mags, 1500.0, 4000.0) / std::max(1.0e-12, bandEnergy(mags, 500.0, 900.0));
    };

    const double bright = upperVsF1(0.005);
    const double soft = upperVsF1(0.060);
    QVERIFY2(bright > soft * 4.0, qPrintable(QString { "bright=%1 vs soft=%2" }.arg(bright).arg(soft)));
}

void FormantVoiceV2Test::test_voicingOnset_shouldBeSofterThanSteadyState()
{
    auto voice = makeVoice();
    const auto sFrames = static_cast<size_t>(SampleRate * 0.08);
    const auto aaFrames = static_cast<size_t>(SampleRate * 0.16);
    const auto rendered = render(voice, { { "S", sFrames }, { "AA", aaFrames } });

    const double onsetRms = rms(rendered, sFrames, sFrames + static_cast<size_t>(SampleRate * 0.015));
    const double steadyRms = rms(rendered, sFrames + static_cast<size_t>(SampleRate * 0.10), sFrames + static_cast<size_t>(SampleRate * 0.15));
    QVERIFY2(onsetRms < steadyRms * 0.65, qPrintable(QString { "onsetRms=%1 vs steadyRms=%2" }.arg(onsetRms).arg(steadyRms)));

    auto cleanOnsetVoice = makeVoice(200.0);
    const auto cleanRendered = render(cleanOnsetVoice, { { "AA", aaFrames } });
    constexpr int onsetFft = 1024;
    const auto onsetSpec = spectrum(cleanRendered, 0, onsetFft);
    const auto steadySpec = spectrum(cleanRendered, static_cast<size_t>(SampleRate * 0.10), onsetFft);
    const double onsetF1VsH1 = bandEnergy(onsetSpec, 550.0, 950.0, onsetFft) / std::max(1.0e-12, bandEnergy(onsetSpec, 140.0, 260.0, onsetFft));
    const double steadyF1VsH1 = bandEnergy(steadySpec, 550.0, 950.0, onsetFft) / std::max(1.0e-12, bandEnergy(steadySpec, 140.0, 260.0, onsetFft));
    QVERIFY2(onsetF1VsH1 < steadyF1VsH1 * 0.85, qPrintable(QString { "onsetF1/H1=%1 vs steadyF1/H1=%2" }.arg(onsetF1VsH1).arg(steadyF1VsH1)));
}

void FormantVoiceV2Test::test_phraseFinal_shouldTaperAndSoftenAtTail()
{
    const auto renderAtProgress = [](double progress) {
        auto voice = makeVoice();
        voice.setPhraseDynamics(1.0);
        voice.setUtteranceProgress(progress);
        return render(voice, { { "AA", SettleFrames + 8192 } });
    };

    const auto midAudio = renderAtProgress(0.50);
    const auto tailAudio = renderAtProgress(0.97);
    const double midRms = rms(midAudio, SettleFrames, midAudio.size());
    const double tailRms = rms(tailAudio, SettleFrames, tailAudio.size());
    QVERIFY2(tailRms < midRms * 0.85, qPrintable(QString { "tailRms=%1 vs midRms=%2" }.arg(tailRms).arg(midRms)));

    const auto midSpec = spectrum(midAudio, SettleFrames, 8192);
    const auto tailSpec = spectrum(tailAudio, SettleFrames, 8192);
    const double midBright = bandEnergy(midSpec, 1500.0, 4000.0, 8192) / std::max(1.0e-12, bandEnergy(midSpec, 80.0, 350.0, 8192));
    const double tailBright = bandEnergy(tailSpec, 1500.0, 4000.0, 8192) / std::max(1.0e-12, bandEnergy(tailSpec, 80.0, 350.0, 8192));
    QVERIFY2(tailBright < midBright * 0.6, qPrintable(QString { "tailBright=%1 vs midBright=%2" }.arg(tailBright).arg(midBright)));
}

void FormantVoiceV2Test::test_diphthong_shouldHoldNucleusThenGlideSmoothly()
{
    auto voice = makeVoice();
    const auto frames = static_cast<size_t>(SampleRate * 0.24);
    const auto rendered = render(voice, { { "AY", frames } });
    constexpr int win = 1024;
    const auto f2At = [&](size_t offset) {
        return centroidInBand(spectrum(rendered, offset, win), 700.0, 2800.0, win);
    };

    const double earlyNucleus = f2At(300);    // ~7%
    const double lateNucleus = f2At(1400);    // ~17% (still in 0..25% hold)
    const double settledOffglide = f2At(10000); // ~90% (settled after 80% glide)

    QVERIFY2(std::abs(lateNucleus - earlyNucleus) < 150.0,
             qPrintable(QString { "early=%1 vs late=%2" }.arg(earlyNucleus).arg(lateNucleus)));
    QVERIFY2(settledOffglide > lateNucleus + 600.0,
             qPrintable(QString { "lateNucleus=%1 vs offglide=%2" }.arg(lateNucleus).arg(settledOffglide)));
}

void FormantVoiceV2Test::test_allophones_darkLAndFlap()
{
    const auto lFrames = static_cast<size_t>(SampleRate * 0.14);
    const auto lCentroid = [&](const Utterance & utterance, size_t offset) {
        auto voice = makeVoice();
        const auto rendered = render(voice, utterance);
        return centroidInBand(spectrum(rendered, offset, 2048), 750.0, 1800.0, 2048);
    };

    const double onsetL = lCentroid({ { "L", lFrames }, { "AA", lFrames } }, 1500);
    const double codaL = lCentroid({ { "IY", lFrames }, { "L", lFrames } }, lFrames + 2000);
    QVERIFY2(onsetL > codaL + 200.0, qPrintable(QString { "onsetL=%1 vs codaL=%2" }.arg(onsetL).arg(codaL)));

    // Alveolar flap DX produces a brief voiced dip without clicking
    auto voice = makeVoice();
    const size_t vFrames = static_cast<size_t>(SampleRate * 0.12);
    const size_t dxFrames = static_cast<size_t>(SampleRate * 0.032);
    const auto & dx = speechFlapPhoneme();
    const auto * ae = speechPhoneme("AE");
    const auto * er = speechPhoneme("ER");
    Q_ASSERT(ae && er);

    std::vector<double> out;
    const auto appendSegment = [&](const PhonemeSpec & spec, const PhonemeSpec * next, size_t frames) {
        voice.setPhoneme(spec, next, static_cast<double>(frames) / SampleRate);
        for (size_t i = 0; i < frames; i++) {
            voice.setPhonemeProgress(static_cast<double>(i) / static_cast<double>(frames));
            out.push_back(voice.nextSample());
        }
    };
    appendSegment(*ae, &dx, vFrames);
    appendSegment(dx, er, dxFrames);
    appendSegment(*er, nullptr, vFrames);

    for (const double s : out) {
        QVERIFY(std::isfinite(s));
    }
    const double aeRms = rms(out, vFrames / 3, vFrames * 2 / 3);
    const double dxRms = rms(out, vFrames + 200, vFrames + dxFrames - 100);
    QVERIFY2(dxRms > 0.005 && dxRms < aeRms * 0.85, qPrintable(QString { "dxRms=%1 vs aeRms=%2" }.arg(dxRms).arg(aeRms)));
}

void FormantVoiceV2Test::test_unstressedVowel_shouldCentralizeTowardSchwa()
{
    const auto measureCentroid = [](const std::string & vowel, bool stressed, double lowHz, double highHz) {
        auto voice = makeVoice();
        voice.setPhonemeStressed(stressed);
        const auto rendered = render(voice, { { vowel, SettleFrames + FftSize } });
        return centroidInBand(spectrum(rendered, SettleFrames), lowHz, highHz, FftSize);
    };

    const double stressedIyF2 = measureCentroid("IY", true, 1400.0, 2600.0);
    const double unstressedIyF2 = measureCentroid("IY", false, 1400.0, 2600.0);
    QVERIFY2(unstressedIyF2 < stressedIyF2 - 120.0,
             qPrintable(QString { "stressed=%1 vs unstressed=%2" }.arg(stressedIyF2).arg(unstressedIyF2)));

    const double stressedAaF1 = measureCentroid("AA", true, 350.0, 950.0);
    const double unstressedAaF1 = measureCentroid("AA", false, 350.0, 950.0);
    QVERIFY2(unstressedAaF1 < stressedAaF1 - 40.0,
             qPrintable(QString { "stressedAA=%1 vs unstressedAA=%2" }.arg(stressedAaF1).arg(unstressedAaF1)));
}

void FormantVoiceV2Test::test_breathiness_shouldAddHighFrequencyAirWithoutLowRumble()
{
    const auto bands = [](double breathiness) {
        auto voice = makeVoice();
        voice.setBreathiness(breathiness);
        const auto rendered = render(voice, { { "AA", SettleFrames + FftSize } });
        const auto mags = spectrum(rendered, SettleFrames);
        return std::pair {
            bandEnergy(mags, 100.0, 400.0),
            bandEnergy(mags, 2000.0, 5000.0)
        };
    };

    const auto [lowDry, highDry] = bands(0.0);
    const auto [lowBreathy, highBreathy] = bands(0.5);
    QVERIFY2(highBreathy > highDry * 1.8, qPrintable(QString { "highDry=%1 vs highBreathy=%2" }.arg(highDry).arg(highBreathy)));
    QVERIFY2(std::abs(lowBreathy - lowDry) < lowDry * 0.20,
             qPrintable(QString { "lowDry=%1 vs lowBreathy=%2" }.arg(lowDry).arg(lowBreathy)));
}

void FormantVoiceV2Test::test_afterReset_shouldBeSilent()
{
    auto voice = makeVoice();
    render(voice, { { "AA", 4000 } });
    voice.reset();

    std::vector<double> out;
    for (size_t i = 0; i < 1000; i++) {
        out.push_back(voice.nextSample());
    }
    QCOMPARE(peakAmplitude(out), 0.0);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::FormantVoiceV2Test)
