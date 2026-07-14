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

#include "../../../../../common/constants.hpp"
#include "../../../../../domain/dsp/audio_context.hpp"
#include "../../../../../domain/dsp/cascaded_svf.hpp"
#include "../../../../../domain/dsp/drum/kick_pm_engine.hpp"
#include "../../kick_synth_device.hpp"
#include "../../kick_synth_presets.hpp"
#include "../../kick_synth_preview.hpp"

#include <QString>
#include <QTest>

#include <cmath>
#include <vector>

namespace noteahead {

namespace {

namespace Xml = Constants::NahdXml;

constexpr double SampleRate = 44100.0;

std::vector<double> render(KickPmEngine & engine, float velocity, double seconds, double sampleRate = SampleRate)
{
    engine.setSampleRate(sampleRate);
    engine.trigger(velocity);
    std::vector<double> out;
    for (size_t i = 0; i < static_cast<size_t>(seconds * sampleRate); i++) {
        out.push_back(static_cast<double>(engine.nextSample()));
    }
    return out;
}

double bandEnergyDb(const std::vector<double> & signal, double lowHz, double highHz, double fromSeconds, double toSeconds, double sampleRate = SampleRate)
{
    CascadedSvf highPass {};
    highPass.setMode(CascadedSvf::Mode::HighPass);
    CascadedSvf lowPass {};
    lowPass.setMode(CascadedSvf::Mode::LowPass);
    const auto cutoff = [sampleRate](double hz) {
        const double maxFreq = std::min(20000.0, sampleRate * 0.49);
        return std::clamp(std::log2(hz / 20.0) / std::log2(maxFreq / 20.0), 0.0, 1.0);
    };
    for (auto * filter : { &highPass, &lowPass }) {
        filter->setSampleRate(sampleRate);
        filter->setOrder(4);
        filter->setResonance(0.29);
    }
    highPass.setCutoff(cutoff(lowHz));
    lowPass.setCutoff(cutoff(highHz));
    double energy = 1.0e-20;
    const size_t from = static_cast<size_t>(fromSeconds * sampleRate);
    const size_t to = std::min(signal.size(), static_cast<size_t>(toSeconds * sampleRate));
    for (size_t i = 0; i < signal.size(); i++) {
        const double filtered = lowPass.process(highPass.process(signal[i]));
        if (i >= from && i < to) {
            energy += filtered * filtered;
        }
    }
    return 10.0 * std::log10(energy);
}

double windowRmsDbRelativeToPeak(const std::vector<double> & signal, double peakValue, double fromSeconds, double toSeconds)
{
    const size_t from = static_cast<size_t>(fromSeconds * SampleRate);
    const size_t to = std::min(signal.size(), static_cast<size_t>(toSeconds * SampleRate));
    double sumSq = 1.0e-20;
    size_t count = 0;
    for (size_t i = from; i < to; i++) {
        sumSq += signal.at(i) * signal.at(i);
        count++;
    }
    const double rms = std::sqrt(sumSq / static_cast<double>(std::max<size_t>(1, count)));
    return 20.0 * std::log10(std::max(rms, 1.0e-12) / std::max(peakValue, 1.0e-12));
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

void KickSynthTest::test_engine_damping_shouldDecayConcaveUpNotCliffDrop()
{
    KickPmEngine engine {};
    engine.setTune(45.0f);
    engine.setDecay(1.5f);
    engine.setDamping(0.85f);
    engine.setFrontHead(0.6f);
    engine.setClick(0.0f);
    engine.setRing(0.0f);

    const auto hit = render(engine, 1.0f, 0.6);
    const double pk = peak(hit);
    const double earlyDb = windowRmsDbRelativeToPeak(hit, pk, 0.010, 0.030);
    const double midDb = windowRmsDbRelativeToPeak(hit, pk, 0.050, 0.100);
    const double tailDb = windowRmsDbRelativeToPeak(hit, pk, 0.150, 0.300);

    QVERIFY2(midDb < earlyDb - 6.0, qPrintable(QString::number(earlyDb - midDb)));
    QVERIFY2(tailDb > -55.0, qPrintable(QString::number(tailDb)));
}

void KickSynthTest::test_engine_velocity_shouldBrightenHardHits()
{
    KickPmEngine soft {};
    KickPmEngine hard {};
    for (auto * engine : { &soft, &hard }) {
        engine->setClick(0.5f);
        engine->setRing(0.3f);
        engine->setTone(0.5f);
        engine->setBeater(0.5f);
        engine->setDrive(0.0f);
    }

    const auto softHit = render(soft, 0.25f, 0.25);
    const auto hardHit = render(hard, 1.0f, 0.25);

    const auto brightness = [](const std::vector<double> & hit) {
        return bandEnergyDb(hit, 1500.0, 8000.0, 0.0, 0.25) - bandEnergyDb(hit, 30.0, 120.0, 0.0, 0.25);
    };
    const double softBrightness = brightness(softHit);
    const double hardBrightness = brightness(hardHit);

    QVERIFY2(hardBrightness > softBrightness + 4.0, qPrintable(QString::number(hardBrightness - softBrightness)));
}

void KickSynthTest::test_engine_upperModes_highTone_shouldRingInTail()
{
    KickPmEngine lowTone {};
    KickPmEngine highTone {};
    for (auto * engine : { &lowTone, &highTone }) {
        engine->setTune(85.0f);
        engine->setDecay(1.2f);
        engine->setPosition(0.4f);
        engine->setDamping(0.1f);
        engine->setClick(0.0f);
        engine->setRing(0.0f);
        engine->setFrontHead(0.0f);
    }
    lowTone.setTone(0.0f);
    highTone.setTone(0.9f);

    const auto lowHit = render(lowTone, 1.0f, 0.35);
    const auto highHit = render(highTone, 1.0f, 0.35);

    const double highTail = bandEnergyDb(highHit, 220.0, 600.0, 0.10, 0.30);
    const double lowTail = bandEnergyDb(lowHit, 220.0, 600.0, 0.10, 0.30);
    QVERIFY2(highTail > lowTail + 8.0, qPrintable(QString::number(highTail - lowTail)));
}

void KickSynthTest::test_engine_sampleRate_shouldPreserveNoiseBalance()
{
    KickPmEngine e44 {};
    KickPmEngine e96 {};
    for (auto * engine : { &e44, &e96 }) {
        engine->setClick(0.5f);
        engine->setRing(0.5f);
        engine->setDrive(0.0f);
    }

    const auto hit44 = render(e44, 1.0f, 0.25, 44100.0);
    const auto hit96 = render(e96, 1.0f, 0.25, 96000.0);

    const double balance44 = bandEnergyDb(hit44, 1000.0, 6000.0, 0.0, 0.25, 44100.0) - bandEnergyDb(hit44, 30.0, 150.0, 0.0, 0.25, 44100.0);
    const double balance96 = bandEnergyDb(hit96, 1000.0, 6000.0, 0.0, 0.25, 96000.0) - bandEnergyDb(hit96, 30.0, 150.0, 0.0, 0.25, 96000.0);
    QVERIFY2(std::abs(balance96 - balance44) < 1.5, qPrintable(QString::number(balance96 - balance44)));
}

void KickSynthTest::test_engine_pitchDrop_shouldSettleQuickly()
{
    auto engine = bareHead();
    engine.setTune(70.0f);
    engine.setDecay(2.5f);
    engine.setPitchDrop(0.7f);
    engine.setDamping(0.0f);

    const auto hit = render(engine, 1.0f, 0.5);
    const double startPitch = pitchHz(hit, 0.0, 0.04);
    const double latePitch = pitchHz(hit, 0.20, 0.35);

    QVERIFY2(startPitch > 70.0 * 1.15, qPrintable(QString::number(startPitch)));
    QVERIFY2(latePitch < 70.0 * 1.06, qPrintable(QString::number(latePitch)));
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
