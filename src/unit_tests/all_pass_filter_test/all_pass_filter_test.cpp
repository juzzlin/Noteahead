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

#include "all_pass_filter_test.hpp"

#include "../../common/constants.hpp"
#include "../../domain/dsp/all_pass_filter.hpp"

#include <QTest>

#include <cmath>

namespace noteahead {

void AllPassFilterTest::test_allPassFilter_defaultParameters_shouldMatchXmlDefaults()
{
    AllPassFilter filter;

    const auto freqParam = filter.parameter(Constants::NahdXml::xmlKeyAllPassFilterFrequency().toStdString());
    QVERIFY(freqParam);
    QCOMPARE(freqParam->get().xmlValue(), freqParam->get().xmlDefault());

    const auto qParam = filter.parameter(Constants::NahdXml::xmlKeyAllPassFilterQ().toStdString());
    QVERIFY(qParam);
    QCOMPARE(qParam->get().xmlValue(), qParam->get().xmlDefault());

    const auto stagesParam = filter.parameter(Constants::NahdXml::xmlKeyAllPassFilterStages().toStdString());
    QVERIFY(stagesParam);
    QCOMPARE(stagesParam->get().xmlValue(), stagesParam->get().xmlDefault());
}

void AllPassFilterTest::test_allPassFilter_magnitude_shouldBeUnity()
{
    AllPassFilter filter;
    filter.setSampleRate(48000.0);

    // Feed a long sine at 200 Hz to let the filter settle, then measure RMS in/out
    const double freq = 200.0;
    const double sr = 48000.0;
    const int settle = 4800;
    const int measure = 4800;

    double sumIn = 0.0;
    double sumOut = 0.0;

    for (int n = 0; n < settle + measure; n++) {
        const double sample = std::sin(2.0 * M_PI * freq / sr * n);
        double left = sample;
        double right = sample;
        filter.process(left, right);
        if (n >= settle) {
            sumIn += sample * sample;
            sumOut += left * left;
        }
    }

    const double rmsIn = std::sqrt(sumIn / measure);
    const double rmsOut = std::sqrt(sumOut / measure);
    QVERIFY(std::abs(rmsOut - rmsIn) < 1e-4);
}

void AllPassFilterTest::test_allPassFilter_stages_shouldProcessWithoutInstability()
{
    AllPassFilter filter;
    filter.setSampleRate(48000.0);

    // Set 4 stages via parameter
    if (const auto p = filter.parameter(Constants::NahdXml::xmlKeyAllPassFilterStages().toStdString()); p) {
        p->get().setFromXml(4);
    }
    filter.sync();

    for (int n = 0; n < 48000; n++) {
        double left = (n == 0) ? 1.0 : 0.0;
        double right = left;
        filter.process(left, right);
        QVERIFY(std::isfinite(left));
        QVERIFY(std::isfinite(right));
    }
}

void AllPassFilterTest::test_allPassFilter_reset_shouldClearState()
{
    AllPassFilter filter;
    filter.setSampleRate(48000.0);

    // Push some signal through
    for (int n = 0; n < 100; n++) {
        double left = 1.0;
        double right = 1.0;
        filter.process(left, right);
    }

    filter.reset();

    // After reset an impulse should produce finite, bounded output
    double left = 1.0;
    double right = 1.0;
    filter.process(left, right);
    QVERIFY(std::isfinite(left));
    QVERIFY(std::abs(left) <= 2.0);
}

} // namespace noteahead

QTEST_MAIN(noteahead::AllPassFilterTest)
