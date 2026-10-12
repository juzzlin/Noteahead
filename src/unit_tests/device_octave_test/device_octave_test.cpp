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

#include "device_octave_test.hpp"

#include "../../common/constants.hpp"
#include "../../domain/devices/device.hpp"
#include "../../domain/devices/device_factory.hpp"
#include "../../domain/devices/drum_synth_device.hpp"
#include "../../domain/devices/drum_synth_v2_device.hpp"
#include "../../domain/devices/sampler_device.hpp"
#include "../../domain/devices/sub_mixer_device.hpp"
#include "../../domain/devices/synth_device.hpp"
#include "../../infra/xml/nahd_xml_reader.hpp"
#include "../../infra/xml/nahd_xml_writer.hpp"
#include "../../view/controllers/synth_controller.hpp"

#include <QSignalSpy>
#include <QTest>

#include <memory>
#include <vector>

namespace noteahead {

namespace {

//! Records the notes it is handed, after the octave has been applied.
class RecordingDevice : public Device
{
public:
    explicit RecordingDevice(bool octave = true)
      : m_octave { octave }
    {
    }

    std::string name() const override
    {
        return "Recorder";
    }

    std::string category() const override
    {
        return "Test";
    }

    std::string typeName() const override
    {
        return "Recorder";
    }

    std::string typeId() const override
    {
        return "recorder";
    }

    bool supportsOctave() const override
    {
        return m_octave;
    }

    void processDeviceMidiNoteOn(uint8_t note, uint8_t) override
    {
        ons.push_back(note);
    }

    void processDeviceMidiNoteOff(uint8_t note) override
    {
        offs.push_back(note);
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

    void reset() override
    {
    }

    void resetAudio() override
    {
    }

    void serializeToXml(ProjectWriter &) const override
    {
    }

    void deserializeFromXml(ProjectReader &) override
    {
    }

    std::vector<uint8_t> ons;
    std::vector<uint8_t> offs;

private:
    bool m_octave { true };
};

} // namespace

void DeviceOctaveTest::test_processMidiNoteOn_octave_shouldShiftByTwelveSemitones()
{
    RecordingDevice device;
    device.processMidiNoteOn(60, 100);
    device.setOctave(1);
    device.processMidiNoteOn(62, 100);
    device.setOctave(-2);
    device.processMidiNoteOn(64, 100);
    QCOMPARE(device.ons, (std::vector<uint8_t> { 60, 74, 40 }));
}

void DeviceOctaveTest::test_processMidiNoteOff_octaveMovedWhileHeld_shouldReleaseWhatItStarted()
{
    // Otherwise moving the octave with a key held would leave the note it started hanging
    RecordingDevice device;
    device.setOctave(1);
    device.processMidiNoteOn(60, 100);
    device.setOctave(-1);
    device.processMidiNoteOff(60);
    QCOMPARE(device.ons, (std::vector<uint8_t> { 72 }));
    QCOMPARE(device.offs, (std::vector<uint8_t> { 72 }));
}

void DeviceOctaveTest::test_processMidiNoteOff_withoutNoteOn_shouldUseTheCurrentOctave()
{
    // As the transport sends when it stops: released where a note-on now would have put it
    RecordingDevice device;
    device.setOctave(2);
    device.processMidiNoteOff(48);
    QCOMPARE(device.offs, (std::vector<uint8_t> { 72 }));
}

void DeviceOctaveTest::test_processMidiNoteOn_shiftedOutOfMidi_shouldDropTheNoteAndItsOff()
{
    RecordingDevice device;
    device.setOctave(1);
    device.processMidiNoteOn(120, 100);
    device.processMidiNoteOff(120);
    device.setOctave(-2);
    device.processMidiNoteOn(10, 100);
    device.processMidiNoteOff(10);
    QVERIFY(device.ons.empty());
    QVERIFY(device.offs.empty());
}

void DeviceOctaveTest::test_setOctave_outOfRange_shouldClampAndSignalOnce()
{
    RecordingDevice device;
    QSignalSpy spy { &device, &Device::dataChanged };
    device.setOctave(5);
    QCOMPARE(device.octave(), Device::MaxOctave);
    device.setOctave(5);
    QCOMPARE(spy.count(), 1);
    device.setOctave(-9);
    QCOMPARE(device.octave(), Device::MinOctave);
    QCOMPARE(spy.count(), 2);
}

void DeviceOctaveTest::test_setOctave_unsupportedDevice_shouldStayAtZero()
{
    RecordingDevice device { false };
    device.setOctave(2);
    QCOMPARE(device.octave(), 0);
    device.processMidiNoteOn(36, 100);
    QCOMPARE(device.ons, (std::vector<uint8_t> { 36 }));
}

void DeviceOctaveTest::test_supportsOctave_shouldLeaveOutPadsAndDrums()
{
    // Their notes choose a pad or a drum, so an octave would play something else entirely
    QVERIFY(!SamplerDevice { "Sampler" }.supportsOctave());
    QVERIFY(!DrumSynthDevice { "Drums" }.supportsOctave());
    QVERIFY(!DrumSynthV2Device { "Drums" }.supportsOctave());
    QVERIFY(!SubMixerDevice { "Mixer" }.supportsOctave());
    QVERIFY(SynthDevice { "Synth" }.supportsOctave());
}

void DeviceOctaveTest::test_serializeToXml_shouldKeepTheOctaveAndOmitZero()
{
    const auto roundTrip = [](int octave, QString & xml) {
        SynthDevice out { "Synth" };
        out.setOctave(octave);
        {
            NahdXmlWriter writer { xml };
            out.serializeToXml(writer);
        }
        NahdXmlReader reader { xml };
        while (reader.readNextStartElement() && reader.name() != Constants::NahdXml::xmlKeyDevice()) {
        }
        SynthDevice in { "Synth" };
        in.setOctave(2);
        in.deserializeFromXml(reader);
        return in.octave();
    };
    QString moved;
    QCOMPARE(roundTrip(-1, moved), -1);
    QVERIFY(moved.contains("octave=\"-1\""));

    // Left out at 0, so a project that never touches it is written exactly as before, and a device
    // read without it is at 0 whatever it was before
    QString unmoved;
    QCOMPARE(roundTrip(0, unmoved), 0);
    QVERIFY(!unmoved.contains("octave="));
}

void DeviceOctaveTest::test_loadPreset_shouldLeaveTheOctaveAlone()
{
    // How the device is being played, not how it sounds: a preset neither carries nor resets it
    const auto synth = std::make_shared<SynthDevice>("Synth");
    SynthController controller { synth };
    synth->setOctave(-2);
    QVERIFY(controller.presetNames().size() > 1);
    controller.loadPreset(1);
    QCOMPARE(synth->octave(), -2);
}

void DeviceOctaveTest::test_controller_octave_shouldReachTheDevice()
{
    const auto synth = std::make_shared<SynthDevice>("Synth");
    SynthController controller { synth };
    QSignalSpy spy { &controller, &DeviceController::octaveChanged };
    controller.setOctave(1);
    QCOMPARE(synth->octave(), 1);
    QCOMPARE(controller.octave(), 1);
    QVERIFY(spy.count() >= 1);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::DeviceOctaveTest)
