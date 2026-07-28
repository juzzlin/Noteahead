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

#include "sub_mixer_test.hpp"

#include "../../application/service/device_service.hpp"
#include "../../common/constants.hpp"
#include "../../domain/devices/device.hpp"
#include "../../domain/devices/sub_mixer_device.hpp"
#include "../../domain/effects/effect.hpp"
#include "../../domain/effects/effect_rack.hpp"
#include "../../infra/audio/audio_engine.hpp"
#include "../../infra/data_service.hpp"

#include <QTest>

#include <cmath>
#include <memory>
#include <numbers>
#include <vector>

namespace noteahead {

namespace {

constexpr uint32_t FrameCount = 64;
constexpr uint32_t SampleRate = 44100;

//! Every device applies a constant-power pan law, so a centred signal comes out at cos(45 deg).
//! The SubMixer is no exception, so expected sums carry this factor.
const double CenterPanGain = std::cos(std::numbers::pi * 0.25);

//! Emits a constant DC level on both channels, which makes summing trivial to assert.
class ToneDevice : public Device
{
public:
    explicit ToneDevice(std::string name, double level)
      : m_name { std::move(name) }
      , m_level { level }
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
        return "ToneDevice";
    }

    std::string typeId() const override
    {
        return "tone-device-id";
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
        for (uint32_t i = 0; i < context.frameCount * 2; i++) {
            context.buffer[i] = m_level;
        }
    }

private:
    std::string m_name;
    double m_level;
};

//! Doubles whatever reaches it, so it is obvious whether it ran on the group or on one member.
class GainEffect : public Effect
{
public:
    std::string type() const override
    {
        return "test-gain";
    }

    std::string typeId() const override
    {
        return "test-gain-id";
    }

    void process(double & left, double & right) override
    {
        left *= 2.0;
        right *= 2.0;
    }
};

std::shared_ptr<SubMixerDevice> makeSubMixer(const std::string & name)
{
    return std::make_shared<SubMixerDevice>(name);
}

//! Runs one engine callback and returns the first output sample.
double renderFirstSample(AudioEngine & engine)
{
    std::vector<double> buffer(FrameCount * 2, 0.0);
    AudioContext context { std::span<double>(buffer.data(), buffer.size()), FrameCount, SampleRate, 120.0, {}, 1 };
    engine.process(context);
    return buffer[0];
}

//! DeviceService needs a live engine; this bundles the two so tests can drive membership rules.
struct ServiceFixture
{
    std::shared_ptr<AudioEngine> engine { std::make_shared<AudioEngine>() };
    DeviceService service { engine, std::make_shared<DataService>() };
};

} // namespace

void SubMixerTest::test_subMixer_noMembers_shouldStaySilent()
{
    AudioEngine engine;
    engine.setDevice(0, makeSubMixer("Sub"));

    QCOMPARE(renderFirstSample(engine), 0.0);
}

void SubMixerTest::test_subMixer_members_shouldSumMemberOutput()
{
    AudioEngine engine;
    engine.setDevice(0, std::make_shared<ToneDevice>("A", 0.25));
    engine.setDevice(1, std::make_shared<ToneDevice>("B", 0.5));

    auto subMixer = makeSubMixer("Sub");
    subMixer->setMembers({ 0, 1 });
    engine.setDevice(2, subMixer);

    // Only the SubMixer reaches the master, carrying the sum of both members.
    QVERIFY(std::abs(renderFirstSample(engine) - 0.75 * CenterPanGain) < 1.0e-9);
}

void SubMixerTest::test_subMixer_members_shouldNotReachMasterDirectly()
{
    AudioEngine engine;
    engine.setDevice(0, std::make_shared<ToneDevice>("A", 0.25));

    auto subMixer = makeSubMixer("Sub");
    subMixer->setMembers({ 0 });
    engine.setDevice(1, subMixer);

    // The regression this whole feature hinges on: without suppression the member would be heard
    // once dry straight into the master and once through the SubMixer.
    const auto claimed = renderFirstSample(engine);
    QVERIFY(std::abs(claimed - 0.25 * CenterPanGain) < 1.0e-9);
    QVERIFY(std::abs(claimed - (0.25 + 0.25 * CenterPanGain)) > 1.0e-6);
}

void SubMixerTest::test_subMixer_members_shouldRenderBeforeSubMixer()
{
    // The SubMixer sits in a lower slot than its member, so only the dependency-driven topological
    // sort can make it read a rendered buffer rather than a stale, zeroed one.
    AudioEngine engine;
    auto subMixer = makeSubMixer("Sub");
    subMixer->setMembers({ 5 });
    engine.setDevice(0, subMixer);
    engine.setDevice(5, std::make_shared<ToneDevice>("Late", 0.5));

    QVERIFY(std::abs(renderFirstSample(engine) - 0.5 * CenterPanGain) < 1.0e-9);
}

void SubMixerTest::test_subMixer_insertEffects_shouldApplyToWholeGroup()
{
    AudioEngine engine;
    engine.setDevice(0, std::make_shared<ToneDevice>("A", 0.25));
    engine.setDevice(1, std::make_shared<ToneDevice>("B", 0.25));

    auto subMixer = makeSubMixer("Sub");
    subMixer->setMembers({ 0, 1 });
    subMixer->insertEffectRack().setEffect(0, std::make_shared<GainEffect>());
    engine.setDevice(2, subMixer);

    // The effect runs once on the summed group, not once per member.
    QVERIFY(std::abs(renderFirstSample(engine) - 0.5 * CenterPanGain * 2.0) < 1.0e-9);
}

void SubMixerTest::test_subMixer_nested_shouldSumThroughChain()
{
    AudioEngine engine;
    engine.setDevice(0, std::make_shared<ToneDevice>("A", 0.5));

    auto inner = makeSubMixer("Inner");
    inner->setMembers({ 0 });
    engine.setDevice(1, inner);

    auto outer = makeSubMixer("Outer");
    outer->setMembers({ 1 });
    engine.setDevice(2, outer);

    // Each stage centres its own pan, so the level picks up the pan law twice.
    QVERIFY(std::abs(renderFirstSample(engine) - 0.5 * CenterPanGain * CenterPanGain) < 1.0e-9);
}

void SubMixerTest::test_subMixer_nonMember_shouldStillReachMasterDirectly()
{
    AudioEngine engine;
    engine.setDevice(0, std::make_shared<ToneDevice>("Claimed", 0.25));
    engine.setDevice(3, std::make_shared<ToneDevice>("Free", 0.5));

    auto subMixer = makeSubMixer("Sub");
    subMixer->setMembers({ 0 });
    engine.setDevice(1, subMixer);

    // The unclaimed device keeps its direct path, so the master carries it plus the group.
    QVERIFY(std::abs(renderFirstSample(engine) - (0.5 + 0.25 * CenterPanGain)) < 1.0e-9);
}

void SubMixerTest::test_subMixer_memberSends_shouldStillReachSendBus()
{
    AudioEngine engine;
    auto member = std::make_shared<ToneDevice>("A", 0.25);
    member->setReverbSend(0, 1.0f);
    engine.setDevice(0, member);

    auto subMixer = makeSubMixer("Sub");
    subMixer->setMembers({ 0 });
    engine.setDevice(1, subMixer);

    engine.sendEffectRack().setEffect(0, std::make_shared<GainEffect>());

    // A send bus is a parallel tap, not part of the master sum, so joining a group must not kill a
    // device's reverb. The rack returns only the wet delta: (0.25 * 2) - 0.25.
    constexpr double expectedWet = 0.25;
    QVERIFY(std::abs(renderFirstSample(engine) - (0.25 * CenterPanGain + expectedWet)) < 1.0e-9);
}

void SubMixerTest::test_subMixer_memberSends_shouldMatchUngroupedSend()
{
    const auto sendReturn = [](bool grouped) {
        AudioEngine engine;
        auto member = std::make_shared<ToneDevice>("A", 0.25);
        member->setReverbSend(0, 1.0f);
        engine.setDevice(0, member);

        if (grouped) {
            auto subMixer = makeSubMixer("Sub");
            subMixer->setMembers({ 0 });
            engine.setDevice(1, subMixer);
        }

        engine.sendEffectRack().setEffect(0, std::make_shared<GainEffect>());

        // Subtracting the dry path leaves just what the send bus contributed.
        const auto dry = grouped ? 0.25 * CenterPanGain : 0.25;
        return renderFirstSample(engine) - dry;
    };

    // Grouping changes where the dry signal goes, never how much the device feeds the send.
    QVERIFY(std::abs(sendReturn(true) - sendReturn(false)) < 1.0e-9);
}

void SubMixerTest::test_membership_addedTwice_shouldNotDuplicate()
{
    ServiceFixture fixture;
    fixture.service.setDevice(0, std::make_shared<ToneDevice>("A", 0.5));
    fixture.service.setDevice(1, makeSubMixer("Sub"));

    QVERIFY(fixture.service.addSubMixerMember(1, 0));
    QVERIFY(fixture.service.addSubMixerMember(1, 0));
    QCOMPARE(fixture.service.subMixerMembers(1).size(), 1);
}

void SubMixerTest::test_membership_secondSubMixer_shouldTakeOverExclusively()
{
    ServiceFixture fixture;
    fixture.service.setDevice(0, std::make_shared<ToneDevice>("A", 0.5));
    fixture.service.setDevice(1, makeSubMixer("First"));
    fixture.service.setDevice(2, makeSubMixer("Second"));

    QVERIFY(fixture.service.addSubMixerMember(1, 0));
    QVERIFY(fixture.service.addSubMixerMember(2, 0));

    // Belonging to two SubMixers would sum the device twice, so the newer claim wins outright.
    QCOMPARE(fixture.service.subMixerMembers(1).size(), 0);
    QCOMPARE(fixture.service.subMixerMembers(2).size(), 1);
    QCOMPARE(fixture.service.subMixerOwningSlot(0), 2);
}

void SubMixerTest::test_membership_selfReference_shouldBeRejected()
{
    ServiceFixture fixture;
    fixture.service.setDevice(1, makeSubMixer("Sub"));

    QVERIFY(!fixture.service.addSubMixerMember(1, 1));
    QCOMPARE(fixture.service.subMixerMembers(1).size(), 0);
}

void SubMixerTest::test_membership_cycle_shouldBeRejected()
{
    ServiceFixture fixture;
    fixture.service.setDevice(0, makeSubMixer("A"));
    fixture.service.setDevice(1, makeSubMixer("B"));

    QVERIFY(fixture.service.addSubMixerMember(0, 1));
    // B already feeds A, so feeding A into B would close the loop.
    QVERIFY(!fixture.service.addSubMixerMember(1, 0));
    QCOMPARE(fixture.service.subMixerMembers(1).size(), 0);
}

void SubMixerTest::test_membership_indirectCycle_shouldBeRejected()
{
    ServiceFixture fixture;
    fixture.service.setDevice(0, makeSubMixer("A"));
    fixture.service.setDevice(1, makeSubMixer("B"));
    fixture.service.setDevice(2, makeSubMixer("C"));

    QVERIFY(fixture.service.addSubMixerMember(0, 1));
    QVERIFY(fixture.service.addSubMixerMember(1, 2));
    // A <- B <- C already; adding A into C would close a three-hop loop.
    QVERIFY(!fixture.service.addSubMixerMember(2, 0));
}

void SubMixerTest::test_membership_clearedDevice_shouldBePruned()
{
    ServiceFixture fixture;
    fixture.service.setDevice(0, std::make_shared<ToneDevice>("A", 0.5));
    fixture.service.setDevice(1, makeSubMixer("Sub"));

    QVERIFY(fixture.service.addSubMixerMember(1, 0));
    QCOMPARE(fixture.service.subMixerMembers(1).size(), 1);

    // Deleting a member must not leave the SubMixer claiming an empty slot.
    fixture.service.clearDevice(0);
    QCOMPARE(fixture.service.subMixerMembers(1).size(), 0);
}

void SubMixerTest::test_membership_nonSubMixerTarget_shouldBeRejected()
{
    ServiceFixture fixture;
    fixture.service.setDevice(0, std::make_shared<ToneDevice>("A", 0.5));
    fixture.service.setDevice(1, std::make_shared<ToneDevice>("B", 0.5));

    QVERIFY(!fixture.service.addSubMixerMember(1, 0));
}

void SubMixerTest::test_midiCc_volume_shouldScaleGroup()
{
    AudioEngine engine;
    engine.setDevice(0, std::make_shared<ToneDevice>("A", 0.5));

    auto subMixer = makeSubMixer("Sub");
    subMixer->setMembers({ 0 });
    engine.setDevice(1, subMixer);

    // CC 7 at half scale. The group is a normal device, so riding it from a track works like riding
    // any other device.
    subMixer->processMidiCc(7, 64, 0);

    const auto expected = 0.5 * (64.0 / 127.0) * CenterPanGain;
    QVERIFY(std::abs(renderFirstSample(engine) - expected) < 1.0e-9);
}

void SubMixerTest::test_midiCc_pan_shouldMoveGroup()
{
    AudioEngine engine;
    engine.setDevice(0, std::make_shared<ToneDevice>("A", 0.5));

    auto subMixer = makeSubMixer("Sub");
    subMixer->setMembers({ 0 });
    engine.setDevice(1, subMixer);

    std::vector<double> buffer(FrameCount * 2, 0.0);
    AudioContext context { std::span<double>(buffer.data(), buffer.size()), FrameCount, SampleRate, 120.0, {}, 1 };

    // CC 10 hard left: the right channel should fall away while the left rises.
    subMixer->processMidiCc(10, 0, 0);
    engine.process(context);

    QVERIFY(buffer[0] > 0.5 * CenterPanGain);
    QVERIFY(std::abs(buffer[1]) < 1.0e-9);
}

void SubMixerTest::test_midiCc_resetAllControllers_shouldRestoreManualValues()
{
    auto subMixer = makeSubMixer("Sub");
    subMixer->setVolume(0.8f);
    subMixer->setPan(0.25f);

    subMixer->processMidiCc(7, 0, 0);
    subMixer->processMidiCc(10, 127, 0);
    QVERIFY(std::abs(subMixer->volume()) < 0.001f);

    // CC 121 restores what the knobs were set to, not the values CC rode them to.
    subMixer->processMidiCc(121, 0, 0);
    QVERIFY(std::abs(subMixer->volume() - 0.8f) < 0.001f);
    QVERIFY(std::abs(subMixer->pan() - 0.25f) < 0.001f);
}

void SubMixerTest::test_midiCc_availableControllers_shouldOfferVolumeAndPan()
{
    const auto controllers = makeSubMixer("Sub")->availableMidiCcControllers();

    QCOMPARE(controllers.size(), size_t { 2 });
    QCOMPARE(controllers[0].number, uint8_t { 7 });
    QCOMPARE(controllers[1].number, uint8_t { 10 });
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::SubMixerTest)
