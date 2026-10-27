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

#include "noise_fx_test.hpp"

#include "../../common/constants.hpp"
#include "../../domain/devices/noise_fx_device.hpp"
#include "../../domain/devices/noise_fx_presets.hpp"
#include "../../domain/dsp/audio_context.hpp"
#include "../../domain/dsp/lfo.hpp"

#include <QTest>

#include <algorithm>
#include <cmath>
#include <set>
#include <vector>

namespace noteahead {

namespace {

constexpr uint32_t SampleRate = 48000;
constexpr uint32_t BlockSize = 512;

std::vector<double> render(NoiseFxDevice & device, uint32_t frames, double bpm = 120.0)
{
    std::vector<double> buffer(frames * 2, 0.0);
    for (uint32_t offset = 0; offset < frames; offset += BlockSize) {
        const uint32_t count = std::min(BlockSize, frames - offset);
        AudioContext context {
            .buffer = std::span<double>(buffer).subspan(offset * 2, count * 2),
            .frameCount = count,
            .sampleRate = SampleRate,
            .bpm = bpm
        };
        device.processAudio(context);
    }
    return buffer;
}

//! Frames rendered until the device goes quiet, capped at @p limit.
uint32_t framesUntilInactive(NoiseFxDevice & device, uint32_t limit, double bpm = 120.0)
{
    uint32_t frames = 0;
    while (device.hasActiveAudio() && frames < limit) {
        render(device, BlockSize, bpm);
        frames += BlockSize;
    }
    return frames;
}

double rms(const std::vector<double> & buffer, size_t firstFrame, size_t frameCount)
{
    double sum = 0.0;
    for (size_t frame = firstFrame; frame < firstFrame + frameCount; frame++) {
        sum += buffer.at(frame * 2) * buffer.at(frame * 2);
    }
    return std::sqrt(sum / static_cast<double>(frameCount));
}

//! RMS of the first difference, which weighs the top of the spectrum: a brightness measure.
double differenceRms(const std::vector<double> & buffer, size_t firstFrame, size_t frameCount)
{
    double sum = 0.0;
    for (size_t frame = firstFrame + 1; frame < firstFrame + frameCount; frame++) {
        const double difference = buffer.at(frame * 2) - buffer.at((frame - 1) * 2);
        sum += difference * difference;
    }
    return std::sqrt(sum / static_cast<double>(frameCount));
}

//! Pitch of the left channel over the given frames, from its rising zero crossings.
double frequency(const std::vector<double> & buffer, size_t firstFrame, size_t frameCount)
{
    int crossings = 0;
    for (size_t frame = firstFrame + 1; frame < firstFrame + frameCount; frame++) {
        if (buffer.at((frame - 1) * 2) < 0.0 && buffer.at(frame * 2) >= 0.0) {
            crossings++;
        }
    }
    return crossings * static_cast<double>(SampleRate) / static_cast<double>(frameCount);
}

double correlation(const std::vector<double> & buffer)
{
    double leftRight = 0.0;
    double leftLeft = 0.0;
    double rightRight = 0.0;
    for (size_t frame = 0; frame < buffer.size() / 2; frame++) {
        leftRight += buffer.at(frame * 2) * buffer.at(frame * 2 + 1);
        leftLeft += buffer.at(frame * 2) * buffer.at(frame * 2);
        rightRight += buffer.at(frame * 2 + 1) * buffer.at(frame * 2 + 1);
    }
    return leftRight / std::sqrt(leftLeft * rightRight);
}

//! A flat, fast patch: nothing swept, the filter open, a short release.
void makeNeutral(NoiseFxDevice & device)
{
    device.setPlayToEnd(false);
    device.setSweepTarget(0, 0, NoiseFxDevice::Destination::None);
    device.setCutoff(1.0f);
    device.setResonance(0.0f);
    device.setAmpRelease(0.0f);
    device.setVelocitySensitivity(0.0f);
}

} // namespace

void NoiseFxTest::test_noiseFx_noteOn_shouldProduceAudio()
{
    NoiseFxDevice device { "Noise FX" };
    QVERIFY(!device.hasActiveAudio());

    device.processDeviceMidiNoteOn(60, 100);
    QVERIFY(device.hasActiveAudio());

    const auto buffer = render(device, SampleRate);
    QVERIFY(rms(buffer, 0, SampleRate) > 0.001);
}

void NoiseFxTest::test_noiseFx_noteOn_untriggered_shouldBeSilent()
{
    NoiseFxDevice device { "Noise FX" };
    const auto buffer = render(device, 4096);
    for (const auto sample : buffer) {
        QCOMPARE(sample, 0.0);
    }
}

void NoiseFxTest::test_noiseFx_playToEnd_shouldReleaseAtSweepEnd_data()
{
    QTest::addColumn<double>("bpm");
    QTest::addColumn<int>("syncLength");
    QTest::addColumn<double>("expectedSeconds");

    // Index 4 is one bar, index 5 two.
    QTest::newRow("120 bpm, 1 bar") << 120.0 << 4 << 2.0;
    QTest::newRow("240 bpm, 1 bar") << 240.0 << 4 << 1.0;
    QTest::newRow("120 bpm, 2 bars") << 120.0 << 5 << 4.0;
}

void NoiseFxTest::test_noiseFx_playToEnd_shouldReleaseAtSweepEnd()
{
    QFETCH(double, bpm);
    QFETCH(int, syncLength);
    QFETCH(double, expectedSeconds);

    NoiseFxDevice device { "Noise FX" };
    makeNeutral(device);
    device.setPlayToEnd(true);
    device.setSweepParameterValue(0, Constants::NahdXml::xmlKeySyncLength(), static_cast<float>(syncLength));

    device.processDeviceMidiNoteOn(60, 100);
    const auto frames = framesUntilInactive(device, SampleRate * 20, bpm);
    const double seconds = static_cast<double>(frames) / SampleRate;
    // The release is 5 ms and the device is polled per block.
    QVERIFY2(std::abs(seconds - expectedSeconds) < 0.03, qPrintable(QString::number(seconds)));
}

void NoiseFxTest::test_noiseFx_playToEnd_noteOff_shouldBeIgnored()
{
    NoiseFxDevice device { "Noise FX" };
    makeNeutral(device);
    device.setPlayToEnd(true);
    device.setSweepParameterValue(0, Constants::NahdXml::xmlKeySyncLength(), 4.0f);

    device.processDeviceMidiNoteOn(60, 100);
    render(device, BlockSize);
    device.processDeviceMidiNoteOff(60);
    const auto buffer = render(device, SampleRate);
    QVERIFY(device.hasActiveAudio());
    QVERIFY(rms(buffer, SampleRate - 4800, 4800) > 0.001);
}

void NoiseFxTest::test_noiseFx_playToEnd_off_noteOff_shouldRelease()
{
    NoiseFxDevice device { "Noise FX" };
    makeNeutral(device);

    device.processDeviceMidiNoteOn(60, 100);
    render(device, BlockSize);
    device.processDeviceMidiNoteOff(60);
    QVERIFY(framesUntilInactive(device, SampleRate) < SampleRate / 10);
}

void NoiseFxTest::test_noiseFx_playToEnd_loopingSweep_noteOff_shouldRelease()
{
    NoiseFxDevice device { "Noise FX" };
    makeNeutral(device);
    device.setPlayToEnd(true);
    device.setSweepParameterValue(0, Constants::NahdXml::xmlKeyMode(), static_cast<float>(SweepEnvelope::Mode::Loop));

    device.processDeviceMidiNoteOn(60, 100);
    render(device, SampleRate * 3);
    QVERIFY(device.hasActiveAudio());

    device.processDeviceMidiNoteOff(60);
    QVERIFY(framesUntilInactive(device, SampleRate) < SampleRate / 10);
}

void NoiseFxTest::test_noiseFx_chromatic_shouldScaleTimesWithNote_data()
{
    QTest::addColumn<bool>("chromatic");
    QTest::addColumn<int>("note");
    QTest::addColumn<double>("expectedSeconds");

    // A sweep length of 0.5 is 0.01 * 6000^0.5 = 0.775 s.
    QTest::newRow("off, octave up") << false << 72 << 0.775;
    QTest::newRow("on, middle C") << true << 60 << 0.775;
    QTest::newRow("on, octave up") << true << 72 << 0.387;
    QTest::newRow("on, octave down") << true << 48 << 1.549;
}

void NoiseFxTest::test_noiseFx_chromatic_shouldScaleTimesWithNote()
{
    QFETCH(bool, chromatic);
    QFETCH(int, note);
    QFETCH(double, expectedSeconds);

    NoiseFxDevice device { "Noise FX" };
    makeNeutral(device);
    device.setChromatic(chromatic);
    device.setPlayToEnd(true);
    device.setSweepParameterValue(0, Constants::NahdXml::xmlKeySync(), 0.0f);
    device.setSweepParameterValue(0, Constants::NahdXml::xmlKeyLength(), 0.5f);

    device.processDeviceMidiNoteOn(static_cast<uint8_t>(note), 100);
    const auto frames = framesUntilInactive(device, SampleRate * 5);
    const double seconds = static_cast<double>(frames) / SampleRate;
    QVERIFY2(std::abs(seconds - expectedSeconds) < 0.03, qPrintable(QString::number(seconds)));
}

void NoiseFxTest::test_noiseFx_chromatic_shouldScaleCutoffWithNote_data()
{
    QTest::addColumn<bool>("chromatic");
    QTest::addColumn<double>("minRatio");
    QTest::addColumn<double>("maxRatio");

    QTest::newRow("off") << false << 0.8 << 1.25;
    QTest::newRow("on") << true << 1.6 << 2.5;
}

void NoiseFxTest::test_noiseFx_chromatic_shouldScaleCutoffWithNote()
{
    QFETCH(bool, chromatic);
    QFETCH(double, minRatio);
    QFETCH(double, maxRatio);

    // Brightness an octave up over brightness at middle C, through a low-pass at 160 Hz.
    const auto brightness = [chromatic](uint8_t note) {
        NoiseFxDevice device { "Noise FX" };
        makeNeutral(device);
        device.setChromatic(chromatic);
        device.setCutoff(0.3f);
        device.processDeviceMidiNoteOn(note, 100);
        const auto buffer = render(device, SampleRate / 2);
        return differenceRms(buffer, 4800, 9600) / rms(buffer, 4800, 9600);
    };
    const double ratio = brightness(72) / brightness(60);
    QVERIFY2(ratio > minRatio && ratio < maxRatio, qPrintable(QString::number(ratio)));
}

void NoiseFxTest::test_noiseFx_sweepCutoff_shouldOpenFilter()
{
    // The default patch: Sweep A opens a low-pass over four bars.
    NoiseFxDevice device { "Noise FX" };
    device.setSweepParameterValue(0, Constants::NahdXml::xmlKeySyncLength(), 4.0f);

    device.processDeviceMidiNoteOn(60, 127);
    const auto buffer = render(device, SampleRate * 2);

    const size_t window = 4800;
    const double brightnessStart = differenceRms(buffer, 0, window) / rms(buffer, 0, window);
    const double brightnessEnd = differenceRms(buffer, SampleRate * 2 - window - 1000, window) / rms(buffer, SampleRate * 2 - window - 1000, window);
    QVERIFY2(brightnessEnd > brightnessStart * 10.0, qPrintable(QString { "%1 -> %2" }.arg(brightnessStart).arg(brightnessEnd)));
}

void NoiseFxTest::test_noiseFx_sweepLevel_positiveDepth_shouldFadeIn()
{
    NoiseFxDevice device { "Noise FX" };
    makeNeutral(device);
    device.setSweepTarget(0, 0, NoiseFxDevice::Destination::Level);
    device.setSweepDepth(0, 0, 1.0f);
    device.setSweepParameterValue(0, Constants::NahdXml::xmlKeyCurve(), 0.5f);
    device.setSweepParameterValue(0, Constants::NahdXml::xmlKeySyncLength(), 4.0f);

    device.processDeviceMidiNoteOn(60, 127);
    const auto buffer = render(device, SampleRate * 2);
    const double start = rms(buffer, 0, 2400);
    const double end = rms(buffer, SampleRate * 2 - 4800, 2400);
    QVERIFY2(start < end * 0.05, qPrintable(QString { "%1 -> %2" }.arg(start).arg(end)));
}

void NoiseFxTest::test_noiseFx_resetAudio_shouldRenderIdentically()
{
    NoiseFxDevice device { "Noise FX" };
    device.setCrackle(0.5f);

    device.processDeviceMidiNoteOn(60, 100);
    const auto first = render(device, 8192);
    device.resetAudio();
    device.processDeviceMidiNoteOn(60, 100);
    const auto second = render(device, 8192);
    QCOMPARE(first, second);
}

void NoiseFxTest::test_noiseFx_width_zero_shouldPlaySameNoiseOnBothChannels()
{
    NoiseFxDevice device { "Noise FX" };
    makeNeutral(device);
    device.setWidth(0.0f);

    device.processDeviceMidiNoteOn(60, 127);
    const auto buffer = render(device, 8192);
    for (size_t frame = 0; frame < buffer.size() / 2; frame++) {
        QVERIFY(std::abs(buffer.at(frame * 2) - buffer.at(frame * 2 + 1)) < 1e-12);
    }
}

void NoiseFxTest::test_noiseFx_width_full_shouldDecorrelateChannels()
{
    NoiseFxDevice device { "Noise FX" };
    makeNeutral(device);
    device.setWidth(1.0f);

    device.processDeviceMidiNoteOn(60, 127);
    const auto buffer = render(device, SampleRate);
    const double leftRight = correlation(buffer);
    QVERIFY2(std::abs(leftRight) < 0.05, qPrintable(QString::number(leftRight)));

    // Width spreads the noise without changing its level.
    NoiseFxDevice narrow { "Noise FX" };
    makeNeutral(narrow);
    narrow.setWidth(0.0f);
    narrow.processDeviceMidiNoteOn(60, 127);
    const double levelDb = 20.0 * std::log10(rms(buffer, 0, SampleRate) / rms(render(narrow, SampleRate), 0, SampleRate));
    QVERIFY2(std::abs(levelDb) < 0.3, qPrintable(QString::number(levelDb)));
}

void NoiseFxTest::test_noiseFx_tone_shouldPlayNotePitch_data()
{
    QTest::addColumn<int>("note");
    QTest::addColumn<float>("pitch");
    QTest::addColumn<double>("expectedHz");

    QTest::newRow("A4") << 69 << 0.5f << 440.0;
    QTest::newRow("A3") << 57 << 0.5f << 220.0;
    QTest::newRow("A4 + octave") << 69 << 0.75f << 880.0;
    QTest::newRow("A4 - 2 octaves") << 69 << 0.0f << 110.0;
}

void NoiseFxTest::test_noiseFx_tone_shouldPlayNotePitch()
{
    QFETCH(int, note);
    QFETCH(float, pitch);
    QFETCH(double, expectedHz);

    NoiseFxDevice device { "Noise FX" };
    makeNeutral(device);
    device.setBlend(1.0f);
    device.setPitch(pitch);

    device.processDeviceMidiNoteOn(static_cast<uint8_t>(note), 127);
    const auto buffer = render(device, SampleRate);
    const double hz = frequency(buffer, SampleRate / 10, SampleRate * 9 / 10);
    QVERIFY2(std::abs(hz - expectedHz) < expectedHz * 0.01, qPrintable(QString::number(hz)));
}

void NoiseFxTest::test_noiseFx_sweepPitch_shouldRaisePitch()
{
    NoiseFxDevice device { "Noise FX" };
    makeNeutral(device);
    device.setBlend(1.0f);
    // Half depth over one bar: 24 semitones up by the end.
    device.setSweepTarget(0, 0, NoiseFxDevice::Destination::Pitch);
    device.setSweepDepth(0, 0, 0.75f);
    device.setSweepParameterValue(0, Constants::NahdXml::xmlKeySyncLength(), 4.0f);

    device.processDeviceMidiNoteOn(69, 127);
    const auto buffer = render(device, SampleRate * 3);
    const double startHz = frequency(buffer, 0, SampleRate / 20);
    const double endHz = frequency(buffer, SampleRate * 2 + SampleRate / 10, SampleRate / 2);
    QVERIFY2(std::abs(startHz - 440.0) < 40.0, qPrintable(QString::number(startHz)));
    QVERIFY2(std::abs(endHz - 1760.0) < 1760.0 * 0.01, qPrintable(QString::number(endHz)));
}

void NoiseFxTest::test_noiseFx_ringMod_shouldKeepToneLevel()
{
    const auto toneRms = [](float ringMod) {
        NoiseFxDevice device { "Noise FX" };
        makeNeutral(device);
        device.setWidth(0.0f);
        device.setBlend(1.0f);
        device.setRingMod(ringMod);
        // Ring modulation spreads the tone over the whole band, which a low-pass would trim: a
        // notch parked at the bottom passes all of it.
        device.setFilterType(NoiseFxDevice::FilterType::Notch);
        device.setCutoff(0.0f);
        device.processDeviceMidiNoteOn(69, 127);
        return rms(render(device, SampleRate), 0, SampleRate);
    };
    const double levelDb = 20.0 * std::log10(toneRms(1.0f) / toneRms(0.0f));
    QVERIFY2(std::abs(levelDb) < 1.0, qPrintable(QString::number(levelDb)));
}

void NoiseFxTest::test_noiseFx_gate_shouldChopAtDivision()
{
    NoiseFxDevice device { "Noise FX" };
    makeNeutral(device);
    device.setGateParameterValue(Constants::NahdXml::xmlKeyDepth(), 1.0f);
    device.setGateParameterValue(Constants::NahdXml::xmlKeySmoothing(), 0.0f);
    // An eighth at 120 bpm: open for the first 125 ms of every 250.
    device.setGateParameterValue(Constants::NahdXml::xmlKeyRate(), 3.0f);
    device.setGateParameterValue(Constants::NahdXml::xmlKeyWidth(), 0.5f);

    device.processDeviceMidiNoteOn(60, 127);
    const auto buffer = render(device, SampleRate);
    for (int cycle = 0; cycle < 4; cycle++) {
        const size_t start = static_cast<size_t>(cycle) * SampleRate / 4;
        const double open = rms(buffer, start + 480, 4800);
        const double closed = rms(buffer, start + SampleRate / 8 + 480, 4800);
        QVERIFY2(closed < open * 0.01, qPrintable(QString { "cycle %1: %2 / %3" }.arg(cycle).arg(closed).arg(open)));
    }
}

void NoiseFxTest::test_noiseFx_lfoLevel_shouldModulateLevel()
{
    NoiseFxDevice device { "Noise FX" };
    makeNeutral(device);
    device.setBlend(1.0f);
    device.setLfoTarget(0, NoiseFxDevice::Destination::Level);
    device.setLfoDepth(0, 1.0f);
    // Synced at the top of the knob: a 1/32 bar cycle, 62.5 ms at 120 bpm.
    device.setLfoParameterValue(Constants::NahdXml::xmlKeySync(), 1.0f);
    device.setLfoParameterValue(Constants::NahdXml::xmlKeyRate(), 1.0f);

    device.processDeviceMidiNoteOn(69, 127);
    const auto buffer = render(device, SampleRate / 2);
    double quietest = 1.0;
    double loudest = 0.0;
    for (size_t start = 0; start + 240 < SampleRate / 2; start += 240) {
        const double level = rms(buffer, start, 240);
        quietest = std::min(quietest, level);
        loudest = std::max(loudest, level);
    }
    QVERIFY2(quietest < loudest * 0.1, qPrintable(QString { "%1 / %2" }.arg(quietest).arg(loudest)));
}

namespace {

//! The largest jump of a tone's peak level from one window to the next, as a share of the loudest:
//! how big a step the LFO moving its level takes. Each 200-frame window holds at least one peak of
//! the 262 Hz tone.
double largestLevelStep(int lfoWaveform)
{
    NoiseFxDevice device { "Noise FX" };
    makeNeutral(device);
    device.setBlend(1.0f);
    device.setLfoParameterValue(Constants::NahdXml::xmlKeyWaveform(), static_cast<float>(lfoWaveform));
    device.setLfoTarget(0, NoiseFxDevice::Destination::Level);
    device.setLfoDepth(0, 1.0f);
    // Synced: four cycles a bar, every half a second at 120 bpm
    device.setLfoParameterValue(Constants::NahdXml::xmlKeySync(), 1.0f);
    const auto last = static_cast<double>(NoiseFxDevice::lfoSyncBars().size() - 1);
    device.setLfoParameterValue(Constants::NahdXml::xmlKeyRate(), static_cast<float>(4.0 / last));

    device.processDeviceMidiNoteOn(60, 127);
    const auto buffer = render(device, SampleRate * 4);
    std::vector<double> peaks;
    for (size_t start = 0; start + 200 <= SampleRate * 4; start += 200) {
        double peak = 0.0;
        for (size_t frame = start; frame < start + 200; frame++) {
            peak = std::max(peak, std::abs(buffer.at(frame * 2)));
        }
        peaks.push_back(peak);
    }
    double largestStep = 0.0;
    for (size_t i = 1; i < peaks.size(); i++) {
        largestStep = std::max(largestStep, std::abs(peaks.at(i) - peaks.at(i - 1)));
    }
    return largestStep / std::ranges::max(peaks);
}

} // namespace

void NoiseFxTest::test_noiseFx_lfoLevel_smoothRandom_shouldNotStep()
{
    // A glide of a whole half second moves the level by about 1 % per window at the most
    const double step = largestLevelStep(NoiseFxDevice::SmoothRandomWaveform);
    QVERIFY2(step < 0.03, qPrintable(QString::number(step)));
}

void NoiseFxTest::test_noiseFx_lfoLevel_random_shouldStep()
{
    const double step = largestLevelStep(static_cast<int>(Lfo::Waveform::Random));
    QVERIFY2(step > 0.2, qPrintable(QString::number(step)));
}

void NoiseFxTest::test_noiseFx_noiseLevel_shouldScaleTheNoise()
{
    const auto level = [](float noiseLevel) {
        NoiseFxDevice device { "Noise FX" };
        makeNeutral(device);
        device.setNoiseLevel(noiseLevel);
        device.processDeviceMidiNoteOn(60, 127);
        return rms(render(device, SampleRate / 2), 0, SampleRate / 2);
    };

    // Squared: half way down is a quarter of the gain, -12 dB
    const double decibels = 20.0 * std::log10(level(0.5f) / level(1.0f));
    QVERIFY2(std::abs(decibels + 12.04) < 0.1, qPrintable(QString::number(decibels)));
}

void NoiseFxTest::test_noiseFx_noiseLevel_zero_shouldLeaveTheCrackle()
{
    const auto level = [](float crackle) {
        NoiseFxDevice device { "Noise FX" };
        makeNeutral(device);
        device.setNoiseLevel(0.0f);
        device.setCrackle(crackle);
        device.processDeviceMidiNoteOn(60, 127);
        return rms(render(device, SampleRate), 0, SampleRate);
    };

    QCOMPARE(level(0.0f), 0.0);
    QVERIFY(level(0.5f) > 1.0e-3);
}

void NoiseFxTest::test_noiseFx_sweepKey_shouldPrefixSweepIndex()
{
    QCOMPARE(NoiseFxDevice::sweepKey(0, Constants::NahdXml::xmlKeyStart()), std::string { "sweep1_start" });
    QCOMPARE(NoiseFxDevice::sweepDestinationKey(1, Constants::NahdXml::xmlKeyTarget(), 1), std::string { "sweep2_target2" });
    QCOMPARE(NoiseFxDevice::lfoDestinationKey(Constants::NahdXml::xmlKeyDepth(), 0), std::string { "lfo_depth1" });
    QCOMPARE(NoiseFxDevice::gateKey(Constants::NahdXml::xmlKeyRate()), std::string { "gate_rate" });
}

void NoiseFxTest::test_noiseFx_sweepParameters_shouldRoundTrip()
{
    NoiseFxDevice device { "Noise FX" };
    device.setSweepParameterValue(1, Constants::NahdXml::xmlKeyStart(), 0.8f);
    device.setSweepParameterValue(1, Constants::NahdXml::xmlKeyMode(), 2.0f);
    device.setSweepParameterValue(1, Constants::NahdXml::xmlKeySync(), 0.0f);
    device.setSweepTarget(1, 1, NoiseFxDevice::Destination::Pan);
    device.setSweepDepth(1, 1, 0.1f);

    QVERIFY(std::abs(device.sweepParameterValue(1, Constants::NahdXml::xmlKeyStart()) - 0.8f) < 0.001f);
    QCOMPARE(device.sweepParameterValue(1, Constants::NahdXml::xmlKeyMode()), 2.0f);
    QCOMPARE(device.sweepParameterValue(1, Constants::NahdXml::xmlKeySync()), 0.0f);
    QCOMPARE(device.sweepTarget(1, 1), NoiseFxDevice::Destination::Pan);
    QVERIFY(std::abs(device.sweepDepth(1, 1) - 0.1f) < 0.001f);
    // Sweep A is untouched.
    QCOMPARE(device.sweepParameterValue(0, Constants::NahdXml::xmlKeySync()), 1.0f);
}

void NoiseFxTest::test_noiseFx_presets_shouldAllLoadAndBeNamedOnce()
{
    const auto & presets = NoiseFxPresets::presets();
    QVERIFY(presets.size() > 10);
    QCOMPARE(presets.front().name, std::string { "Init" });

    std::set<std::string> names;
    for (int i = 0; i < static_cast<int>(presets.size()); i++) {
        const auto & preset = presets.at(static_cast<size_t>(i));
        QVERIFY2(names.insert(preset.name).second, preset.name.c_str());

        NoiseFxDevice device { "Noise FX" };
        device.loadPreset(i);
        for (auto && [key, value] : preset.parameters) {
            // A key that is not a parameter silently does nothing, and one out of range is clamped
            // to something the patch never meant
            const auto parameter = device.parameter(key);
            QVERIFY2(parameter.has_value(), (preset.name + ": " + key).c_str());
            QVERIFY2(std::abs(parameter->get().value() - value) < 1.0e-4f, (preset.name + ": " + key).c_str());
        }

        device.processDeviceMidiNoteOn(60, 100);
        const auto buffer = render(device, SampleRate);
        QVERIFY2(rms(buffer, 0, SampleRate) > 1.0e-4, preset.name.c_str());
    }
}

void NoiseFxTest::test_noiseFx_programChange_shouldLoadThePreset()
{
    const auto & presets = NoiseFxPresets::presets();
    const auto siren = std::ranges::find_if(presets, [](auto && preset) { return preset.name == "Siren"; });
    QVERIFY(siren != presets.end());

    NoiseFxDevice device { "Noise FX" };
    device.processMidiProgramChange(static_cast<uint8_t>(std::distance(presets.begin(), siren)), 0);

    QCOMPARE(device.lfoTarget(0), NoiseFxDevice::Destination::Pitch);
    QCOMPARE(device.sweepTarget(0, 0), NoiseFxDevice::Destination::None);
    QVERIFY(!device.playToEnd());
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::NoiseFxTest)
