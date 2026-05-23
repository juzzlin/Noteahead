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
#include "../../domain/devices/delay_effect.hpp"
#include "../../domain/devices/high_pass_filter_effect.hpp"
#include "../../domain/devices/low_pass_filter_effect.hpp"
#include "../../domain/devices/panning_effect.hpp"
#include "../../domain/devices/volume_effect.hpp"
#include "../../domain/dsp/cascaded_svf.hpp"
#include "../../domain/dsp/compressor_effect.hpp"
#include "../../domain/dsp/eq_8_band_parametric_effect.hpp"
#include "../../domain/dsp/reverb_effect.hpp"

#include <QTest>

#include <cmath>

namespace noteahead {

void EffectsTest::test_volumeEffect_shouldApplyGainToSignal()
{
    VolumeEffect effect;
    float left = 1.0f;
    float right = 1.0f;

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

    // Center
    {
        float left = 1.0f;
        float right = 1.0f;
        effect.setPan(0.5f);
        effect.process(left, right);
        QCOMPARE(left, 1.0f);
        QCOMPARE(right, 1.0f);
    }

    // Full Left
    {
        float left = 1.0f;
        float right = 1.0f;
        effect.setPan(0.0f);
        effect.process(left, right);
        QCOMPARE(left, 1.0f);
        QCOMPARE(right, 0.0f);
    }

    // Full Right
    {
        float left = 1.0f;
        float right = 1.0f;
        effect.setPan(1.0f);
        effect.process(left, right);
        QCOMPARE(left, 0.0f);
        QCOMPARE(right, 1.0f);
    }
}

void EffectsTest::test_lowPassFilterEffect_shouldProcessAudioStablely()
{
    LowPassFilterEffect effect;
    float left = 1.0f;
    float right = 1.0f;

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
        float left = 1.0f;
        float right = 1.0f;
        effect.setCutoff(0.0f);
        effect.process(left, right);
        QCOMPARE(left, 1.0f);
        QCOMPARE(right, 1.0f);
    }

    // Stability and NaN check
    effect.setCutoff(0.5f);
    for (int i = 0; i < 100; ++i) {
        float left = 1.0f;
        float right = 1.0f;
        effect.process(left, right);
        QVERIFY(!std::isnan(left));
        QVERIFY(!std::isnan(right));
    }

    // Extreme cutoff stability
    effect.setCutoff(0.99f);
    for (int i = 0; i < 100; ++i) {
        float left = 1.0f;
        float right = 1.0f;
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
    reverb.sync();

    float l = 1.0f;
    float r = 1.0f;
    
    // Process many samples to ensure any internal state is active
    // Reverb tail needs some samples to build up
    for (int i = 0; i < 5000; i++) {
        float tl = 1.0f;
        float tr = 1.0f;
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
        float tl = 1.0f;
        float tr = 1.0f;
        reverb.process(tl, tr);
    }

    l = 1.0f;
    r = 1.0f;
    reverb.process(l, r);
    
    // With mix 1.0, output should be different from input
    QVERIFY(l != 1.0f || r != 1.0f);
    
    // With additive mix 1.0 and DC 1.0 input, output should be > 1.0 
    // (dry + wet, where wet is also derived from 1.0)
    QVERIFY(std::abs(l) > 1.0f);
    QVERIFY(std::abs(r) > 1.0f);
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
    float left = 1.0f;
    float right = 1.0f;
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
    float left = 1.0f;
    float right = 1.0f;
    effect.process(left, right);
    QCOMPARE(left, 0.0f);
    QCOMPARE(right, 0.0f);

    // Process enough samples to reach the delay time
    const int delaySamples = static_cast<int>(0.1f * sampleRate);
    for (int i = 0; i + 1 < delaySamples; i++) {
        float l = 0.0f;
        float r = 0.0f;
        effect.process(l, r);
    }

    // Now with feedback 0.0, the output should still be silence
    left = 0.0f;
    right = 0.0f;
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
        float l = 0.0f;
        float r = 0.0f;
        effect.process(l, r);
    }

    left = 0.0f;
    right = 0.0f;
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
        float l = 0.0f;
        float r = 0.0f;
        effect.process(l, r);
    }
    
    left = 0.0f;
    right = 0.0f;
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
    float left = 1.0f;
    float right = 1.0f;
    effect.process(left, right);
    // Output should be silence (mix is wet, buffer empty)
    QCOMPARE(left, 0.0f);

    // Wait for 1st echo
    for (int i = 0; i + 1 < delaySamples; i++) {
        float l = 0.0f;
        float r = 0.0f;
        effect.process(l, r);
    }
    left = 0.0f;
    right = 0.0f;
    effect.process(left, right);
    // 1st echo should be 0.5 (1.0 * feedback)
    QVERIFY(std::abs(left - 0.5f) < 1.0e-3f);

    // Wait for 2nd echo
    for (int i = 0; i + 1 < delaySamples; i++) {
        float l = 0.0f;
        float r = 0.0f;
        effect.process(l, r);
    }
    left = 0.0f;
    right = 0.0f;
    effect.process(left, right);
    // 2nd echo should be 0.25 (0.5 * feedback)
    QVERIFY(std::abs(left - 0.25f) < 1.0e-3f);

    // Wait for 3rd echo
    for (int i = 0; i + 1 < delaySamples; i++) {
        float l = 0.0f;
        float r = 0.0f;
        effect.process(l, r);
    }
    left = 0.0f;
    right = 0.0f;
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
    float left = 1.0f;
    float right = 0.0f;
    effect.process(left, right);

    int echoes = 0;
    
    // Process 1 second (10 echoes expected)
    for (int i = 0; i < 1 * 44100; i++) {
        float l = 0.0f; float r = 0.0f;
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
    float left = 1.0f;
    float right = 1.0f;
    effect.process(left, right);

    int echoes = 0;
    float lastEchoVal = 1.1f;
    
    // Process 2 seconds
    for (int i = 0; i < 2 * 44100; i++) {
        float l = 0.0f; float r = 0.0f;
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
    float left = 1.0f;
    float right = 0.0f;
    effect.process(left, right);

    // Wait for 1st echo
    for (int i = 0; i + 1 < delaySamples; i++) {
        float l = 0.0f; float r = 0.0f;
        effect.process(l, r);
    }
    
    left = 0.0f; right = 0.0f;
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
    float left = 1.0f;
    float right = 0.0f;
    effect.process(left, right);

    // Wait for 1st echo
    for (int i = 0; i + 1 < delaySamples; i++) {
        float l = 0.0f; float r = 0.0f;
        effect.process(l, r);
    }
    
    left = 0.0f; right = 0.0f;
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
        float l = 0.0f; float r = 0.0f;
        effect.process(l, r);
    }
    
    left = 0.0f; right = 0.0f;
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
        float left = pulseVal; float right = pulseVal;
        effect.process(left, right);
    }

    bool foundEcho = false;
    // We expect the echo around delaySamples. Let's check a window.
    for (int i = 0; i < delaySamples + 100; i++) {
        float left = 0.0f; float right = 0.0f;
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

void EffectsTest::test_compressorEffect_shouldReduceGainAndHandleLookahead()
{
    CompressorEffect effect;
    effect.setSampleRate(44100.0);

    // Default: Threshold -20dB, Ratio 4:1, Attack 10ms, Release 100ms, Makeup 0dB, Lookahead 0ms

    // Test bypass (signal below threshold)
    {
        // -30dB signal
        float val = Utils::Dsp::dbToLinear(-30.0f);
        float left = val;
        float right = val;
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
        float val = 1.0f; // 0dB
        float left = val;
        float right = val;
        
        // Process long enough for attack to settle
        for (int i = 0; i < 5000; i++) {
            float tl = val;
            float tr = val;
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

        float left = 1.0f;
        float right = 1.0f;
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

    // Test bypass (all bands default to bypass)
    {
        float left = 1.0f;
        float right = 1.0f;
        effect.process(left, right);
        QCOMPARE(left, 1.0f);
        QCOMPARE(right, 1.0f);
    }

    // Test Bell filter
    {
        effect.reset();
        // Band 1: Bell, 1000Hz, +12dB, Q=1.0
        if (auto p = effect.parameter(Constants::NahdXml::xmlKeyEq8BandParametricType(0).toStdString()); p) {
            p->get().setValue(1.0f / 6.0f); // Bell
        }
        if (auto p = effect.parameter(Constants::NahdXml::xmlKeyEq8BandParametricFreq(0).toStdString()); p) {
            p->get().setValue(0.5f); // 1000Hz approx
        }
        if (auto p = effect.parameter(Constants::NahdXml::xmlKeyEq8BandParametricGain(0).toStdString()); p) {
            p->get().setValue(0.75f); // +12dB
        }
        effect.sync();

        float left = 1.0f;
        float right = 1.0f;
        effect.process(left, right);

        // At 0Hz (DC), a bell filter at 1000Hz with Q=1.0 should have some gain
        // but not the full +12dB. Output should be > 1.0.
        QVERIFY(left > 1.0f);
        QVERIFY(right > 1.0f);
    }

    // Test stability
    {
        for (int i = 0; i < 1000; i++) {
            float left = 1.0f;
            float right = 1.0f;
            effect.process(left, right);
            QVERIFY(!std::isnan(left));
            QVERIFY(!std::isinf(left));
        }
    }
}

void EffectsTest::test_filterStability_shouldHandleChangingCutoff()
{
    LowPassFilterEffect lp;
    HighPassFilterEffect hp;

    for (int i = 0; i < 1000; ++i) {
        float left = 1.0f;
        float right = 1.0f;
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
    CascadedSvf filter{};
    filter.setSampleRate(static_cast<uint32_t>(Constants::defaultSampleRate()));
    
    // Stress test: Rapidly change parameters
    for (int i = 0; i < 1000; ++i) {
        filter.setCutoff(0.5 + 0.49 * std::sin(i * 0.1));
        filter.setResonance(0.5 + 0.49 * std::cos(i * 0.05));
        
        float out = filter.process(1.0f);
        QVERIFY(!std::isnan(out));
        QVERIFY(!std::isinf(out));
    }
    
    // Check for NaN recovery
    filter.setCutoff(0.5);
    filter.setResonance(0.5);
    float out = filter.process(1.0f);
    QVERIFY(!std::isnan(out));
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::EffectsTest)
