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

#include "oversampler_test.hpp"
#include "domain/dsp/oversampler.hpp"

#include <QTest>

namespace noteahead {

void OversamplerTest::test_process_dcInput_shouldReturnSameValue()
{
    Oversampler2x oversampler;
    const float dcValue { 0.5f };

    // Feed DC value for a while to stabilize filter
    float output { 0.0f };
    for (int i { 0 }; i < 100; ++i) {
        output = oversampler.process(dcValue, dcValue);
    }

    QCOMPARE(output, dcValue);
}

void OversamplerTest::test_process_nyquistFrequency_shouldBeAttenuated()
{
    Oversampler2x oversampler;
    // High rate signal at low rate Nyquist frequency: 1, -1, 1, -1, ...
    float output { 0.0f };
    for (int i { 0 }; i < 100; ++i) {
        output = oversampler.process(1.0f, -1.0f);
    }
    // Should be near zero
    QVERIFY(std::abs(output) < 0.0001f);
}

void OversamplerTest::test_process_lowFrequency_shouldPassUnattenuated()
{
    Oversampler2x oversampler;
    const float freq { 100.0f };
    const float fsHigh { 88200.0f };
    const float dt { 2.0f * std::numbers::pi_v<float> * freq / fsHigh };

    float maxOut { 0.0f };
    for (int i { 0 }; i < 1000; ++i) {
        const float s0 { std::sin(static_cast<float>(i * 2) * dt) };
        const float s1 { std::sin(static_cast<float>(i * 2 + 1) * dt) };
        const float output { oversampler.process(s0, s1) };
        maxOut = std::max(maxOut, std::abs(output));
    }

    // Should be very close to 1.0 (amplitude of input)
    QVERIFY(maxOut > 0.99f);
    QVERIFY(maxOut <= 1.01f);
}

void OversamplerTest::test_process_silence_shouldReturnSilence()
{
    Oversampler2x oversampler;
    QCOMPARE(oversampler.process(0.0f, 0.0f), 0.0f);
}

void OversamplerTest::test_reset_shouldClearState()
{
    Oversampler2x oversampler;
    oversampler.process(1.0f, 1.0f);
    oversampler.reset();
    QCOMPARE(oversampler.process(0.0f, 0.0f), 0.0f);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::OversamplerTest)
