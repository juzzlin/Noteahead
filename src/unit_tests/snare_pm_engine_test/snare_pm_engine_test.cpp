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

#include "snare_pm_engine_test.hpp"
#include "domain/dsp/cascaded_svf.hpp"
#include "domain/dsp/drum/snare_pm_engine.hpp"

#include <QString>
#include <QTest>
#include <cmath>
#include <vector>

namespace noteahead {

namespace {

constexpr double TestSampleRate = 44100.0;

std::vector<double> renderMono(SnarePmEngine & engine, int samples)
{
    std::vector<double> out(static_cast<size_t>(samples));
    engine.trigger(1.0f);
    for (auto & sample : out) {
        float left = 0.0f;
        float right = 0.0f;
        engine.nextSample(left, right);
        sample = (static_cast<double>(left) + static_cast<double>(right)) * 0.5;
    }
    return out;
}

double seconds(double s)
{
    return s * TestSampleRate;
}

//! Energy from @p fromSeconds to @p toSeconds in the band from @p lowHz to @p highHz, in dB
double bandEnergyDb(const std::vector<double> & signal, double lowHz, double highHz, double fromSeconds, double toSeconds)
{
    CascadedSvf highPass {};
    highPass.setMode(CascadedSvf::Mode::HighPass);
    CascadedSvf lowPass {};
    lowPass.setMode(CascadedSvf::Mode::LowPass);
    const auto cutoff = [](double hz) { return std::log2(hz / 20.0) / std::log2(20000.0 / 20.0); };
    for (auto * filter : { &highPass, &lowPass }) {
        filter->setSampleRate(TestSampleRate);
        filter->setOrder(4);
        filter->setResonance(0.29);
    }
    highPass.setCutoff(cutoff(lowHz));
    lowPass.setCutoff(cutoff(highHz));
    double energy = 1.0e-20;
    for (size_t i = 0; i < signal.size(); i++) {
        const double filtered = lowPass.process(highPass.process(signal[i]));
        if (i >= static_cast<size_t>(seconds(fromSeconds)) && i < static_cast<size_t>(seconds(toSeconds))) {
            energy += filtered * filtered;
        }
    }
    return 10.0 * std::log10(energy);
}

SnarePmEngine makeEngine()
{
    SnarePmEngine engine {};
    engine.setSampleRate(TestSampleRate);
    engine.setPanSpread(0.0f);
    return engine;
}

} // namespace

void SnarePmEngineTest::test_trigger_shouldActivateEngine()
{
    SnarePmEngine engine {};
    engine.setSampleRate(44100.0);
    QCOMPARE(engine.isActive(), false);

    engine.trigger(0.8f);
    QCOMPARE(engine.isActive(), true);

    bool renderedNonZero = false;
    for (int i = 0; i < 44100 * 2; i++) {
        const float sample = engine.nextSample();
        if (std::abs(sample) > 0.001f) {
            renderedNonZero = true;
        }
        if (!engine.isActive()) {
            break;
        }
    }

    QVERIFY(renderedNonZero);
    QCOMPARE(engine.isActive(), false);
}

void SnarePmEngineTest::test_trigger_twice_shouldRenderDeterministically()
{
    SnarePmEngine engine {};
    engine.setSampleRate(44100.0);

    engine.reset();
    engine.trigger(1.0f);
    std::vector<float> runA(2048);
    for (int i = 0; i < 2048; i++) {
        runA[i] = engine.nextSample();
    }

    engine.reset();
    engine.trigger(1.0f);
    std::vector<float> runB(2048);
    for (int i = 0; i < 2048; i++) {
        runB[i] = engine.nextSample();
    }

    for (int i = 0; i < 2048; i++) {
        QCOMPARE(runA[i], runB[i]);
    }
}

void SnarePmEngineTest::test_panSpread_zero_shouldBeStrictlyMono()
{
    SnarePmEngine engine {};
    engine.setSampleRate(44100.0);
    engine.setPanSpread(0.0f);
    engine.setSnappy(0.8f);
    engine.trigger(1.0f);

    for (int i = 0; i < 4096; i++) {
        float left = 0.0f;
        float right = 0.0f;
        engine.nextSample(left, right);
        QCOMPARE(left, right);
    }
}

void SnarePmEngineTest::test_panSpread_full_shouldCreateStereoDifference()
{
    SnarePmEngine engine {};
    engine.setSampleRate(44100.0);
    engine.setPanSpread(1.0f);
    engine.setSnappy(0.8f);
    engine.trigger(1.0f);

    float maxDiff = 0.0f;
    for (int i = 0; i < 4096; i++) {
        float left = 0.0f;
        float right = 0.0f;
        engine.nextSample(left, right);
        const float diff = std::abs(left - right);
        if (diff > maxDiff) {
            maxDiff = diff;
        }
    }

    QVERIFY(maxDiff > 0.001f);
}

void SnarePmEngineTest::test_snappy_zero_shouldBePureMembrane()
{
    SnarePmEngine engine {};
    engine.setSampleRate(44100.0);
    engine.setPanSpread(1.0f);
    engine.setSnappy(0.0f);
    engine.trigger(1.0f);

    for (int i = 0; i < 4096; i++) {
        float left = 0.0f;
        float right = 0.0f;
        engine.nextSample(left, right);
        QCOMPARE(left, right);
    }
}

void SnarePmEngineTest::test_snareBuzz_shouldIncreaseSustainedWireRattleEnergy()
{
    SnarePmEngine engineZeroBuzz {};
    engineZeroBuzz.setSampleRate(44100.0);
    engineZeroBuzz.setSnappy(0.8f);
    engineZeroBuzz.setSnareBuzz(0.0f);
    engineZeroBuzz.setSnareDecay(0.1f);
    engineZeroBuzz.trigger(1.0f);

    for (int i = 0; i < 4410; i++) {
        engineZeroBuzz.nextSample();
    }
    double tailEnergyZero = 0.0;
    for (int i = 0; i < 8820; i++) {
        const float s = engineZeroBuzz.nextSample();
        tailEnergyZero += s * s;
    }

    SnarePmEngine engineFullBuzz {};
    engineFullBuzz.setSampleRate(44100.0);
    engineFullBuzz.setSnappy(0.8f);
    engineFullBuzz.setSnareBuzz(1.0f);
    engineFullBuzz.setSnareDecay(0.1f);
    engineFullBuzz.trigger(1.0f);

    for (int i = 0; i < 4410; i++) {
        engineFullBuzz.nextSample();
    }
    double tailEnergyFull = 0.0;
    for (int i = 0; i < 8820; i++) {
        const float s = engineFullBuzz.nextSample();
        tailEnergyFull += s * s;
    }

    QVERIFY(tailEnergyFull > tailEnergyZero * 1.5);
}

void SnarePmEngineTest::test_position_changesModalResponse()
{
    SnarePmEngine engineCenter {};
    engineCenter.setSampleRate(44100.0);
    engineCenter.setPosition(0.0f);
    engineCenter.setSnappy(0.0f);
    engineCenter.trigger(1.0f);

    SnarePmEngine engineRim {};
    engineRim.setSampleRate(44100.0);
    engineRim.setPosition(0.9f);
    engineRim.setSnappy(0.0f);
    engineRim.trigger(1.0f);

    float maxDiff = 0.0f;
    for (int i = 0; i < 2048; i++) {
        const float diff = std::abs(engineCenter.nextSample() - engineRim.nextSample());
        if (diff > maxDiff) {
            maxDiff = diff;
        }
    }

    QVERIFY(maxDiff > 0.05f);
}

void SnarePmEngineTest::test_click_changesInitialAttackEnergy()
{
    SnarePmEngine engineZeroClick {};
    engineZeroClick.setSampleRate(44100.0);
    engineZeroClick.setSnappy(0.0f);
    engineZeroClick.setClick(0.0f);
    engineZeroClick.trigger(1.0f);

    SnarePmEngine engineFullClick {};
    engineFullClick.setSampleRate(44100.0);
    engineFullClick.setSnappy(0.0f);
    engineFullClick.setClick(1.0f);
    engineFullClick.trigger(1.0f);

    float maxDiff = 0.0f;
    for (int i = 0; i < 256; i++) {
        const float diff = std::abs(engineFullClick.nextSample() - engineZeroClick.nextSample());
        if (diff > maxDiff) {
            maxDiff = diff;
        }
    }

    QVERIFY(maxDiff > 0.2f);
}

void SnarePmEngineTest::test_hardness_changesModalBrightnessAndAttack()
{
    SnarePmEngine engineSoft {};
    engineSoft.setSampleRate(44100.0);
    engineSoft.setSnappy(0.0f);
    engineSoft.setHardness(0.0f);
    engineSoft.trigger(1.0f);

    SnarePmEngine engineHard {};
    engineHard.setSampleRate(44100.0);
    engineHard.setSnappy(0.0f);
    engineHard.setHardness(1.0f);
    engineHard.trigger(1.0f);

    float maxDiff = 0.0f;
    for (int i = 0; i < 2048; i++) {
        const float diff = std::abs(engineSoft.nextSample() - engineHard.nextSample());
        if (diff > maxDiff) {
            maxDiff = diff;
        }
    }

    QVERIFY(maxDiff > 0.05f);
}

void SnarePmEngineTest::test_body_changesFundamentalWeightAndResonance()
{
    SnarePmEngine engineLean {};
    engineLean.setSampleRate(44100.0);
    engineLean.setSnappy(0.0f);
    engineLean.setBody(0.0f);
    engineLean.trigger(1.0f);

    SnarePmEngine engineFull {};
    engineFull.setSampleRate(44100.0);
    engineFull.setSnappy(0.0f);
    engineFull.setBody(1.0f);
    engineFull.trigger(1.0f);

    float maxDiff = 0.0f;
    float leanTailEnergy = 0.0f;
    float fullTailEnergy = 0.0f;
    for (int i = 0; i < 2048; i++) {
        const float sLean = engineLean.nextSample();
        const float sFull = engineFull.nextSample();
        const float diff = std::abs(sFull - sLean);
        if (diff > maxDiff) {
            maxDiff = diff;
        }
        if (i >= 512) {
            leanTailEnergy += sLean * sLean;
            fullTailEnergy += sFull * sFull;
        }
    }

    QVERIFY(maxDiff > 0.1f);
    QVERIFY(fullTailEnergy > leanTailEnergy * 1.5f);
}

void SnarePmEngineTest::test_stop_shouldDeactivateAndSilenceEngine()
{
    SnarePmEngine engine {};
    engine.setSampleRate(44100.0);
    engine.trigger(1.0f);
    QCOMPARE(engine.isActive(), true);

    engine.stop();
    QCOMPARE(engine.isActive(), false);
    QCOMPARE(engine.nextSample(), 0.0f);
}

void SnarePmEngineTest::test_sampleRateStability()
{
    const std::vector<double> rates { 44100.0, 48000.0, 88200.0, 96000.0, 192000.0 };
    for (double sr : rates) {
        SnarePmEngine engine {};
        engine.setSampleRate(sr);
        engine.trigger(1.0f);

        for (int i = 0; i < 1000; i++) {
            float left = 0.0f;
            float right = 0.0f;
            engine.nextSample(left, right);
            QVERIFY(!std::isnan(left));
            QVERIFY(!std::isnan(right));
            QVERIFY(!std::isinf(left));
            QVERIFY(!std::isinf(right));
        }
    }
}

void SnarePmEngineTest::test_crack_shouldFillTheMidsOfTheHit()
{
    auto plain = makeEngine();
    plain.setSnappy(0.0f);
    plain.setClick(0.0f);
    auto cracked = makeEngine();
    cracked.setSnappy(0.0f);
    cracked.setClick(0.0f);
    cracked.setCrack(1.0f);

    const auto plainHit = renderMono(plain, static_cast<int>(seconds(0.1)));
    const auto crackedHit = renderMono(cracked, static_cast<int>(seconds(0.1)));

    // The hole above the head's modes and below the wires that made a hard hit sound thin
    const double crackedMids = bandEnergyDb(crackedHit, 900.0, 2000.0, 0.0, 0.04);
    const double plainMids = bandEnergyDb(plainHit, 900.0, 2000.0, 0.0, 0.04);
    QVERIFY2(crackedMids > plainMids + 3.0, qPrintable(QString::number(crackedMids - plainMids)));
    // And it is gone again by 100 ms: a crack, not a ring
    QVERIFY(bandEnergyDb(crackedHit, 900.0, 2000.0, 0.08, 0.1) < bandEnergyDb(crackedHit, 900.0, 2000.0, 0.0, 0.02) - 20.0);
}

void SnarePmEngineTest::test_wireWidth_shouldReachDownIntoTheMids()
{
    auto narrow = makeEngine();
    narrow.setSnappy(1.0f);
    auto wide = makeEngine();
    wide.setSnappy(1.0f);
    wide.setWireWidth(1.0f);

    const auto narrowHit = renderMono(narrow, static_cast<int>(seconds(0.3)));
    const auto wideHit = renderMono(wide, static_cast<int>(seconds(0.3)));

    QVERIFY(bandEnergyDb(wideHit, 500.0, 1200.0, 0.05, 0.2) > bandEnergyDb(narrowHit, 500.0, 1200.0, 0.05, 0.2) + 6.0);
}

void SnarePmEngineTest::test_punch_shouldFrontLoadTheHeads()
{
    auto plain = makeEngine();
    plain.setSnappy(0.0f);
    auto punched = makeEngine();
    punched.setSnappy(0.0f);
    punched.setPunch(1.0f);

    const auto plainHit = renderMono(plain, static_cast<int>(seconds(0.3)));
    const auto punchedHit = renderMono(punched, static_cast<int>(seconds(0.3)));

    const auto drop = [](const std::vector<double> & hit) {
        return bandEnergyDb(hit, 80.0, 400.0, 0.0, 0.02) - bandEnergyDb(hit, 80.0, 400.0, 0.08, 0.2);
    };
    QVERIFY(drop(punchedHit) > drop(plainHit) + 6.0);
    // The ring after it is the drum's own
    QVERIFY(std::abs(bandEnergyDb(punchedHit, 80.0, 400.0, 0.15, 0.3) - bandEnergyDb(plainHit, 80.0, 400.0, 0.15, 0.3)) < 1.0);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::SnarePmEngineTest)
