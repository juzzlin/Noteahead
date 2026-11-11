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

#include "kick_909_test.hpp"

#include "../../../../../common/constants.hpp"
#include "../../../../../domain/dsp/audio_context.hpp"
#include "../../../../../domain/dsp/drum/kick_909_engine.hpp"
#include "../../kick_909_device.hpp"
#include "../../kick_909_presets.hpp"
#include "../../kick_909_preview.hpp"

#include <QTest>

#include <cmath>
#include <optional>
#include <vector>

namespace noteahead {

namespace {

namespace Xml = Constants::NahdXml;

constexpr double SampleRate = 44100.0;

std::vector<double> render(Kick909Engine & engine, double seconds, float velocity = 1.0f)
{
    engine.setSampleRate(SampleRate);
    engine.trigger(velocity);
    std::vector<double> out;
    for (size_t i = 0; i < static_cast<size_t>(seconds * SampleRate); i++) {
        out.push_back(static_cast<double>(engine.nextSample()));
    }
    return out;
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

//! Energy of the second difference over [from, to) seconds: what lies well above the drum
double highs(const std::vector<double> & x, double from, double to)
{
    double energy = 0.0;
    for (size_t i = static_cast<size_t>(from * SampleRate) + 2; i < static_cast<size_t>(to * SampleRate); i++) {
        const double d = x.at(i) - 2.0 * x.at(i - 1) + x.at(i - 2);
        energy += d * d;
    }
    return energy;
}

std::vector<double> renderDevice(Kick909Device & device, double seconds, uint8_t note = 36, std::optional<double> noteOffAt = {})
{
    device.processDeviceMidiNoteOn(note, 127);
    const auto frames = static_cast<uint32_t>(seconds * SampleRate);
    std::vector<double> left;
    std::vector<double> block(512 * 2);
    for (uint32_t done = 0; done < frames; done += 512) {
        if (noteOffAt && done >= static_cast<uint32_t>(*noteOffAt * SampleRate)) {
            device.processDeviceMidiNoteOff(note);
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

void Kick909Test::test_engine_init_shouldSettleAtTheRd9Pitch()
{
    // An RD-9 with every knob at noon is still 2 Hz above it at 300 ms, its sweep is that long
    Kick909Engine engine;
    engine.setAttack(0.0f);
    engine.setDecay(1.0f);
    const auto hit = render(engine, 0.8);
    QVERIFY2(std::abs(pitchHz(hit, 0.35, 0.7) - Kick909Engine::DefaultPitchHz) < 2.0, qPrintable(QString::number(pitchHz(hit, 0.35, 0.7))));
}

void Kick909Test::test_engine_sweep_shouldStartAsTheRd9Does()
{
    // Without Attack's noise, which crosses zero as well
    Kick909Engine engine;
    engine.setAttack(0.0f);
    const auto hit = render(engine, 0.1);
    // The RD-9 at noon: 270 Hz over its first 6 ms, 284 Hz to 12 ms
    const double start = pitchHz(hit, 0.0, 0.012);
    QVERIFY2(start > 250.0 && start < 300.0, qPrintable(QString::number(start)));
}

void Kick909Test::test_engine_tune_shouldLengthenTheSweep()
{
    // The 909 recordings at 20 to 30 ms: about 70 Hz at Tune 0, 150 Hz at Tune 10, around noon
    // scaled to the RD-9's own
    Kick909Engine low;
    low.setTune(0.0f);
    low.setAttack(0.0f);
    Kick909Engine high;
    high.setTune(1.0f);
    high.setAttack(0.0f);
    const double lowHz = pitchHz(render(low, 0.1), 0.015, 0.035);
    const double highHz = pitchHz(render(high, 0.1), 0.015, 0.035);
    QVERIFY2(highHz / lowHz > 1.5, qPrintable(QString::number(lowHz) + " " + QString::number(highHz)));
}

void Kick909Test::test_engine_decay_shouldMatchTheRecordings()
{
    Kick909Engine shortest;
    shortest.setDecay(0.0f);
    Kick909Engine longest;
    longest.setDecay(1.0f);
    // -40 dB at about 0.1 s and 0.45 s
    const double shortSeconds = decaySeconds(render(shortest, 1.0));
    const double longSeconds = decaySeconds(render(longest, 1.0));
    QVERIFY2(shortSeconds > 0.08 && shortSeconds < 0.14, qPrintable(QString::number(shortSeconds)));
    QVERIFY2(longSeconds > 0.36 && longSeconds < 0.56, qPrintable(QString::number(longSeconds)));
}

void Kick909Test::test_engine_attack_shouldAddNoiseAboveOneKilohertz()
{
    Kick909Engine dull;
    dull.setAttack(0.0f);
    Kick909Engine sharp;
    sharp.setAttack(1.0f);
    const double dullHighs = highs(render(dull, 0.05), 0.004, 0.025);
    const double sharpHighs = highs(render(sharp, 0.05), 0.004, 0.025);
    QVERIFY(10.0 * std::log10(sharpHighs / dullHighs) > 3.0);
}

void Kick909Test::test_engine_attack_full_shouldRaiseTheClick()
{
    // The click as the oscillator starts, 1 ms in: Attack at full goes 12 dB past the recordings
    Kick909Engine dull;
    dull.setAttack(0.0f);
    Kick909Engine sharp;
    sharp.setAttack(1.0f);
    const double dullClick = highs(render(dull, 0.01), 0.0005, 0.004);
    const double sharpClick = highs(render(sharp, 0.01), 0.0005, 0.004);
    QVERIFY2(10.0 * std::log10(sharpClick / dullClick) > 8.0, qPrintable(QString::number(10.0 * std::log10(sharpClick / dullClick))));
}

void Kick909Test::test_engine_pitch_shouldMoveTheWholeSweep()
{
    // The RD-9 at Pitch 8: 1.79 times noon, the sweep and its end alike
    Kick909Engine noon;
    noon.setAttack(0.0f);
    Kick909Engine high;
    high.setAttack(0.0f);
    high.setPitch(0.8f);
    const auto noonHit = render(noon, 0.5);
    const auto highHit = render(high, 0.5);
    QVERIFY(std::abs(pitchHz(highHit, 0.15, 0.4) / pitchHz(noonHit, 0.15, 0.4) - 1.79) < 0.05);
    QVERIFY(std::abs(pitchHz(highHit, 0.015, 0.035) / pitchHz(noonHit, 0.015, 0.035) - 1.79) < 0.15);
}

void Kick909Test::test_engine_pitchDepth_zero_shouldFallToZeroHertz()
{
    // As on the RD-9: the sweep runs on down past the drum, to a standstill
    Kick909Engine engine;
    engine.setAttack(0.0f);
    engine.setPitchDepth(0.0f);
    const auto hit = render(engine, 0.5);
    // The RD-9 at Pitch Depth 0: about 100 Hz at 6 to 12 ms, 80 Hz to 25 ms, 60 Hz to 50 ms
    const double early = pitchHz(hit, 0.006, 0.05);
    QVERIFY2(early > 45.0 && early < 110.0, qPrintable(QString::number(early)));
    QVERIFY2(pitchHz(hit, 0.2, 0.5) < 10.0, qPrintable(QString::number(pitchHz(hit, 0.2, 0.5))));
}

void Kick909Test::test_engine_pitchDepth_full_shouldRaiseTheEnd()
{
    // The RD-9 at Pitch Depth 10 ends 2.11 times as high as at noon
    Kick909Engine engine;
    engine.setAttack(0.0f);
    engine.setPitchDepth(1.0f);
    QVERIFY(std::abs(pitchHz(render(engine, 0.5), 0.2, 0.45) - 2.114 * Kick909Engine::DefaultPitchHz) < 4.0);
}

void Kick909Test::test_engine_drive_full_shouldKeepThePeak()
{
    Kick909Engine clean;
    Kick909Engine driven;
    driven.setDrive(1.0f);
    const double cleanDb = 20.0 * std::log10(peak(render(clean, 0.3)));
    const double drivenDb = 20.0 * std::log10(peak(render(driven, 0.3)));
    QVERIFY2(std::abs(drivenDb - cleanDb) < 3.0, qPrintable(QString::number(drivenDb - cleanDb)));
}

void Kick909Test::test_engine_trigger_shouldFallSilent()
{
    Kick909Engine engine;
    engine.setDecay(1.0f);
    const auto hit = render(engine, 2.0);
    QVERIFY(peak(hit) > 0.5);
    QVERIFY(!engine.isActive());
    for (const auto sample : hit) {
        QVERIFY(std::isfinite(sample));
    }
}

void Kick909Test::test_presets_every_shouldPeakNearInit()
{
    double initPeak = 0.0;
    for (size_t i = 0; i < Kick909Presets::presets().size(); i++) {
        Kick909Device device { "Kick" };
        device.loadPreset(static_cast<int>(i));
        const double level = 20.0 * std::log10(peak(renderDevice(device, 1.0)));
        if (i == 0) {
            initPeak = level;
            // Where the Kick Synth puts its own
            QVERIFY2(std::abs(level + 5.0) < 1.0, qPrintable(QString::number(level)));
        }
        QVERIFY2(std::abs(level - initPeak) < 1.0, qPrintable(QString::fromStdString(Kick909Presets::presets().at(i).name) + " " + QString::number(level - initPeak)));
    }
}

void Kick909Test::test_device_chromatic_shouldMovePitchWithTheNote()
{
    Kick909Device device { "Kick" };
    device.setChromatic(true);
    device.setKnob(Xml::xmlKeyDecay(), 1.0f);
    QVERIFY(std::abs(pitchHz(renderDevice(device, 0.8, 48), 0.35, 0.7) - 2.0 * Kick909Engine::DefaultPitchHz) < 4.0);
    device.setChromatic(false);
    QVERIFY(std::abs(pitchHz(renderDevice(device, 0.8, 48), 0.35, 0.7) - Kick909Engine::DefaultPitchHz) < 2.0);
}

void Kick909Test::test_device_ampEnvelope_shouldShortenTheHit()
{
    Kick909Device open { "Kick" };
    Kick909Device gated { "Kick" };
    QVERIFY(!gated.ampEnvelopeEnabled());
    gated.setKnob(Xml::xmlKeyAmpSustain(), 0.0f);
    gated.setKnob(Xml::xmlKeyAmpDecay(), 0.1f);
    QVERIFY(gated.ampEnvelopeEnabled());
    QVERIFY(decaySeconds(renderDevice(gated, 1.0)) < decaySeconds(renderDevice(open, 1.0)) * 0.5);
}

void Kick909Test::test_preview_render_shouldDrawTheHit()
{
    Kick909Device device { "Kick" };
    const auto preview = Kick909Preview::render(device, 200, SampleRate);
    QCOMPARE(preview.peaks.size(), 200);
    QVERIFY(preview.durationSeconds > 0.2);
    QVERIFY(std::abs(preview.audibleSeconds - preview.durationSeconds) < 0.01);
    QVERIFY(!device.hasActiveAudio());
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::Kick909Test)
