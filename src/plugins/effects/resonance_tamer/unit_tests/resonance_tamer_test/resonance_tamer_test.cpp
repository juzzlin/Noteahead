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

#include "resonance_tamer_test.hpp"

#include "../../../../../common/constants.hpp"
#include "../../../../../domain/dsp/audio_context.hpp"
#include "../../../../../domain/dsp/fft.hpp"
#include "../../resonance_tamer.hpp"

#include <QTest>

#include <algorithm>
#include <array>
#include <cmath>
#include <memory>
#include <numbers>
#include <random>
#include <set>
#include <span>
#include <string>
#include <vector>

namespace noteahead {

namespace {

constexpr double Rate { 48000.0 };
constexpr size_t BlockFrames { 512 };
constexpr size_t SpectrumSize { 4096 };
namespace C = Constants::NahdXml;

void setParameter(ResonanceTamer & tamer, const QString & key, float value)
{
    if (const auto p = tamer.parameter(key.toStdString()); p) {
        p->get().update(value);
    }
    tamer.sync();
}

std::unique_ptr<ResonanceTamer> makeTamer()
{
    auto tamer = std::make_unique<ResonanceTamer>();
    tamer->setSampleRate(Rate);
    return tamer;
}

size_t framesOf(double seconds)
{
    return static_cast<size_t>(seconds * Rate);
}

//! Pink noise of about @p rms, independent on the two sides (Paul Kellet's filter).
std::vector<double> pinkNoise(size_t frames, double rms, unsigned seed = 1)
{
    std::mt19937 generator { seed };
    std::normal_distribution<double> white { 0.0, 1.0 };
    std::vector<double> stereo(frames * 2);
    for (size_t channel = 0; channel < 2; channel++) {
        std::array<double, 7> b {};
        double sum = 0.0;
        std::vector<double> mono(frames);
        for (size_t i = 0; i < frames; i++) {
            const double w = white(generator);
            b[0] = 0.99886 * b[0] + w * 0.0555179;
            b[1] = 0.99332 * b[1] + w * 0.0750759;
            b[2] = 0.96900 * b[2] + w * 0.1538520;
            b[3] = 0.86650 * b[3] + w * 0.3104856;
            b[4] = 0.55000 * b[4] + w * 0.5329522;
            b[5] = -0.7616 * b[5] - w * 0.0168980;
            mono[i] = b[0] + b[1] + b[2] + b[3] + b[4] + b[5] + b[6] + w * 0.5362;
            b[6] = w * 0.115926;
            sum += mono[i] * mono[i];
        }
        const double scale = rms / std::sqrt(sum / static_cast<double>(frames));
        for (size_t i = 0; i < frames; i++) {
            stereo[i * 2 + channel] = mono[i] * scale;
        }
    }
    return stereo;
}

//! A sine on the sides @p channels names (bit 0 left, bit 1 right), gliding exponentially from
//! @p fromHz to @p toHz.
void addSine(std::vector<double> & stereo, double fromHz, double toHz, double amplitude, int channels = 3)
{
    const size_t frames = stereo.size() / 2;
    double phase = 0.0;
    for (size_t i = 0; i < frames; i++) {
        const double hz = fromHz * std::pow(toHz / fromHz, static_cast<double>(i) / static_cast<double>(frames));
        const double value = amplitude * std::sin(phase);
        phase += 2.0 * std::numbers::pi * hz / Rate;
        for (size_t channel = 0; channel < 2; channel++) {
            if (channels & (1 << channel)) {
                stereo[i * 2 + channel] += value;
            }
        }
    }
}

std::vector<double> render(ResonanceTamer & tamer, std::vector<double> stereo)
{
    for (size_t start = 0; start < stereo.size() / 2; start += BlockFrames) {
        const auto frames = static_cast<uint32_t>(std::min(BlockFrames, stereo.size() / 2 - start));
        AudioContext context { std::span(stereo.data() + start * 2, frames * 2), frames, static_cast<uint32_t>(Rate) };
        tamer.process(context);
    }
    return stereo;
}

//! The level of a steady sine at @p hz on @p channel over the last second, in dB.
double sineLevelDb(const std::vector<double> & stereo, double hz, size_t channel)
{
    const size_t frames = stereo.size() / 2;
    const size_t from = frames - framesOf(1.0);
    double inPhase = 0.0;
    double quadrature = 0.0;
    for (size_t i = from; i < frames; i++) {
        const double angle = 2.0 * std::numbers::pi * hz * static_cast<double>(i) / Rate;
        inPhase += stereo[i * 2 + channel] * std::cos(angle);
        quadrature += stereo[i * 2 + channel] * std::sin(angle);
    }
    const double amplitude = 2.0 * std::hypot(inPhase, quadrature) / static_cast<double>(frames - from);
    return 20.0 * std::log10(amplitude);
}

//! The level of @p channel over the last @p seconds, in dB.
double rmsDb(const std::vector<double> & stereo, size_t channel, double seconds)
{
    const size_t frames = stereo.size() / 2;
    double sum = 0.0;
    for (size_t i = frames - framesOf(seconds); i < frames; i++) {
        sum += stereo[i * 2 + channel] * stereo[i * 2 + channel];
    }
    return 10.0 * std::log10(sum / static_cast<double>(framesOf(seconds)));
}

//! The power spectrum of @p channel over the second half, averaged over half-overlapping windows.
std::vector<double> powerSpectrum(const std::vector<double> & stereo, size_t channel)
{
    const size_t frames = stereo.size() / 2;
    std::vector<double> power(SpectrumSize / 2 + 1, 0.0);
    std::vector<double> input(SpectrumSize);
    std::vector<double> re(SpectrumSize);
    std::vector<double> im(SpectrumSize);
    for (size_t start = frames / 2; start + SpectrumSize <= frames; start += SpectrumSize / 2) {
        for (size_t i = 0; i < SpectrumSize; i++) {
            const double window = 0.5 - 0.5 * std::cos(2.0 * std::numbers::pi * static_cast<double>(i) / SpectrumSize);
            input[i] = stereo[(start + i) * 2 + channel] * window;
        }
        Fft::forwardReal(input.data(), re.data(), im.data(), static_cast<int>(SpectrumSize));
        for (size_t k = 0; k < power.size(); k++) {
            power[k] += re[k] * re[k] + im[k] * im[k];
        }
    }
    return power;
}

double bandDb(const std::vector<double> & power, double lowHz, double highHz)
{
    double sum = 0.0;
    for (size_t k = 0; k < power.size(); k++) {
        if (const double hz = static_cast<double>(k) * Rate / SpectrumSize; hz >= lowHz && hz < highHz) {
            sum += power[k];
        }
    }
    return 10.0 * std::log10(sum + 1.0e-30);
}

//! The largest change of a third-octave band between @p input and @p output on @p channel, from
//! 100 Hz to 16 kHz, leaving out the bands that touch @p skipLowHz..@p skipHighHz.
double largestBandChangeDb(const std::vector<double> & input, const std::vector<double> & output, size_t channel, double skipLowHz = 0.0, double skipHighHz = 0.0)
{
    const auto in = powerSpectrum(input, channel);
    const auto out = powerSpectrum(output, channel);
    double largest = 0.0;
    for (double low = 100.0; low < 16000.0; low *= std::pow(2.0, 1.0 / 3.0)) {
        const double high = low * std::pow(2.0, 1.0 / 3.0);
        if (high > skipLowHz && low < skipHighHz) {
            continue;
        }
        largest = std::max(largest, std::abs(bandDb(out, low, high) - bandDb(in, low, high)));
    }
    return largest;
}

std::vector<double> sineOverNoise(double hz, double seconds = 3.0)
{
    auto signal = pinkNoise(framesOf(seconds), 0.05);
    addSine(signal, hz, hz, 0.05);
    return signal;
}

} // namespace

void ResonanceTamerTest::test_processBlock_sineOverNoise_shouldCutTheSineByDepth()
{
    // Far out of the outline, so the whole of Depth: the default 6 dB
    auto tamer = makeTamer();
    const auto input = sineOverNoise(3000.0);
    const auto output = render(*tamer, input);

    const double cutDb = sineLevelDb(input, 3000.0, 0) - sineLevelDb(output, 3000.0, 0);
    QVERIFY2(cutDb > 5.0 && cutDb < 6.5, qPrintable(QString::number(cutDb)));
    QVERIFY(tamer->deepestCutDb() < -5.0);
}

void ResonanceTamerTest::test_processBlock_sineOverNoise_shouldLeaveTheNoiseAlone()
{
    auto tamer = makeTamer();
    const auto input = sineOverNoise(3000.0);
    const auto output = render(*tamer, input);

    // Half an octave either side of the sine is the bell's to take
    const double changeDb = largestBandChangeDb(input, output, 0, 3000.0 / std::sqrt(2.0), 3000.0 * std::sqrt(2.0));
    QVERIFY2(changeDb < 0.5, qPrintable(QString::number(changeDb)));
}

void ResonanceTamerTest::test_processBlock_pinkNoise_shouldBeLeftAlone()
{
    // Nothing stands out of noise for long enough to be cut
    auto tamer = makeTamer();
    const auto input = pinkNoise(framesOf(3.0), 0.05);
    const auto output = render(*tamer, input);

    const double changeDb = largestBandChangeDb(input, output, 0);
    QVERIFY2(changeDb < 0.5, qPrintable(QString::number(changeDb)));
    QVERIFY2(std::abs(rmsDb(output, 0, 1.0) - rmsDb(input, 0, 1.0)) < 0.2, qPrintable(QString::number(rmsDb(output, 0, 1.0) - rmsDb(input, 0, 1.0))));
}

void ResonanceTamerTest::test_processBlock_sineOutsideRange_shouldBeLeftAlone()
{
    auto tamer = makeTamer();
    setParameter(*tamer, C::xmlKeyLowFreq(), ResonanceTamer::frequencyPosition(6000.0));
    const auto input = sineOverNoise(3000.0);
    const auto output = render(*tamer, input);

    const double cutDb = sineLevelDb(input, 3000.0, 0) - sineLevelDb(output, 3000.0, 0);
    QVERIFY2(std::abs(cutDb) < 0.1, qPrintable(QString::number(cutDb)));
}

void ResonanceTamerTest::test_processBlock_zeroDepth_shouldBeTransparent()
{
    auto tamer = makeTamer();
    setParameter(*tamer, C::xmlKeyDepth(), 0.0f);
    const auto input = sineOverNoise(3000.0, 1.0);
    const auto output = render(*tamer, input);

    for (size_t i = 0; i < input.size(); i++) {
        QCOMPARE(output[i], input[i]);
    }
}

void ResonanceTamerTest::test_processBlock_sweptSine_shouldFollowIt()
{
    // An octave in three seconds: the bell has to glide after it rather than be left behind
    auto tamer = makeTamer();
    auto input = pinkNoise(framesOf(3.0), 0.005);
    addSine(input, 1000.0, 2000.0, 0.1);
    const auto output = render(*tamer, input);

    const double cutDb = rmsDb(input, 0, 0.5) - rmsDb(output, 0, 0.5);
    QVERIFY2(cutDb > 4.0, qPrintable(QString::number(cutDb)));
}

void ResonanceTamerTest::test_processBlock_stereoMode_shouldCutOnlyTheRingingSide()
{
    auto tamer = makeTamer();
    setParameter(*tamer, C::xmlKeyStereoMode(), static_cast<float>(ResonanceTamer::StereoMode::Stereo));
    auto input = pinkNoise(framesOf(3.0), 0.05);
    addSine(input, 3000.0, 3000.0, 0.05, 1);
    const auto output = render(*tamer, input);

    const double leftCutDb = sineLevelDb(input, 3000.0, 0) - sineLevelDb(output, 3000.0, 0);
    QVERIFY2(leftCutDb > 5.0, qPrintable(QString::number(leftCutDb)));
    const double rightChangeDb = largestBandChangeDb(input, output, 1);
    QVERIFY2(rightChangeDb < 0.5, qPrintable(QString::number(rightChangeDb)));
}

void ResonanceTamerTest::test_process_solo_shouldBeWhatIsTakenOut()
{
    auto tamer = makeTamer();
    auto delta = makeTamer();
    setParameter(*delta, C::xmlKeySolo(), 1.0f);
    const auto input = sineOverNoise(3000.0, 1.0);
    const auto output = render(*tamer, input);
    const auto removed = render(*delta, input);

    for (size_t i = 0; i < input.size(); i++) {
        QVERIFY(std::abs(removed[i] - (output[i] - input[i])) < 1.0e-12);
    }
    QVERIFY(rmsDb(removed, 0, 0.5) > -60.0);
}

void ResonanceTamerTest::test_latencyFrames_shouldBeZero()
{
    // The bells are in the time domain: nothing to compensate, wherever it is inserted
    const auto tamer = makeTamer();
    QCOMPARE(tamer->latencyFrames(Rate), size_t { 0 });
}

void ResonanceTamerTest::test_factoryPresets_shouldAllProduceFiniteOutput()
{
    auto tamer = makeTamer();
    const auto & presets = tamer->factoryPresets();
    QVERIFY(presets.size() > 1);
    std::set<std::string> names;
    for (size_t i = 0; i < presets.size(); i++) {
        names.insert(presets.at(i).name);
        QVERIFY(tamer->applyFactoryPreset(i));
        tamer->sync();
        tamer->reset();
        const auto output = render(*tamer, sineOverNoise(3000.0, 1.0));
        QVERIFY(std::all_of(output.begin(), output.end(), [](double value) {
            return std::isfinite(value);
        }));
    }
    QCOMPARE(names.size(), presets.size());
}

void ResonanceTamerTest::test_reset_shouldReplayDeterministically()
{
    auto tamer = makeTamer();
    QVERIFY(tamer->applyFactoryPreset(1));
    setParameter(*tamer, C::xmlKeyLowFreq(), ResonanceTamer::frequencyPosition(400.0));
    setParameter(*tamer, C::xmlKeyHighFreq(), ResonanceTamer::frequencyPosition(20000.0));
    setParameter(*tamer, C::xmlKeyDepth(), 0.75f);
    tamer->reset();

    const auto input = sineOverNoise(3000.0, 1.0);
    const auto first = render(*tamer, input);

    // Changing settings mid-stream and resetting must rebuild the bin cache cleanly
    setParameter(*tamer, C::xmlKeyLowFreq(), ResonanceTamer::frequencyPosition(8000.0));
    tamer->reset();
    const auto outOfRange = render(*tamer, input);
    const double cutSkipped = sineLevelDb(input, 3000.0, 0) - sineLevelDb(outOfRange, 3000.0, 0);
    QVERIFY2(std::abs(cutSkipped) < 0.2, qPrintable(QString::number(cutSkipped)));

    // Restoring settings must replay identically to first run
    QVERIFY(tamer->applyFactoryPreset(1));
    setParameter(*tamer, C::xmlKeyLowFreq(), ResonanceTamer::frequencyPosition(400.0));
    setParameter(*tamer, C::xmlKeyHighFreq(), ResonanceTamer::frequencyPosition(20000.0));
    setParameter(*tamer, C::xmlKeyDepth(), 0.75f);
    tamer->reset();
    const auto second = render(*tamer, input);

    for (size_t i = 0; i < first.size(); i++) {
        QVERIFY2(std::abs(first[i] - second[i]) < 1.0e-12, qPrintable(QString { "Replay mismatch at %1" }.arg(i)));
    }
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::ResonanceTamerTest)
