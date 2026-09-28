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

#include "gate_test.hpp"

#include "../../common/constants.hpp"
#include "../../domain/dsp/audio_context.hpp"
#include "../../domain/effects/effect.hpp"
#include "../../domain/effects/gate.hpp"

#include <QTest>

#include <algorithm>
#include <cmath>
#include <numbers>
#include <span>
#include <vector>

namespace noteahead {

namespace {

constexpr double SampleRate = 48000.0;

//! Runs a block through the effect and hands the buffer back.
std::vector<double> render(Effect & effect, const std::vector<double> & input, double bpm = 120.0)
{
    std::vector<double> buffer = input;
    AudioContext context { std::span<double>(buffer.data(), buffer.size()), static_cast<uint32_t>(buffer.size() / 2), static_cast<uint32_t>(SampleRate), bpm, {}, 1, false };
    effect.process(context);
    return buffer;
}

//! A stereo sine at one amplitude, as interleaved frames.
std::vector<double> tone(double frequency, double amplitude, size_t frames)
{
    std::vector<double> buffer(frames * 2, 0.0);
    for (size_t i = 0; i < frames; i++) {
        const double sample = amplitude * std::sin(2.0 * std::numbers::pi * frequency * static_cast<double>(i) / SampleRate);
        buffer[i * 2] = sample;
        buffer[i * 2 + 1] = sample;
    }
    return buffer;
}

double peak(const std::vector<double> & buffer)
{
    double value = 0.0;
    for (auto && sample : buffer) {
        value = std::max(value, std::abs(sample));
    }
    return value;
}

//! Peak over the last quarter of a buffer.
//!
//! A gate starts open and takes its release to close, so the loudest sample in a gated buffer is
//! always one of the first: measured over the whole thing, every one of these tests would be
//! reading the gate's release time rather than what it settles at.
double settledPeak(const std::vector<double> & buffer)
{
    const auto from = buffer.size() / 4 * 3;
    double value = 0.0;
    for (size_t i = from; i < buffer.size(); i++) {
        value = std::max(value, std::abs(buffer[i]));
    }
    return value;
}

void setParameter(Effect & effect, const QString & key, float value)
{
    if (auto p = effect.parameter(key.toStdString()); p) {
        p->get().update(value);
    }
    effect.sync();
}

} // namespace

void GateTest::test_loudSignal_shouldPassThrough()
{
    Gate effect;
    effect.sync();
    const auto input = tone(440.0, 0.5, 4096);
    const auto output = render(effect, input);
    // Well above the -40 dB threshold, so the gate opens and stays open.
    QVERIFY2(peak(output) > peak(input) * 0.9, qPrintable(QString::number(peak(output))));
}

void GateTest::test_quietSignal_shouldBeGated()
{
    Gate effect;
    effect.sync();
    // -60 dB, which is twenty under the threshold: at the default ratio of eight that is far more
    // reduction than the range allows, so the range is what decides where it lands.
    const auto input = tone(440.0, 0.001, 48000);
    const auto output = render(effect, input);
    QVERIFY2(settledPeak(output) < peak(input) * 0.2, qPrintable(QString::number(settledPeak(output) / peak(input))));
}

void GateTest::test_ratioOne_shouldPassEverything()
{
    Gate effect;
    setParameter(effect, Constants::NahdXml::xmlKeyRatio(), 0.0f);
    // A ratio of one is no expansion at all, whatever the level: every dB below the threshold costs
    // nothing, so the gate is a wire.
    const auto input = tone(440.0, 0.001, 48000);
    const auto output = render(effect, input);
    QVERIFY2(settledPeak(output) > peak(input) * 0.9, qPrintable(QString::number(settledPeak(output) / peak(input))));
}

void GateTest::test_range_shouldFloorTheReduction()
{
    const auto reductionWithRange = [](float range) {
        Gate effect;
        setParameter(effect, Constants::NahdXml::xmlKeyRange(), range);
        const auto input = tone(440.0, 0.001, 48000);
        return settledPeak(render(effect, input)) / peak(input);
    };

    // Twelve dB of range can only ever take twelve dB away, which is what makes this a tightener
    // rather than a silencer.
    const auto shallow = reductionWithRange(12.0f / 90.0f);
    const auto deep = reductionWithRange(1.0f);
    QVERIFY2(shallow > deep, qPrintable(QString::number(shallow) + " vs " + QString::number(deep)));
    QVERIFY2(std::abs(20.0 * std::log10(shallow) + 12.0) < 2.0, qPrintable(QString::number(20.0 * std::log10(shallow))));
}

void GateTest::test_hold_shouldKeepItOpenAfterTheSignalStops()
{
    Gate effect;
    // Half a second of hold, far longer than the gap below.
    setParameter(effect, Constants::NahdXml::xmlKeyHold(), 1.0f);

    // Loud enough to open it, then silence.
    render(effect, tone(440.0, 0.5, 4096));
    const auto afterGap = render(effect, std::vector<double>(4096 * 2, 0.0));
    QVERIFY(afterGap.size() == 4096 * 2);
    // Nothing to hear either way, so what is asserted is the gain: still open, because the hold has
    // not run out.
    QVERIFY2(effect.gainDb() > -1.0, qPrintable(QString::number(effect.gainDb())));
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::GateTest)
