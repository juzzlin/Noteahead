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
#include "../../domain/effects/auto_filter.hpp"
#include "../../domain/effects/gain.hpp"
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
        // Fader + Pan + LPF + HPF + (16 pads * 4 CCs per pad) + Expression = 4 + 64 + 1 = 69
        QCOMPARE(controllers.size(), 69);
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
        // Device appends this to every device's list, so it is last whatever the device offers
        QCOMPARE(controllers.at(68).toMap()["name"].toString(), "11: Expression");
        // Device-wide CCs drive no single note and stay unqualified
        QCOMPARE(controllers.at(1).toMap()["name"].toString(), "10: Pan");
    }

    // Test DrumSynth CCs
    {
        const auto controllers = propertyService.getAvailableMidiControllers(drumSynthPortName);
        // Volume + Pan + (11 voices * 3 CCs per voice) + Expression = 2 + 33 + 1 = 36
        QCOMPARE(controllers.size(), 36);
        QCOMPARE(controllers.at(0).toMap()["name"].toString(), "7: Fader");
        QCOMPARE(controllers.at(1).toMap()["name"].toString(), "10: Pan");
        QCOMPARE(controllers.at(2).toMap()["name"].toString(), "14: Kick Pan");
    }

    // Test BassSynth CCs
    {
        const auto controllers = propertyService.getAvailableMidiControllers(bassSynthPortName);
        QCOMPARE(controllers.size(), 5);
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
        QCOMPARE(controllers.size(), 69);
        QCOMPARE(controllers.at(0).toMap()["name"].toString(), "7: Fader");
    }
}

namespace {

QVariantMap parameterOf(const QVariantMap & target, const QString & key)
{
    for (auto && parameter : target["parameters"].toList()) {
        if (const auto map = parameter.toMap(); map["key"].toString() == key) {
            return map;
        }
    }
    return {};
}

} // namespace

void PropertyServiceTest::test_automationTargets_shouldListEveryEffectWithItsLocation()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
    PropertyService propertyService;
    propertyService.setDeviceService(deviceService);

    const auto synth = std::make_shared<SynthDevice>("Synth");
    const auto inDevice = std::make_shared<AutoFilter>();
    synth->insertEffectRack().setEffect(3, inDevice);
    deviceService->setDevice(2, synth);
    const auto onMaster = std::make_shared<AutoFilter>();
    deviceService->insertEffectRack().setEffect(1, onMaster);

    const auto targets = propertyService.automationTargets();

    QCOMPARE(targets.size(), qsizetype { 2 });
    const auto device = targets.at(0).toMap();
    QCOMPARE(device["effectId"].toString(), QString::fromStdString(inDevice->id()));
    QCOMPARE(device["typeId"].toString(), QString::fromStdString(AutoFilter::typeIdString()));
    // What the effect gallery names effects by
    QCOMPARE(device["type"].toString(), Constants::RackEffectType::autoFilter());
    QCOMPARE(device["rack"].toInt(), static_cast<int>(DeviceService::EffectLocation::Rack::DeviceInsert));
    QCOMPARE(device["deviceSlot"].toInt(), 2);
    QCOMPARE(device["effectSlot"].toInt(), 3);
    const auto master = targets.at(1).toMap();
    QCOMPARE(master["effectId"].toString(), QString::fromStdString(onMaster->id()));
    QCOMPARE(master["rack"].toInt(), static_cast<int>(DeviceService::EffectLocation::Rack::MasterInsert));
    QCOMPARE(master["effectSlot"].toInt(), 1);
}

void PropertyServiceTest::test_automationTargets_autoFilter_shouldDescribeItsParameters()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
    PropertyService propertyService;
    propertyService.setDeviceService(deviceService);
    deviceService->insertEffectRack().setEffect(0, std::make_shared<AutoFilter>());

    const auto target = propertyService.automationTargets().at(0).toMap();

    const auto cutoff = parameterOf(target, Constants::NahdXml::xmlKeyCutoff());
    QCOMPARE(cutoff["label"].toString(), QString { "Cutoff" });
    QCOMPARE(cutoff["unit"].toString(), QString { "Hz" });
    QCOMPARE(cutoff["mapping"].toString(), QString { "logFrequency" });
    QCOMPARE(cutoff["min"].toDouble(), 20.0);
    QCOMPARE(cutoff["max"].toDouble(), 20000.0);
    QCOMPARE(cutoff["rest"].toDouble(), 0.0);
    QCOMPARE(parameterOf(target, Constants::NahdXml::xmlKeyLfoIntensity())["rest"].toDouble(), 0.5);
    // A switch is not offered: a position between two of its choices means nothing
    QVERIFY(parameterOf(target, Constants::NahdXml::xmlKeyFilterType()).isEmpty());
}

void PropertyServiceTest::test_automationTargets_parameterWithoutDisplay_shouldBePercentOfTravel()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
    PropertyService propertyService;
    propertyService.setDeviceService(deviceService);
    const auto gain = std::make_shared<Gain>();
    deviceService->insertEffectRack().setEffect(0, gain);

    const auto parameter = propertyService.automationParameter(QString::fromStdString(gain->id()), Constants::NahdXml::xmlKeyGain());

    QCOMPARE(parameter["label"].toString(), Constants::NahdXml::xmlKeyGain());
    QCOMPARE(parameter["unit"].toString(), QString { "%" });
    QCOMPARE(parameter["mapping"].toString(), QString { "linear" });
    QCOMPARE(parameter["max"].toDouble(), 100.0);
}

void PropertyServiceTest::test_automationParameter_missing_shouldBeEmpty()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
    PropertyService propertyService;
    propertyService.setDeviceService(deviceService);
    const auto filter = std::make_shared<AutoFilter>();
    deviceService->insertEffectRack().setEffect(0, filter);

    QVERIFY(propertyService.automationParameter("no-such-effect", Constants::NahdXml::xmlKeyCutoff()).isEmpty());
    QVERIFY(propertyService.automationParameter(QString::fromStdString(filter->id()), "noSuchParameter").isEmpty());
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::PropertyServiceTest)
