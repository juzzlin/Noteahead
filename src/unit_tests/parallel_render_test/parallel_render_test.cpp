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

#include "parallel_render_test.hpp"

#include "../../domain/devices/drum_synth_device.hpp"
#include "../../domain/devices/string_ensemble_device.hpp"
#include "../../domain/devices/sub_mixer_device.hpp"
#include "../../domain/devices/synth_device.hpp"
#include "../../domain/devices/wavetable_synth_device.hpp"
#include "../../domain/effects/effect_rack.hpp"
#include "../../domain/effects/reverb.hpp"
#include "../../infra/audio/audio_engine.hpp"

#include <QTest>

#include <cmath>
#include <memory>
#include <span>
#include <vector>

namespace noteahead {

namespace {

constexpr uint32_t SampleRate { 48000 };
constexpr uint32_t FrameCount { 128 };
constexpr int BufferCount { 60 };

//! Threading changes which lane a device's output is accumulated into, and the lanes are summed in
//! a fixed order afterwards, so the additions are grouped differently. That shifts the last bits of
//! the result — inaudible at around -180 dBFS, but it means the two paths are equal to within
//! floating-point summation order rather than bit-identical.
constexpr double Tolerance { 1.0e-9 };

//! Populates an engine identically every time, so two renders differ only in how they were driven.
//! The devices seed their own generators deterministically, so this is reproducible.
void populate(AudioEngine & engine, bool withSubMixer)
{
    const auto synth = std::make_shared<SynthDevice>("Synth");
    synth->processMidiNoteOn(48, 100);
    synth->processMidiNoteOn(55, 90);
    engine.setDevice(0, synth);

    const auto wavetable = std::make_shared<WavetableSynthDevice>("Wavetable");
    wavetable->processMidiNoteOn(60, 110);
    engine.setDevice(1, wavetable);

    const auto drums = std::make_shared<DrumSynthDevice>("Drums");
    drums->processMidiNoteOn(36, 127);
    drums->processMidiNoteOn(42, 100);
    engine.setDevice(2, drums);

    const auto strings = std::make_shared<StringEnsembleDevice>("Strings");
    strings->processMidiNoteOn(64, 96);
    engine.setDevice(3, strings);

    if (withSubMixer) {
        // A Sub Mixer depends on its members, which puts it in a second processing layer and so
        // exercises the barrier between layers as well as the fan-out within one.
        const auto subMixer = std::make_shared<SubMixerDevice>("SubMixer");
        subMixer->setMembers({ 0, 1 });
        engine.setDevice(4, subMixer);

        // A send bus, so the parallel send accumulation is covered too.
        strings->setReverbSend(0, 0.5f);
        drums->setReverbSend(0, 0.3f);
    }
}

std::vector<double> render(bool threaded, bool withSubMixer)
{
    AudioEngine engine;
    if (withSubMixer) {
        engine.sendEffectRack().setEffect(0, std::make_shared<Reverb>());
    }
    populate(engine, withSubMixer);
    engine.setIsExclusive(threaded);

    std::vector<double> collected;
    collected.reserve(static_cast<size_t>(BufferCount) * FrameCount * 2);
    std::vector<double> buffer(static_cast<size_t>(FrameCount) * 2, 0.0);
    for (int i = 0; i < BufferCount; i++) {
        std::fill(buffer.begin(), buffer.end(), 0.0);
        AudioContext context { std::span(buffer.data(), buffer.size()), FrameCount, SampleRate };
        engine.process(context);
        collected.insert(collected.end(), buffer.begin(), buffer.end());
    }
    return collected;
}

double peakLevel(const std::vector<double> & samples)
{
    double peak = 0.0;
    for (const auto sample : samples) {
        peak = std::max(peak, std::abs(sample));
    }
    return peak;
}

void compare(const std::vector<double> & a, const std::vector<double> & b)
{
    QCOMPARE(a.size(), b.size());
    QVERIFY2(peakLevel(a) > 0.001, "The fixture produced no signal, so this would prove nothing");

    double worst = 0.0;
    for (size_t i = 0; i < a.size(); i++) {
        worst = std::max(worst, std::abs(a[i] - b[i]));
    }
    QVERIFY2(worst < Tolerance, qPrintable(QString { "worst difference %1" }.arg(worst)));
}

} // namespace

void ParallelRenderTest::test_realDevices_serialAndThreaded_shouldMatch()
{
    // Unlike a fixture of devices all emitting the same constant, real devices produce values whose
    // summation order actually matters, so this can see a fan-out that mixes work up.
    compare(render(false, false), render(true, false));
}

void ParallelRenderTest::test_subMixerAndSends_serialAndThreaded_shouldMatch()
{
    compare(render(false, true), render(true, true));
}

void ParallelRenderTest::test_threadedRender_repeated_shouldBeDeterministic()
{
    // Two threaded renders of the same project must agree, or an export would differ run to run.
    compare(render(true, true), render(true, true));
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::ParallelRenderTest)
