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

#include "vintage_passive_eq_test.hpp"

#include "../../common/constants.hpp"
#include "../../domain/effects/vintage_passive_eq.hpp"

#include <QTest>

#include <cmath>
#include <numbers>

namespace noteahead {

namespace {

constexpr double SampleRate = 44100.0;

// Frequency-selector indices, matching the tables in VintagePassiveEq.
constexpr int LowFreq20Hz = 0;
constexpr int LowFreq100Hz = 3;
constexpr int HighBoostFreq5kHz = 2;
constexpr int HighAttenFreq10kHz = 1;

void setParameter(VintagePassiveEq & effect, const QString & key, float value)
{
    if (auto p = effect.parameter(key.toStdString()); p) {
        p->get().update(value);
    }
    effect.sync();
}

VintagePassiveEq makeEq()
{
    VintagePassiveEq effect;
    effect.setSampleRate(SampleRate);
    return effect;
}

//! Steady-state gain in dB that the effect applies to a unit sine at the given frequency.
//!
//! The signal is fed to both channels; we discard an initial transient window and measure the
//! output RMS over an equal window, comparing it against the known input RMS (unit-amplitude sine).
double measureGainDb(VintagePassiveEq & effect, double frequency)
{
    effect.reset();

    const auto warmupSamples = static_cast<int>(SampleRate * 0.5);
    const auto measureSamples = static_cast<int>(SampleRate * 0.5);
    const double omega = 2.0 * std::numbers::pi * frequency / SampleRate;

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

} // namespace

void VintagePassiveEqTest::test_neutral_allZero_shouldPassThrough()
{
    auto effect = makeEq();
    // Freshly constructed: every boost/atten is at zero, so all stages bypass.
    for (double input = -0.8; input <= 0.8; input += 0.2) {
        double l = input;
        double r = -input;
        effect.process(l, r);
        QVERIFY(qFuzzyCompare(l + 1.0, input + 1.0));
        QVERIFY(qFuzzyCompare(r + 1.0, -input + 1.0));
    }
}

void VintagePassiveEqTest::test_lowBoost_engaged_shouldAmplifyLowFrequency()
{
    auto effect = makeEq();
    setParameter(effect, Constants::NahdXml::xmlKeyLowFreq(), static_cast<float>(LowFreq100Hz));
    setParameter(effect, Constants::NahdXml::xmlKeyLowBoost(), 1.0f);

    // 50 Hz sits well below the 100 Hz shelf corner, so it rides the boost plateau.
    QVERIFY(measureGainDb(effect, 50.0) > 6.0);
}

void VintagePassiveEqTest::test_lowAtten_engaged_shouldAttenuateLowFrequency()
{
    auto effect = makeEq();
    setParameter(effect, Constants::NahdXml::xmlKeyLowFreq(), static_cast<float>(LowFreq100Hz));
    setParameter(effect, Constants::NahdXml::xmlKeyLowAtten(), 1.0f);

    // The attenuation shelf sits above the boost corner, but at 50 Hz it still cuts substantially.
    QVERIFY(measureGainDb(effect, 50.0) < -6.0);
}

void VintagePassiveEqTest::test_highBoost_engaged_shouldAmplifyCenterFrequency()
{
    auto effect = makeEq();
    setParameter(effect, Constants::NahdXml::xmlKeyHighBoostFreq(), static_cast<float>(HighBoostFreq5kHz));
    setParameter(effect, Constants::NahdXml::xmlKeyHighBoost(), 1.0f);

    // The bell peaks at its selected 5 kHz center.
    QVERIFY(measureGainDb(effect, 5000.0) > 6.0);
}

void VintagePassiveEqTest::test_highAtten_engaged_shouldAttenuateHighFrequency()
{
    auto effect = makeEq();
    setParameter(effect, Constants::NahdXml::xmlKeyHighAttenFreq(), static_cast<float>(HighAttenFreq10kHz));
    setParameter(effect, Constants::NahdXml::xmlKeyHighAtten(), 1.0f);

    // 15 kHz is above the 10 kHz shelf corner, so it rides the attenuation plateau.
    QVERIFY(measureGainDb(effect, 15000.0) < -6.0);
}

void VintagePassiveEqTest::test_lowFrequencySelector_higherCorner_shouldBoostWiderRange()
{
    auto lowCorner = makeEq();
    setParameter(lowCorner, Constants::NahdXml::xmlKeyLowFreq(), static_cast<float>(LowFreq20Hz));
    setParameter(lowCorner, Constants::NahdXml::xmlKeyLowBoost(), 1.0f);

    auto highCorner = makeEq();
    setParameter(highCorner, Constants::NahdXml::xmlKeyLowFreq(), static_cast<float>(LowFreq100Hz));
    setParameter(highCorner, Constants::NahdXml::xmlKeyLowBoost(), 1.0f);

    // At 60 Hz the 100 Hz corner is still on its boost plateau, while the 20 Hz corner has rolled off.
    const double gainLowCorner = measureGainDb(lowCorner, 60.0);
    const double gainHighCorner = measureGainDb(highCorner, 60.0);
    QVERIFY(gainHighCorner > gainLowCorner + 1.0);
}

void VintagePassiveEqTest::test_bandwidth_broader_shouldBoostWiderSkirt()
{
    auto broad = makeEq();
    setParameter(broad, Constants::NahdXml::xmlKeyHighBoostFreq(), static_cast<float>(HighBoostFreq5kHz));
    setParameter(broad, Constants::NahdXml::xmlKeyHighBoost(), 1.0f);
    setParameter(broad, Constants::NahdXml::xmlKeyBandwidth(), 0.0f); // Lowest Q -> broadest bell

    auto narrow = makeEq();
    setParameter(narrow, Constants::NahdXml::xmlKeyHighBoostFreq(), static_cast<float>(HighBoostFreq5kHz));
    setParameter(narrow, Constants::NahdXml::xmlKeyHighBoost(), 1.0f);
    setParameter(narrow, Constants::NahdXml::xmlKeyBandwidth(), 1.0f); // Highest Q -> narrowest bell

    // Away from the 5 kHz center, a broad bell still lifts the skirt where a narrow one has fallen off.
    const double gainBroad = measureGainDb(broad, 2500.0);
    const double gainNarrow = measureGainDb(narrow, 2500.0);
    QVERIFY(gainBroad > gainNarrow + 1.0);
}

void VintagePassiveEqTest::test_lowSection_boostAndAtten_shouldInteract()
{
    auto boostOnly = makeEq();
    setParameter(boostOnly, Constants::NahdXml::xmlKeyLowFreq(), static_cast<float>(LowFreq100Hz));
    setParameter(boostOnly, Constants::NahdXml::xmlKeyLowBoost(), 1.0f);

    auto boostAndAtten = makeEq();
    setParameter(boostAndAtten, Constants::NahdXml::xmlKeyLowFreq(), static_cast<float>(LowFreq100Hz));
    setParameter(boostAndAtten, Constants::NahdXml::xmlKeyLowBoost(), 1.0f);
    setParameter(boostAndAtten, Constants::NahdXml::xmlKeyLowAtten(), 1.0f);

    // The passive trick: engaging the attenuator alongside the boost pulls the deep bass back down,
    // so the combined response at 30 Hz is markedly lower than boost alone.
    const double gainBoostOnly = measureGainDb(boostOnly, 30.0);
    const double gainCombined = measureGainDb(boostAndAtten, 30.0);
    QVERIFY(gainCombined < gainBoostOnly - 3.0);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::VintagePassiveEqTest)
