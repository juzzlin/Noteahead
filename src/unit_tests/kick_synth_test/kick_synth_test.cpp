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

#include "kick_synth_test.hpp"

#include "../../application/service/kick_synth_preview.hpp"
#include "../../common/constants.hpp"
#include "../../domain/devices/kick_synth_device.hpp"
#include "../../domain/devices/kick_synth_presets.hpp"
#include "../../domain/dsp/audio_context.hpp"
#include "../../domain/dsp/drum/kick_pm_engine.hpp"

#include <QTest>

#include <cmath>
#include <vector>

namespace noteahead {

namespace {

namespace Xml = Constants::NahdXml;

constexpr double SampleRate = 44100.0;

std::vector<double> render(KickPmEngine & engine, float velocity, double seconds)
{
    engine.setSampleRate(SampleRate);
    engine.trigger(velocity);
    std::vector<double> out;
    for (size_t i = 0; i < static_cast<size_t>(seconds * SampleRate); i++) {
        out.push_back(static_cast<double>(engine.nextSample()));
    }
    return out;
}

//! The head alone: no click, ring, shell or front head to blur its pitch
KickPmEngine bareHead()
{
    KickPmEngine engine;
    engine.setClick(0.0f);
    engine.setRing(0.0f);
    engine.setShell(0.0f);
    engine.setFrontHead(0.0f);
    engine.setPosition(0.0f);
    engine.setTone(0.0f);
    engine.setDamping(0.0f);
    engine.setDrive(0.0f);
    return engine;
}

//! The pitch over [from, to) seconds, from the spacing of the upward zero crossings
double pitchHz(const std::vector<double> & x, double from, double to)
{
    std::vector<double> crossings;
    for (size_t i = static_cast<size_t>(from * SampleRate) + 1; i < std::min(x.size(), static_cast<size_t>(to * SampleRate)); i++) {
        if (x.at(i - 1) < 0.0 && x.at(i) >= 0.0) {
            crossings.push_back(static_cast<double>(i - 1) + x.at(i - 1) / (x.at(i - 1) - x.at(i)));
        }
    }
    return crossings.size() < 2 ? 0.0 : SampleRate * static_cast<double>(crossings.size() - 1) / (crossings.back() - crossings.front());
}

double peak(const std::vector<double> & x, size_t from = 0, size_t to = SIZE_MAX)
{
    double p = 0.0;
    for (size_t i = from; i < std::min(to, x.size()); i++) {
        p = std::max(p, std::abs(x.at(i)));
    }
    return p;
}

//! Seconds until the signal last reaches 40 dB under its peak
double decaySeconds(const std::vector<double> & x)
{
    const double floor = peak(x) * 0.01;
    for (size_t i = x.size(); i > 0; i--) {
        if (std::abs(x.at(i - 1)) > floor) {
            return static_cast<double>(i) / SampleRate;
        }
    }
    return 0.0;
}

//! Energy of the first difference over the first few milliseconds: what a click adds
double attackHighs(const std::vector<double> & x)
{
    double energy = 0.0;
    for (size_t i = 1; i < static_cast<size_t>(0.005 * SampleRate); i++) {
        energy += std::pow(x.at(i) - x.at(i - 1), 2.0);
    }
    return energy;
}

std::vector<double> renderDevice(KickSynthDevice & device, double seconds, uint8_t velocity = 127, std::optional<double> noteOffAt = {})
{
    device.processDeviceMidiNoteOn(36, velocity);
    const auto frames = static_cast<uint32_t>(seconds * SampleRate);
    std::vector<double> left;
    std::vector<double> block(512 * 2);
    for (uint32_t done = 0; done < frames; done += 512) {
        if (noteOffAt && done >= static_cast<uint32_t>(*noteOffAt * SampleRate)) {
            device.processDeviceMidiNoteOff(36);
            noteOffAt.reset();
        }
        std::ranges::fill(block, 0.0);
        AudioContext context { .buffer = block, .frameCount = 512, .sampleRate = static_cast<uint32_t>(SampleRate) };
        device.processAudio(context);
        for (size_t i = 0; i < 512; i++) {
            left.push_back(block.at(i * 2));
        }
    }
    return left;
}

} // namespace

void KickSynthTest::test_engine_trigger_shouldSoundAndFallSilent()
{
    KickPmEngine engine;
    const auto out = render(engine, 1.0f, 8.0);
    QVERIFY(peak(out, 0, 4410) > 0.1);
    QVERIFY(!engine.isActive());
}

void KickSynthTest::test_engine_hardHit_shouldStartSharperThanASoftOne()
{
    auto hard = bareHead();
    auto soft = bareHead();
    const auto hardHit = render(hard, 1.0f, 1.0);
    const auto softHit = render(soft, 0.2f, 1.0);

    // Both settle to the same pitch, as the tension the hit added goes
    const double settled = pitchHz(hardHit, 0.5, 0.9);
    QVERIFY(std::abs(settled - 55.0) < 1.0);
    QVERIFY(std::abs(pitchHz(softHit, 0.5, 0.9) - settled) < 0.5);

    // A real kick hit hard starts 15-30 % sharp, a soft one only a little
    const double hardStart = pitchHz(hardHit, 0.0, 0.04) / settled;
    const double softStart = pitchHz(softHit, 0.0, 0.04) / settled;
    QVERIFY(hardStart > 1.12);
    QVERIFY(softStart < 1.06);
}

void KickSynthTest::test_engine_damping_shouldShortenTheDrum()
{
    auto open = bareHead();
    auto damped = bareHead();
    damped.setDamping(1.0f);
    QVERIFY(decaySeconds(render(damped, 1.0f, 4.0)) < decaySeconds(render(open, 1.0f, 4.0)) * 0.3);
}

void KickSynthTest::test_engine_frontHead_solid_shouldRingLongerThanPorted()
{
    auto ported = bareHead();
    auto solid = bareHead();
    solid.setFrontHead(1.0f);
    // Late in the tail, once the batter head has died, what is left is the boom of the front
    const auto portedHit = render(ported, 1.0f, 2.0);
    const auto solidHit = render(solid, 1.0f, 2.0);
    const auto late = static_cast<size_t>(0.8 * SampleRate);
    QVERIFY(peak(solidHit, late) > peak(portedHit, late) * 3.0);
}

void KickSynthTest::test_engine_beater_hard_shouldClickMoreThanFelt()
{
    KickPmEngine felt;
    KickPmEngine hard;
    felt.setBeater(0.0f);
    hard.setBeater(1.0f);
    QVERIFY(attackHighs(render(hard, 1.0f, 0.1)) > attackHighs(render(felt, 1.0f, 0.1)) * 2.0);
}

void KickSynthTest::test_engine_retrigger_damped_shouldNotJumpTheTail()
{
    // Once the pillow has pulled the drum down, the next hit takes over what is left of it. A
    // silent hit leaves only that tail, which must fade from where the drum was, not jump back up
    // to where it would be without the pillow.
    KickPmEngine engine;
    engine.setDamping(0.8f);
    const auto first = render(engine, 1.0f, 0.15);
    const double before = peak(first, first.size() - static_cast<size_t>(0.02 * SampleRate));
    engine.trigger(0.0f);
    std::vector<double> tail;
    for (size_t i = 0; i < static_cast<size_t>(0.02 * SampleRate); i++) {
        tail.push_back(static_cast<double>(engine.nextSample()));
    }
    QVERIFY(peak(tail) < before * 1.2);
}

void KickSynthTest::test_presets_every_shouldPeakNearInit()
{
    double initPeak = 0.0;
    for (size_t i = 0; i < KickSynthPresets::presets().size(); i++) {
        KickSynthDevice device { "Kick" };
        device.loadPreset(static_cast<int>(i));
        const double level = 20.0 * std::log10(peak(renderDevice(device, 1.0)));
        if (i == 0) {
            initPeak = level;
            // About where the Snare Synth puts its own
            QVERIFY(std::abs(level + 5.0) < 1.0);
        }
        QVERIFY2(std::abs(level - initPeak) < 1.0, KickSynthPresets::presets().at(i).name.c_str());
    }
}

void KickSynthTest::test_device_noteOff_withoutEnvelope_shouldLetTheDrumRing()
{
    KickSynthDevice held { "Kick" };
    KickSynthDevice released { "Kick" };
    const auto heldHit = renderDevice(held, 1.0);
    const auto releasedHit = renderDevice(released, 1.0, 127, 0.05);
    QCOMPARE(releasedHit, heldHit);
}

void KickSynthTest::test_device_ampEnvelope_shouldShortenTheHit()
{
    KickSynthDevice open { "Kick" };
    KickSynthDevice gated { "Kick" };
    QVERIFY(!gated.ampEnvelopeEnabled());
    gated.setKnob(Xml::xmlKeyAmpSustain(), 0.0f);
    gated.setKnob(Xml::xmlKeyAmpDecay(), 0.2f);
    QVERIFY(gated.ampEnvelopeEnabled());
    QVERIFY(decaySeconds(renderDevice(gated, 2.0)) < decaySeconds(renderDevice(open, 2.0)) * 0.5);
}

void KickSynthTest::test_preview_render_shouldDrawTheHit()
{
    KickSynthDevice device { "Kick" };
    const auto preview = KickSynthPreview::render(device, 200, SampleRate);
    QCOMPARE(preview.peaks.size(), 200);
    QVERIFY(preview.durationSeconds > 0.2);
    QVERIFY(std::abs(preview.audibleSeconds - preview.durationSeconds) < 0.01);
    // And the device in the rack was not struck by it
    QVERIFY(!device.hasActiveAudio());
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::KickSynthTest)
