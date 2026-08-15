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

#include "property_service_test.hpp"

#include "../../application/service/device_service.hpp"
#include "../../application/service/property_service.hpp"
#include "../../common/constants.hpp"
#include "../../domain/devices/bass_synth_device.hpp"
#include "../../domain/devices/drum_synth_device.hpp"
#include "../../domain/devices/sampler_device.hpp"
#include "../../domain/devices/synth_device.hpp"
#include "../../infra/audio/audio_engine.hpp"
#include "../../infra/data_service.hpp"
#include "../../infra/midi/midi_cc_mapping.hpp"

#include <QTest>
#include <memory>

namespace noteahead {

void PropertyServiceTest::test_availableMidiControllers_shouldReturnCorrectControllers()
{
    PropertyService service;
    const auto controllers = service.availableMidiControllers();

    // Verify size
    QCOMPARE(controllers.size(), 128);

    // Verify first element (BankSelectMSB)
    const auto first = controllers.first().toMap();
    QCOMPARE(first["number"].toInt(), 0);
    QCOMPARE(first["name"].toString(), QString { "0: BankSelectMSB" });

    // Verify last element (PolyModeOn)
    const auto last = controllers.last().toMap();
    QCOMPARE(last["number"].toInt(), 127);
    QCOMPARE(last["name"].toString(), QString { "127: PolyModeOn" });
}

void PropertyServiceTest::test_valueRange_internalDeviceFader_shouldExtendPastMidi1()
{
    auto audioEngine = std::make_shared<AudioEngine>();
    auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
    PropertyService propertyService;
    propertyService.setDeviceService(deviceService);

    deviceService->setDevice(0, std::make_shared<SynthDevice>("Synth 1"));
    const QString portName = Constants::internalDevicePortPrefix() + " 1";

    const auto volumeCc = static_cast<int>(MidiCcMapping::Controller::ChannelVolumeMSB);
    const auto panCc = static_cast<int>(MidiCcMapping::Controller::PanMSB);

    // The fader reaches past MIDI 1.0 so automation can drive it into the boost range
    QCOMPARE(propertyService.minValue(volumeCc, portName), 0);
    QCOMPARE(propertyService.maxValue(volumeCc, portName), Constants::faderMaxMidiCcValue());
    QVERIFY(Constants::faderMaxMidiCcValue() > 127);

    // Everything else on the same device stays on the MIDI 1.0 range
    QCOMPARE(propertyService.maxValue(panCc, portName), 127);

    // As does the same controller once it is bound for real MIDI gear
    QCOMPARE(propertyService.maxValue(volumeCc, "Some External Port"), 127);
    QCOMPARE(propertyService.maxValue(volumeCc), 127);
    QCOMPARE(propertyService.maxValue(volumeCc, ""), 127);
}

void PropertyServiceTest::test_getAvailableMidiControllers_internalDevice_shouldCarryDeviceRanges()
{
    auto audioEngine = std::make_shared<AudioEngine>();
    auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
    PropertyService propertyService;
    propertyService.setDeviceService(deviceService);

    deviceService->setDevice(0, std::make_shared<SynthDevice>("Synth 1"));
    const QString portName = Constants::internalDevicePortPrefix() + " 1";

    // The ranges used to be hardcoded here, which is what kept the device's own range from ever
    // reaching the automation editor
    const auto controllers = propertyService.getAvailableMidiControllers(portName);
    QVERIFY(!controllers.isEmpty());

    const auto rangeOf = [&controllers](int number) {
        for (auto && controller : controllers) {
            if (const auto map = controller.toMap(); map["number"].toInt() == number) {
                return map["maxValue"].toInt();
            }
        }
        return -1;
    };

    QCOMPARE(rangeOf(static_cast<int>(MidiCcMapping::Controller::ChannelVolumeMSB)), Constants::faderMaxMidiCcValue());
    QCOMPARE(rangeOf(static_cast<int>(MidiCcMapping::Controller::PanMSB)), 127);

    // A port with no internal device behind it stays on MIDI 1.0 throughout
    for (auto && controller : propertyService.getAvailableMidiControllers("Some External Port")) {
        QCOMPARE(controller.toMap()["maxValue"].toInt(), 127);
    }
}

void PropertyServiceTest::test_getAvailableMidiControllers_withInternalDevice_shouldReturnDeviceSpecificControllers()
{
    auto audioEngine = std::make_shared<AudioEngine>();
    auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
    PropertyService propertyService;
    propertyService.setDeviceService(deviceService);

    // Set up a Sampler in slot 0
    auto sampler = std::make_shared<SamplerDevice>("Sampler 1");
    deviceService->setDevice(0, sampler);
    const QString samplerPortName = Constants::internalDevicePortPrefix() + " 1";

    // Set up a DrumSynth in slot 1
    auto drumSynth = std::make_shared<DrumSynthDevice>("Drums 1");
    deviceService->setDevice(1, drumSynth);
    const QString drumSynthPortName = Constants::internalDevicePortPrefix() + " 2";

    // Set up a BassSynth in slot 2
    auto bassSynth = std::make_shared<BassSynthDevice>("Bass 1");
    deviceService->setDevice(2, bassSynth);
    const QString bassSynthPortName = Constants::internalDevicePortPrefix() + " 3";

    // Test Sampler CCs
    {
        const auto controllers = propertyService.getAvailableMidiControllers(samplerPortName);
        // Fader + Pan + LPF + HPF + (16 pads * 4 CCs per pad) = 4 + 64 = 68
        QCOMPARE(controllers.size(), 68);
        QCOMPARE(controllers.at(0).toMap()["name"].toString(), "7: Fader");
        QCOMPARE(controllers.at(1).toMap()["name"].toString(), "10: Pan");
        QCOMPARE(controllers.at(2).toMap()["name"].toString(), "74: LPF");
        QCOMPARE(controllers.at(3).toMap()["name"].toString(), "81: HPF");
        // Pad CCs name the note they drive, so the list can be read against the tracker
        QCOMPARE(controllers.at(4).toMap()["name"].toString(), "16: Pad 1 Pan (C-3)");
        QCOMPARE(controllers.at(5).toMap()["name"].toString(), "32: Pad 1 Volume (C-3)");
        QCOMPARE(controllers.at(6).toMap()["name"].toString(), "48: Pad 1 LPF (C-3)");
        QCOMPARE(controllers.at(7).toMap()["name"].toString(), "102: Pad 1 HPF (C-3)");
        QCOMPARE(controllers.at(67).toMap()["name"].toString(), "117: Pad 16 HPF (D#4)");
        // Device-wide CCs drive no single note and stay unqualified
        QCOMPARE(controllers.at(1).toMap()["name"].toString(), "10: Pan");
    }

    // Test DrumSynth CCs
    {
        const auto controllers = propertyService.getAvailableMidiControllers(drumSynthPortName);
        // Volume + Pan + (11 voices * 3 CCs per voice) = 2 + 33 = 35
        QCOMPARE(controllers.size(), 35);
        QCOMPARE(controllers.at(0).toMap()["name"].toString(), "7: Fader");
        QCOMPARE(controllers.at(1).toMap()["name"].toString(), "10: Pan");
        QCOMPARE(controllers.at(2).toMap()["name"].toString(), "14: Kick Pan");
    }

    // Test BassSynth CCs
    {
        const auto controllers = propertyService.getAvailableMidiControllers(bassSynthPortName);
        QCOMPARE(controllers.size(), 4);
        QCOMPARE(controllers.at(2).toMap()["name"].toString(), "74: LPF");
        QCOMPARE(controllers.at(3).toMap()["name"].toString(), "81: HPF");
    }

    // Test non-existent internal device (should fall back to 128 CCs)
    {
        const QString invalidPortName = Constants::internalDevicePortPrefix() + " 8";
        const auto controllers = propertyService.getAvailableMidiControllers(invalidPortName);
        QCOMPARE(controllers.size(), 128);
    }

    // Test with custom device name
    {
        const auto controllers = propertyService.getAvailableMidiControllers("Sampler 1");
        QCOMPARE(controllers.size(), 68);
        QCOMPARE(controllers.at(0).toMap()["name"].toString(), "7: Fader");
    }
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::PropertyServiceTest)
