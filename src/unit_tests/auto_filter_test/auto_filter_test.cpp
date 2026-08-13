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

#include "auto_filter_test.hpp"

#include "../../common/constants.hpp"
#include "../../domain/effects/auto_filter.hpp"

#include <QTest>

#include <cmath>
#include <memory>
#include <numbers>
#include <vector>

namespace noteahead {

namespace {

constexpr double testSampleRate = 44100.0;

//! Window the RMS readings are taken over. Every test tone fits a whole number of cycles into it,
//! so a window's reading only moves when the filter does.
constexpr size_t wholeCycleWindow = 441;

//! Internal value that leaves a bipolar parameter at its centre, which is no modulation at all.
constexpr float noModulation = 0.5f;

void setParameter(AutoFilter & effect, const QString & key, float value)
{
    const auto parameter = effect.parameter(key.toStdString());
    QVERIFY(parameter.has_value());
    parameter->get().setValue(value);
}

//! A filter that only does what its knobs say: both LFOs and the envelope follower centred.
void silenceModulation(AutoFilter & effect)
{
    setParameter(effect, Constants::NahdXml::xmlKeyLfoIntensity(), noModulation);
    setParameter(effect, Constants::NahdXml::xmlKeyLfo2Intensity(), noModulation);
    setParameter(effect, Constants::NahdXml::xmlKeyEnvMod(), noModulation);
}

double sineAt(double frequency, size_t index, double amplitude = 1.0)
{
    return amplitude * std::sin(2.0 * std::numbers::pi * frequency * static_cast<double>(index) / testSampleRate);
}

//! Output RMS of a sine fed through the effect, past the quarter second the filter needs to settle.
double renderRms(AutoFilter & effect, double frequency, double seconds = 0.4, double amplitude = 1.0)
{
    const auto sampleCount = static_cast<size_t>(testSampleRate * seconds);
    const auto skipCount = sampleCount / 4;
    double sum = 0.0;
    size_t counted = 0;
    for (size_t i = 0; i < sampleCount; i++) {
        double left = sineAt(frequency, i, amplitude);
        double right = left;
        effect.process(left, right);
        if (i >= skipCount) {
            sum += left * left;
            counted++;
        }
    }
    return std::sqrt(sum / static_cast<double>(counted));
}

//! Output RMS window by window, which is what a sweep looks like from the outside.
std::vector<double> renderRmsEnvelope(AutoFilter & effect, double frequency, double seconds, size_t windowSize = wholeCycleWindow)
{
    const auto sampleCount = static_cast<size_t>(testSampleRate * seconds);
    std::vector<double> envelope;
    double sum = 0.0;
    for (size_t i = 0; i < sampleCount; i++) {
        double left = sineAt(frequency, i);
        double right = left;
        effect.process(left, right);
        sum += left * left;
        if ((i + 1) % windowSize == 0) {
            envelope.push_back(std::sqrt(sum / static_cast<double>(windowSize)));
            sum = 0.0;
        }
    }
    return envelope;
}

//! How many times the sweep came back around, counted as upward crossings of the halfway level.
size_t countCycles(const std::vector<double> & envelope)
{
    const auto [min, max] = std::minmax_element(envelope.begin(), envelope.end());
    const double threshold = (*min + *max) / 2.0;
    size_t cycles = 0;
    for (size_t i = 1; i < envelope.size(); i++) {
        if (envelope[i - 1] <= threshold && envelope[i] > threshold) {
            cycles++;
        }
    }
    return cycles;
}

//! A discrete parameter holds its setting as the setting itself, not as a position between its
//! ends the way a continuous one does.
float discrete(int value)
{
    return static_cast<float>(value);
}

//! Rate that gives the LFO the requested frequency in its Hz mode, inverting the curve
//! ParameterMapper::mapLfoFrequency puts the rate knob on.
float rateForHz(double hz)
{
    return static_cast<float>(std::log(hz + 0.95) / std::log(20.0));
}

//! Cutoff that puts the filter's corner at the requested frequency, the inverse of the exponential
//! 20 Hz to 20 kHz sweep the filter reads its normalized cutoff as.
float cutoffForHz(double hz)
{
    return static_cast<float>(std::log(hz / 20.0) / std::log(1000.0));
}

enum FilterType
{
    LowPass = 0,
    HighPass = 1,
    BandPass = 2,
    Notch = 3
};

//! An unmodulated filter of the given type, cornered at the given frequency.
std::unique_ptr<AutoFilter> makeFilter(int filterType, int slope, float resonance, double cutoffHz)
{
    auto effect = std::make_unique<AutoFilter>();
    effect->setSampleRate(testSampleRate);
    silenceModulation(*effect);
    setParameter(*effect, Constants::NahdXml::xmlKeyFilterType(), discrete(filterType));
    setParameter(*effect, Constants::NahdXml::xmlKeyFilterSlope(), discrete(slope));
    setParameter(*effect, Constants::NahdXml::xmlKeyResonance(), resonance);
    setParameter(*effect, Constants::NahdXml::xmlKeyCutoff(), cutoffForHz(cutoffHz));
    effect->sync();
    return effect;
}

//! Output level relative to the unit sine that went in.
double renderGainDb(AutoFilter & effect, double frequency)
{
    static constexpr double unitSineRms = 0.70710678;
    return 20.0 * std::log10(std::max(1.0e-9, renderRms(effect, frequency) / unitSineRms));
}

} // namespace

void AutoFilterTest::test_process_lowPass_shouldAttenuateHighFrequencies()
{
    AutoFilter effect;
    effect.setSampleRate(testSampleRate);
    silenceModulation(effect);
    setParameter(effect, Constants::NahdXml::xmlKeyFilterType(), discrete(LowPass));
    setParameter(effect, Constants::NahdXml::xmlKeyResonance(), 0.0f);
    setParameter(effect, Constants::NahdXml::xmlKeyCutoff(), cutoffForHz(1000.0));
    effect.sync();

    const double passed = renderRms(effect, 100.0);
    effect.reset();
    const double stopped = renderRms(effect, 10000.0);

    // A unit sine has an RMS of about 0.707 and the pass band leaves it there.
    QVERIFY(passed > 0.6);
    QVERIFY(stopped < passed * 0.1);
}

void AutoFilterTest::test_process_highPass_shouldAttenuateLowFrequencies()
{
    AutoFilter effect;
    effect.setSampleRate(testSampleRate);
    silenceModulation(effect);
    setParameter(effect, Constants::NahdXml::xmlKeyFilterType(), discrete(HighPass));
    setParameter(effect, Constants::NahdXml::xmlKeyResonance(), 0.0f);
    setParameter(effect, Constants::NahdXml::xmlKeyCutoff(), cutoffForHz(1000.0));
    effect.sync();

    const double passed = renderRms(effect, 10000.0);
    effect.reset();
    const double stopped = renderRms(effect, 100.0);

    QVERIFY(passed > 0.6);
    QVERIFY(stopped < passed * 0.1);
}

void AutoFilterTest::test_process_bandPass_shouldPassItsBandAtUnity()
{
    // The band-pass tap the filter is built on peaks at 1/k per stage, so its level would otherwise
    // ride on the resonance: 12 dB down at the bottom of the range and 28 dB up at the top.
    for (auto && slope : { 0, 1 }) {
        for (auto && resonance : { 0.0f, 0.3f, 0.9f }) {
            const auto effect = makeFilter(BandPass, slope, resonance, 1000.0);
            QVERIFY(std::abs(renderGainDb(*effect, 1000.0)) < 0.5);
        }
    }

    // Resonance narrows the band rather than turning it up, so a higher setting rejects more of
    // what sits outside it.
    const auto wide = makeFilter(BandPass, 1, 0.0f, 1000.0);
    const auto narrow = makeFilter(BandPass, 1, 0.9f, 1000.0);
    QVERIFY(renderGainDb(*narrow, 250.0) < renderGainDb(*wide, 250.0) - 10.0);
}

void AutoFilterTest::test_process_notch_shouldRejectItsBand()
{
    const auto effect = makeFilter(Notch, 1, 0.3f, 1000.0);

    QVERIFY(renderGainDb(*effect, 1000.0) < -40.0);
    // Both sides of the notch come back whole.
    QVERIFY(std::abs(renderGainDb(*effect, 100.0)) < 1.0);
    QVERIFY(std::abs(renderGainDb(*effect, 5000.0)) < 1.0);
}

void AutoFilterTest::test_cutoffLfo_zeroIntensity_shouldNotSweep()
{
    AutoFilter effect;
    effect.setSampleRate(testSampleRate);
    silenceModulation(effect);
    setParameter(effect, Constants::NahdXml::xmlKeyResonance(), 0.0f);
    setParameter(effect, Constants::NahdXml::xmlKeyCutoff(), cutoffForHz(1000.0));
    effect.sync();

    const auto envelope = renderRmsEnvelope(effect, 1000.0, 1.0);
    const auto [min, max] = std::minmax_element(envelope.begin() + 10, envelope.end());

    // Nothing is moving, so every window past the filter's settling reads the same.
    QVERIFY(*max - *min < 0.001);
}

void AutoFilterTest::test_cutoffLfo_fullIntensity_shouldSweepCutoff()
{
    AutoFilter effect;
    effect.setSampleRate(testSampleRate);
    silenceModulation(effect);
    setParameter(effect, Constants::NahdXml::xmlKeyResonance(), 0.0f);
    setParameter(effect, Constants::NahdXml::xmlKeyCutoff(), cutoffForHz(500.0));
    setParameter(effect, Constants::NahdXml::xmlKeyLfoWaveform(), discrete(static_cast<int>(Lfo::Waveform::Sine)));
    setParameter(effect, Constants::NahdXml::xmlKeyLfoRate(), rateForHz(2.0));
    setParameter(effect, Constants::NahdXml::xmlKeyLfoIntensity(), 1.0f);
    effect.sync();

    // Five octaves up from 500 Hz clears 8 kHz and five octaves down buries it.
    const auto envelope = renderRmsEnvelope(effect, 8000.0, 1.0);
    const auto [min, max] = std::minmax_element(envelope.begin(), envelope.end());

    QVERIFY(*max > *min * 10.0);
    QCOMPARE(countCycles(envelope), 2u);
}

void AutoFilterTest::test_cutoffLfo_negativeIntensity_shouldInvertSweep()
{
    const auto quarterCycleRms = [](float intensity) {
        AutoFilter effect;
        effect.setSampleRate(testSampleRate);
        silenceModulation(effect);
        setParameter(effect, Constants::NahdXml::xmlKeyResonance(), 0.0f);
        setParameter(effect, Constants::NahdXml::xmlKeyCutoff(), cutoffForHz(500.0));
        setParameter(effect, Constants::NahdXml::xmlKeyLfoWaveform(), discrete(static_cast<int>(Lfo::Waveform::Sine)));
        setParameter(effect, Constants::NahdXml::xmlKeyLfoRate(), rateForHz(1.0));
        setParameter(effect, Constants::NahdXml::xmlKeyLfoIntensity(), intensity);
        effect.sync();
        // A sine LFO starts at zero and rises, so a quarter of a cycle in it is at its peak: the
        // cutoff is as far up as a positive intensity takes it and as far down as a negative one
        // does.
        const auto envelope = renderRmsEnvelope(effect, 8000.0, 0.25);
        return envelope.back();
    };

    QVERIFY(quarterCycleRms(1.0f) > quarterCycleRms(0.0f) * 10.0);
}

void AutoFilterTest::test_cutoffLfo_bpmMode_shouldFollowTempo()
{
    const auto cycles = [](float bpm) {
        AutoFilter effect;
        effect.setSampleRate(testSampleRate);
        silenceModulation(effect);
        setParameter(effect, Constants::NahdXml::xmlKeyResonance(), 0.0f);
        setParameter(effect, Constants::NahdXml::xmlKeyCutoff(), cutoffForHz(500.0));
        setParameter(effect, Constants::NahdXml::xmlKeyLfoWaveform(), discrete(static_cast<int>(Lfo::Waveform::Sine)));
        setParameter(effect, Constants::NahdXml::xmlKeyLfoMode(), discrete(static_cast<int>(Lfo::Mode::BPM)));
        // A quarter-note division, which is one cycle per beat.
        setParameter(effect, Constants::NahdXml::xmlKeyLfoRate(), 0.25f);
        setParameter(effect, Constants::NahdXml::xmlKeyLfoIntensity(), 1.0f);
        effect.sync();
        effect.setBpm(bpm);
        return countCycles(renderRmsEnvelope(effect, 8000.0, 2.0));
    };

    // Two seconds hold two beats at 60 BPM and four at 120.
    QCOMPARE(cycles(60.0f), 2u);
    QCOMPARE(cycles(120.0f), 4u);
}

void AutoFilterTest::test_resonanceLfo_fullIntensity_shouldModulateResonance()
{
    AutoFilter effect;
    effect.setSampleRate(testSampleRate);
    silenceModulation(effect);
    setParameter(effect, Constants::NahdXml::xmlKeyResonance(), 0.0f);
    setParameter(effect, Constants::NahdXml::xmlKeyCutoff(), cutoffForHz(1000.0));
    setParameter(effect, Constants::NahdXml::xmlKeyLfo2Waveform(), discrete(static_cast<int>(Lfo::Waveform::Sine)));
    setParameter(effect, Constants::NahdXml::xmlKeyLfo2Rate(), rateForHz(2.0));
    setParameter(effect, Constants::NahdXml::xmlKeyLfo2Intensity(), 1.0f);
    effect.sync();

    // A tone sitting on the corner is the one the resonance lifts.
    const auto envelope = renderRmsEnvelope(effect, 1000.0, 1.0);
    const auto [min, max] = std::minmax_element(envelope.begin(), envelope.end());

    QVERIFY(*max > *min * 1.5);
}

void AutoFilterTest::test_resonanceLfo_fullModulation_shouldStayFinite()
{
    AutoFilter effect;
    effect.setSampleRate(testSampleRate);
    setParameter(effect, Constants::NahdXml::xmlKeyResonance(), 1.0f);
    setParameter(effect, Constants::NahdXml::xmlKeyLfoIntensity(), 1.0f);
    setParameter(effect, Constants::NahdXml::xmlKeyLfo2Intensity(), 1.0f);
    setParameter(effect, Constants::NahdXml::xmlKeyEnvMod(), 1.0f);
    effect.sync();

    // Everything at once, into a filter kept just short of self-oscillation.
    double peak = 0.0;
    for (size_t i = 0; i < static_cast<size_t>(testSampleRate * 2.0); i++) {
        double left = sineAt(1000.0, i);
        double right = left;
        effect.process(left, right);
        QVERIFY(std::isfinite(left));
        QVERIFY(std::isfinite(right));
        peak = std::max(peak, std::abs(left));
    }
    QVERIFY(peak < 100.0);
}

void AutoFilterTest::test_envelopeFollower_positiveAmount_shouldOpenFilter()
{
    const auto rmsWithEnvelopeAmount = [](float amount) {
        AutoFilter effect;
        effect.setSampleRate(testSampleRate);
        silenceModulation(effect);
        setParameter(effect, Constants::NahdXml::xmlKeyResonance(), 0.0f);
        setParameter(effect, Constants::NahdXml::xmlKeyCutoff(), cutoffForHz(200.0));
        setParameter(effect, Constants::NahdXml::xmlKeyEnvMod(), amount);
        effect.sync();
        return renderRms(effect, 4000.0);
    };

    // A full-scale input is a fully open follower, which takes the corner five octaves past 4 kHz.
    QVERIFY(rmsWithEnvelopeAmount(1.0f) > rmsWithEnvelopeAmount(noModulation) * 10.0);
}

void AutoFilterTest::test_envelopeFollower_negativeAmount_shouldCloseFilter()
{
    const auto rmsWithEnvelopeAmount = [](float amount) {
        AutoFilter effect;
        effect.setSampleRate(testSampleRate);
        silenceModulation(effect);
        setParameter(effect, Constants::NahdXml::xmlKeyResonance(), 0.0f);
        setParameter(effect, Constants::NahdXml::xmlKeyCutoff(), cutoffForHz(8000.0));
        setParameter(effect, Constants::NahdXml::xmlKeyEnvMod(), amount);
        effect.sync();
        return renderRms(effect, 1000.0);
    };

    QVERIFY(rmsWithEnvelopeAmount(0.0f) < rmsWithEnvelopeAmount(noModulation) * 0.1);
}

void AutoFilterTest::test_envelopeFollower_zeroAmount_shouldNotFollowLevel()
{
    const auto rmsAtAmplitude = [](double amplitude) {
        AutoFilter effect;
        effect.setSampleRate(testSampleRate);
        silenceModulation(effect);
        setParameter(effect, Constants::NahdXml::xmlKeyResonance(), 0.0f);
        setParameter(effect, Constants::NahdXml::xmlKeyCutoff(), cutoffForHz(1000.0));
        effect.sync();
        return renderRms(effect, 2000.0, 0.4, amplitude) / amplitude;
    };

    // Level in, level out: without the follower the filter does not care how loud the input is.
    QVERIFY(std::abs(rmsAtAmplitude(1.0) - rmsAtAmplitude(0.01)) < 0.001);
}

void AutoFilterTest::test_stereoPhase_halfCycle_shouldOffsetChannels()
{
    AutoFilter effect;
    effect.setSampleRate(testSampleRate);
    silenceModulation(effect);
    setParameter(effect, Constants::NahdXml::xmlKeyResonance(), 0.0f);
    setParameter(effect, Constants::NahdXml::xmlKeyCutoff(), cutoffForHz(500.0));
    setParameter(effect, Constants::NahdXml::xmlKeyLfoWaveform(), discrete(static_cast<int>(Lfo::Waveform::Sine)));
    setParameter(effect, Constants::NahdXml::xmlKeyLfoRate(), rateForHz(2.0));
    setParameter(effect, Constants::NahdXml::xmlKeyLfoIntensity(), 1.0f);
    setParameter(effect, Constants::NahdXml::xmlKeyStereoPhase(), 1.0f); // 180 degrees
    effect.sync();

    double maxDifference = 0.0;
    for (size_t i = 0; i < static_cast<size_t>(testSampleRate); i++) {
        double left = sineAt(8000.0, i);
        double right = left;
        effect.process(left, right);
        maxDifference = std::max(maxDifference, std::abs(left - right));
    }

    // The two channels sweep against each other, so at some point one passes what the other stops.
    QVERIFY(maxDifference > 0.5);
}

void AutoFilterTest::test_stereoPhase_zero_shouldKeepChannelsIdentical()
{
    AutoFilter effect;
    effect.setSampleRate(testSampleRate);
    setParameter(effect, Constants::NahdXml::xmlKeyLfoIntensity(), 1.0f);
    setParameter(effect, Constants::NahdXml::xmlKeyLfo2Intensity(), 1.0f);
    effect.sync();

    for (size_t i = 0; i < static_cast<size_t>(testSampleRate); i++) {
        double left = sineAt(8000.0, i);
        double right = left;
        effect.process(left, right);
        QCOMPARE(left, right);
    }
}

void AutoFilterTest::test_reset_shouldRestartDeterministically()
{
    AutoFilter effect;
    effect.setSampleRate(testSampleRate);
    setParameter(effect, Constants::NahdXml::xmlKeyLfoWaveform(), discrete(static_cast<int>(Lfo::Waveform::Random)));
    setParameter(effect, Constants::NahdXml::xmlKeyLfoIntensity(), 1.0f);
    setParameter(effect, Constants::NahdXml::xmlKeyStereoPhase(), 0.5f);
    setParameter(effect, Constants::NahdXml::xmlKeyEnvMod(), 1.0f);
    effect.sync();

    const auto render = [&effect] {
        std::vector<double> output;
        for (size_t i = 0; i < 8192; i++) {
            double left = sineAt(1000.0, i);
            double right = left;
            effect.process(left, right);
            output.push_back(left);
        }
        return output;
    };

    const auto first = render();
    effect.reset();
    const auto second = render();

    // An offline render has to come out as the same audio as playback did.
    QCOMPARE(first, second);
}

void AutoFilterTest::test_mix_zero_shouldPassThroughDry()
{
    AutoFilter effect;
    effect.setSampleRate(testSampleRate);
    setParameter(effect, Constants::NahdXml::xmlKeyMix(), 0.0f);
    effect.sync();

    for (size_t i = 0; i < 1000; i++) {
        const double dry = sineAt(8000.0, i);
        double left = dry;
        double right = dry;
        effect.process(left, right);
        QCOMPARE(left, dry);
        QCOMPARE(right, dry);
    }
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::AutoFilterTest)
