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
#include <mutex>
#include <vector>

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

//! Plays an automation of where @p axis of @p pad stands, as a recorded take does: @p position from
//! -1 to 1
void automatePosition(XyPadDevice & device, size_t pad, Axis axis, double position)
{
    QVERIFY(device.applyParameterAutomation(XyPadDevice::positionKey(pad, axis), ParameterAutomation::Mode::Absolute, (position + 1.0) * 0.5));
}

void setReturnSeconds(XyPadDevice & device, size_t pad, double returnSeconds)
{
    QVERIFY(device.updatePadParameter(pad, NahdXml::xmlKeyReturnTime(), static_cast<float>(std::cbrt(returnSeconds / XyPadDevice::MaxReturnSeconds))));
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

void XyPadDeviceTest::test_evaluate_positionAutomated_shouldMoveThePad()
{
    XyPadDevice device { "XY" };
    device.setTarget(0, Axis::X, cutoffTarget());
    automatePosition(device, 0, Axis::X, 0.5);

    const auto steps = stepsOf(run(device, 0, seconds(0.1)), XyPadDevice::slot(0, Axis::X));
    QVERIFY(!steps.empty());
    QVERIFY(std::abs(steps.back().source - 0.5) < 1.0e-3);
    QVERIFY(std::abs(device.positionX(0) - 0.5f) < 1.0e-3f);
    QVERIFY(!device.isPressed(0));
    QVERIFY(!device.isHandActive(0));

    // Let go of, the pad is back where it rests
    device.clearParameterAutomation();
    run(device, seconds(0.1), seconds(0.1));
    QCOMPARE(device.positionX(0), 0.0f);
}

void XyPadDeviceTest::test_evaluate_positionAutomatedWhileHeld_shouldFollowTheHand()
{
    XyPadDevice device { "XY" };
    automatePosition(device, 0, Axis::Y, 0.8);
    device.press(0, 0.0f, -0.4f);
    run(device, 0, seconds(0.1));
    QVERIFY(std::abs(device.positionY(0) + 0.4f) < 1.0e-3f);
}

void XyPadDeviceTest::test_evaluate_positionAutomatedWhileSpringingBack_shouldSpringBackFirst()
{
    XyPadDevice device { "XY" };
    setReturnSeconds(device, 0, 1.0);
    automatePosition(device, 0, Axis::X, -0.5);
    device.press(0, 1.0f, 0.0f);
    run(device, 0, seconds(0.1));
    device.release(0);

    // Halfway home, on the way to the centre, not to the automation
    run(device, seconds(0.1), seconds(0.5));
    QVERIFY2(std::abs(device.positionX(0) - 0.5f) < 0.01f, qPrintable(QString::number(device.positionX(0))));

    // Then the automation has it
    run(device, seconds(0.6), seconds(0.6));
    QVERIFY(std::abs(device.positionX(0) + 0.5f) < 1.0e-3f);
}

void XyPadDeviceTest::test_isHandActive_shouldLastUntilTheSpringBackEnds()
{
    XyPadDevice device { "XY" };
    setReturnSeconds(device, 0, 0.5);
    QVERIFY(!device.isHandActive(0));
    device.press(0, 1.0f, 1.0f);
    QVERIFY(device.isHandActive(0));
    run(device, 0, seconds(0.1));

    // Let go of but not yet seen by the audio thread, and then springing back
    device.release(0);
    QVERIFY(device.isHandActive(0));
    run(device, seconds(0.1), seconds(0.25));
    QVERIFY(device.isHandActive(0));

    run(device, seconds(0.35), seconds(0.3));
    QVERIFY(!device.isHandActive(0));
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

void XyPadDeviceTest::test_automatableParameters_position_shouldBeListed()
{
    const XyPadDevice device { "XY" };
    const auto list = device.automatableParameters();
    const auto key = XyPadDevice::positionKey(3, Axis::Y);
    QCOMPARE(key, std::string { "pad4_y_position" });
    const auto entry = std::ranges::find_if(list, [&key](auto && candidate) { return candidate.key == key; });
    QVERIFY(entry != list.end());
    QCOMPARE(entry->groupName, std::string { "Pad 4" });
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

namespace {

ParameterAutomation::Target midiCcTarget(const std::string & port, uint8_t channel, uint8_t controller, uint8_t ccMin = 0, uint8_t ccMax = 127)
{
    ParameterAutomation::Target target { ParameterAutomation::Target::Owner::MidiCc, port, ParameterAutomation::Target::midiCcKey(channel, controller) };
    target.ccMin = ccMin;
    target.ccMax = ccMax;
    return target;
}

void runEngineFor(AudioEngine & engine, uint32_t frames)
{
    constexpr uint32_t block { 256 };
    std::vector<double> buffer(block * 2);
    for (uint32_t done = 0; done < frames; done += block) {
        std::ranges::fill(buffer, 0.0);
        AudioContext context { std::span(buffer.data(), buffer.size()), block, static_cast<uint32_t>(SampleRate) };
        engine.process(context);
    }
}

//! What the engine handed over for external gear, as it arrived
struct SentMessages
{
    std::mutex mutex;
    std::vector<std::pair<std::string, uint8_t>> messages;

    std::vector<std::pair<std::string, uint8_t>> take()
    {
        const std::lock_guard<std::mutex> lock { mutex };
        return messages;
    }
};

} // namespace

void XyPadDeviceTest::test_midiCcTarget_key_shouldRoundTripTheAddress()
{
    const auto target = midiCcTarget("Port", 15, 74);
    QCOMPARE(target.parameter, std::string { "15:74" });
    QVERIFY(target.midiCcAddress());
    QCOMPARE(target.midiCcAddress()->channel, uint8_t { 15 });
    QCOMPARE(target.midiCcAddress()->controller, uint8_t { 74 });
    QCOMPARE(ParameterAutomation::Target::ownerFromString(ParameterAutomation::Target::ownerToString(target.owner)), ParameterAutomation::Target::Owner::MidiCc);
    // Not a CC, or one that does not parse
    QVERIFY(!cutoffTarget().midiCcAddress());
    const ParameterAutomation::Target badChannel { ParameterAutomation::Target::Owner::MidiCc, "Port", "16:1" };
    const ParameterAutomation::Target badController { ParameterAutomation::Target::Owner::MidiCc, "Port", "1:x" };
    QVERIFY(!badChannel.midiCcAddress());
    QVERIFY(!badController.midiCcAddress());
}

void XyPadDeviceTest::test_midiCcTarget_value_shouldSpanMinToMax()
{
    const auto target = midiCcTarget("Port", 0, 1, 20, 100);
    QCOMPARE(target.midiCcValue(0.0), uint8_t { 20 });
    QCOMPARE(target.midiCcValue(0.5), uint8_t { 60 });
    QCOMPARE(target.midiCcValue(1.0), uint8_t { 100 });
    // Min above Max turns it around
    QCOMPARE(midiCcTarget("Port", 0, 1, 127, 0).midiCcValue(0.0), uint8_t { 127 });
    QCOMPARE(midiCcTarget("Port", 0, 1, 127, 0).midiCcValue(1.0), uint8_t { 0 });
}

void XyPadDeviceTest::test_midiCcTarget_withRangeOf_shouldKeepTheRangeOnlyBetweenControllers()
{
    const auto previous = midiCcTarget("Port", 0, 1, 10, 90);
    const auto picked = midiCcTarget("Port", 0, 74).withRangeOf(previous);
    QCOMPARE(picked.ccMin, uint8_t { 10 });
    QCOMPARE(picked.ccMax, uint8_t { 90 });
    QCOMPARE(picked.parameter, std::string { "0:74" });
    QCOMPARE(cutoffTarget().withRangeOf(previous), cutoffTarget());
}

void XyPadDeviceTest::test_xmlSerialization_midiCcTarget_shouldRestoreTheRange()
{
    XyPadDevice original { "XY" };
    original.setTarget(0, Axis::X, midiCcTarget("Port", 3, 74, 100, 20));
    original.setTarget(0, Axis::Y, midiCcTarget("Port", 3, 71));
    QString xml;
    {
        NahdXmlWriter writer { xml };
        original.serializeToXml(writer);
    }
    // The whole range is the default, so only the narrowed axis says it
    const QRegularExpression padWithRange { QString { "<%1 [^>]*%2=" }.arg(NahdXml::xmlKeyPad(), NahdXml::xmlKeyMin()) };
    QCOMPARE(xml.count(padWithRange), 1);

    XyPadDevice restored { "XY" };
    NahdXmlReader reader { xml };
    while (reader.readNextStartElement() && reader.name() != NahdXml::xmlKeyDevice()) {
    }
    restored.deserializeFromXml(reader);
    QCOMPARE(restored.target(0, Axis::X), midiCcTarget("Port", 3, 74, 100, 20));
    QCOMPARE(restored.target(0, Axis::Y), midiCcTarget("Port", 3, 71));
}

void XyPadDeviceTest::test_engine_padOnInternalMidiCc_shouldReachTheDeviceThroughItsMappings()
{
    AudioEngine engine;
    const auto synth = std::make_shared<SynthDevice>("Synth");
    const auto pad = std::make_shared<XyPadDevice>("XY");
    const auto key = NahdXml::xmlKeyLpfCutoff().toStdString();
    synth->setMidiMappings({ { MidiMapping::Kind::Cc, 20, key, 0.0, 1.0 } });
    QVERIFY(pad->updatePadParameter(0, NahdXml::xmlKeyReturnTime(), 0.0f));
    pad->setTarget(0, Axis::X, midiCcTarget(synth->name(), 0, 20));
    engine.setDevice(0, pad);
    engine.setDevice(1, synth);

    pad->press(0, 1.0f, 0.0f);
    runEngineFor(engine, 4800);
    QCOMPARE(synth->parameter(key)->get().value(), 1.0f);

    pad->move(0, -1.0f, 0.0f);
    runEngineFor(engine, 4800);
    QCOMPARE(synth->parameter(key)->get().value(), 0.0f);
}

void XyPadDeviceTest::test_engine_padOnExternalMidiCc_shouldSendEachValueOnceAndEndAtTheCentre()
{
    AudioEngine engine;
    SentMessages sent;
    engine.midiCcOutput().setMaxRate(1000);
    engine.midiCcOutput().setSender([&sent](const std::string & port, uint8_t, uint8_t, uint8_t value) {
        const std::lock_guard<std::mutex> lock { sent.mutex };
        sent.messages.emplace_back(port, value);
    });
    const auto pad = std::make_shared<XyPadDevice>("XY");
    pad->setTarget(0, Axis::X, midiCcTarget("External", 1, 74));
    engine.setDevice(0, pad);

    pad->press(0, 1.0f, 0.0f);
    runEngineFor(engine, 4800);
    QTRY_VERIFY(!sent.take().empty() && sent.take().back().second == 127);
    pad->release(0);
    runEngineFor(engine, static_cast<uint32_t>(SampleRate * 6.0));
    // Home is the middle of the range
    QTRY_VERIFY(sent.take().back().second == 64);

    const auto messages = sent.take();
    QCOMPARE(messages.front().first, std::string { "External" });
    for (size_t i = 1; i < messages.size(); i++) {
        QVERIFY(messages.at(i).second != messages.at(i - 1).second);
    }
}

void XyPadDeviceTest::test_engine_render_shouldSendNothingToExternalGear()
{
    AudioEngine engine;
    SentMessages sent;
    engine.midiCcOutput().setSender([&sent](const std::string & port, uint8_t, uint8_t, uint8_t value) {
        const std::lock_guard<std::mutex> lock { sent.mutex };
        sent.messages.emplace_back(port, value);
    });
    engine.setIsExclusive(true);
    const auto pad = std::make_shared<XyPadDevice>("XY");
    pad->setTarget(0, Axis::X, midiCcTarget("External", 0, 74));
    engine.setDevice(0, pad);

    pad->press(0, 1.0f, 0.0f);
    runEngineFor(engine, 4800);
    QTest::qWait(50);
    QVERIFY(sent.take().empty());
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::XyPadDeviceTest)
