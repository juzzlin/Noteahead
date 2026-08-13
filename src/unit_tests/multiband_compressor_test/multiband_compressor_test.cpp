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

#include "multiband_compressor_test.hpp"

#include "../../common/constants.hpp"
#include "../../common/parameter_mapper.hpp"
#include "../../common/utils.hpp"
#include "../../domain/dsp/audio_context.hpp"
#include "../../domain/dsp/linkwitz_riley_crossover.hpp"
#include "../../domain/effects/multiband_compressor.hpp"

#include <QTest>

#include <cmath>
#include <numbers>
#include <span>
#include <vector>

namespace noteahead {

namespace {

constexpr double sampleRate = 48000.0;
constexpr uint32_t frameCount = 512;

void setParameter(MultibandCompressor & effect, const QString & key, float value)
{
    if (auto p = effect.parameter(key.toStdString()); p) {
        p->get().setValue(value);
    }
    effect.sync();
}

void setCrossoverHz(MultibandCompressor & effect, size_t index, double frequency)
{
    setParameter(effect, Constants::NahdXml::xmlKeyCrossoverFreq(index), static_cast<float>(ParameterMapper::unmapLogFrequency(frequency, 20.0, 20000.0)));
}

void setThresholdDb(MultibandCompressor & effect, size_t band, double thresholdDb)
{
    setParameter(effect, Constants::NahdXml::xmlKeyBandThreshold(band), static_cast<float>((thresholdDb + 60.0) / 60.0));
}

//! Hard knee everywhere, so a level under the threshold is left strictly alone.
void setHardKnee(MultibandCompressor & effect)
{
    for (size_t band = 0; band < MultibandCompressor::NumBands; band++) {
        setParameter(effect, Constants::NahdXml::xmlKeyBandKnee(band), 0.0f);
    }
}

//! Puts every band's threshold out of reach, leaving the crossovers as the only thing acting on the
//! signal. That is what makes the reconstruction tests measure the split rather than the dynamics.
void disableCompression(MultibandCompressor & effect)
{
    setHardKnee(effect);
    for (size_t band = 0; band < MultibandCompressor::NumBands; band++) {
        setThresholdDb(effect, band, 0.0);
    }
}

double rms(const std::vector<double> & samples, size_t fromIndex)
{
    double sum = 0.0;
    for (size_t i = fromIndex; i < samples.size(); i++) {
        sum += samples[i] * samples[i];
    }
    return std::sqrt(sum / static_cast<double>(samples.size() - fromIndex));
}

//! How much of a sine survived, as a ratio against the RMS a sine of that amplitude has.
//!
//! Measured over a whole number of periods at the end of the run, which is what makes the answer
//! trustworthy: a Linkwitz-Riley split reconstructs the spectrum but rotates the phase, so a window
//! cut mid-period would catch input and output at different points in their cycle and report a
//! difference that is not there. The test frequencies all divide the sample rate exactly.
double magnitudeRatio(const std::vector<double> & output, double frequency, double amplitude)
{
    const size_t samplesPerPeriod = static_cast<size_t>(std::round(sampleRate / frequency));
    const size_t periods = (output.size() / 2) / samplesPerPeriod;
    const size_t fromIndex = output.size() - periods * samplesPerPeriod;

    return rms(output, fromIndex) / (amplitude / std::numbers::sqrt2);
}

//! Runs a sine through the effect and returns how much of it came out the other end.
double sineMagnitudeRatio(MultibandCompressor & effect, double frequency, double amplitude, int frameTotal)
{
    std::vector<double> output;
    output.reserve(static_cast<size_t>(frameTotal));

    for (int i = 0; i < frameTotal; i++) {
        const double sample = amplitude * std::sin(2.0 * std::numbers::pi * frequency * static_cast<double>(i) / sampleRate);
        double left = sample;
        double right = sample;
        effect.process(left, right);
        output.push_back(left);
    }

    return magnitudeRatio(output, frequency, amplitude);
}

//! Runs blocks of DC through the effect, optionally with a side chain device buffer, and returns the
//! last output sample. DC sits entirely in the low band and holds the peak detector at a known level
//! with no envelope ripple to average out.
double processDc(MultibandCompressor & effect, double input, double sideChain, bool useSideChain, uint32_t blockCount)
{
    std::vector<double> buffer(frameCount * 2, 0.0);
    std::vector<double> sideChainBuffer(frameCount * 2, sideChain);
    std::vector<std::span<const double>> deviceBuffers { std::span<const double> { sideChainBuffer } };

    double lastSample = 0.0;
    for (uint32_t block = 0; block < blockCount; block++) {
        std::fill(buffer.begin(), buffer.end(), input);

        AudioContext context;
        context.buffer = buffer;
        context.frameCount = frameCount;
        context.sampleRate = static_cast<uint32_t>(sampleRate);
        if (useSideChain) {
            context.deviceOutputBuffers = deviceBuffers;
        }

        effect.process(context);
        lastSample = buffer[(frameCount - 1) * 2];
    }

    return lastSample;
}

} // namespace

void MultibandCompressorTest::test_crossover_twoBands_shouldSumToUnityMagnitude()
{
    for (const double frequency : { 50.0, 200.0, 500.0, 1000.0, 2000.0, 6000.0, 12000.0 }) {
        LinkwitzRileyCrossover crossover;
        crossover.setSampleRate(sampleRate);
        crossover.setCutoff(1000.0);

        std::vector<double> output;
        const int frameTotal = 16384;
        for (int i = 0; i < frameTotal; i++) {
            const double sample = std::sin(2.0 * std::numbers::pi * frequency * static_cast<double>(i) / sampleRate);
            double low = 0.0;
            double high = 0.0;
            crossover.process(sample, low, high);
            output.push_back(low + high);
        }

        // Right on the corner each band is 6dB down, and it is only because they sum to an all-pass
        // that the total comes back to unity.
        const double ratio = magnitudeRatio(output, frequency, 1.0);
        QVERIFY2(std::abs(ratio - 1.0) < 0.01, qPrintable(QString { "Ratio %1 at %2 Hz" }.arg(ratio).arg(frequency)));
    }
}

void MultibandCompressorTest::test_crossover_threeBands_shouldSumToUnityMagnitude()
{
    for (const double frequency : { 50.0, 200.0, 500.0, 2000.0, 6000.0, 12000.0 }) {
        MultibandCompressor effect;
        effect.setSampleRate(sampleRate);
        disableCompression(effect);
        setCrossoverHz(effect, 0, 200.0);
        setCrossoverHz(effect, 1, 2000.0);

        // The all-pass correction on the low band is what this measures: without it the sum notches
        // around the upper corner instead of staying flat.
        const double ratio = sineMagnitudeRatio(effect, frequency, 0.1, 16384);
        QVERIFY2(std::abs(ratio - 1.0) < 0.01, qPrintable(QString { "Ratio %1 at %2 Hz" }.arg(ratio).arg(frequency)));
    }
}

void MultibandCompressorTest::test_crossover_bandSplit_shouldSeparateByFrequency()
{
    const auto soloedBandRatio = [](size_t band, double frequency) {
        MultibandCompressor effect;
        effect.setSampleRate(sampleRate);
        disableCompression(effect);
        setCrossoverHz(effect, 0, 200.0);
        setCrossoverHz(effect, 1, 2000.0);
        setParameter(effect, Constants::NahdXml::xmlKeyBandSolo(band), 1.0f);
        return sineMagnitudeRatio(effect, frequency, 0.1, 16384);
    };

    // Well inside a band the soloed band carries the signal whole, and the other two reject it.
    QVERIFY(soloedBandRatio(0, 50.0) > 0.95);
    QVERIFY(soloedBandRatio(1, 50.0) < 0.05);
    QVERIFY(soloedBandRatio(2, 50.0) < 0.05);

    QVERIFY(soloedBandRatio(1, 800.0) > 0.95);
    QVERIFY(soloedBandRatio(0, 800.0) < 0.05);
    QVERIFY(soloedBandRatio(2, 800.0) < 0.05);

    QVERIFY(soloedBandRatio(2, 8000.0) > 0.95);
    QVERIFY(soloedBandRatio(0, 8000.0) < 0.05);
    QVERIFY(soloedBandRatio(1, 8000.0) < 0.05);
}

void MultibandCompressorTest::test_gain_quietSignal_shouldPassThroughAtUnityMagnitude()
{
    MultibandCompressor effect;
    effect.setSampleRate(sampleRate);
    setHardKnee(effect);

    // -30dB is below the -20dB default threshold, so nothing should be held down.
    const double output = processDc(effect, Utils::Dsp::dbToLinear(-30.0), 0.0, false, 20);

    QVERIFY(std::abs(output - Utils::Dsp::dbToLinear(-30.0)) < 1.0e-4);
    for (size_t band = 0; band < MultibandCompressor::NumBands; band++) {
        QCOMPARE(effect.bandReductionDb(band), 0.0f);
    }
}

void MultibandCompressorTest::test_gain_loudBand_shouldCompressOnlyThatBand()
{
    MultibandCompressor effect;
    effect.setSampleRate(sampleRate);
    setHardKnee(effect);

    // DC lands entirely in the low band, so only that band's detector should ever see it.
    processDc(effect, 1.0, 0.0, false, 40);

    QVERIFY(effect.bandReductionDb(0) < -10.0f);

    // Not exactly zero: the signal starting from silence is a step, and a step is broadband, so the
    // upper bands do catch that one edge and are still releasing from it.
    QVERIFY(effect.bandReductionDb(1) > -0.1f);
    QVERIFY(effect.bandReductionDb(2) > -0.1f);
}

void MultibandCompressorTest::test_gain_ratio_shouldSettleToExpectedReduction()
{
    MultibandCompressor effect;
    effect.setSampleRate(sampleRate);
    setHardKnee(effect);
    setThresholdDb(effect, 0, -20.0);

    // 0dB into a -20dB threshold at the default 4:1 overshoots by 20dB, so it settles 15dB down.
    const double output = processDc(effect, 1.0, 0.0, false, 40);

    QVERIFY(effect.bandReductionDb(0) < -14.5f);
    QVERIFY(effect.bandReductionDb(0) > -15.5f);
    QVERIFY(std::abs(output - Utils::Dsp::dbToLinear(-15.0)) < 0.02);
}

void MultibandCompressorTest::test_makeup_shouldLiftOnlyItsOwnBand()
{
    MultibandCompressor effect;
    effect.setSampleRate(sampleRate);
    setHardKnee(effect);

    // Internal 0.75 of a -12..+12dB range is +6dB.
    setParameter(effect, Constants::NahdXml::xmlKeyBandMakeup(0), 0.75f);

    const double level = Utils::Dsp::dbToLinear(-30.0);
    const double output = processDc(effect, level, 0.0, false, 20);

    QCOMPARE(effect.bandReductionDb(0), 0.0f);
    QVERIFY(std::abs(output - level * Utils::Dsp::dbToLinear(6.0)) < 1.0e-4);
}

void MultibandCompressorTest::test_bypass_loudBand_shouldNotApplyGain()
{
    MultibandCompressor effect;
    effect.setSampleRate(sampleRate);
    setHardKnee(effect);
    setParameter(effect, Constants::NahdXml::xmlKeyBandBypass(0), 1.0f);

    const double output = processDc(effect, 1.0, 0.0, false, 40);

    // The band passes untouched, but its detector keeps running so that clearing bypass does not step.
    QVERIFY(std::abs(output - 1.0) < 1.0e-4);
    QVERIFY(effect.bandReductionDb(0) < -10.0f);
}

void MultibandCompressorTest::test_solo_shouldPassOnlySoloedBands()
{
    MultibandCompressor effect;
    effect.setSampleRate(sampleRate);
    disableCompression(effect);
    setParameter(effect, Constants::NahdXml::xmlKeyBandSolo(2), 1.0f);

    // DC belongs to the low band, which is not the soloed one.
    const double output = processDc(effect, 1.0, 0.0, false, 20);

    QVERIFY(std::abs(output) < 1.0e-4);
}

void MultibandCompressorTest::test_sideChain_loudSource_shouldCompressMatchingBand()
{
    MultibandCompressor effect;
    effect.setSampleRate(sampleRate);
    setHardKnee(effect);
    setParameter(effect, Constants::NahdXml::xmlKeySideChainSourceDevice(), 0.0f);

    const double level = Utils::Dsp::dbToLinear(-30.0);
    const double output = processDc(effect, level, 1.0, true, 40);

    // The input alone would never trip the threshold; the side chain is what pulls the band down.
    QVERIFY(effect.bandReductionDb(0) < -10.0f);
    QVERIFY(output < level * 0.5);
    QVERIFY(effect.bandReductionDb(2) > -0.1f);
}

void MultibandCompressorTest::test_sideChainSourceDeviceIndex_unset_shouldBeEmpty()
{
    MultibandCompressor effect;

    QVERIFY(!effect.sidechainSourceDeviceIndex());
}

void MultibandCompressorTest::test_crossoverFrequencies_inverted_shouldKeepBandOrder()
{
    MultibandCompressor effect;
    effect.setSampleRate(sampleRate);
    disableCompression(effect);

    // Asking for an upper corner below the lower one must not fold the bands over each other.
    setCrossoverHz(effect, 0, 5000.0);
    setCrossoverHz(effect, 1, 100.0);

    for (const double frequency : { 50.0, 1000.0, 8000.0 }) {
        const double ratio = sineMagnitudeRatio(effect, frequency, 0.1, 16384);
        QVERIFY2(std::abs(ratio - 1.0) < 0.01, qPrintable(QString { "Ratio %1 at %2 Hz" }.arg(ratio).arg(frequency)));
    }
}

void MultibandCompressorTest::test_reset_afterCompressing_shouldReturnToUnity()
{
    MultibandCompressor effect;
    effect.setSampleRate(sampleRate);
    setHardKnee(effect);

    processDc(effect, 1.0, 0.0, false, 40);
    QVERIFY(effect.bandReductionDb(0) < -10.0f);

    effect.reset();

    for (size_t band = 0; band < MultibandCompressor::NumBands; band++) {
        QCOMPARE(effect.bandReductionDb(band), 0.0f);
    }
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::MultibandCompressorTest)
