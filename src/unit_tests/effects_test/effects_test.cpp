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

void EffectsTest::test_volumeEffect()
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

void EffectsTest::test_panningEffect()
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

void EffectsTest::test_lowPassFilterEffect()
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

void EffectsTest::test_highPassFilterEffect()
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

void EffectsTest::test_reverb_mix()
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

void EffectsTest::test_delayEffect()
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

void EffectsTest::test_compressorEffect()
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

void EffectsTest::test_eq8BandParametricEffect()
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

void EffectsTest::test_filterStability()
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

void EffectsTest::test_cascadedSvfStability()
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
