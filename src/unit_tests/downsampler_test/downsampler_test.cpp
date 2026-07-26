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

#include "downsampler_test.hpp"
#include "../../domain/dsp/downsampler.hpp"

#include <QTest>

#include <array>
#include <cmath>

namespace noteahead {

void DownsamplerTest::test_clampOversampleFactor_shouldAllowSupportedValues()
{
    QCOMPARE(clampOversampleFactor(1), static_cast<uint8_t>(1));
    QCOMPARE(clampOversampleFactor(2), static_cast<uint8_t>(2));
    QCOMPARE(clampOversampleFactor(4), static_cast<uint8_t>(4));
}

void DownsamplerTest::test_clampOversampleFactor_shouldFallBackToTwo()
{
    QCOMPARE(clampOversampleFactor(0), static_cast<uint8_t>(2));
    QCOMPARE(clampOversampleFactor(3), static_cast<uint8_t>(2));
    QCOMPARE(clampOversampleFactor(8), static_cast<uint8_t>(2));
}

void DownsamplerTest::test_process_factorOne_shouldPassThrough()
{
    Downsampler downsampler;
    const std::array<float, 4> samples { 0.42f, -1.0f, 0.0f, 0.0f };
    QCOMPARE(downsampler.process(samples.data(), 1), 0.42f);
}

void DownsamplerTest::test_process_factorTwo_dcInput_shouldPreserveLevel()
{
    Downsampler downsampler;
    const std::array<float, 4> dc { 1.0f, 1.0f, 1.0f, 1.0f };
    float out = 0.0f;
    // Run long enough for the FIR to fill; the half-band filter has unity DC gain.
    for (int i = 0; i < 64; i++) {
        out = downsampler.process(dc.data(), 2);
    }
    QVERIFY(std::abs(out - 1.0f) < 0.001f);
}

void DownsamplerTest::test_process_factorFour_dcInput_shouldPreserveLevel()
{
    Downsampler downsampler;
    const std::array<float, 4> dc { 1.0f, 1.0f, 1.0f, 1.0f };
    float out = 0.0f;
    for (int i = 0; i < 64; i++) {
        out = downsampler.process(dc.data(), 4);
    }
    // Two cascaded unity-DC-gain stages must still preserve DC.
    QVERIFY(std::abs(out - 1.0f) < 0.001f);
}

void DownsamplerTest::test_process_factorTwo_nyquistInput_shouldBeAttenuated()
{
    Downsampler downsampler;
    // A full-rate Nyquist tone (+1, -1, +1, -1 ...) must be rejected by the half-band filter.
    const std::array<float, 4> nyquist { 1.0f, -1.0f, 0.0f, 0.0f };
    float peak = 0.0f;
    for (int i = 0; i < 64; i++) {
        const float out = downsampler.process(nyquist.data(), 2);
        if (i > 16) {
            peak = std::max(peak, std::abs(out));
        }
    }
    QVERIFY(peak < 0.05f);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::DownsamplerTest)
