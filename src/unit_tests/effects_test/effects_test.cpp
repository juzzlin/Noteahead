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

#include <QtTest>

#include <cmath>

namespace noteahead {

void EffectsTest::test_volumeEffect()
{
    VolumeEffect effect;
    float left = 1.0f;
    float right = 1.0f;

    effect.setVolume(0.5f);
    effect.process(left, right, 44100);

    QCOMPARE(left, 0.5f);
    QCOMPARE(right, 0.5f);

    effect.setVolume(0.0f);
    left = 1.0f;
    right = 1.0f;
    effect.process(left, right, 44100);
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
        effect.process(left, right, 44100);
        QCOMPARE(left, 1.0f);
        QCOMPARE(right, 1.0f);
    }

    // Full Left
    {
        float left = 1.0f;
        float right = 1.0f;
        effect.setPan(0.0f);
        effect.process(left, right, 44100);
        QCOMPARE(left, 1.0f);
        QCOMPARE(right, 0.0f);
    }

    // Full Right
    {
        float left = 1.0f;
        float right = 1.0f;
        effect.setPan(1.0f);
        effect.process(left, right, 44100);
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
    effect.process(left, right, 44100);
    QCOMPARE(left, 1.0f);
    QCOMPARE(right, 1.0f);

    // Filter processing stability
    effect.setCutoff(0.5f);
    for (int i = 0; i < 100; ++i) {
        left = 1.0f;
        right = 1.0f;
        effect.process(left, right, 44100);
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
        effect.process(left, right, 44100);
        QCOMPARE(left, 1.0f);
        QCOMPARE(right, 1.0f);
    }

    // Stability and NaN check
    effect.setCutoff(0.5f);
    for (int i = 0; i < 100; ++i) {
        float left = 1.0f;
        float right = 1.0f;
        effect.process(left, right, 44100);
        QVERIFY(!std::isnan(left));
        QVERIFY(!std::isnan(right));
    }

    // Extreme cutoff stability
    effect.setCutoff(0.99f);
    for (int i = 0; i < 100; ++i) {
        float left = 1.0f;
        float right = 1.0f;
        effect.process(left, right, 44100);
        QVERIFY(!std::isnan(left));
        QVERIFY(!std::isnan(right));
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

        lp.process(left, right, 44100);
        hp.process(left, right, 44100);

        QVERIFY(!std::isnan(left));
        QVERIFY(!std::isnan(right));
    }
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::EffectsTest)
