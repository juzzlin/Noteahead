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

#include <QTest>

#include <algorithm>
#include <cmath>
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

    // Upper formants provide real energy above 3 kHz
    const double body = bandEnergy(aaSpec, 200.0, 3000.0);
    const double high = bandEnergy(aaSpec, 3000.0, 6000.0);
    QVERIFY2(high > body * 1.0e-5, qPrintable(QString::number(10.0 * std::log10(high / body), 'f', 1)));

    const double iyBody = bandEnergy(iySpec, 200.0, 3000.0);
    const double iyHigh = bandEnergy(iySpec, 3000.0, 6000.0);
    QVERIFY2(iyHigh > iyBody * 5.0e-4, qPrintable(QString::number(10.0 * std::log10(iyHigh / iyBody), 'f', 1)));
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

    // Phrase-final stop is unreleased
    {
        auto endVoice = makeVoice();
        const auto atEnd = render(endVoice, { { "AA", frames }, { "T", frames } });
        auto relVoice = makeVoice();
        const auto released = render(relVoice, { { "AA", frames }, { "T", frames }, { "AA", frames } });
        const auto tailPeak = [&](const std::vector<double> & s) {
            return peakAmplitude(std::vector<double>(s.begin() + static_cast<long>(frames + frames / 2), s.begin() + static_cast<long>(2 * frames)));
        };
        QVERIFY(tailPeak(atEnd) < tailPeak(released) * 0.25);
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

void FormantVoiceV2Test::test_highPitch_shouldPreserveFundamentalThroughF1Floor()
{
    auto voice = makeVoice(600.0);
    const auto rendered = render(voice, { { "IY", SettleFrames + FftSize } });
    const auto mags = spectrum(rendered, SettleFrames);
    const double fundamentalEnergy = bandEnergy(mags, 550.0, 650.0);
    const double totalVocal = bandEnergy(mags, 200.0, 4000.0);
    QVERIFY2(fundamentalEnergy > totalVocal * 0.02, qPrintable(QString::number(fundamentalEnergy / totalVocal)));
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
