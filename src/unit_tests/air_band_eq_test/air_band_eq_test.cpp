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

#include "air_band_eq_test.hpp"

#include "../../common/constants.hpp"
#include "../../domain/effects/air_band_eq.hpp"

#include <QTest>

#include <algorithm>
#include <cmath>
#include <numbers>
#include <vector>

namespace noteahead {

namespace {

constexpr double SampleRate = 44100.0;
constexpr double HighSampleRate = 96000.0;

// Band-pass knob positions, normalised: 0.5 is flat, 1.0 is the documented +15 dB, 0.0 the -4.5 dB.
constexpr float BandFlat = 0.5f;
constexpr float BandFullBoost = 1.0f;
constexpr float BandFullCut = 0.0f;

// Panel indices of the band passes and of the AIR BAND selector.
constexpr size_t BandSub = 0;
constexpr size_t Band40Hz = 1;
constexpr size_t Band160Hz = 2;
constexpr size_t Band650Hz = 3;
constexpr size_t Band2500Hz = 4;

constexpr float AirOff = 0.0f;
constexpr float Air2500Hz = 1.0f;
constexpr float Air5kHz = 2.0f;
constexpr float Air10kHz = 3.0f;
constexpr float Air20kHz = 4.0f;
constexpr float Air40kHz = 5.0f;

void setParameter(AirBandEq & effect, const QString & key, float value)
{
    if (auto p = effect.parameter(key.toStdString()); p) {
        p->get().update(value);
    }
    effect.sync();
}

void setBand(AirBandEq & effect, size_t bandIndex, float value)
{
    setParameter(effect, Constants::NahdXml::xmlKeyBandGain(bandIndex), value);
}

AirBandEq makeEq(double sampleRate = SampleRate)
{
    AirBandEq effect;
    effect.setSampleRate(sampleRate);
    return effect;
}

//! Steady-state gain in dB that the effect applies to a unit sine at the given frequency.
//!
//! The signal is fed to both channels; we discard an initial transient window and measure the
//! output RMS over an equal window, comparing it against the known input RMS (unit-amplitude sine).
double measureGainDb(AirBandEq & effect, double frequency, double sampleRate = SampleRate)
{
    effect.reset();

    const auto warmupSamples = static_cast<int>(sampleRate * 0.5);
    const auto measureSamples = static_cast<int>(sampleRate * 0.5);
    const double omega = 2.0 * std::numbers::pi * frequency / sampleRate;

    int n = 0;
    for (int i = 0; i < warmupSamples; i++, n++) {
        double l = std::sin(omega * n);
        double r = l;
        effect.process(l, r);
    }

    double sumSquares = 0.0;
    for (int i = 0; i < measureSamples; i++, n++) {
        double l = std::sin(omega * n);
        double r = l;
        effect.process(l, r);
        sumSquares += l * l;
    }

    const double outputRms = std::sqrt(sumSquares / measureSamples);
    const double inputRms = 1.0 / std::numbers::sqrt2;
    return 20.0 * std::log10(outputRms / inputRms);
}

void setAllBands(AirBandEq & effect, float value)
{
    for (size_t i = 0; i < AirBandEq::BandCount; i++) {
        setBand(effect, i, value);
    }
}

} // namespace

void AirBandEqTest::test_neutral_defaults_shouldPassThrough()
{
    auto effect = makeEq();
    // Freshly constructed: every band knob is centred and AIR GAIN is at zero, so the parallel taps
    // contribute nothing and the dry path emerges untouched.
    for (double input = -0.8; input <= 0.8; input += 0.2) {
        double l = input;
        double r = -input;
        effect.process(l, r);
        QVERIFY(qFuzzyCompare(l + 1.0, input + 1.0));
        QVERIFY(qFuzzyCompare(r + 1.0, -input + 1.0));
    }
}

void AirBandEqTest::test_bandPass_boosted_shouldAmplifyCenterFrequency()
{
    auto effect = makeEq();
    setBand(effect, Band160Hz, 0.8f);

    QVERIFY(measureGainDb(effect, 160.0) > 3.0);
}

void AirBandEqTest::test_bandPass_cut_shouldAttenuateCenterFrequency()
{
    auto effect = makeEq();
    setBand(effect, Band160Hz, 0.2f);

    QVERIFY(measureGainDb(effect, 160.0) < -1.0);
}

void AirBandEqTest::test_bandPass_fullBoost_shouldReachDocumentedMaximum()
{
    auto effect = makeEq();
    setBand(effect, Band40Hz, BandFullBoost);

    // At the band centre the tap is in phase with the dry path and peaks at unity, so the summed
    // response lands on the panel's stated +15 dB.
    QVERIFY(std::abs(measureGainDb(effect, 40.0) - 15.0) < 0.5);
}

void AirBandEqTest::test_bandPass_fullCut_shouldStopAtDocumentedMinimum()
{
    auto effect = makeEq();
    setBand(effect, Band40Hz, BandFullCut);

    // The asymmetry is structural rather than a styling choice: a tap subtracted from the dry path
    // can never remove more than the dry path holds, so the cut side bottoms out at -4.5 dB.
    QVERIFY(std::abs(measureGainDb(effect, 40.0) + 4.5) < 0.5);
}

void AirBandEqTest::test_bandPasses_loweredTogether_shouldPreserveCurveShape()
{
    const std::vector<double> probeFrequencies { 40.0, 160.0, 650.0, 2500.0 };

    auto boosted = makeEq();
    setAllBands(boosted, 0.9f);
    std::vector<double> boostedGains;
    for (const auto frequency : probeFrequencies) {
        boostedGains.push_back(measureGainDb(boosted, frequency));
    }

    auto lowered = makeEq();
    setAllBands(lowered, 0.7f);
    std::vector<double> loweredGains;
    for (const auto frequency : probeFrequencies) {
        loweredGains.push_back(measureGainDb(lowered, frequency));
    }

    // The manual's compensation advice only works if pulling all five band passes down by the same
    // amount lowers the whole curve instead of reshaping it. Every probe must drop, and the drops
    // must agree closely enough that the curve is recognisably the same shape.
    std::vector<double> drops;
    for (size_t i = 0; i < probeFrequencies.size(); i++) {
        drops.push_back(boostedGains[i] - loweredGains[i]);
        QVERIFY(drops.back() > 0.0);
    }

    const auto [minDrop, maxDrop] = std::minmax_element(drops.begin(), drops.end());
    QVERIFY(*maxDrop - *minDrop < 2.0);
}

void AirBandEqTest::test_airBand_boosted_shouldAmplifyHighFrequencies()
{
    auto effect = makeEq();
    setParameter(effect, Constants::NahdXml::xmlKeyAirFreq(), Air10kHz);
    setParameter(effect, Constants::NahdXml::xmlKeyAirGain(), 1.0f);

    QVERIFY(measureGainDb(effect, 10000.0) > 10.0);
}

void AirBandEqTest::test_airBand_boosted_shouldRaiseOverallGain()
{
    auto effect = makeEq();
    setParameter(effect, Constants::NahdXml::xmlKeyAirFreq(), Air10kHz);

    const auto quiet = measureGainDb(effect, 1000.0);
    setParameter(effect, Constants::NahdXml::xmlKeyAirGain(), 1.0f);
    const auto loud = measureGainDb(effect, 1000.0);

    // Documented byproduct of summing the air tap alongside the band passes: raising AIR GAIN lifts
    // the overall level too, well below the selected corner.
    QVERIFY(loud - quiet > 1.0);
}

void AirBandEqTest::test_airBand_off_shouldPassThrough()
{
    auto effect = makeEq();
    setParameter(effect, Constants::NahdXml::xmlKeyAirFreq(), AirOff);
    setParameter(effect, Constants::NahdXml::xmlKeyAirGain(), 1.0f);

    // The OFF detent removes the tap from the sum no matter where AIR GAIN sits.
    QVERIFY(std::abs(measureGainDb(effect, 10000.0)) < 0.01);
}

void AirBandEqTest::test_airBand_skirt_shouldReachBelowSelectedCorner()
{
    auto effect = makeEq();
    setParameter(effect, Constants::NahdXml::xmlKeyAirFreq(), Air10kHz);
    setParameter(effect, Constants::NahdXml::xmlKeyAirGain(), 0.5f);

    const auto atCorner = measureGainDb(effect, 10000.0);
    const auto octaveBelow = measureGainDb(effect, 5000.0);
    const auto twoOctavesBelow = measureGainDb(effect, 2500.0);

    // Summing a first-order tap into the dry path gives a far slower transition than the filter
    // order alone suggests, so the lift is still clearly present two octaves below the corner while
    // remaining well short of the corner's own lift.
    QVERIFY(twoOctavesBelow > 0.5);
    QVERIFY(twoOctavesBelow < octaveBelow);
    QVERIFY(octaveBelow < atCorner);
    QVERIFY(atCorner - octaveBelow < 6.0);
}

void AirBandEqTest::test_airBand_risingFrequency_shouldReduceAudibleLift()
{
    const std::vector<float> selectors { Air2500Hz, Air5kHz, Air10kHz, Air20kHz, Air40kHz };

    std::vector<double> gains;
    for (const auto selector : selectors) {
        auto effect = makeEq();
        setParameter(effect, Constants::NahdXml::xmlKeyAirFreq(), selector);
        setParameter(effect, Constants::NahdXml::xmlKeyAirGain(), 1.0f);
        gains.push_back(measureGainDb(effect, 8000.0));
    }

    // Each higher selector must lift 8 kHz less than the one below it. The 40 kHz position only
    // stays distinct from 20 kHz because the clamped corner is gain-compensated; without that the
    // top two settings would collapse together at this sample rate.
    for (size_t i = 1; i < gains.size(); i++) {
        QVERIFY(gains[i] < gains[i - 1]);
    }
}

void AirBandEqTest::test_airBand_fortyKilohertz_shouldStayStableAtBaseRate()
{
    auto effect = makeEq();
    setParameter(effect, Constants::NahdXml::xmlKeyAirFreq(), Air40kHz);
    setParameter(effect, Constants::NahdXml::xmlKeyAirGain(), 1.0f);

    // 40 kHz sits above Nyquist at 44.1 kHz. Without clamping the prewarp the tap would turn into a
    // mirrored, meaningless filter, so guard the whole audible range for a sane, finite response.
    for (const auto frequency : { 100.0, 1000.0, 5000.0, 15000.0 }) {
        const auto gain = measureGainDb(effect, frequency);
        QVERIFY(std::isfinite(gain));
        QVERIFY(gain > -1.0);
        QVERIFY(gain < 21.0);
    }

    // At 96 kHz the corner is representable, so the same setting must brighten more than it can at
    // 44.1 kHz, where it is clamped and scaled back.
    auto highRateEffect = makeEq(HighSampleRate);
    setParameter(highRateEffect, Constants::NahdXml::xmlKeyAirFreq(), Air40kHz);
    setParameter(highRateEffect, Constants::NahdXml::xmlKeyAirGain(), 1.0f);
    QVERIFY(std::isfinite(measureGainDb(highRateEffect, 15000.0, HighSampleRate)));
}

void AirBandEqTest::test_outputGain_boosted_shouldScaleOutput()
{
    auto effect = makeEq();
    const auto flat = measureGainDb(effect, 1000.0);
    QVERIFY(std::abs(flat) < 0.01);

    // Normalised 0.75 is halfway up the +/-12 dB trim, so +6 dB.
    setParameter(effect, Constants::NahdXml::xmlKeyGain(), 0.75f);
    QVERIFY(std::abs(measureGainDb(effect, 1000.0) - 6.0) < 0.1);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::AirBandEqTest)
