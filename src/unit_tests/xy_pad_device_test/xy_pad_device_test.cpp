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

#include "xy_pad_device_test.hpp"

#include "../../common/constants.hpp"
#include "../../domain/devices/lfo_device.hpp"
#include "../../domain/devices/synth_device.hpp"
#include "../../domain/devices/xy_pad_device.hpp"
#include "../../domain/dsp/audio_context.hpp"
#include "../../domain/effects/effect_rack.hpp"
#include "../../domain/effects/gain.hpp"
#include "../../infra/audio/audio_engine.hpp"
#include "../../infra/xml/nahd_xml_reader.hpp"
#include "../../infra/xml/nahd_xml_writer.hpp"

#include <QRegularExpression>
#include <QTest>

#include <cmath>

namespace noteahead {

namespace {

namespace NahdXml = Constants::NahdXml;
using Axis = XyPadDevice::Axis;

constexpr double SampleRate { 48000.0 };
constexpr uint64_t GridFrames { 48 };

ParameterAutomation::Target cutoffTarget()
{
    return { ParameterAutomation::Target::Owner::Device, "synth-uuid", "lpfCutoff" };
}

ParameterAutomation::Target resonanceTarget()
{
    return { ParameterAutomation::Target::Owner::Device, "synth-uuid", "lpfResonance" };
}

std::vector<ModulationSource::Step> run(XyPadDevice & device, uint64_t from, uint64_t frames, uint32_t block = 512)
{
    std::vector<ModulationSource::Step> steps;
    for (uint64_t start = from; start < from + frames; start += block) {
        device.evaluate(start, static_cast<uint32_t>(std::min<uint64_t>(block, from + frames - start)), SampleRate, 120.0, steps);
    }
    return steps;
}

std::vector<ModulationSource::Step> stepsOf(const std::vector<ModulationSource::Step> & steps, size_t slot)
{
    std::vector<ModulationSource::Step> result;
    std::ranges::copy_if(steps, std::back_inserter(result), [slot](auto && step) { return step.slot == slot; });
    return result;
}

QString toXml(const XyPadDevice & device)
{
    QString xml;
    NahdXmlWriter writer { xml };
    device.serializeToXml(writer);
    return xml.remove(QRegularExpression { " uuid=\"[^\"]*\"" });
}

uint64_t seconds(double value)
{
    return static_cast<uint64_t>(value * SampleRate);
}

} // namespace

void XyPadDeviceTest::test_typeId_shouldBeAModulator()
{
    const XyPadDevice device { "XY" };
    QCOMPARE(device.typeName(), Constants::xyPadDeviceName().toStdString());
    QCOMPARE(device.category(), NahdXml::xmlValueModulators().toStdString());
    QVERIFY(!device.hasActiveAudio());
    QVERIFY(!device.producesAudio());
}

void XyPadDeviceTest::test_evaluate_noTarget_shouldSendNothing()
{
    XyPadDevice device { "XY" };
    device.press(0, 0.5f, 0.5f);
    QVERIFY(run(device, 0, seconds(0.1)).empty());
    // The pad still moves, so the crosshair follows the hand with nothing assigned
    QVERIFY(device.positionX(0) > 0.45f);
}

void XyPadDeviceTest::test_evaluate_atRest_shouldSendNothing()
{
    XyPadDevice device { "XY" };
    device.setTarget(0, Axis::X, cutoffTarget());
    QVERIFY(run(device, 0, seconds(0.5)).empty());
    QVERIFY(!device.isMoving(0));
}

void XyPadDeviceTest::test_evaluate_press_shouldSlewToTheHand()
{
    XyPadDevice device { "XY" };
    device.setTarget(1, Axis::X, cutoffTarget());
    device.setTarget(1, Axis::Y, resonanceTarget());
    device.press(1, 1.0f, -0.5f);

    const auto steps = run(device, 0, seconds(0.1));
    const auto x = stepsOf(steps, XyPadDevice::slot(1, Axis::X));
    const auto y = stepsOf(steps, XyPadDevice::slot(1, Axis::Y));
    QVERIFY(!x.empty() && !y.empty());

    // Slewed, not jumped: a mouse's step would zip
    QVERIFY2(x.front().source < 0.2, qPrintable(QString::number(x.front().source)));
    QVERIFY(x.front().frame % GridFrames == 0);
    // And there well within the time a hand can notice
    QVERIFY(std::abs(x.back().source - 1.0) < 1.0e-3);
    QVERIFY(std::abs(y.back().source + 0.5) < 1.0e-3);
    QCOMPARE(x.back().mode, std::optional { ParameterModulation::Mode::Bipolar });
    QCOMPARE(x.back().target, cutoffTarget());
    QVERIFY(device.isPressed(1));
    QVERIFY(std::abs(device.positionX(1) - 1.0f) < 1.0e-3f);

    // Nothing changes while the hand rests, so nothing is sent
    QVERIFY(run(device, seconds(0.1), seconds(0.1)).empty());
}

void XyPadDeviceTest::test_evaluate_release_shouldSpringBackWithinTheReturnTime()
{
    XyPadDevice device { "XY" };
    device.setTarget(0, Axis::X, cutoffTarget());
    QVERIFY(device.updatePadParameter(0, NahdXml::xmlKeyReturnTime(), static_cast<float>(std::cbrt(1.0 / XyPadDevice::MaxReturnSeconds))));
    device.press(0, 1.0f, 0.0f);
    run(device, 0, seconds(0.1));

    device.release(0);
    QVERIFY(!device.isPressed(0));
    const auto halfway = stepsOf(run(device, seconds(0.1), seconds(0.5)), XyPadDevice::slot(0, Axis::X));
    QVERIFY(!halfway.empty());
    // Smoothly down, never past the centre, and halfway at half the time
    for (size_t i = 1; i < halfway.size(); i++) {
        QVERIFY(halfway.at(i).source < halfway.at(i - 1).source);
    }
    // Eased off the hand: the first step barely moves
    QVERIFY(halfway.front().source > 0.999);
    QVERIFY2(std::abs(halfway.back().source - 0.5) < 0.01, qPrintable(QString::number(halfway.back().source)));
    QVERIFY(device.isMoving(0));

    // Home exactly when the return time is up, and the target let go of
    const auto rest = stepsOf(run(device, seconds(0.6), seconds(0.5) + GridFrames), XyPadDevice::slot(0, Axis::X));
    QVERIFY(!rest.empty());
    QCOMPARE(rest.back().mode, std::optional<ParameterModulation::Mode> {});
    QCOMPARE(device.positionX(0), 0.0f);
    QVERIFY(!device.isMoving(0));
}

void XyPadDeviceTest::test_evaluate_zeroReturnTime_shouldHold()
{
    XyPadDevice device { "XY" };
    device.setTarget(0, Axis::Y, cutoffTarget());
    QVERIFY(device.updatePadParameter(0, NahdXml::xmlKeyReturnTime(), 0.0f));
    device.press(0, 0.0f, 0.6f);
    run(device, 0, seconds(0.1));

    device.release(0);
    QVERIFY(run(device, seconds(0.1), seconds(2.0)).empty());
    QVERIFY(std::abs(device.positionY(0) - 0.6f) < 1.0e-3f);
    QVERIFY(!device.isMoving(0));

    // The next touch picks it up from there
    device.press(0, 0.0f, -0.6f);
    const auto steps = run(device, seconds(2.1), seconds(0.005));
    QVERIFY(!steps.empty());
    QVERIFY(steps.front().source > 0.4);
}

void XyPadDeviceTest::test_evaluate_depth_shouldBeSent()
{
    XyPadDevice device { "XY" };
    device.setTarget(2, Axis::X, cutoffTarget());
    QVERIFY(device.updateAxisParameter(2, Axis::X, NahdXml::xmlKeyIntensity(), 0.25f));
    QCOMPARE(device.axisParameterValue(2, Axis::X, NahdXml::xmlKeyIntensity()), 0.25f);
    QCOMPARE(device.axisParameterValue(2, Axis::Y, NahdXml::xmlKeyIntensity()), 1.0f);
    device.press(2, 0.5f, 0.0f);

    const auto steps = run(device, 0, seconds(0.05));
    QVERIFY(!steps.empty());
    QCOMPARE(steps.back().depth, 0.25);
}

void XyPadDeviceTest::test_evaluate_anyBufferSize_shouldStepTheSame()
{
    const auto stepsWith = [](uint32_t block) {
        XyPadDevice device { "XY" };
        device.setTarget(0, Axis::X, cutoffTarget());
        device.press(0, 0.8f, 0.0f);
        auto steps = run(device, 0, 4096, block);
        device.release(0);
        const auto more = run(device, 4096, seconds(0.5), block);
        steps.insert(steps.end(), more.begin(), more.end());
        return steps;
    };
    const auto small = stepsWith(64);
    const auto large = stepsWith(1024);
    QCOMPARE(small.size(), large.size());
    for (size_t i = 0; i < small.size(); i++) {
        QCOMPARE(small.at(i).frame, large.at(i).frame);
        QCOMPARE(small.at(i).source, large.at(i).source);
    }
}

void XyPadDeviceTest::test_setTarget_shouldReleaseTheOldTarget()
{
    XyPadDevice device { "XY" };
    device.setTarget(0, Axis::X, cutoffTarget());
    device.press(0, 0.5f, 0.0f);
    run(device, 0, seconds(0.05));

    device.setTarget(0, Axis::X, resonanceTarget());
    const auto steps = run(device, seconds(0.05), 512);
    QVERIFY(steps.size() >= 2);
    QCOMPARE(steps.front().target, cutoffTarget());
    QCOMPARE(steps.front().mode, std::optional<ParameterModulation::Mode> {});
    QCOMPARE(steps.at(1).target, resonanceTarget());
    QVERIFY(steps.at(1).mode.has_value());
}

void XyPadDeviceTest::test_press_outOfRange_shouldClamp()
{
    XyPadDevice device { "XY" };
    device.press(3, 4.0f, -4.0f);
    run(device, 0, seconds(0.2));
    QVERIFY(std::abs(device.positionX(3) - 1.0f) < 1.0e-3f);
    QVERIFY(std::abs(device.positionY(3) + 1.0f) < 1.0e-3f);

    // A pad that is not there is ignored
    device.press(XyPadDevice::PadCount, 1.0f, 1.0f);
    QVERIFY(!device.isPressed(XyPadDevice::PadCount));
}

void XyPadDeviceTest::test_xmlSerialization_shouldRestoreTargetsAndControls()
{
    XyPadDevice original { "XY" };
    QVERIFY(original.updatePadParameter(2, NahdXml::xmlKeyReturnTime(), 0.0f));
    QVERIFY(original.updateAxisParameter(3, Axis::Y, NahdXml::xmlKeyIntensity(), 0.4f));
    original.setTarget(3, Axis::Y, cutoffTarget());
    original.setTarget(1, Axis::X, { ParameterAutomation::Target::Owner::Effect, "effect-id", "mix" });

    QString xml;
    {
        NahdXmlWriter writer { xml };
        original.serializeToXml(writer);
    }
    XyPadDevice restored { "XY" };
    NahdXmlReader reader { xml };
    while (reader.readNextStartElement() && reader.name() != NahdXml::xmlKeyDevice()) {
    }
    restored.deserializeFromXml(reader);

    QCOMPARE(restored.target(3, Axis::Y), cutoffTarget());
    QVERIFY(restored.target(3, Axis::X).ownerId.empty());
    QCOMPARE(restored.target(1, Axis::X).owner, ParameterAutomation::Target::Owner::Effect);
    QCOMPARE(restored.padParameterValue(2, NahdXml::xmlKeyReturnTime()), 0.0f);
    QVERIFY(std::abs(restored.axisParameterValue(3, Axis::Y, NahdXml::xmlKeyIntensity()) - 0.4f) < 1.0e-4f);
    QCOMPARE(toXml(restored), toXml(original));
}

void XyPadDeviceTest::test_xmlSerialization_untouched_shouldWriteNoTargets()
{
    QVERIFY(!toXml(XyPadDevice { "XY" }).contains("<" + NahdXml::xmlKeyPad() + " "));
}

void XyPadDeviceTest::test_automatableParameters_shouldBeGroupedByPad()
{
    const XyPadDevice device { "XY" };
    const auto list = device.automatableParameters();
    for (size_t pad = 0; pad < XyPadDevice::PadCount; pad++) {
        const auto key = XyPadDevice::padKey(pad, NahdXml::xmlKeyReturnTime());
        const auto entry = std::ranges::find_if(list, [&key](auto && candidate) { return candidate.key == key; });
        QVERIFY(entry != list.end());
        QCOMPARE(entry->groupName, "Pad " + std::to_string(pad + 1));
    }
}

void XyPadDeviceTest::test_engine_padAndLfoOnOneCutoff_shouldAddUp()
{
    AudioEngine engine;
    const auto synth = std::make_shared<SynthDevice>("Synth");
    const auto pad = std::make_shared<XyPadDevice>("XY");
    const auto lfo = std::make_shared<LfoDevice>("LFO");
    const auto key = NahdXml::xmlKeyLpfCutoff().toStdString();
    synth->parameter(key)->get().setValue(0.5f);
    const ParameterAutomation::Target target { ParameterAutomation::Target::Owner::Device, synth->uuid(), key };
    pad->setTarget(0, Axis::X, target);
    lfo->setTarget(0, target);
    engine.setDevice(0, pad);
    engine.setDevice(1, lfo);
    engine.setDevice(2, synth);

    const auto runEngine = [&engine](uint32_t frames) {
        constexpr uint32_t block { 256 };
        std::vector<double> buffer(block * 2);
        for (uint32_t done = 0; done < frames; done += block) {
            std::ranges::fill(buffer, 0.0);
            AudioContext context { std::span(buffer.data(), buffer.size()), block, static_cast<uint32_t>(SampleRate) };
            engine.process(context);
        }
    };

    pad->press(0, 1.0f, 0.0f);
    runEngine(4800);
    QCOMPARE(synth->parameter(key)->get().modulationSourceCount(), size_t { 2 });

    // Let go, and once the pad is home only the LFO is left
    pad->release(0);
    runEngine(static_cast<uint32_t>(seconds(1.5)));
    QCOMPARE(synth->parameter(key)->get().modulationSourceCount(), size_t { 1 });
    QCOMPARE(synth->parameter(key)->get().shownValue(), 0.5f);
}

void XyPadDeviceTest::test_engine_padOnEffect_shouldLetGoAtHome()
{
    AudioEngine engine;
    const auto pad = std::make_shared<XyPadDevice>("XY");
    const auto gain = std::make_shared<Gain>();
    engine.insertEffectRack().setEffect(0, gain);
    const auto key = NahdXml::xmlKeyGain().toStdString();
    const float authored = gain->parameter(key)->get().value();
    pad->setTarget(0, Axis::Y, { ParameterAutomation::Target::Owner::Effect, gain->id(), key });
    engine.setDevice(0, pad);

    const auto runEngine = [&engine](uint32_t frames) {
        constexpr uint32_t block { 1024 };
        std::vector<double> buffer(block * 2);
        for (uint32_t done = 0; done < frames; done += block) {
            std::ranges::fill(buffer, 0.0);
            AudioContext context { std::span(buffer.data(), buffer.size()), block, static_cast<uint32_t>(SampleRate) };
            engine.process(context);
        }
    };

    pad->press(0, 0.0f, -1.0f);
    runEngine(4800);
    QVERIFY(gain->parameter(key)->get().isModulated());
    QVERIFY(gain->parameter(key)->get().value() < authored);

    // The block it arrives in also holds its last steps on the way: the let-go must still win
    pad->release(0);
    runEngine(static_cast<uint32_t>(seconds(1.5)));
    QVERIFY(!gain->parameter(key)->get().isModulated());
    QCOMPARE(gain->parameter(key)->get().value(), authored);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::XyPadDeviceTest)
