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

#include "upsampler_test.hpp"
#include "../../domain/dsp/upsampler.hpp"

#include <QTest>

#include <array>
#include <cmath>
#include <numbers>

namespace noteahead {

namespace {
// Peak amplitude of a low-frequency sine after an upsample -> (identity) -> downsample round trip.
float roundTripPeak(uint8_t factor)
{
    Upsampler up;
    Decimator down;
    std::array<float, 4> high {};
    float peak = 0.0f;
    const int samples = 2000;
    for (int n = 0; n < samples; n++) {
        const float x = 0.5f * std::sin(2.0f * std::numbers::pi_v<float> * static_cast<float>(n) / 64.0f);
        up.process(x, high.data(), factor);
        const float y = down.process(high.data(), factor);
        if (n > 64) { // Skip filter warmup
            peak = std::max(peak, std::abs(y));
        }
    }
    return peak;
}
} // namespace

void UpsamplerTest::test_process_factorOne_shouldPassThrough()
{
    Upsampler up;
    std::array<float, 4> out {};
    up.process(0.33f, out.data(), 1);
    QCOMPARE(out[0], 0.33f);
}

void UpsamplerTest::test_process_dcInput_shouldPreserveLevel()
{
    Upsampler2x up;
    float a = 0.0f;
    float b = 0.0f;
    for (int i = 0; i < 64; i++) {
        up.process(1.0f, a, b);
    }
    // Each polyphase branch has unity DC gain, so both output phases must reproduce the DC level.
    QVERIFY(std::abs(a - 1.0f) < 0.001f);
    QVERIFY(std::abs(b - 1.0f) < 0.001f);
}

void UpsamplerTest::test_roundTrip_factorTwo_lowFrequency_shouldReconstruct()
{
    // A low-frequency sine of amplitude 0.5 must survive the round trip with its level intact.
    QVERIFY(std::abs(roundTripPeak(2) - 0.5f) < 0.02f);
}

void UpsamplerTest::test_roundTrip_factorFour_lowFrequency_shouldReconstruct()
{
    QVERIFY(std::abs(roundTripPeak(4) - 0.5f) < 0.02f);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::UpsamplerTest)
