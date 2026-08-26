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

#include "cascaded_svf_test.hpp"

#include "../../domain/dsp/cascaded_svf.hpp"

#include <QTest>

#include <algorithm>
#include <cmath>
#include <numbers>
#include <vector>

namespace noteahead {

namespace {

constexpr double SampleRate = 48000.0;
constexpr double ToneHz = 220.0;

//! Largest jump in the second difference of @p signal, divided by its RMS. A smoothly filtered
//! tone stays far below 1; a filter re-entering with stale state puts a step in the output that
//! shows up here as a spike, which is what a listener hears as a click.
double peakSecondDifferenceOverRms(const std::vector<double> & signal)
{
    double sumSquares = 0.0;
    for (auto && sample : signal) {
        sumSquares += sample * sample;
    }
    const double rms = std::sqrt(sumSquares / static_cast<double>(signal.size()));
    if (rms <= 1.0e-12) {
        return 0.0;
    }
    double peak = 0.0;
    for (size_t i = 1; i + 1 < signal.size(); i++) {
        peak = std::max(peak, std::abs(signal[i + 1] - 2.0 * signal[i] + signal[i - 1]));
    }
    return peak / rms;
}

//! Renders a steady tone through three phases, the way a mod envelope actually drives a filter:
//! the cutoff sits somewhere active, then parks at the end of its range where the filter hands the
//! signal straight through, then sweeps back into range. Only the last phase is returned -- that is
//! where a filter whose state stopped updating while it was parked re-enters with stale state.
std::vector<double> parkThenSweep(CascadedSvf::Mode mode, double active, double parked, size_t phaseFrames)
{
    CascadedSvf filter;
    filter.setMode(mode);
    filter.setSampleRate(SampleRate);
    filter.setResonance(0.0);

    size_t frame = 0;
    const auto render = [&](double cutoff) {
        filter.setCutoff(cutoff);
        const double phase = 2.0 * std::numbers::pi * ToneHz * static_cast<double>(frame++) / SampleRate;
        return filter.process(std::sin(phase));
    };

    for (size_t i = 0; i < phaseFrames; i++) {
        render(active);
    }
    for (size_t i = 0; i < phaseFrames; i++) {
        render(parked);
    }

    std::vector<double> output;
    output.reserve(phaseFrames);
    for (size_t i = 0; i < phaseFrames; i++) {
        const double t = static_cast<double>(i) / static_cast<double>(phaseFrames - 1);
        output.push_back(render(parked + (active - parked) * t));
    }
    return output;
}

} // namespace

void CascadedSvfTest::test_lowPass_parkedFullyOpen_shouldPassThrough()
{
    CascadedSvf filter;
    filter.setMode(CascadedSvf::Mode::LowPass);
    filter.setSampleRate(SampleRate);
    filter.setCutoff(1.0);

    for (size_t i = 0; i < 512; i++) {
        const double input = std::sin(2.0 * std::numbers::pi * ToneHz * static_cast<double>(i) / SampleRate);
        QCOMPARE(filter.process(input), input);
    }
}

void CascadedSvfTest::test_highPass_parkedFullyClosed_shouldPassThrough()
{
    CascadedSvf filter;
    filter.setMode(CascadedSvf::Mode::HighPass);
    filter.setSampleRate(SampleRate);
    filter.setCutoff(0.0);

    for (size_t i = 0; i < 512; i++) {
        const double input = std::sin(2.0 * std::numbers::pi * ToneHz * static_cast<double>(i) / SampleRate);
        QCOMPARE(filter.process(input), input);
    }
}

void CascadedSvfTest::test_lowPass_sweptDownAcrossOpenThreshold_shouldNotStep()
{
    // A mod envelope opening the filter past the top and sweeping back down. The filter is handed
    // through while it is fully open, so unless it keeps running underneath, it re-enters at 0.999
    // with state from before it parked and steps the output.
    const auto swept = parkThenSweep(CascadedSvf::Mode::LowPass, 0.5, 1.0, 4096);
    QVERIFY(peakSecondDifferenceOverRms(swept) < 0.1);
}

void CascadedSvfTest::test_highPass_sweptUpAcrossClosedThreshold_shouldNotStep()
{
    const auto swept = parkThenSweep(CascadedSvf::Mode::HighPass, 0.5, 0.0, 4096);
    QVERIFY(peakSecondDifferenceOverRms(swept) < 0.1);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::CascadedSvfTest)
