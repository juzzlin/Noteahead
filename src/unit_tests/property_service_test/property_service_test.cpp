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

#include "application/service/device_service.hpp"
#include "application/service/property_service.hpp"
#include "common/constants.hpp"
#include "domain/devices/bass_synth_device.hpp"
#include "domain/devices/drum_synth_device.hpp"
#include "domain/devices/sampler_device.hpp"
#include "domain/devices/synth_device.hpp"
#include "infra/audio/audio_engine.hpp"

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

void PropertyServiceTest::test_getAvailableMidiControllers_withInternalDevice_shouldReturnDeviceSpecificControllers()
{
    auto audioEngine = std::make_shared<AudioEngine>();
    auto deviceService = std::make_shared<DeviceService>(audioEngine);
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
        QCOMPARE(controllers.size(), 4);
        QCOMPARE(controllers.at(0).toMap()["name"].toString(), "7: Volume");
        QCOMPARE(controllers.at(1).toMap()["name"].toString(), "10: Pan");
        QCOMPARE(controllers.at(2).toMap()["name"].toString(), "74: LPF");
        QCOMPARE(controllers.at(3).toMap()["name"].toString(), "81: HPF");
    }

    // Test DrumSynth CCs
    {
        const auto controllers = propertyService.getAvailableMidiControllers(drumSynthPortName);
        // Volume + Pan + (11 voices * 3 CCs per voice) = 2 + 33 = 35
        QCOMPARE(controllers.size(), 35);
        QCOMPARE(controllers.at(0).toMap()["name"].toString(), "7: Volume");
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
        QCOMPARE(controllers.size(), 4);
        QCOMPARE(controllers.at(0).toMap()["name"].toString(), "7: Volume");
    }
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::PropertyServiceTest)
