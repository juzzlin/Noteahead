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

#include "device_active_notes_test.hpp"

#include "../../domain/devices/device.hpp"
#include "../../plugins/devices/synth/synth_device.hpp"
#include "../../plugins/devices/synth/synth_dialog_model.hpp"

#include <QSignalSpy>
#include <QTest>

#include <memory>

namespace noteahead {

namespace {

class ActiveNotesStubDevice : public Device
{
public:
    std::string name() const override
    {
        return "Stub";
    }

    std::string category() const override
    {
        return "Test";
    }

    std::string typeName() const override
    {
        return "Stub";
    }

    std::string typeId() const override
    {
        return "stub";
    }

    void processDeviceMidiNoteOn(uint8_t, uint8_t) override
    {
    }

    void processDeviceMidiNoteOff(uint8_t) override
    {
    }

    void processDeviceMidiCc(uint8_t, uint8_t, uint8_t) override
    {
    }

    void processMidiAllNotesOff() override
    {
    }

    void processAudio(AudioContext &) override
    {
    }

    bool hasActiveAudio() const override
    {
        return false;
    }
};

} // namespace

void DeviceActiveNotesTest::test_noteOnAndOff_shouldTrackHeldNotesAcrossBothMaskWords()
{
    ActiveNotesStubDevice device {};
    device.processMidiNoteOn(21, 100);
    device.processMidiNoteOn(63, 100);
    device.processMidiNoteOn(64, 100);
    device.processMidiNoteOn(108, 100);

    const auto held1 = device.pollActiveNoteMask();
    QCOMPARE(held1.at(0), (uint64_t { 1 } << 21) | (uint64_t { 1 } << 63));
    QCOMPARE(held1.at(1), (uint64_t { 1 } << (64 - 64)) | (uint64_t { 1 } << (108 - 64)));

    device.processMidiNoteOff(63);
    device.processMidiNoteOff(108);
    const auto held2 = device.pollActiveNoteMask();
    QCOMPARE(held2.at(0), (uint64_t { 1 } << 21));
    QCOMPARE(held2.at(1), (uint64_t { 1 } << (64 - 64)));
}

void DeviceActiveNotesTest::test_shortPulse_noteOnThenOffBeforePoll_shouldFlashOnceThenClear()
{
    ActiveNotesStubDevice device {};
    device.processMidiNoteOn(60, 127);
    device.processMidiNoteOff(60);

    const auto firstPoll = device.pollActiveNoteMask();
    QCOMPARE(firstPoll.at(0), (uint64_t { 1 } << 60));
    QCOMPARE(firstPoll.at(1), uint64_t { 0 });

    const auto secondPoll = device.pollActiveNoteMask();
    QCOMPARE(secondPoll.at(0), uint64_t { 0 });
    QCOMPARE(secondPoll.at(1), uint64_t { 0 });
}

void DeviceActiveNotesTest::test_zeroVelocityNoteOn_shouldClearHeldNote()
{
    ActiveNotesStubDevice device {};
    device.processMidiNoteOn(60, 100);
    QCOMPARE(device.pollActiveNoteMask().at(0), (uint64_t { 1 } << 60));

    device.processMidiNoteOn(60, 0);
    const auto mask = device.pollActiveNoteMask();
    QCOMPARE(mask.at(0), uint64_t { 0 });
    QCOMPARE(mask.at(1), uint64_t { 0 });
}

void DeviceActiveNotesTest::test_octaveShiftedOutOfRange_shouldNotMarkActive()
{
    ActiveNotesStubDevice device {};
    device.setOctave(1);
    device.processMidiNoteOn(120, 100);
    QCOMPARE(device.pollActiveNoteMask().at(0), uint64_t { 0 });
    QCOMPARE(device.pollActiveNoteMask().at(1), uint64_t { 0 });

    device.processMidiNoteOn(60, 100);
    QCOMPARE(device.pollActiveNoteMask().at(0), (uint64_t { 1 } << 60));
}

void DeviceActiveNotesTest::test_clearActiveNotesAndResetAudioAndScheduledAllNotesOff_shouldClearAll()
{
    ActiveNotesStubDevice device {};
    device.processMidiNoteOn(48, 100);
    device.resetAudio();
    QCOMPARE(device.pollActiveNoteMask().at(0), uint64_t { 0 });

    device.processMidiNoteOn(52, 100);
    Device::ScheduledEvent ev {};
    ev.type = Device::ScheduledEvent::Type::AllNotesOff;
    ev.frame = 0;
    device.scheduleMidiEvent(ev);
    device.applyScheduledEvents(0);
    QCOMPARE(device.pollActiveNoteMask().at(0), uint64_t { 0 });

    device.processMidiNoteOn(55, 100);
    device.clearActiveNotes();
    QCOMPARE(device.pollActiveNoteMask().at(0), uint64_t { 0 });
}

void DeviceActiveNotesTest::test_clearHeldNote_shouldPreserveSingleTriggeredFlash()
{
    ActiveNotesStubDevice device {};
    device.processMidiNoteOn(36, 127);
    device.clearHeldNote(36);

    const auto firstPoll = device.pollActiveNoteMask();
    QCOMPARE(firstPoll.at(0), (uint64_t { 1 } << 36));
    QCOMPARE(device.pollActiveNoteMask().at(0), uint64_t { 0 });
}

void DeviceActiveNotesTest::test_dialogModel_pollPlayingNotes_shouldEmitOnlyOnChangeAndClearStaleOnDeviceAttach()
{
    const auto synth = std::make_shared<SynthDevice>("Synth");
    // Pre-existing short pulse before any dialog attaches should not flash when dialog opens
    synth->processMidiNoteOn(40, 100);
    synth->processMidiNoteOff(40);
    // Currently held note should show immediately after attach
    synth->processMidiNoteOn(60, 100);

    SynthDialogModel controller { synth };
    QCOMPARE(controller.playingNotes(), (QVariantList { 60 }));

    QSignalSpy spy { &controller, &DeviceDialogModel::playingNotesChanged };
    controller.pollPlayingNotes();
    QCOMPARE(spy.count(), 0);

    synth->processMidiNoteOn(64, 90);
    controller.pollPlayingNotes();
    QCOMPARE(spy.count(), 1);
    QCOMPARE(controller.playingNotes(), (QVariantList { 60, 64 }));

    // Calling again with no change emits nothing
    controller.pollPlayingNotes();
    QCOMPARE(spy.count(), 1);

    synth->processMidiNoteOff(60);
    synth->processMidiNoteOff(64);
    controller.pollPlayingNotes();
    QCOMPARE(spy.count(), 2);
    QCOMPARE(controller.playingNotes(), QVariantList {});
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::DeviceActiveNotesTest)
