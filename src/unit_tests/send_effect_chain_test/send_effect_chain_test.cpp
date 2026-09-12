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

#include "send_effect_chain_test.hpp"

#include "../../domain/devices/device.hpp"
#include "../../domain/dsp/volume.hpp"
#include "../../domain/effects/effect_rack.hpp"
#include "../../infra/audio/audio_engine.hpp"

#include <QTest>

#include <cmath>
#include <memory>
#include <span>
#include <vector>

namespace noteahead {

namespace {

//! Emits a steady 1.0 on both channels, so that everything the send path does to it can be read off
//! a single sample. Volume is the effect used throughout for the same reason: it carries no Mix
//! control, so nothing blends behind the arithmetic being asserted.
class SignalDevice : public Device
{
public:
    std::string name() const override
    {
        return "Source";
    }

    std::string category() const override
    {
        return "Mock";
    }

    std::string typeName() const override
    {
        return "SignalDevice";
    }

    std::string typeId() const override
    {
        return "signal-device-id";
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
            context.buffer[i * 2] = 1.0;
            context.buffer[i * 2 + 1] = 1.0;
        }
    }
};

//! An engine with one device emitting 1.0, routed in full to send bus 0.
struct SendFixture
{
    SendFixture()
      : device { std::make_shared<SignalDevice>() }
      , buffer(128, 0.0)
    {
        device->setReverbSend(0, 1.0f);
        engine.setDevice(0, device);
        context = AudioContext { std::span(buffer.data(), 128), 64, 44100 };
    }

    //! One block, from silence, returning the first sample of the master output.
    double render()
    {
        std::fill(buffer.begin(), buffer.end(), 0.0);
        engine.process(context);
        return buffer[0];
    }

    AudioEngine engine;
    std::shared_ptr<SignalDevice> device;
    std::vector<double> buffer;
    AudioContext context;
};

std::shared_ptr<Volume> makeVolume(float volume)
{
    auto effect = std::make_shared<Volume>();
    effect->setVolume(volume);
    return effect;
}

constexpr double tolerance = 1.0e-6;

} // namespace

void SendEffectChainTest::test_sendChain_empty_shouldMatchTheSendEffectAlone()
{
    // The compatibility guard: a bus with nothing in its chain is a bus as it was before chains
    // existed, which is every bus in every project saved until now.
    SendFixture fixture;
    fixture.engine.sendEffectRack().setEffect(0, makeVolume(0.5f));

    // Dry 1.0 plus what the send adds, which is the difference the effect made to the bus:
    // 0.5 * 1.0 - 1.0 = -0.5.
    QVERIFY(std::abs(fixture.render() - 0.5) < tolerance);
}

void SendEffectChainTest::test_sendChain_withEffect_shouldShapeWhatTheSendReturns()
{
    SendFixture fixture;
    fixture.engine.sendEffectRack().setEffect(0, makeVolume(0.5f));
    fixture.engine.sendChainRack(0).setEffect(0, makeVolume(0.5f));

    // The send returns -0.5 as above, and the chain runs as an insert on that return alone:
    // -0.5 * 0.5 = -0.25, against an untouched dry of 1.0.
    QVERIFY(std::abs(fixture.render() - 0.75) < tolerance);
}

void SendEffectChainTest::test_sendChain_withoutSendEffect_shouldNotDoubleTheDry()
{
    // The rule that makes an empty send slot safe. Were the chain simply added to the master, its
    // output would carry the bus's dry signal with it and the source would be heard twice -- 1.5
    // here. Instead the chain's first effect takes the send role and only its difference returns.
    SendFixture fixture;
    fixture.engine.sendChainRack(0).setEffect(0, makeVolume(0.5f));

    QVERIFY(std::abs(fixture.render() - 0.5) < tolerance);
}

void SendEffectChainTest::test_sendChain_withDisabledSendEffect_shouldTakeOverTheSendRole()
{
    // Bypassing the send effect must not promote the chain to adding the dry back either: it is the
    // first *enabled* effect of the bus that runs in send mode, wherever it sits.
    SendFixture fixture;
    auto head = makeVolume(0.5f);
    head->setEnabled(false);
    fixture.engine.sendEffectRack().setEffect(0, head);
    fixture.engine.sendChainRack(0).setEffect(0, makeVolume(0.25f));

    // 0.25 * 1.0 - 1.0 = -0.75, against a dry of 1.0.
    QVERIFY(std::abs(fixture.render() - 0.25) < tolerance);
}

void SendEffectChainTest::test_sendChain_disabledRack_shouldMatchTheSendEffectAlone()
{
    SendFixture fixture;
    fixture.engine.sendEffectRack().setEffect(0, makeVolume(0.5f));
    fixture.engine.sendChainRack(0).setEffect(0, makeVolume(0.5f));
    fixture.engine.sendChainRack(0).setEnabled(false);

    QVERIFY(std::abs(fixture.render() - 0.5) < tolerance);

    // And back again: bypassing leaves the rack's contents alone, so nothing but the flag has moved.
    fixture.engine.sendChainRack(0).setEnabled(true);
    QVERIFY(std::abs(fixture.render() - 0.75) < tolerance);
}

void SendEffectChainTest::test_sendChain_disabledEffect_shouldBeSkipped()
{
    SendFixture fixture;
    fixture.engine.sendEffectRack().setEffect(0, makeVolume(0.5f));
    auto chained = makeVolume(0.5f);
    chained->setEnabled(false);
    fixture.engine.sendChainRack(0).setEffect(0, chained);

    QVERIFY(std::abs(fixture.render() - 0.5) < tolerance);

    // The flag is on the effect rather than on the rack, so it does not bump the rack's version and
    // the cached snapshot cannot be what honours it. Re-enabling has to be heard on the next block.
    chained->setEnabled(true);
    QVERIFY(std::abs(fixture.render() - 0.75) < tolerance);
}

void SendEffectChainTest::test_sendChain_addedAfterProcess_shouldBeApplied()
{
    // The engine caches a snapshot of each chain and refreshes it only when that rack's version
    // moves, so a chain filled after the first block still has to be picked up.
    SendFixture fixture;
    fixture.engine.sendEffectRack().setEffect(0, makeVolume(0.5f));

    QVERIFY(std::abs(fixture.render() - 0.5) < tolerance);

    fixture.engine.sendChainRack(0).setEffect(0, makeVolume(0.5f));
    QVERIFY(std::abs(fixture.render() - 0.75) < tolerance);

    fixture.engine.sendChainRack(0).setEffect(0, nullptr);
    QVERIFY(std::abs(fixture.render() - 0.5) < tolerance);
}

void SendEffectChainTest::test_sendChain_sendRackBypassed_shouldStopToo()
{
    // A chain is the tail of a send bus, not a rack of its own. Bypassing the send rack has to take
    // the chains with it, or the bypass would turn into a way of hearing them alone.
    SendFixture fixture;
    fixture.engine.sendEffectRack().setEffect(0, makeVolume(0.5f));
    fixture.engine.sendChainRack(0).setEffect(0, makeVolume(0.5f));
    fixture.engine.sendEffectRack().setEnabled(false);

    QVERIFY(std::abs(fixture.render() - 1.0) < tolerance);
}

void SendEffectChainTest::test_sendChain_ordering_shouldRunInSlotOrder()
{
    // Two effects that do not commute would be the sharper test, but gains at least prove every slot
    // is reached and none is run twice.
    SendFixture fixture;
    fixture.engine.sendEffectRack().setEffect(0, makeVolume(0.5f));
    fixture.engine.sendChainRack(0).setEffect(0, makeVolume(0.5f));
    fixture.engine.sendChainRack(0).setEffect(2, makeVolume(0.5f));

    // -0.5 through two halvings, leaving -0.125 against a dry of 1.0. Slot 1 is empty and skipped.
    QVERIFY(std::abs(fixture.render() - 0.875) < tolerance);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::SendEffectChainTest)
