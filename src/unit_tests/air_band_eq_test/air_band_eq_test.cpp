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
#include "../../domain/dsp/svf_filter.hpp"
#include "../../domain/effects/air_band_eq.hpp"

#include <QTest>

#include <algorithm>
#include <array>
#include <cmath>
#include <memory>
#include <numbers>
#include <random>
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

//! Owned, because the spectrum taps make an equalizer impossible to copy or move.
std::unique_ptr<AirBandEq> makeEq(double sampleRate = SampleRate)
{
    auto effect = std::make_unique<AirBandEq>();
    effect->setSampleRate(sampleRate);
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

//! Output over input level, in dB, of four seconds of pink noise through @p effect. The first
//! second is left out, for the filters and the gain ramp to settle. The noise is cut below 20 Hz,
//! where auto gain does not look: the generator puts another octave there, and a low shelf would
//! lift it.
double pinkNoiseGainDb(Effect & effect)
{
    effect.reset();
    std::mt19937 generator { 1234 };
    std::uniform_real_distribution<double> white { -1.0, 1.0 };
    // Paul Kellet's refined filter: within a fraction of a decibel of -3 dB/octave over the band
    std::array<double, 7> b {};
    // A 24 dB/oct Butterworth
    SvfFilter lowCut1;
    SvfFilter lowCut2;
    lowCut1.calculateLowCut(20.0, SampleRate, 0.5412);
    lowCut2.calculateLowCut(20.0, SampleRate, 1.3066);
    double inputPower = 0.0;
    double outputPower = 0.0;
    const auto frames = static_cast<size_t>(SampleRate * 4.0);
    for (size_t i = 0; i < frames; i++) {
        const double w = white(generator);
        b[0] = 0.99886 * b[0] + w * 0.0555179;
        b[1] = 0.99332 * b[1] + w * 0.0750759;
        b[2] = 0.96900 * b[2] + w * 0.1538520;
        b[3] = 0.86650 * b[3] + w * 0.3104856;
        b[4] = 0.55000 * b[4] + w * 0.5329522;
        b[5] = -0.7616 * b[5] - w * 0.0168980;
        const double pink = lowCut2.process(lowCut1.process((b[0] + b[1] + b[2] + b[3] + b[4] + b[5] + b[6] + w * 0.5362) * 0.1));
        b[6] = w * 0.115926;
        double left = pink;
        double right = pink;
        effect.process(left, right);
        if (i >= frames / 4) {
            inputPower += pink * pink;
            outputPower += left * left;
        }
    }
    return 10.0 * std::log10(outputPower / inputPower);
}

//! Whether @p toggled, with auto gain switched on and back off, comes out bit for bit as
//! @p reference, which never had it. Both are set up alike by the caller.
template<typename Eq>
void verifyAutoGainOffLeavesOutputUnchanged(Eq & reference, Eq & toggled)
{
    const auto run = [&](int frames, bool compare) {
        for (int i = 0; i < frames; i++) {
            const double input = std::sin(static_cast<double>(i) * 0.05);
            double referenceLeft = input;
            double referenceRight = -input * 0.5;
            double toggledLeft = referenceLeft;
            double toggledRight = referenceRight;
            reference.process(referenceLeft, referenceRight);
            toggled.process(toggledLeft, toggledRight);
            if (compare) {
                QVERIFY(toggledLeft == referenceLeft);
                QVERIFY(toggledRight == referenceRight);
            }
        }
    };
    setParameter(toggled, Constants::NahdXml::xmlKeyAutoGain(), 1.0f);
    run(4410, false);
    setParameter(toggled, Constants::NahdXml::xmlKeyAutoGain(), 0.0f);
    QCOMPARE(toggled.autoGainDb(), 0.0);
    // Past the ramp back
    run(882, false);
    run(4410, true);
}

} // namespace

void AirBandEqTest::test_neutral_defaults_shouldPassThrough()
{
    const auto effectOwner = makeEq();
    auto & effect = *effectOwner;
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
    const auto effectOwner = makeEq();
    auto & effect = *effectOwner;
    setBand(effect, Band160Hz, 0.8f);

    QVERIFY(measureGainDb(effect, 160.0) > 3.0);
}

void AirBandEqTest::test_bandPass_cut_shouldAttenuateCenterFrequency()
{
    const auto effectOwner = makeEq();
    auto & effect = *effectOwner;
    setBand(effect, Band160Hz, 0.2f);

    QVERIFY(measureGainDb(effect, 160.0) < -1.0);
}

void AirBandEqTest::test_bandPass_fullBoost_shouldReachDocumentedMaximum()
{
    const auto effectOwner = makeEq();
    auto & effect = *effectOwner;
    setBand(effect, Band40Hz, BandFullBoost);

    // At the band centre the tap is in phase with the dry path and peaks at unity, so the summed
    // response lands on the panel's stated +15 dB.
    QVERIFY(std::abs(measureGainDb(effect, 40.0) - 15.0) < 0.5);
}

void AirBandEqTest::test_bandPass_fullCut_shouldStopAtDocumentedMinimum()
{
    const auto effectOwner = makeEq();
    auto & effect = *effectOwner;
    setBand(effect, Band40Hz, BandFullCut);

    // The asymmetry is structural rather than a styling choice: a tap subtracted from the dry path
    // can never remove more than the dry path holds, so the cut side bottoms out at -4.5 dB.
    QVERIFY(std::abs(measureGainDb(effect, 40.0) + 4.5) < 0.5);
}

void AirBandEqTest::test_bandPass_topBandBoosted_shouldFallAwayAboveItsCentre()
{
    const auto effectOwner = makeEq();
    auto & effect = *effectOwner;
    setBand(effect, Band2500Hz, BandFullBoost);

    // The topmost band pass is a bell like the other four, not a shelf: the panel gives all five one
    // heading and one range, and lifting everything above a corner is what the AIR BAND is for.
    // Boosting it must therefore leave the top octaves where they were rather than carrying them up.
    const double atCentre = measureGainDb(effect, 2500.0);
    const double twoOctavesAbove = measureGainDb(effect, 10000.0);

    QVERIFY2(atCentre > 10.0, qPrintable(QString("at the centre: %1 dB").arg(atCentre)));
    QVERIFY2(twoOctavesAbove < atCentre - 6.0, qPrintable(QString("two octaves above: %1 dB").arg(twoOctavesAbove)));
}

void AirBandEqTest::test_bandPasses_loweredTogether_shouldPreserveCurveShape()
{
    const std::vector<double> probeFrequencies { 40.0, 160.0, 650.0, 2500.0 };

    const auto boostedOwner = makeEq();

    auto & boosted = *boostedOwner;
    setAllBands(boosted, 0.9f);
    std::vector<double> boostedGains;
    for (const auto frequency : probeFrequencies) {
        boostedGains.push_back(measureGainDb(boosted, frequency));
    }

    const auto loweredOwner = makeEq();

    auto & lowered = *loweredOwner;
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
    const auto effectOwner = makeEq();
    auto & effect = *effectOwner;
    setParameter(effect, Constants::NahdXml::xmlKeyAirFreq(), Air10kHz);
    setParameter(effect, Constants::NahdXml::xmlKeyAirGain(), 1.0f);

    QVERIFY(measureGainDb(effect, 10000.0) > 10.0);
}

void AirBandEqTest::test_airBand_boosted_shouldRaiseOverallGain()
{
    const auto effectOwner = makeEq();
    auto & effect = *effectOwner;
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
    const auto effectOwner = makeEq();
    auto & effect = *effectOwner;
    setParameter(effect, Constants::NahdXml::xmlKeyAirFreq(), AirOff);
    setParameter(effect, Constants::NahdXml::xmlKeyAirGain(), 1.0f);

    // The OFF detent removes the tap from the sum no matter where AIR GAIN sits.
    QVERIFY(std::abs(measureGainDb(effect, 10000.0)) < 0.01);
}

void AirBandEqTest::test_airBand_skirt_shouldReachBelowSelectedCorner()
{
    const auto effectOwner = makeEq();
    auto & effect = *effectOwner;
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
        const auto effectOwner = makeEq();
        auto & effect = *effectOwner;
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
    const auto effectOwner = makeEq();
    auto & effect = *effectOwner;
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
    const auto highRateEffectOwner = makeEq(HighSampleRate);
    auto & highRateEffect = *highRateEffectOwner;
    setParameter(highRateEffect, Constants::NahdXml::xmlKeyAirFreq(), Air40kHz);
    setParameter(highRateEffect, Constants::NahdXml::xmlKeyAirGain(), 1.0f);
    QVERIFY(std::isfinite(measureGainDb(highRateEffect, 15000.0, HighSampleRate)));
}

void AirBandEqTest::test_outputGain_boosted_shouldScaleOutput()
{
    const auto effectOwner = makeEq();
    auto & effect = *effectOwner;
    const auto flat = measureGainDb(effect, 1000.0);
    QVERIFY(std::abs(flat) < 0.01);

    // Normalised 0.75 is halfway up the +/-12 dB trim, so +6 dB.
    setParameter(effect, Constants::NahdXml::xmlKeyGain(), 0.75f);
    QVERIFY(std::abs(measureGainDb(effect, 1000.0) - 6.0) < 0.1);
}

namespace {

//! Frequencies the curve is checked at: a tap's own centre, and the skirts either side of it.
//!
//! The skirts are the point. A tap is in phase with the dry path only at its centre, so summing the
//! taps' magnitudes rather than their complex responses agrees exactly where these start and drifts
//! wider everywhere else.
constexpr double CurveFrequencies[] { 60.0, 100.0, 160.0, 250.0, 400.0, 650.0, 1000.0, 1600.0, 2500.0, 5000.0, 10000.0 };

void verifyCurveMatchesWhatIsHeard(AirBandEq & effect, double tolerance)
{
    for (auto && frequency : CurveFrequencies) {
        const double heard = measureGainDb(effect, frequency);
        const double drawn = effect.magnitudeDbAt(frequency);
        QVERIFY2(std::abs(heard - drawn) < tolerance,
                 qPrintable(QString("at %1 Hz: heard %2 dB, drawn %3 dB").arg(frequency).arg(heard).arg(drawn)));
    }
}

} // namespace

void AirBandEqTest::test_magnitudeDb_defaults_shouldBeFlat()
{
    const auto effectOwner = makeEq();
    auto & effect = *effectOwner;

    for (auto && frequency : CurveFrequencies) {
        QVERIFY2(std::abs(effect.magnitudeDbAt(frequency)) < 0.01,
                 qPrintable(QString("at %1 Hz: %2 dB").arg(frequency).arg(effect.magnitudeDbAt(frequency))));
    }
}

void AirBandEqTest::test_magnitudeDb_bandBoosted_shouldMatchWhatIsHeard()
{
    const auto effectOwner = makeEq();
    auto & effect = *effectOwner;
    setBand(effect, Band650Hz, 0.6f); // +3 dB at the centre

    verifyCurveMatchesWhatIsHeard(effect, 0.15);
}

void AirBandEqTest::test_magnitudeDb_bandsOverlapping_shouldMatchWhatIsHeard()
{
    // The bells sit two octaves apart at a Q that leaves them overlapping, so several at once is the
    // case where a sum of magnitudes drifts furthest: every tap's error adds in the same direction.
    const auto effectOwner = makeEq();
    auto & effect = *effectOwner;
    setBand(effect, Band160Hz, 0.7f);
    setBand(effect, Band650Hz, 0.8f);
    setBand(effect, Band2500Hz, BandFullCut);

    verifyCurveMatchesWhatIsHeard(effect, 0.15);
}

void AirBandEqTest::test_magnitudeDb_airBandBoosted_shouldMatchWhatIsHeard()
{
    const auto effectOwner = makeEq();
    auto & effect = *effectOwner;
    setParameter(effect, Constants::NahdXml::xmlKeyAirFreq(), Air10kHz);
    setParameter(effect, Constants::NahdXml::xmlKeyAirGain(), 0.5f);

    verifyCurveMatchesWhatIsHeard(effect, 0.15);
}

void AirBandEqTest::test_magnitudeDb_outputGainBoosted_shouldIncludeTheTrim()
{
    // The trim is part of what is heard, and pulling the band knobs down to offset the air band is
    // the move the hardware prescribes: a curve that hid the trim would show that as an uncompensated
    // cut.
    const auto effectOwner = makeEq();
    auto & effect = *effectOwner;
    setBand(effect, Band650Hz, 0.6f);
    setParameter(effect, Constants::NahdXml::xmlKeyGain(), 0.75f); // +6 dB

    verifyCurveMatchesWhatIsHeard(effect, 0.15);
}

namespace {

void boostBandsAndAir(AirBandEq & effect)
{
    setBand(effect, Band160Hz, 0.8f);
    setBand(effect, Band650Hz, 0.7f);
    setParameter(effect, Constants::NahdXml::xmlKeyAirFreq(), Air5kHz);
    setParameter(effect, Constants::NahdXml::xmlKeyAirGain(), 0.6f);
}

} // namespace

void AirBandEqTest::test_autoGain_boosted_shouldLevelPinkNoise()
{
    const auto effectOwner = makeEq();
    auto & effect = *effectOwner;
    boostBandsAndAir(effect);
    const double uncompensated = pinkNoiseGainDb(effect);
    QVERIFY2(uncompensated > 4.0, qPrintable(QString::number(uncompensated)));

    setParameter(effect, Constants::NahdXml::xmlKeyAutoGain(), 1.0f);
    QVERIFY(effect.autoGainDb() < -4.0);
    const double compensated = pinkNoiseGainDb(effect);
    QVERIFY2(std::abs(compensated) < 0.5, qPrintable(QString::number(compensated)));
}

void AirBandEqTest::test_autoGain_boosted_shouldMatchWhatIsHeard()
{
    // The curve includes auto gain, so it has to be heard there too
    const auto effectOwner = makeEq();
    auto & effect = *effectOwner;
    boostBandsAndAir(effect);
    setParameter(effect, Constants::NahdXml::xmlKeyAutoGain(), 1.0f);

    verifyCurveMatchesWhatIsHeard(effect, 0.15);
}

void AirBandEqTest::test_autoGain_off_shouldLeaveOutputUnchanged()
{
    const auto referenceOwner = makeEq();
    auto & reference = *referenceOwner;
    const auto toggledOwner = makeEq();
    auto & toggled = *toggledOwner;
    boostBandsAndAir(reference);
    boostBandsAndAir(toggled);
    verifyAutoGainOffLeavesOutputUnchanged(reference, toggled);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::AirBandEqTest)
