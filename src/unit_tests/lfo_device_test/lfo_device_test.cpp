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

#include "lfo_device_test.hpp"

#include "../../common/constants.hpp"
#include "../../domain/devices/lfo_device.hpp"
#include "../../infra/xml/nahd_xml_reader.hpp"
#include "../../infra/xml/nahd_xml_writer.hpp"

#include <QRegularExpression>
#include <QTest>

#include <cmath>

namespace noteahead {

namespace {

namespace NahdXml = Constants::NahdXml;

constexpr double SampleRate { 48000.0 };
constexpr uint64_t GridFrames { 48 };

ParameterAutomation::Target cutoffTarget()
{
    return { ParameterAutomation::Target::Owner::Device, "synth-uuid", "lpfCutoff" };
}

void set(LfoDevice & device, size_t lfo, const QString & key, float value)
{
    QVERIFY(device.updateLfoParameter(lfo, key, value));
}

std::vector<LfoDevice::Step> run(LfoDevice & device, uint64_t from, uint64_t frames, uint32_t block = 512, double bpm = 120.0)
{
    std::vector<LfoDevice::Step> steps;
    for (uint64_t start = from; start < from + frames; start += block) {
        device.evaluate(start, static_cast<uint32_t>(std::min<uint64_t>(block, from + frames - start)), SampleRate, bpm, steps);
    }
    return steps;
}

//! Rising crossings of zero in the steps' sources, in seconds
std::vector<double> risingCrossings(const std::vector<LfoDevice::Step> & steps)
{
    std::vector<double> crossings;
    for (size_t i = 1; i < steps.size(); i++) {
        if (steps.at(i - 1).source < 0.0 && steps.at(i).source >= 0.0) {
            crossings.push_back(static_cast<double>(steps.at(i).frame) / SampleRate);
        }
    }
    return crossings;
}

Device::ScheduledEvent noteOn(uint64_t frame, uint8_t velocity)
{
    Device::ScheduledEvent event;
    event.type = Device::ScheduledEvent::Type::NoteOn;
    event.frame = frame;
    event.note = 60;
    event.velocity = velocity;
    return event;
}

QString toXml(const LfoDevice & device)
{
    QString xml;
    NahdXmlWriter writer { xml };
    device.serializeToXml(writer);
    return xml.remove(QRegularExpression { " uuid=\"[^\"]*\"" });
}

} // namespace

void LfoDeviceTest::test_typeId_shouldBeAModulator()
{
    const LfoDevice device { "LFO" };
    QCOMPARE(device.typeName(), Constants::lfoDeviceName().toStdString());
    QCOMPARE(device.category(), NahdXml::xmlValueModulators().toStdString());
    QVERIFY(!device.hasActiveAudio());
}

void LfoDeviceTest::test_evaluate_withoutTarget_shouldSendNothing()
{
    LfoDevice device { "LFO" };
    QVERIFY(run(device, 0, 48000).empty());
}

void LfoDeviceTest::test_evaluate_shouldStepOnTheMillisecondGrid()
{
    LfoDevice device { "LFO" };
    set(device, 0, NahdXml::xmlKeyWaveform(), static_cast<float>(Lfo::Waveform::Sine));
    device.setTarget(0, cutoffTarget());

    // Starting between two grid points: the first step is the next one on the timeline, not the block
    const auto steps = run(device, 100, 4800);

    QVERIFY(steps.size() > 90);
    QCOMPARE(steps.front().frame, uint64_t { 144 });
    for (auto && step : steps) {
        QCOMPARE(step.frame % GridFrames, uint64_t { 0 });
        QCOMPARE(step.target, cutoffTarget());
        QCOMPARE(step.mode, std::optional { ParameterModulation::Mode::Bipolar });
    }
}

void LfoDeviceTest::test_evaluate_anyBlockSize_shouldGiveTheSameSteps()
{
    // What makes a render match playback: the blocks are cut differently, the steps are not
    const auto stepsWith = [](std::vector<uint32_t> blocks) {
        LfoDevice device { "LFO" };
        set(device, 1, NahdXml::xmlKeyWaveform(), static_cast<float>(Lfo::Waveform::Sine));
        set(device, 1, NahdXml::xmlKeyRate(), 0.8f);
        device.setTarget(1, cutoffTarget());
        std::vector<LfoDevice::Step> steps;
        uint64_t start = 0;
        for (size_t i = 0; start < 96000; i++) {
            const uint32_t block = blocks.at(i % blocks.size());
            device.evaluate(start, block, SampleRate, 120.0, steps);
            start += block;
        }
        return steps;
    };
    const auto small = stepsWith({ 512 });
    const auto large = stepsWith({ 2048 });
    const auto ragged = stepsWith({ 300, 17, 1031, 4 });

    QVERIFY(small.size() > 1000);
    for (const auto & other : { large, ragged }) {
        QVERIFY(other.size() >= small.size());
        for (size_t i = 0; i < small.size(); i++) {
            QCOMPARE(other.at(i).frame, small.at(i).frame);
            QCOMPARE(other.at(i).source, small.at(i).source);
        }
    }
}

void LfoDeviceTest::test_evaluate_unchangedValue_shouldSendNothing()
{
    // A square only changes twice a cycle, so only those steps -- and the first -- are sent
    LfoDevice device { "LFO" };
    set(device, 0, NahdXml::xmlKeyWaveform(), static_cast<float>(Lfo::Waveform::Square));
    set(device, 0, NahdXml::xmlKeyRate(), 0.0f); // 0.05 Hz: one flip in ten seconds
    device.setTarget(0, cutoffTarget());

    const auto steps = run(device, 0, static_cast<uint64_t>(SampleRate * 11));

    QCOMPARE(steps.size(), size_t { 2 });
}

void LfoDeviceTest::test_rateDivider_shouldSlowTheLfo()
{
    const auto period = [](int divider) {
        LfoDevice device { "LFO" };
        set(device, 0, NahdXml::xmlKeyWaveform(), static_cast<float>(Lfo::Waveform::Sine));
        set(device, 0, NahdXml::xmlKeyRate(), 1.0f);
        set(device, 0, NahdXml::xmlKeyRateDivider(), static_cast<float>(divider));
        device.setTarget(0, cutoffTarget());
        const auto crossings = risingCrossings(run(device, 0, static_cast<uint64_t>(SampleRate)));
        return (crossings.back() - crossings.front()) / static_cast<double>(crossings.size() - 1);
    };

    const double top = 1.0 / LfoDevice::MaxRateHz;
    QVERIFY2(std::abs(period(1) - top) < 0.002, qPrintable(QString::number(period(1))));
    QVERIFY2(std::abs(period(4) - 4.0 * top) < 0.002, qPrintable(QString::number(period(4))));
}

void LfoDeviceTest::test_sync_quarterNote_shouldRunAtTheBeat()
{
    LfoDevice device { "LFO" };
    set(device, 0, NahdXml::xmlKeyWaveform(), static_cast<float>(Lfo::Waveform::Sine));
    set(device, 0, NahdXml::xmlKeyMode(), static_cast<float>(Lfo::Mode::BPM));
    set(device, 0, NahdXml::xmlKeyRate(), 0.25f);
    device.setTarget(0, cutoffTarget());

    const auto crossings = risingCrossings(run(device, 0, static_cast<uint64_t>(SampleRate * 3), 512, 120.0));

    QVERIFY(std::abs((crossings.back() - crossings.front()) / static_cast<double>(crossings.size() - 1) - 0.5) < 0.002);
}

void LfoDeviceTest::test_retrigger_note_shouldRestartAtItsStep()
{
    // A saw starts at its bottom, so a restart shows as a fall to -1 at the step the note lands on
    LfoDevice device { "LFO" };
    set(device, 0, NahdXml::xmlKeyWaveform(), static_cast<float>(Lfo::Waveform::Saw));
    set(device, 0, NahdXml::xmlKeyRetrigger(), 1.0f);
    device.setTarget(0, cutoffTarget());
    run(device, 0, 24000);

    device.scheduleMidiEvent(noteOn(24100, 127));
    const auto steps = run(device, 24000, 4800);

    const auto restart = std::ranges::find_if(steps, [](auto && step) { return step.frame == 24144; });
    QVERIFY(restart != steps.end());
    QVERIFY(std::abs(restart->source + 1.0) < 1.0e-9);
}

void LfoDeviceTest::test_retrigger_off_shouldIgnoreNotes()
{
    const auto stepsWith = [](bool note) {
        LfoDevice device { "LFO" };
        set(device, 0, NahdXml::xmlKeyWaveform(), static_cast<float>(Lfo::Waveform::Saw));
        device.setTarget(0, cutoffTarget());
        run(device, 0, 24000);
        if (note) {
            device.scheduleMidiEvent(noteOn(24100, 127));
        }
        return run(device, 24000, 4800);
    };
    const auto withNote = stepsWith(true);
    const auto without = stepsWith(false);
    QCOMPARE(withNote.size(), without.size());
    for (size_t i = 0; i < without.size(); i++) {
        QCOMPARE(withNote.at(i).source, without.at(i).source);
    }
}

void LfoDeviceTest::test_velocitySensitivity_shouldScaleTheDepth()
{
    LfoDevice device { "LFO" };
    set(device, 0, NahdXml::xmlKeyIntensity(), 0.8f);
    set(device, 0, NahdXml::xmlKeyVelocitySensitivity(), 1.0f);
    device.setTarget(0, cutoffTarget());
    QVERIFY(std::abs(run(device, 0, 480).back().depth - 0.8) < 1.0e-6);

    device.scheduleMidiEvent(noteOn(480, 64));
    const auto steps = run(device, 480, 480);

    QVERIFY(std::abs(steps.back().depth - 0.8 * 64.0 / 127.0) < 1.0e-6);
}

void LfoDeviceTest::test_setTarget_shouldReleaseTheOldTarget()
{
    LfoDevice device { "LFO" };
    device.setTarget(2, cutoffTarget());
    run(device, 0, 480);
    const ParameterAutomation::Target other { ParameterAutomation::Target::Owner::Effect, "effect-id", "mix" };

    device.setTarget(2, other);
    const auto steps = run(device, 480, 480);

    QVERIFY(!steps.empty());
    QCOMPARE(steps.front().target, cutoffTarget());
    QVERIFY(!steps.front().mode.has_value());
    QCOMPARE(steps.back().target, other);
}

void LfoDeviceTest::test_smoothing_shouldSoftenASquare()
{
    const auto largestJump = [](float smoothing) {
        LfoDevice device { "LFO" };
        set(device, 0, NahdXml::xmlKeyWaveform(), static_cast<float>(Lfo::Waveform::Square));
        set(device, 0, NahdXml::xmlKeyRate(), 0.7f);
        set(device, 0, NahdXml::xmlKeySmoothing(), smoothing);
        device.setTarget(0, cutoffTarget());
        const auto steps = run(device, 0, 96000);
        double jump = 0.0;
        for (size_t i = 1; i < steps.size(); i++) {
            jump = std::max(jump, std::abs(steps.at(i).source - steps.at(i - 1).source));
        }
        return jump;
    };

    QVERIFY(largestJump(0.0f) > 1.9);
    QVERIFY(largestJump(0.6f) < 0.2);
}

void LfoDeviceTest::test_phase_shouldOffsetTheStart()
{
    // A quarter of the way into a sine is its top
    LfoDevice device { "LFO" };
    set(device, 0, NahdXml::xmlKeyWaveform(), static_cast<float>(Lfo::Waveform::Sine));
    set(device, 0, NahdXml::xmlKeyPhase(), 0.25f);
    device.resetAudio();
    device.setTarget(0, cutoffTarget());

    QVERIFY(std::abs(run(device, 0, 48).front().source - 1.0) < 1.0e-9);
}

void LfoDeviceTest::test_xmlSerialization_shouldRestoreTargetsAndControls()
{
    LfoDevice original { "LFO" };
    set(original, 3, NahdXml::xmlKeyRateDivider(), 16.0f);
    set(original, 3, NahdXml::xmlKeyModulationMode(), static_cast<float>(ParameterModulation::Mode::Tremolo));
    original.setTarget(3, cutoffTarget());
    original.setTarget(1, { ParameterAutomation::Target::Owner::Effect, "effect-id", "mix" });

    QString xml;
    {
        NahdXmlWriter writer { xml };
        original.serializeToXml(writer);
    }
    LfoDevice restored { "LFO" };
    NahdXmlReader reader { xml };
    while (reader.readNextStartElement() && reader.name() != NahdXml::xmlKeyDevice()) {
    }
    restored.deserializeFromXml(reader);

    QCOMPARE(restored.target(3), cutoffTarget());
    QCOMPARE(restored.target(1).owner, ParameterAutomation::Target::Owner::Effect);
    QVERIFY(restored.target(0).ownerId.empty());
    QCOMPARE(toXml(restored), toXml(original));
}

void LfoDeviceTest::test_xmlSerialization_untouched_shouldWriteNoTargets()
{
    QVERIFY(!toXml(LfoDevice { "LFO" }).contains("<" + NahdXml::xmlKeyLfo() + " "));
}

void LfoDeviceTest::test_automatableParameters_shouldBeGroupedByLfo()
{
    const LfoDevice device { "LFO" };
    const auto list = device.automatableParameters();
    QVERIFY(!list.empty());
    // The channel strip first, ungrouped, then LFO 1 to 4 in order
    QVERIFY(list.front().groupName.empty());
    std::vector<std::string> groups;
    for (auto && entry : list) {
        if (!entry.groupName.empty() && (groups.empty() || groups.back() != entry.groupName)) {
            groups.push_back(entry.groupName);
        }
    }
    QCOMPARE(groups, (std::vector<std::string> { "LFO 1", "LFO 2", "LFO 3", "LFO 4" }));
    QVERIFY(std::ranges::any_of(list, [](auto && entry) { return entry.key == LfoDevice::lfoKey(1, NahdXml::xmlKeyRate()); }));
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::LfoDeviceTest)
