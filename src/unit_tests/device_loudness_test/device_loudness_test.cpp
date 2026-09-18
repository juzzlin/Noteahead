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

#include "device_loudness_test.hpp"

#include "../../common/parameter_mapper.hpp"
#include "../../domain/devices/device.hpp"
#include "../../domain/effects/effect.hpp"
#include "../../domain/utility/loudness_meter.hpp"
#include "../../domain/utility/lufs_meter.hpp"
#include "../../infra/audio/audio_engine.hpp"

#include <QTest>

#include <cmath>
#include <numbers>
#include <vector>

namespace noteahead {

namespace {

constexpr uint32_t SampleRate { 48000 };
constexpr uint32_t FrameCount { 512 };
constexpr double ToneFrequency { 1000.0 };

//! Loudness needs seconds of signal: a gating block is 400 ms, and the integrated reading only
//! begins once four 100-ms blocks exist.
constexpr double MeasureSeconds { 4.0 };

//! Interleaved stereo sine, the signal every measurement here is made on.
std::vector<double> makeSine(double amplitude, double seconds, size_t phaseOffset = 0)
{
    const auto frames = static_cast<size_t>(SampleRate * seconds);
    std::vector<double> data(frames * 2, 0.0);
    for (size_t i = 0; i < frames; i++) {
        const double s = amplitude * std::sin(2.0 * std::numbers::pi * ToneFrequency * static_cast<double>(i + phaseOffset) / SampleRate);
        data[i * 2] = s;
        data[i * 2 + 1] = s;
    }
    return data;
}

//! Hands an interleaved signal to a meter the way the engine does, one buffer at a time.
void writeInBuffers(LoudnessMeter & meter, const std::vector<double> & interleaved)
{
    for (size_t frame = 0; frame * 2 < interleaved.size(); frame += FrameCount) {
        const auto frames = std::min(static_cast<size_t>(FrameCount), interleaved.size() / 2 - frame);
        meter.write(interleaved.data() + frame * 2, static_cast<uint32_t>(frames), SampleRate);
    }
}

//! Plays a 1 kHz sine for as long as it is asked to, so that a device's output is something whose
//! loudness is known before the strip touches it.
class SineDevice : public Device
{
public:
    explicit SineDevice(double amplitude)
      : m_amplitude { amplitude }
    {
    }

    std::string name() const override
    {
        return "Sine";
    }

    std::string category() const override
    {
        return "Mock";
    }

    std::string typeName() const override
    {
        return "SineDevice";
    }

    std::string typeId() const override
    {
        return "sine-device-id";
    }

    void processMidiNoteOn(uint8_t, uint8_t) override
    {
    }

    void processMidiNoteOff(uint8_t) override
    {
    }

    void processDeviceMidiCc(uint8_t, uint8_t, uint8_t) override
    {
    }

    void processMidiAllNotesOff() override
    {
    }

    void processAudio(AudioContext & context) override
    {
        for (uint32_t i = 0; i < context.frameCount; i++) {
            const double s = m_amplitude * std::sin(2.0 * std::numbers::pi * ToneFrequency * m_phase / SampleRate);
            m_phase += 1.0;
            context.buffer[i * 2] = s;
            context.buffer[i * 2 + 1] = s;
        }
    }

    bool hasActiveAudio() const override
    {
        return m_amplitude > 0.0;
    }

    void setAmplitude(double amplitude)
    {
        m_amplitude = amplitude;
    }

private:
    double m_amplitude;
    double m_phase { 0.0 };
};

//! Halves the signal, so an insert's effect on the output loudness is a known -6 dB.
class HalvingEffect : public Effect
{
public:
    std::string type() const override
    {
        return "TestHalver";
    }

    std::string typeId() const override
    {
        return "test-halver";
    }

    void processSample(double & left, double & right) override
    {
        left *= 0.5;
        right *= 0.5;
    }
};

//! Runs the engine for the given time and hands back what the device's output tap made of it.
float measureThroughEngine(AudioEngine & engine, Device & device, double seconds = MeasureSeconds)
{
    device.meter().setActive(true);
    device.outputLoudnessMeter().setActive(true);
    device.outputLoudnessMeter().requestReset();
    // The level tap of the same point, which the caller reads off the device afterwards. Reset by
    // hand because setActive() only clears on the way down, and a second measurement would
    // otherwise start on the peak the first one left.
    device.outputMeter().setActive(true);
    device.outputMeter().reset();

    std::vector<double> buffer(static_cast<size_t>(FrameCount) * 2, 0.0);
    AudioContext context { std::span(buffer.data(), buffer.size()), FrameCount, SampleRate };
    const auto blocks = static_cast<int>(seconds * SampleRate / FrameCount);
    for (int i = 0; i < blocks; i++) {
        std::fill(buffer.begin(), buffer.end(), 0.0);
        engine.process(context);
    }

    return device.outputLoudnessMeter().integratedLufs();
}

} // namespace

void DeviceLoudnessTest::test_loudnessMeter_inactive_shouldNotMeasure()
{
    // The gate is what keeps a per-device tap free while nothing is on screen: sixteen devices'
    // worth of K-weighting would otherwise run over every buffer of every song.
    LoudnessMeter meter;
    QVERIFY(!meter.active());

    writeInBuffers(meter, makeSine(0.5, MeasureSeconds));

    QCOMPARE(meter.integratedLufs(), LoudnessMeter::MinimumLufs);
    QCOMPARE(meter.shortTermLufs(), LoudnessMeter::MinimumLufs);
}

void DeviceLoudnessTest::test_loudnessMeter_blockApi_shouldMatchTheSampleApi()
{
    // LufsMeter is the same measurement one sample at a time, and it is the one with the BS.1770
    // tests against the offline analyzer. Feeding both the very same signal is what carries that
    // agreement over to the buffer-at-a-time entry point the engine uses.
    const auto signal = makeSine(0.25, MeasureSeconds);

    LoudnessMeter blockMeter;
    blockMeter.setActive(true);
    writeInBuffers(blockMeter, signal);

    LufsMeter sampleMeter;
    sampleMeter.setSampleRate(SampleRate);
    for (size_t i = 0; i < signal.size(); i += 2) {
        double left = signal[i];
        double right = signal[i + 1];
        sampleMeter.processSample(left, right);
    }

    QVERIFY(blockMeter.integratedLufs() > LoudnessMeter::MinimumLufs);
    QVERIFY(std::abs(blockMeter.integratedLufs() - sampleMeter.integratedLufs()) < 0.01f);
    QVERIFY(std::abs(blockMeter.shortTermLufs() - sampleMeter.shortTermLufs()) < 0.01f);
}

void DeviceLoudnessTest::test_loudnessMeter_setActiveFalse_shouldClearTheReadings()
{
    LoudnessMeter meter;
    meter.setActive(true);
    writeInBuffers(meter, makeSine(0.5, MeasureSeconds));
    QVERIFY(meter.integratedLufs() > LoudnessMeter::MinimumLufs);

    // Closing the mixer has to leave nothing behind: the next time it opens, the reading belongs to
    // what is playing then rather than to the last take.
    meter.setActive(false);

    QCOMPARE(meter.integratedLufs(), LoudnessMeter::MinimumLufs);
    QCOMPARE(meter.shortTermLufs(), LoudnessMeter::MinimumLufs);
}

void DeviceLoudnessTest::test_loudnessMeter_integrated_silenceBetweenHits_shouldNotDragTheReadingDown()
{
    // The property the whole feature rests on. A kick plays for a fraction of the time a pad does,
    // and on peak or RMS it therefore reads far below one it matches by ear. BS.1770 gating drops
    // the silence, so the integrated reading says how loud the device is when it sounds -- which is
    // what two devices can be balanced by.
    LoudnessMeter continuous;
    continuous.setActive(true);
    writeInBuffers(continuous, makeSine(0.5, 16.0));

    LoudnessMeter intermittent;
    intermittent.setActive(true);
    const auto tone = makeSine(0.5, 2.0);
    const std::vector<double> silence(tone.size(), 0.0);
    for (int i = 0; i < 4; i++) {
        writeInBuffers(intermittent, tone);
        writeInBuffers(intermittent, silence);
    }

    QVERIFY(continuous.integratedLufs() > LoudnessMeter::MinimumLufs);
    // Only the gating blocks that straddle a tone's edge are partly filled, and they are what keeps
    // this from being an exact match.
    QVERIFY(std::abs(continuous.integratedLufs() - intermittent.integratedLufs()) < 1.0f);
}

void DeviceLoudnessTest::test_outputLoudness_engine_shouldMeasureAfterTheFader()
{
    AudioEngine engine;
    const auto device = std::make_shared<SineDevice>(0.5);
    engine.setDevice(0, device);

    const auto unity = measureThroughEngine(engine, *device);
    const auto unityLevel = device->meter().rmsDb();
    QVERIFY(unity > LoudnessMeter::MinimumLufs);

    // Half the amplitude out of the fader is 6 dB, and the output tap is the only one that sees it:
    // the gain-staging tap is taken before the fader whichever side of the inserts it sits on.
    device->setVolume(static_cast<float>(ParameterMapper::unmapFader(0.5)));

    const auto faded = measureThroughEngine(engine, *device);

    QVERIFY(std::abs((unity - faded) - 6.02f) < 0.2f);
    QVERIFY(std::abs(device->meter().rmsDb() - unityLevel) < 0.1f);
}

void DeviceLoudnessTest::test_outputLoudness_engine_shouldMeasureAfterTheInserts()
{
    AudioEngine engine;
    const auto device = std::make_shared<SineDevice>(0.5);
    engine.setDevice(0, device);

    const auto dry = measureThroughEngine(engine, *device);
    const auto dryLevel = device->meter().rmsDb();

    device->insertEffectRack().setEffect(0, std::make_shared<HalvingEffect>());

    const auto processed = measureThroughEngine(engine, *device);

    QVERIFY(std::abs((dry - processed) - 6.02f) < 0.2f);
    QVERIFY(std::abs(device->meter().rmsDb() - dryLevel) < 0.1f);
}

void DeviceLoudnessTest::test_outputLoudness_engine_silentDevice_shouldFallBackToTheFloor()
{
    AudioEngine engine;
    const auto device = std::make_shared<SineDevice>(0.5);
    engine.setDevice(0, device);

    QVERIFY(measureThroughEngine(engine, *device) > LoudnessMeter::MinimumLufs);

    // A device the engine stops processing altogether still has to report silence, or its last
    // reading would sit in the mixer for as long as it stays quiet.
    device->setAmplitude(0.0);

    QCOMPARE(measureThroughEngine(engine, *device), LoudnessMeter::MinimumLufs);
}

void DeviceLoudnessTest::test_outputLevel_engine_shouldMeasureAfterTheFader()
{
    AudioEngine engine;
    const auto device = std::make_shared<SineDevice>(0.5);
    engine.setDevice(0, device);

    measureThroughEngine(engine, *device);
    const auto unityRms = device->outputMeter().rmsDb();
    const auto unityInputRms = device->meter().rmsDb();
    // A 0.5 sine is -9.03 dBFS RMS, and the output tap sees it unweighted -- which is the whole
    // point of it next to the loudness tap.
    QVERIFY(std::abs(unityRms - -9.03f) < 0.1f);

    device->setVolume(static_cast<float>(ParameterMapper::unmapFader(0.5)));

    measureThroughEngine(engine, *device);

    // Half the amplitude out of the fader is 6 dB here too, and again only the output tap sees it:
    // the gain-staging tap is taken before the fader.
    QVERIFY(std::abs((unityRms - device->outputMeter().rmsDb()) - 6.02f) < 0.2f);
    QVERIFY(std::abs(device->meter().rmsDb() - unityInputRms) < 0.1f);
}

void DeviceLoudnessTest::test_outputLevel_engine_silentDevice_shouldFallAway()
{
    AudioEngine engine;
    const auto device = std::make_shared<SineDevice>(0.5);
    engine.setDevice(0, device);

    measureThroughEngine(engine, *device);
    QVERIFY(device->outputMeter().rmsDb() > -12.0f);

    // A device the engine stops processing altogether is still fed silence, or the bar would sit
    // where the last note left it for as long as the device stays quiet. Unlike the loudness
    // reading this one falls rather than blanks: the peak slides at a fixed rate and the RMS decays
    // over its window, so what is asserted is that it has fallen away, not that it has hit the floor.
    device->setAmplitude(0.0);

    measureThroughEngine(engine, *device);

    QVERIFY(device->outputMeter().rmsDb() < -60.0f);
    QVERIFY(device->outputMeter().peakDb() < -60.0f);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::DeviceLoudnessTest)
