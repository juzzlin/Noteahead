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

#include "auto_ducker_test.hpp"

#include "../../common/constants.hpp"
#include "../../domain/dsp/audio_context.hpp"
#include "../../domain/effects/auto_ducker.hpp"

#include <QTest>

#include <cmath>
#include <span>
#include <vector>

namespace noteahead {

namespace {

constexpr uint32_t sampleRate = 48000;
constexpr uint32_t frameCount = 512; // ~10.7 ms per block

void setParameter(AutoDucker & effect, const QString & key, float value)
{
    if (auto p = effect.parameter(key.toStdString()); p) {
        p->get().setValue(value);
    }
    effect.sync();
}

void setThresholdDb(AutoDucker & effect, float thresholdDb)
{
    setParameter(effect, Constants::NahdXml::xmlKeyThreshold(), (thresholdDb + 60.0f) / 60.0f);
}

void setAmountDb(AutoDucker & effect, float amountDb)
{
    setParameter(effect, Constants::NahdXml::xmlKeyAmount(), (amountDb + 24.0f) / 48.0f);
}

void setKneeDb(AutoDucker & effect, float kneeDb)
{
    setParameter(effect, Constants::NahdXml::xmlKeyKnee(), kneeDb / 24.0f);
}

void setHoldMs(AutoDucker & effect, float holdMs)
{
    setParameter(effect, Constants::NahdXml::xmlKeyHold(), holdMs / 500.0f);
}

void setSideChainSource(AutoDucker & effect, int deviceIndex)
{
    setParameter(effect, Constants::NahdXml::xmlKeySideChainSourceDevice(), static_cast<float>(deviceIndex));
}

//! Fastest attack and release the ranges allow, so a couple of blocks are enough to settle.
void setFastEnvelope(AutoDucker & effect)
{
    setParameter(effect, Constants::NahdXml::xmlKeyAttack(), 0.0f);
    setParameter(effect, Constants::NahdXml::xmlKeyRelease(), 0.0f);
}

//! Runs blocks of DC through the effect and returns the gain the last block came out with. DC keeps
//! the peak detector at a known level without any envelope ripple to average out.
double processGain(AutoDucker & effect, double input, double sideChain, bool useSideChain, uint32_t blockCount)
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
        context.sampleRate = sampleRate;
        if (useSideChain) {
            context.deviceOutputBuffers = deviceBuffers;
        }

        effect.process(context);
        lastSample = buffer[(frameCount - 1) * 2];
    }

    return std::abs(lastSample / input);
}

} // namespace

void AutoDuckerTest::test_gain_belowThreshold_shouldStayUnity()
{
    AutoDucker effect;
    effect.setSampleRate(sampleRate);
    setFastEnvelope(effect);
    setThresholdDb(effect, -20.0f);
    setAmountDb(effect, -12.0f);
    setKneeDb(effect, 0.0f);

    // -40 dBFS never reaches the threshold, so nothing should move
    const auto gain = processGain(effect, 0.01, 0.0, false, 4);
    QVERIFY2(std::abs(gain - 1.0) < 0.001, qPrintable(QString { "Gain %1" }.arg(gain)));
    QCOMPARE(effect.gainDb(), 0.0f);
}

void AutoDuckerTest::test_gain_negativeAmount_shouldDuck()
{
    AutoDucker effect;
    effect.setSampleRate(sampleRate);
    setFastEnvelope(effect);
    setThresholdDb(effect, -20.0f);
    setAmountDb(effect, -12.0f);
    setKneeDb(effect, 0.0f);

    const auto gain = processGain(effect, 1.0, 0.0, false, 4);
    QVERIFY2(std::abs(gain - 0.2512) < 0.005, qPrintable(QString { "Gain %1" }.arg(gain)));
    QVERIFY2(std::abs(effect.gainDb() + 12.0f) < 0.1f, qPrintable(QString { "Meter %1 dB" }.arg(effect.gainDb())));
}

void AutoDuckerTest::test_gain_positiveAmount_shouldBoost()
{
    AutoDucker effect;
    effect.setSampleRate(sampleRate);
    setFastEnvelope(effect);
    setThresholdDb(effect, -20.0f);
    setAmountDb(effect, 6.0f);
    setKneeDb(effect, 0.0f);

    // The same wiring lifts instead of ducking when Amount is positive
    const auto gain = processGain(effect, 0.5, 0.0, false, 4);
    QVERIFY2(std::abs(gain - 1.9953) < 0.02, qPrintable(QString { "Gain %1" }.arg(gain)));
    QVERIFY2(std::abs(effect.gainDb() - 6.0f) < 0.1f, qPrintable(QString { "Meter %1 dB" }.arg(effect.gainDb())));
}

void AutoDuckerTest::test_gain_knee_shouldEngagePartially()
{
    AutoDucker effect;
    effect.setSampleRate(sampleRate);
    setFastEnvelope(effect);
    setThresholdDb(effect, -20.0f);
    setAmountDb(effect, -12.0f);
    setKneeDb(effect, 24.0f);

    // Sitting exactly at the threshold is the middle of the knee, so half of the amount applies
    const auto gain = processGain(effect, 0.1, 0.0, false, 4);
    QVERIFY2(std::abs(gain - 0.5012) < 0.005, qPrintable(QString { "Gain %1" }.arg(gain)));
}

void AutoDuckerTest::test_sideChain_loudSource_shouldDuckSilentInput()
{
    AutoDucker effect;
    effect.setSampleRate(sampleRate);
    setFastEnvelope(effect);
    setThresholdDb(effect, -20.0f);
    setAmountDb(effect, -12.0f);
    setKneeDb(effect, 0.0f);
    setSideChainSource(effect, 0);

    // The input itself is far below the threshold: only the side chain can open the ducker
    const auto gain = processGain(effect, 0.001, 1.0, true, 4);
    QVERIFY2(std::abs(gain - 0.2512) < 0.005, qPrintable(QString { "Gain %1" }.arg(gain)));
}

void AutoDuckerTest::test_sideChain_silentSource_shouldLeaveInputAlone()
{
    AutoDucker effect;
    effect.setSampleRate(sampleRate);
    setFastEnvelope(effect);
    setThresholdDb(effect, -20.0f);
    setAmountDb(effect, -12.0f);
    setKneeDb(effect, 0.0f);
    setSideChainSource(effect, 0);

    // A full-scale input must not duck itself once the detector listens elsewhere
    const auto gain = processGain(effect, 1.0, 0.0, true, 4);
    QVERIFY2(std::abs(gain - 1.0) < 0.001, qPrintable(QString { "Gain %1" }.arg(gain)));
}

void AutoDuckerTest::test_sideChainSourceDeviceIndex_unset_shouldBeEmpty()
{
    AutoDucker effect;
    QVERIFY(!effect.sidechainSourceDeviceIndex());

    setSideChainSource(effect, 2);
    const auto index = effect.sidechainSourceDeviceIndex();
    QVERIFY(index.has_value());
    QCOMPARE(*index, static_cast<size_t>(2));
}

void AutoDuckerTest::test_hold_afterSourceStops_shouldDelayRelease()
{
    const auto duckThenSilence = [](float holdMs) {
        AutoDucker effect;
        effect.setSampleRate(sampleRate);
        setFastEnvelope(effect);
        setThresholdDb(effect, -20.0f);
        setAmountDb(effect, -12.0f);
        setKneeDb(effect, 0.0f);
        setHoldMs(effect, holdMs);
        setSideChainSource(effect, 0);

        processGain(effect, 1.0, 1.0, true, 4);
        // One block is ~10.7 ms, well inside a 100 ms hold and well past a 1 ms release
        return processGain(effect, 1.0, 0.0, true, 1);
    };

    const auto withoutHold = duckThenSilence(0.0f);
    QVERIFY2(std::abs(withoutHold - 1.0) < 0.001, qPrintable(QString { "Gain %1 without hold" }.arg(withoutHold)));

    const auto withHold = duckThenSilence(100.0f);
    QVERIFY2(std::abs(withHold - 0.2512) < 0.005, qPrintable(QString { "Gain %1 with hold" }.arg(withHold)));
}

void AutoDuckerTest::test_reset_afterDucking_shouldReturnToUnity()
{
    AutoDucker effect;
    effect.setSampleRate(sampleRate);
    setFastEnvelope(effect);
    setThresholdDb(effect, -20.0f);
    setAmountDb(effect, -12.0f);
    setKneeDb(effect, 0.0f);

    processGain(effect, 1.0, 0.0, false, 4);
    QVERIFY(effect.gainDb() < -11.0f);

    effect.reset();
    QCOMPARE(effect.gainDb(), 0.0f);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::AutoDuckerTest)
