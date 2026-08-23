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

#include "monitor_test.hpp"
#include "../../common/constants.hpp"
#include "../../domain/devices/device.hpp"
#include "../../domain/dsp/audio_context.hpp"
#include "../../domain/effects/monitor.hpp"
#include "../../infra/audio/audio_engine.hpp"

#include <QTest>

#include <algorithm>
#include <cmath>
#include <memory>
#include <ranges>
#include <vector>

namespace noteahead {

namespace {

void setMode(Monitor & monitor, Monitor::Mode mode)
{
    const auto parameter = monitor.parameter(Constants::NahdXml::xmlKeyMode().toStdString());
    QVERIFY(parameter.has_value());
    parameter->get().setValue(static_cast<float>(mode));
    monitor.sync();
}

//! Runs one stereo frame through the block path, which is where the offline check lives.
std::pair<double, double> processFrame(Monitor & monitor, double left, double right, bool offline = false)
{
    std::vector<double> buffer { left, right };
    AudioContext context { std::span<double>(buffer.data(), buffer.size()), 1, 48000 };
    context.offline = offline;
    monitor.process(context);
    return { buffer.at(0), buffer.at(1) };
}

//! Emits a hard anti-correlated pair, which a mono fold cancels to nothing. Makes the two
//! outcomes -- folded and not folded -- as far apart as they can be.
class MockDevice : public Device
{
public:
    explicit MockDevice(std::string name)
      : m_name { std::move(name) }
    {
    }

    std::string name() const override
    {
        return m_name;
    }

    std::string category() const override
    {
        return "Mock";
    }

    std::string typeName() const override
    {
        return "MockDevice";
    }

    std::string typeId() const override
    {
        return "mock-device-id";
    }

    void processMidiNoteOn(uint8_t, uint8_t) override
    {
    }

    void processMidiNoteOff(uint8_t) override
    {
    }

    void processMidiCc(uint8_t, uint8_t, uint8_t) override
    {
    }

    void processMidiAllNotesOff() override
    {
    }

    void processAudio(AudioContext & context) override
    {
        for (uint32_t i = 0; i < context.frameCount; i++) {
            context.buffer[i * 2] = 1.0;
            context.buffer[i * 2 + 1] = -1.0;
        }
    }

    bool hasActiveAudio() const override
    {
        return true;
    }

private:
    std::string m_name;
};

//! Loudest sample the engine put out for one block, with the Monitor in the given rack.
double enginePeak(bool offline, bool onDeviceInsertRack)
{
    AudioEngine engine;
    const auto device = std::make_shared<MockDevice>("Device");
    engine.setDevice(0, device);

    auto monitor = std::make_shared<Monitor>();
    if (auto p = monitor->parameter(Constants::NahdXml::xmlKeyMode().toStdString()); p) {
        p->get().setValue(static_cast<float>(Monitor::Mode::Mono));
    }
    monitor->sync();

    if (onDeviceInsertRack) {
        device->insertEffectRack().setEffect(0, monitor);
    } else {
        engine.insertEffectRack().setEffect(0, monitor);
    }

    constexpr uint32_t frameCount = 64;
    std::vector<double> buffer(frameCount * 2, 0.0);
    AudioContext context;
    context.frameCount = frameCount;
    context.sampleRate = 44100;
    context.buffer = std::span<double>(buffer.data(), buffer.size());
    context.offline = offline;
    engine.process(context);

    return std::ranges::max(buffer | std::views::transform([](double v) { return std::abs(v); }));
}

} // namespace

void MonitorTest::test_stereo_shouldPassThroughUntouched()
{
    Monitor monitor;

    // The default, and the one mode that has to be bit-identical: a monitor sitting in a rack unused
    // must not be something the mix goes through.
    const auto [left, right] = processFrame(monitor, 0.3, -0.7);
    QCOMPARE(left, 0.3);
    QCOMPARE(right, -0.7);
}

void MonitorTest::test_mono_shouldSumBothChannelsAtHalf()
{
    Monitor monitor;
    setMode(monitor, Monitor::Mode::Mono);

    const auto [left, right] = processFrame(monitor, 0.8, 0.2);
    QCOMPARE(left, 0.5);
    QCOMPARE(right, 0.5);
}

void MonitorTest::test_mono_correlatedPair_shouldKeepLevel()
{
    Monitor monitor;
    setMode(monitor, Monitor::Mode::Mono);

    // Material that is already the same on both sides must come out at the level it went in at,
    // or the check would report a level change where there is only a phase relationship to judge.
    const auto [left, right] = processFrame(monitor, 0.6, 0.6);
    QCOMPARE(left, 0.6);
    QCOMPARE(right, 0.6);
}

void MonitorTest::test_mono_antiCorrelatedPair_shouldCancel()
{
    Monitor monitor;
    setMode(monitor, Monitor::Mode::Mono);

    const auto [left, right] = processFrame(monitor, 0.6, -0.6);
    QCOMPARE(left, 0.0);
    QCOMPARE(right, 0.0);
}

void MonitorTest::test_left_shouldPlaceLeftOnBothSides()
{
    Monitor monitor;
    setMode(monitor, Monitor::Mode::Left);

    const auto [left, right] = processFrame(monitor, 0.4, -0.9);
    QCOMPARE(left, 0.4);
    QCOMPARE(right, 0.4);
}

void MonitorTest::test_right_shouldPlaceRightOnBothSides()
{
    Monitor monitor;
    setMode(monitor, Monitor::Mode::Right);

    const auto [left, right] = processFrame(monitor, 0.4, -0.9);
    QCOMPARE(left, -0.9);
    QCOMPARE(right, -0.9);
}

void MonitorTest::test_side_correlatedPair_shouldCancel()
{
    Monitor monitor;
    setMode(monitor, Monitor::Mode::Side);

    // Nothing in the difference means nothing is lost to a mono sum, which is the whole reading.
    const auto [left, right] = processFrame(monitor, 0.6, 0.6);
    QCOMPARE(left, 0.0);
    QCOMPARE(right, 0.0);
}

void MonitorTest::test_side_antiCorrelatedPair_shouldSurvive()
{
    Monitor monitor;
    setMode(monitor, Monitor::Mode::Side);

    const auto [left, right] = processFrame(monitor, 0.6, -0.6);
    QCOMPARE(left, 0.6);
    QCOMPARE(right, 0.6);
}

void MonitorTest::test_offline_shouldPassThroughInEveryMode()
{
    // The export guarantee. A monitor left folded must not reach a rendered file, so every mode has
    // to be a no-op on a block marked offline. If a new AudioContext is ever built somewhere that
    // forgets to carry the flag, this is what should say so.
    for (const auto mode : { Monitor::Mode::Stereo, Monitor::Mode::Mono, Monitor::Mode::Left, Monitor::Mode::Right, Monitor::Mode::Side }) {
        Monitor monitor;
        setMode(monitor, mode);

        const auto [left, right] = processFrame(monitor, 0.25, -0.75, true);
        QCOMPARE(left, 0.25);
        QCOMPARE(right, -0.75);
    }
}

void MonitorTest::test_engine_masterInsertRack_shouldFoldOnlyWhenNotOffline()
{
    // Proves the offline flag actually arrives, rather than only that Monitor honours one it is
    // handed. This is the master insert rack, which is given the engine's own context.
    QVERIFY(enginePeak(false, false) < 1.0e-9);
    QVERIFY(enginePeak(true, false) > 0.99);
}

void MonitorTest::test_engine_deviceInsertRack_shouldFoldOnlyWhenNotOffline()
{
    // The same again one level down, where the engine builds a fresh context per device: a field
    // added to AudioContext but not copied there would pass the test above and fail this one.
    QVERIFY(enginePeak(false, true) < 1.0e-9);
    QVERIFY(enginePeak(true, true) > 0.99);
}

void MonitorTest::test_sync_shouldUpdateMode()
{
    Monitor monitor;
    QCOMPARE(monitor.mode(), Monitor::Mode::Stereo);

    setMode(monitor, Monitor::Mode::Side);
    QCOMPARE(monitor.mode(), Monitor::Mode::Side);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::MonitorTest)
