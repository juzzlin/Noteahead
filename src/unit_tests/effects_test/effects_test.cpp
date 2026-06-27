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

#include "effects_test.hpp"

#include "../../common/constants.hpp"
#include "../../common/utils.hpp"
#include "../../domain/dsp/cascaded_svf.hpp"
#include "../../domain/dsp/chorus_effect.hpp"
#include "../../domain/dsp/clipper_effect.hpp"
#include "../../domain/dsp/compressor_effect.hpp"
#include "../../domain/dsp/eq_8_band_parametric_effect.hpp"
#include "../../domain/dsp/reverb_effect.hpp"
#include "../../domain/effects/delay_effect.hpp"
#include "../../domain/effects/high_pass_filter_effect.hpp"
#include "../../domain/effects/low_pass_filter_effect.hpp"
#include "../../domain/effects/panning_effect.hpp"
#include "../../domain/effects/volume_effect.hpp"

#include <QTest>

#include <cmath>
#include <numbers>

namespace noteahead {

void EffectsTest::test_volumeEffect_shouldApplyGainToSignal()
{
    VolumeEffect effect;
    double left = 1.0;
    double right = 1.0;
    effect.setVolume(0.5f);
    effect.process(left, right);

    QCOMPARE(left, 0.5f);
    QCOMPARE(right, 0.5f);

    effect.setVolume(0.0f);
    left = 1.0f;
    right = 1.0f;
    effect.process(left, right);
    QCOMPARE(left, 0.0f);
    QCOMPARE(right, 0.0f);
}

void EffectsTest::test_panningEffect_shouldDistributeSignalToChannels()
{
    PanningEffect effect;

    // Center: constant-power pan gives cos(π/4) on both channels
    {
        double left = 1.0;
        double right = 1.0;
        effect.setPan(0.5f);
        effect.process(left, right);
        const double angle = static_cast<double>(0.5f) * std::numbers::pi * 0.5;
        QCOMPARE(left, std::cos(angle));
        QCOMPARE(right, std::sin(angle));
    }

    // Full Left
    {
        double left = 1.0;
        double right = 1.0;
        effect.setPan(0.0f);
        effect.process(left, right);
        QCOMPARE(left, 1.0f);
        QCOMPARE(right, 0.0f);
    }

    // Full Right
    {
        double left = 1.0;
        double right = 1.0;
        effect.setPan(1.0f);
        effect.process(left, right);
        QCOMPARE(left, 0.0f);
        QCOMPARE(right, 1.0f);
    }
}

void EffectsTest::test_lowPassFilterEffect_shouldProcessAudioStablely()
{
    LowPassFilterEffect effect;
    double left = 1.0;
    double right = 1.0;
    // Cutoff 1.0 (bypass)
    effect.setCutoff(1.0f);
    effect.process(left, right);
    QCOMPARE(left, 1.0f);
    QCOMPARE(right, 1.0f);

    // Filter processing stability
    effect.setCutoff(0.5f);
    for (int i = 0; i < 100; ++i) {
        left = 1.0f;
        right = 1.0f;
        effect.process(left, right);
        QVERIFY(!std::isnan(left));
        QVERIFY(!std::isnan(right));
    }
}

void EffectsTest::test_highPassFilterEffect_shouldProcessAudioStablely()
{
    HighPassFilterEffect effect;

    // Cutoff 0.0 (bypass)
    {
        double left = 1.0;
        double right = 1.0;
        effect.setCutoff(0.0f);
        effect.process(left, right);
        QCOMPARE(left, 1.0f);
        QCOMPARE(right, 1.0f);
    }

    // Stability and NaN check
    effect.setCutoff(0.5f);
    for (int i = 0; i < 100; ++i) {
        double left = 1.0;
        double right = 1.0;
        effect.process(left, right);
        QVERIFY(!std::isnan(left));
        QVERIFY(!std::isnan(right));
    }

    // Extreme cutoff stability
    effect.setCutoff(0.99f);
    for (int i = 0; i < 100; ++i) {
        double left = 1.0;
        double right = 1.0;
        effect.process(left, right);
        QVERIFY(!std::isnan(left));
        QVERIFY(!std::isnan(right));
    }
}

void EffectsTest::test_reverb_mix_shouldApplyEffectBasedOnMixLevel()
{
    ReverbEffect reverb;
    reverb.setSampleRate(44100);
    reverb.setMix(0.0f);
    reverb.setLpfCutoff(1.0f);
    reverb.setHpfCutoff(0.0f);
    reverb.sync();

    double l = 1.0;
    double r = 1.0;

    // Process many samples to ensure any internal state is active
    // Reverb tail needs some samples to build up
    for (int i = 0; i < 5000; i++) {
        double tl = 1.0;
        double tr = 1.0;
        reverb.process(tl, tr);
    }

    reverb.process(l, r);

    // With mix 0, output should be exactly equal to input
    QCOMPARE(l, 1.0f);
    QCOMPARE(r, 1.0f);

    reverb.setMix(1.0f);
    reverb.sync();

    // Reverb tail needs some samples to build up
    for (int i = 0; i < 5000; i++) {
        double tl = 1.0;
        double tr = 1.0;
        reverb.process(tl, tr);
    }

    l = 1.0;
    r = 1.0;
    reverb.process(l, r);

    // With mix 1.0, output should be different from input
    QVERIFY(l != 1.0f || r != 1.0f);

    // With additive mix 1.0 and DC input, at least one decorrelated wet channel should add energy.
    QVERIFY(std::abs(l) > 1.0f || std::abs(r) > 1.0f);
}

void EffectsTest::test_reverb_filters_shouldShapeWetSignal()
{
    auto measureDcWetEnergy = [](float hpfCutoff) {
        ReverbEffect reverb;
        reverb.setSampleRate(44100);
        reverb.setMix(1.0f);
        reverb.setSize(0.6f);
        reverb.setDecay(0.5f);
        reverb.setDamping(0.2f);
        reverb.setPreDelay(0.0f);
        reverb.setLpfCutoff(1.0f);
        reverb.setHpfCutoff(hpfCutoff);
        reverb.sync();

        double energy = 0.0;
        for (int i = 0; i < 12000; ++i) {
            double l = 1.0;
            double r = 1.0;
            reverb.process(l, r);
            if (i > 6000) {
                energy += std::abs(l - 1.0f) + std::abs(r - 1.0f);
            }
        }
        return energy;
    };

    const double openEnergy = measureDcWetEnergy(0.0f);
    const double highPassedEnergy = measureDcWetEnergy(0.8f);

    QVERIFY(openEnergy > 1.0);
    QVERIFY(highPassedEnergy < openEnergy * 0.25);
}

void EffectsTest::test_delayEffect_shouldProcessSignalAndHandleSampleRateChanges()
{
    DelayEffect effect;
    effect.setSampleRate(44100.0);
    effect.setBpm(120.0);
    effect.setSync(true);
    effect.setSyncDivision(0.25f); // 1/4 note

    // 120 BPM, 1/4 note = 0.5 seconds.
    // At 44100 Hz, 0.5 seconds = 22050 samples.

    // We can't easily check internal state, but we can verify it doesn't crash
    // and produces audio if we feed it something.
    double left = 1.0;
    double right = 1.0;
    effect.process(left, right);
    QVERIFY(!std::isnan(left));
    QVERIFY(!std::isnan(right));

    // Test sample rate change
    effect.setSampleRate(48000.0);
    left = 1.0f;
    right = 1.0f;
    effect.process(left, right);
    QCOMPARE(effect.sampleRate(), 48000.0);
}

void EffectsTest::test_delayEffect_shouldProduceDelayedSignal()
{
    DelayEffect effect;
    const float sampleRate = 44100.0f;
    effect.setSampleRate(sampleRate);
    effect.setMix(1.0f); // 100% wet
    effect.setFeedback(0.0f); // No feedback for simplicity
    effect.setTime(0.1f); // 100ms delay = 4410 samples
    effect.setSync(false);

    // Initial output should be silence (buffer is empty)
    double left = 1.0;
    double right = 1.0;
    effect.process(left, right);
    QCOMPARE(left, 0.0f);
    QCOMPARE(right, 0.0f);

    // Process enough samples to reach the delay time
    const int delaySamples = static_cast<int>(0.1f * sampleRate);
    for (int i = 0; i + 1 < delaySamples; i++) {
        double l = 0.0;
        double r = 0.0;
        effect.process(l, r);
    }

    // Now with feedback 0.0, the output should still be silence
    left = 0.0;
    right = 0.0;
    effect.process(left, right);
    QCOMPARE(left, 0.0f);
    QCOMPARE(right, 0.0f);

    // Now re-feed with feedback 1.0 to test delayed signal
    effect.reset();
    effect.setFeedback(1.0f);
    left = 1.0f;
    right = 1.0f;
    effect.process(left, right);

    for (int i = 0; i + 1 < delaySamples; i++) {
        double l = 0.0;
        double r = 0.0;
        effect.process(l, r);
    }

    left = 0.0;
    right = 0.0;
    effect.process(left, right);
    QVERIFY(std::abs(left - 1.0f) < 1.0e-3f);
    QVERIFY(std::abs(right - 1.0f) < 1.0e-3f);

    // Test synced delay
    effect.reset();
    effect.setSync(true);
    effect.setFeedback(1.0f);
    effect.setBpm(120.0f);
    effect.setSyncDivision(0.25f); // 120 BPM, 1/4 note = 0.5s = 22050 samples
    const int syncDelaySamples = static_cast<int>(0.5f * sampleRate);

    left = 1.0f;
    right = 1.0f;
    effect.process(left, right);

    for (int i = 0; i + 1 < syncDelaySamples; i++) {
        double l = 0.0;
        double r = 0.0;
        effect.process(l, r);
    }

    left = 0.0;
    right = 0.0;
    effect.process(left, right);
    QVERIFY(std::abs(left - 1.0f) < 1.0e-3f);
    QVERIFY(std::abs(right - 1.0f) < 1.0e-3f);
}

void EffectsTest::test_delayEffect_shouldMaintainFeedbackLoop()
{
    DelayEffect effect;
    const float sampleRate = 44100.0f;
    effect.setSampleRate(sampleRate);
    effect.setMix(1.0f); // 100% wet
    effect.setFeedback(0.5f); // 50% feedback
    effect.setTime(0.1f); // 100ms delay = 4410 samples
    effect.setSync(false);

    const int delaySamples = static_cast<int>(0.1f * sampleRate);

    // Feed a pulse of 1.0
    double left = 1.0;
    double right = 1.0;
    effect.process(left, right);
    // Output should be silence (mix is wet, buffer empty)
    QCOMPARE(left, 0.0f);

    // Wait for 1st echo
    for (int i = 0; i + 1 < delaySamples; i++) {
        double l = 0.0;
        double r = 0.0;
        effect.process(l, r);
    }
    left = 0.0;
    right = 0.0;
    effect.process(left, right);
    // 1st echo should be 0.5 (1.0 * feedback)
    QVERIFY(std::abs(left - 0.5f) < 1.0e-3f);

    // Wait for 2nd echo
    for (int i = 0; i + 1 < delaySamples; i++) {
        double l = 0.0;
        double r = 0.0;
        effect.process(l, r);
    }
    left = 0.0;
    right = 0.0;
    effect.process(left, right);
    // 2nd echo should be 0.25 (0.5 * feedback)
    QVERIFY(std::abs(left - 0.25f) < 1.0e-3f);

    // Wait for 3rd echo
    for (int i = 0; i + 1 < delaySamples; i++) {
        double l = 0.0;
        double r = 0.0;
        effect.process(l, r);
    }
    left = 0.0;
    right = 0.0;
    effect.process(left, right);
    // 3rd echo should be 0.125 (0.5 * 0.5 * 0.5)
    QVERIFY(std::abs(left - 0.125f) < 1.0e-3f);
}

void EffectsTest::test_delayEffect_shouldMaintainStereoFeedback()
{
    DelayEffect effect;
    const float sampleRate = 44100.0f;
    effect.setSampleRate(sampleRate);
    effect.setType(DelayEffect::Type::Stereo);
    effect.setMix(1.0f); // 100% wet
    effect.setFeedback(0.9f); // 90% feedback
    effect.setTime(0.1f); // 100ms delay = 4410 samples
    effect.setSync(false);

    const int delaySamples = static_cast<int>(0.1f * sampleRate);

    // Feed a pulse of 1.0 to LEFT channel only
    double left = 1.0;
    double right = 0.0;
    effect.process(left, right);

    int echoes = 0;

    // Process 1 second (10 echoes expected)
    for (int i = 0; i < 1 * 44100; i++) {
        double l = 0.0;
        double r = 0.0;
        effect.process(l, r);

        if ((i + 1) % delaySamples == 0) {
            if (l > 0.001f) {
                echoes++;
            }
            // In stereo mode, right channel should remain silent if only left was pulsed
            QCOMPARE(r, 0.0f);
        }
    }

    QCOMPARE(echoes, 10);
}

void EffectsTest::test_delayEffect_shouldProduceDecayingSeriesOfEchoes()
{
    DelayEffect effect;
    const float sampleRate = 44100.0f;
    effect.setSampleRate(sampleRate);
    effect.setMix(1.0f); // 100% wet
    effect.setFeedback(0.9f); // 90% feedback
    effect.setTime(0.1f); // 100ms delay = 4410 samples
    effect.setSync(false);

    const int delaySamples = static_cast<int>(0.1f * sampleRate);

    // Feed a pulse of 1.0
    double left = 1.0;
    double right = 1.0;
    effect.process(left, right);

    int echoes = 0;
    float lastEchoVal = 1.1f;

    // Process 2 seconds
    for (int i = 0; i < 2 * 44100; i++) {
        double l = 0.0;
        double r = 0.0;
        effect.process(l, r);

        // If we see a pulse, count it and verify it's decaying
        if (l > 0.001f) {
            // Pulse should be around delaySamples multiples
            if ((i + 1) % delaySamples == 0) {
                echoes++;
                QVERIFY(l < lastEchoVal); // Decay check
                lastEchoVal = l;
            }
        }
    }

    // We should have seen 20 echoes
    QCOMPARE(echoes, 20);
    // Echo 1 was 1.0, Echo 2 was 0.9, ..., Echo 20 was 0.9^19
    QVERIFY(std::abs(lastEchoVal - std::pow(0.9f, 19.0f)) < 0.05f);
}

void EffectsTest::test_delayEffect_shouldProcessMonoMode()
{
    DelayEffect effect;
    const float sampleRate = 44100.0f;
    effect.setSampleRate(sampleRate);
    effect.setType(DelayEffect::Type::Mono);
    effect.setMix(1.0f);
    effect.setFeedback(1.0f);
    effect.setTime(0.1f);

    const int delaySamples = static_cast<int>(0.1f * sampleRate);

    // Feed a pulse to LEFT channel only
    double left = 1.0;
    double right = 0.0;
    effect.process(left, right);

    // Wait for 1st echo
    for (int i = 0; i + 1 < delaySamples; i++) {
        double l = 0.0;
        double r = 0.0;
        effect.process(l, r);
    }

    left = 0.0;
    right = 0.0;
    effect.process(left, right);

    // In Mono mode, the left-only input should be summed and distributed to both channels
    // (1.0 + 0.0) * 0.5 = 0.5 expected on both channels
    QVERIFY(std::abs(left - 0.5f) < 1.0e-3f);
    QVERIFY(std::abs(right - 0.5f) < 1.0e-3f);
}

void EffectsTest::test_delayEffect_shouldProcessPingPongMode()
{
    DelayEffect effect;
    const float sampleRate = 44100.0f;
    effect.setSampleRate(sampleRate);
    effect.setType(DelayEffect::Type::PingPong);
    effect.setMix(1.0f);
    effect.setFeedback(1.0f);
    effect.setDepth(1.0f); // Max width
    effect.setTime(0.1f);

    const int delaySamples = static_cast<int>(0.1f * sampleRate);

    // Feed a pulse to LEFT channel only
    double left = 1.0;
    double right = 0.0;
    effect.process(left, right);

    // Wait for 1st echo
    for (int i = 0; i + 1 < delaySamples; i++) {
        double l = 0.0;
        double r = 0.0;
        effect.process(l, r);
    }

    left = 0.0;
    right = 0.0;
    effect.process(left, right);

    // Ping-Pong: Left input should first appear on RIGHT channel?
    // Let's check implementation:
    // inL = inputL + inputR * (1.0 - m_depth) = 1.0 + 0.0 = 1.0
    // inR = inputR * (1.0 - m_depth) = 0.0
    // m_bufferL = inL + fbR * m_feedback = 1.0
    // m_bufferR = inR + fbL * m_feedback = 0.0
    // 1st Read (Stereo-like read): outL = bufL, outR = bufR
    // So 1st echo should be Left=1.0, Right=0.0
    QVERIFY(std::abs(left - 1.0f) < 1.0e-3f);
    QVERIFY(std::abs(right - 0.0f) < 1.0e-3f);

    // 2nd echo should bounce: fbL=1.0, fbR=0.0
    // next bufferL = inL + fbR = 0.0 + 0.0 = 0.0
    // next bufferR = inR + fbL = 0.0 + 1.0 = 1.0
    for (int i = 0; i + 1 < delaySamples; i++) {
        double l = 0.0;
        double r = 0.0;
        effect.process(l, r);
    }

    left = 0.0;
    right = 0.0;
    effect.process(left, right);

    QVERIFY(std::abs(left - 0.0f) < 1.0e-3f);
    QVERIFY(std::abs(right - 1.0f) < 1.0e-3f);
}

void EffectsTest::test_delayEffect_shouldProcessTapeMode()
{
    DelayEffect effect;
    const float sampleRate = 44100.0f;
    effect.setSampleRate(sampleRate);
    effect.setType(DelayEffect::Type::Tape);
    effect.setMix(1.0f); // 100% wet
    effect.setFeedback(1.0f);
    effect.setDepth(1.0f); // High saturation
    effect.setTime(0.1f);

    const int delaySamples = static_cast<int>(0.1f * sampleRate);

    // Feed a large signal pulse for 10 samples
    const float pulseVal = 2.0f;
    for (int i = 0; i < 10; i++) {
        double left = pulseVal;
        double right = pulseVal;
        effect.process(left, right);
    }

    bool foundEcho = false;
    // We expect the echo around delaySamples. Let's check a window.
    for (int i = 0; i < delaySamples + 100; i++) {
        double left = 0.0;
        double right = 0.0;
        effect.process(left, right);

        if (left > 0.01f) {
            foundEcho = true;
            if (left >= pulseVal) {
                qDebug() << "Tape mode failed: left =" << left << "pulseVal =" << pulseVal << "i =" << i;
            }
            // In Tape mode, output should be saturated (less than pulseVal)
            QVERIFY(left < pulseVal);
        }
    }

    QVERIFY(foundEcho);
}

void EffectsTest::test_delayEffect_shouldSyncParameters()
{
    DelayEffect effect;
    effect.setSampleRate(44100.0);

    // Initial check (defaults)
    QCOMPARE(effect.feedbackLpf(), 1.0);
    QCOMPARE(effect.feedbackHpf(), 0.0);

    // Test Discrete Type
    if (const auto p = effect.parameter(Constants::NahdXml::xmlKeyDelayType().toStdString()); p) {
        p->get().setFromXml(2); // PingPong
        effect.sync();
    }

    // Test Continuous Time
    if (const auto p = effect.parameter(Constants::NahdXml::xmlKeyDelayTime().toStdString()); p) {
        p->get().setValue(0.123f); // 1.23 seconds because of * 10.0 scaling
        effect.sync();
    }

    // Test Feedback LPF/HPF which have getters
    if (const auto p = effect.parameter(Constants::NahdXml::xmlKeyDelayFeedbackLpf().toStdString()); p) {
        p->get().setValue(0.456f);
        effect.sync();
        QCOMPARE(effect.feedbackLpf(), 0.456f);
    }

    if (const auto p = effect.parameter(Constants::NahdXml::xmlKeyDelayFeedbackHpf().toStdString()); p) {
        p->get().setValue(0.789f);
        effect.sync();
        QCOMPARE(effect.feedbackHpf(), 0.789f);
    }
}

void EffectsTest::test_compressorEffect_shouldReduceGainAndHandleLookahead()
{
    CompressorEffect effect;
    effect.setSampleRate(44100.0);

    // Default: Threshold -20dB, Ratio 4:1, Attack 10ms, Release 100ms, Makeup 0dB, Lookahead 0ms

    // Test bypass (signal below threshold)
    {
        // -30dB signal
        double val = Utils::Dsp::dbToLinear(-30.0);
        double left = val;
        double right = val;
        effect.process(left, right);
        // Should be exactly same if below threshold and no lookahead
        QCOMPARE(left, val);
        QCOMPARE(right, val);
        QCOMPARE(effect.reductionDb(), 0.0f);
    }

    // Test compression (signal above threshold)
    {
        effect.reset();
        // 0dB signal. Threshold is -20dB. Ratio is 4:1.
        // Overshoot is 20dB. Target reduction is 20 * (1 - 1/4) = 15dB.
        double val = 1.0; // 0dB
        double left = val;
        double right = val;

        // Process long enough for attack to settle
        for (int i = 0; i < 5000; i++) {
            double tl = val;
            double tr = val;
            effect.process(tl, tr);
        }

        QVERIFY(effect.reductionDb() < -14.0f);
        QVERIFY(effect.reductionDb() > -16.0f);

        left = val;
        right = val;
        effect.process(left, right);
        QVERIFY(left < Utils::Dsp::dbToLinear(-14.0f));
    }

    // Test lookahead
    {
        effect.reset();
        // Set 10ms lookahead
        if (auto p = effect.parameter(Constants::NahdXml::xmlKeyLookahead().toStdString()); p) {
            p->get().setValue(1.0f); // 100% = 10ms
            effect.sync();
        }

        double left = 1.0;
        double right = 1.0;
        effect.process(left, right);

        // Output should be 0 because of delay line (initial silence)
        QCOMPARE(left, 0.0f);
        QCOMPARE(right, 0.0f);

        // But reduction should already start happening based on the input
        QVERIFY(effect.reductionDb() < 0.0f);
    }
}

void EffectsTest::test_eq8BandParametricEffect_shouldApplyBandsAndBeStable()
{
    Eq8BandParametricEffect effect;
    effect.setSampleRate(44100.0);

    // Test defaults
    {
        if (auto p = effect.parameter(Constants::NahdXml::xmlKeyBandQ(0).toStdString()); p) {
            // Default should be 0.5f (maps to 1.0)
            QCOMPARE(p->get().value(), 0.5f);
        }
    }

    // Test bypass (all bands default to bypass)
    {
        double left = 1.0;
        double right = 1.0;
        effect.process(left, right);
        QCOMPARE(left, 1.0f);
        QCOMPARE(right, 1.0f);
    }

    // Test Bell filter
    {
        effect.reset();
        // Band 1: Bell, 1000Hz, +12dB, Q=1.0
        if (auto p = effect.parameter(Constants::NahdXml::xmlKeyBandType(0).toStdString()); p) {
            p->get().setValue(1.0f); // Bell
        }
        if (auto p = effect.parameter(Constants::NahdXml::xmlKeyBandFreq(0).toStdString()); p) {
            p->get().setValue(0.5f); // 1000Hz approx
        }
        if (auto p = effect.parameter(Constants::NahdXml::xmlKeyBandGain(0).toStdString()); p) {
            p->get().setValue(0.75f); // +12dB
        }
        effect.sync();

        double left = 1.0;
        double right = 1.0;
        effect.process(left, right);

        // At 0Hz (DC), a bell filter at 1000Hz with Q=1.0 should have some gain
        // but not the full +12dB. Output should be > 1.0.
        QVERIFY(left > 1.0f);
        QVERIFY(right > 1.0f);
    }

    // Test stability
    {
        for (int i = 0; i < 1000; i++) {
            double left = 1.0;
            double right = 1.0;
            effect.process(left, right);
            QVERIFY(!std::isnan(left));
            QVERIFY(!std::isinf(left));
        }
    }
}

void EffectsTest::test_clipperEffect_shouldClipSignal()
{
    ClipperEffect effect;

    // Test Hard Clipping
    {
        effect.reset();
        if (const auto p = effect.parameter(Constants::NahdXml::xmlKeyMode().toStdString()); p) {
            p->get().setValue(0.0f); // Hard
        }
        if (const auto p = effect.parameter(Constants::NahdXml::xmlKeyThreshold().toStdString()); p) {
            p->get().setValue(0.5f); // -12dB approx 0.2511
        }
        effect.sync();

        const auto threshold = Utils::Dsp::dbToLinear(-12.0f);
        auto left = 1.0;
        auto right = 1.0;
        effect.process(left, right);

        QCOMPARE(static_cast<float>(left), threshold);
        QCOMPARE(static_cast<float>(right), threshold);

        left = -1.0;
        right = -1.0;
        effect.process(left, right);
        QCOMPARE(static_cast<float>(left), -threshold);
        QCOMPARE(static_cast<float>(right), -threshold);
    }

    // Test Soft Clipping (Tanh)
    {
        effect.reset();
        if (const auto p = effect.parameter(Constants::NahdXml::xmlKeyMode().toStdString()); p) {
            p->get().setValue(1.0f); // Soft
        }
        if (const auto p = effect.parameter(Constants::NahdXml::xmlKeyThreshold().toStdString()); p) {
            p->get().setValue(1.0f); // 0dB = 1.0
        }
        effect.sync();

        auto left = 1.0;
        auto right = 1.0;
        effect.process(left, right);

        // tanh(1.0) is approx 0.7615
        QVERIFY(left < 1.0);
        QVERIFY(left > 0.76);
    }

    // Test Gain
    {
        effect.reset();
        if (const auto p = effect.parameter(Constants::NahdXml::xmlKeyMode().toStdString()); p) {
            p->get().setValue(0.0f); // Hard
        }
        if (const auto p = effect.parameter(Constants::NahdXml::xmlKeyThreshold().toStdString()); p) {
            p->get().setValue(1.0f); // 0dB = 1.0
        }
        if (const auto p = effect.parameter(Constants::NahdXml::xmlKeyGain().toStdString()); p) {
            p->get().setValue(0.75f); // +12dB = 3.98 approx
        }
        effect.sync();

        auto left = 0.5;
        auto right = 0.5;
        effect.process(left, right);

        const auto expected = 0.5 * Utils::Dsp::dbToLinear(12.0f);
        QCOMPARE(static_cast<float>(left), expected);
    }
}

void EffectsTest::test_filterStability_shouldHandleChangingCutoff()
{
    LowPassFilterEffect lp;
    HighPassFilterEffect hp;

    for (int i = 0; i < 1000; ++i) {
        double left = 1.0;
        double right = 1.0;
        const float cutoff = 0.5f + 0.49f * std::sin(i * 0.1f);

        lp.setCutoff(cutoff);
        hp.setCutoff(cutoff);

        lp.process(left, right);
        hp.process(left, right);

        QVERIFY(!std::isnan(left));
        QVERIFY(!std::isnan(right));
    }
}

void EffectsTest::test_cascadedSvfStability_shouldHandleRapidParameterChanges()
{
    CascadedSvf filter {};
    filter.setSampleRate(static_cast<uint32_t>(Constants::defaultSampleRate()));

    // Stress test: Rapidly change parameters
    for (int i = 0; i < 1000; ++i) {
        filter.setCutoff(0.5 + 0.49 * std::sin(i * 0.1));
        filter.setResonance(0.5 + 0.49 * std::cos(i * 0.05));

        double out = filter.process(1.0);
        QVERIFY(!std::isnan(out));
        QVERIFY(!std::isinf(out));
    }

    // Check for NaN recovery
    filter.setCutoff(0.5);
    filter.setResonance(0.5);
    double out = filter.process(1.0);
    QVERIFY(!std::isnan(out));
}

void EffectsTest::test_chorusEffect_shouldProcessAudio()
{
    ChorusEffect effect;
    effect.setSampleRate(44100.0);
    effect.setRate(1.0);
    effect.setDepth(0.5);
    effect.setDelay(20.0);
    effect.setMix(1.0); // Wet only

    // Process some silence to initialize
    for (int i = 0; i < 1000; i++) {
        double l = 0.0, r = 0.0;
        effect.process(l, r);
    }

    bool signalDetected = false;
    // Process enough samples to reach the delay (~20ms @ 44.1kHz = 882 samples)
    // plus some extra for modulation and filters
    for (int i = 0; i < 5000; i++) {
        // Use a sine wave to avoid DC issues with HPF
        double input = std::sin(2.0 * M_PI * 440.0 * i / 44100.0);
        double l = input, r = input;
        effect.process(l, r);
        if (i > 1500) {
            if (std::abs(l) > 0.1 || std::abs(r) > 0.1) {
                signalDetected = true;
                break;
            }
        }
    }
    QVERIFY(signalDetected);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::EffectsTest)
