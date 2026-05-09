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

#include "../../domain/devices/volume_effect.hpp"
#include "../../domain/devices/panning_effect.hpp"
#include "../../domain/devices/low_pass_filter_effect.hpp"
#include "../../domain/devices/high_pass_filter_effect.hpp"
#include "../../domain/devices/delay_effect.hpp"
#include "../../domain/dsp/cascaded_svf.hpp"
#include "../../common/constants.hpp"

#include <QtTest>

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
