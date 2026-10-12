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

#include "../../application/note_converter.hpp"
#include "../../application/service/device_service.hpp"
#include "../../application/service/property_service.hpp"
#include "../../common/constants.hpp"
#include "../../domain/devices/bass_synth_device.hpp"
#include "../../domain/devices/drum_synth_device.hpp"
#include "../../domain/devices/drum_synth_v2_constants.hpp"
#include "../../domain/devices/drum_synth_v2_device.hpp"
#include "../../domain/devices/fm_synth_device.hpp"
#include "../../domain/devices/kick_808_device.hpp"
#include "../../domain/devices/piano_synth_device.hpp"
#include "../../domain/devices/piano_synth_v2_device.hpp"
#include "../../domain/devices/piano_synth_v3_device.hpp"
#include "../../domain/devices/sampler_device.hpp"
#include "../../domain/devices/speech_device.hpp"
#include "../../domain/devices/string_ensemble_device.hpp"
#include "../../domain/devices/string_voice_device.hpp"
#include "../../domain/devices/string_voice_v2_device.hpp"
#include "../../domain/devices/string_voice_v3_device.hpp"
#include "../../domain/devices/sub_mixer_device.hpp"
#include "../../domain/devices/synth_device.hpp"
#include "../../domain/devices/wavetable_synth_device.hpp"
#include "../../domain/effects/auto_filter.hpp"
#include "../../domain/effects/gain.hpp"
#include "../../infra/audio/audio_engine.hpp"
#include "../../infra/audio/backend/audio_file_reader.hpp"
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

void PropertyServiceTest::test_automationTargets_shouldListDevicesThenEffects()
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

    // The Synth itself first, as the track's own instrument is what is most often ridden
    QCOMPARE(targets.size(), qsizetype { 3 });
    const auto synthTarget = targets.at(0).toMap();
    QCOMPARE(synthTarget["owner"].toString(), QString { "device" });
    QCOMPARE(synthTarget["ownerId"].toString(), QString::fromStdString(synth->uuid()));
    QCOMPARE(synthTarget["deviceSlot"].toInt(), 2);
    QCOMPARE(synthTarget["deviceTypeName"].toString(), QString::fromStdString(synth->typeName()));
    const auto device = targets.at(1).toMap();
    QCOMPARE(device["owner"].toString(), QString { "effect" });
    QCOMPARE(device["ownerId"].toString(), QString::fromStdString(inDevice->id()));
    QCOMPARE(device["typeId"].toString(), QString::fromStdString(AutoFilter::typeIdString()));
    // What the effect gallery names effects by
    QCOMPARE(device["type"].toString(), Constants::RackEffectType::autoFilter());
    QCOMPARE(device["rack"].toInt(), static_cast<int>(DeviceService::EffectLocation::Rack::DeviceInsert));
    QCOMPARE(device["deviceSlot"].toInt(), 2);
    QCOMPARE(device["effectSlot"].toInt(), 3);
    const auto master = targets.at(2).toMap();
    QCOMPARE(master["ownerId"].toString(), QString::fromStdString(onMaster->id()));
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

    const auto parameter = propertyService.automationParameter("effect", QString::fromStdString(gain->id()), Constants::NahdXml::xmlKeyGain());

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

    QVERIFY(propertyService.automationParameter("effect", "no-such-effect", Constants::NahdXml::xmlKeyCutoff()).isEmpty());
    QVERIFY(propertyService.automationParameter("effect", QString::fromStdString(filter->id()), "noSuchParameter").isEmpty());
}

void PropertyServiceTest::test_automationTargets_device_shouldOfferOnlyDescribedParameters()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
    PropertyService propertyService;
    propertyService.setDeviceService(deviceService);
    deviceService->setDevice(0, std::make_shared<SynthDevice>("Synth"));

    const auto synthTarget = propertyService.automationTargets().at(0).toMap();

    // A Synth registers well over a hundred; only the controls it describes are offered, the inner
    // workings of its voices left out
    QStringList keys;
    for (auto && parameter : synthTarget["parameters"].toList()) {
        keys.push_back(parameter.toMap()["key"].toString());
    }
    for (auto && key : { Constants::NahdXml::xmlKeyLpfCutoff(), Constants::NahdXml::xmlKeyLpfResonance(), Constants::NahdXml::xmlKeyKeyTrack(), Constants::NahdXml::xmlKeyHpfCutoff(), Constants::NahdXml::xmlKeyLfoRate(), Constants::NahdXml::xmlKeyVco1Shape(), Constants::NahdXml::xmlKeyMixLevel2() }) {
        QVERIFY2(keys.contains(key), qPrintable(key));
    }
    QVERIFY(!keys.contains(Constants::NahdXml::xmlKeyVco1Roundness()));
    QVERIFY(keys.size() < static_cast<qsizetype>(std::make_shared<SynthDevice>("Synth")->parameters().size()) / 3);

    const auto cutoff = parameterOf(synthTarget, Constants::NahdXml::xmlKeyLpfCutoff());
    QCOMPARE(cutoff["label"].toString(), QString { "LPF Cutoff" });
    QCOMPARE(cutoff["unit"].toString(), QString { "Hz" });
    QCOMPARE(cutoff["min"].toDouble(), 20.0);
}

void PropertyServiceTest::test_automationTargets_deviceWithoutDescriptions_shouldOfferItsChannelStrip()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
    PropertyService propertyService;
    propertyService.setDeviceService(deviceService);
    // A sub-mixer has nothing of its own to ride, but it has the fader, gain and pan every device has
    deviceService->setDevice(0, std::make_shared<SubMixerDevice>("Sub Mixer"));

    const auto targets = propertyService.automationTargets();
    QCOMPARE(targets.size(), 1);
    const auto parameters = targets.at(0).toMap()["parameters"].toList();
    QCOMPARE(parameters.size(), 3);
    const auto fader = parameterOf(targets.at(0).toMap(), Constants::NahdXml::xmlKeyFader());
    QCOMPARE(fader["unit"].toString(), QString { "dB" });
    QCOMPARE(fader["mapping"].toString(), QString { "fader" });
    QVERIFY(!parameterOf(targets.at(0).toMap(), Constants::NahdXml::xmlKeyGain()).isEmpty());
    QVERIFY(!parameterOf(targets.at(0).toMap(), Constants::NahdXml::xmlKeyPan()).isEmpty());
}

void PropertyServiceTest::test_automationParameter_device_shouldFindIt()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
    PropertyService propertyService;
    propertyService.setDeviceService(deviceService);
    const auto synth = std::make_shared<SynthDevice>("Synth");
    deviceService->setDevice(3, synth);
    const auto uuid = QString::fromStdString(synth->uuid());

    QCOMPARE(propertyService.automationParameter("device", uuid, Constants::NahdXml::xmlKeyHpfCutoff())["label"].toString(), QString { "HPF Cutoff" });
    // Undescribed device parameters are not offered, and an effect id does not find a device
    QVERIFY(propertyService.automationParameter("device", uuid, Constants::NahdXml::xmlKeyLpfSlope()).isEmpty());
    QVERIFY(propertyService.automationParameter("effect", uuid, Constants::NahdXml::xmlKeyHpfCutoff()).isEmpty());
}

void PropertyServiceTest::test_automationTargetsForPort_shouldOfferOnlyTheDeviceAndItsEffects()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
    PropertyService propertyService;
    propertyService.setDeviceService(deviceService);

    const auto ownSynth = std::make_shared<SynthDevice>("Own");
    const auto ownInsert = std::make_shared<AutoFilter>();
    ownSynth->insertEffectRack().setEffect(2, ownInsert);
    deviceService->setDevice(0, ownSynth);
    const auto otherSynth = std::make_shared<SynthDevice>("Other");
    otherSynth->insertEffectRack().setEffect(0, std::make_shared<AutoFilter>());
    deviceService->setDevice(1, otherSynth);
    deviceService->insertEffectRack().setEffect(0, std::make_shared<AutoFilter>());
    // Everything there is: two devices, and an effect in each rack
    QCOMPARE(propertyService.automationTargets().size(), qsizetype { 5 });

    const auto port = QString::fromStdString(deviceService->internalDeviceNames().at(0));
    const auto targets = propertyService.automationTargetsForPort(port);

    // The track's own device and the effect in its own rack: not the other device, not its effect,
    // not the master
    QCOMPARE(targets.size(), qsizetype { 2 });
    QCOMPARE(targets.at(0).toMap()["owner"].toString(), QString { "device" });
    QCOMPARE(targets.at(0).toMap()["ownerId"].toString(), QString::fromStdString(ownSynth->uuid()));
    QCOMPARE(targets.at(1).toMap()["owner"].toString(), QString { "effect" });
    QCOMPARE(targets.at(1).toMap()["ownerId"].toString(), QString::fromStdString(ownInsert->id()));
}

void PropertyServiceTest::test_automationTargetsForPort_externalPort_shouldBeEmpty()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
    PropertyService propertyService;
    propertyService.setDeviceService(deviceService);
    deviceService->setDevice(0, std::make_shared<SynthDevice>("Synth"));
    deviceService->insertEffectRack().setEffect(0, std::make_shared<AutoFilter>());

    // Nothing behind the port to narrow the list down to
    QVERIFY(propertyService.automationTargetsForPort("Some MIDI Port").isEmpty());
    QVERIFY(propertyService.automationTargetsForPort({}).isEmpty());
}

namespace {

//! Opens anything as a short stereo file of silence, so a pad can be loaded without a file.
class MockAudioFileReader : public AudioFileReader
{
public:
    bool open(const std::string &, Mode, Info & info) override
    {
        info.frames = 1000;
        info.samplerate = static_cast<uint32_t>(Constants::defaultSampleRate());
        info.channels = 2;
        m_info = info;
        m_isOpen = true;
        return true;
    }

    void close() override
    {
        m_isOpen = false;
    }

    void setTag(TagType type, const std::string & value) override
    {
        (void)type;
        (void)value;
    }

    int64_t readFloat(std::span<float> data) override
    {
        std::fill(data.begin(), data.end(), 0.0f);
        return static_cast<int64_t>(data.size() / static_cast<size_t>(m_info.channels));
    }

    int64_t readDouble(std::span<double> data) override
    {
        std::fill(data.begin(), data.end(), 0.0);
        return static_cast<int64_t>(data.size() / static_cast<size_t>(m_info.channels));
    }

    int64_t readInt(std::span<int32_t> data) override
    {
        std::fill(data.begin(), data.end(), 0);
        return static_cast<int64_t>(data.size() / static_cast<size_t>(m_info.channels));
    }

    int64_t writeFloat(std::span<const float> data) override
    {
        return static_cast<int64_t>(data.size() / static_cast<size_t>(m_info.channels));
    }

    int64_t writeInt(std::span<const int32_t> data) override
    {
        return static_cast<int64_t>(data.size() / static_cast<size_t>(m_info.channels));
    }

    bool seek(int64_t, int) override
    {
        return true;
    }

    bool isOpen() const override
    {
        return m_isOpen;
    }

    Info info() const override
    {
        return m_info;
    }

private:
    bool m_isOpen = false;
    Info m_info;
};

} // namespace

void PropertyServiceTest::test_automationTargets_sampler_shouldListEachLoadedPad()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
    PropertyService propertyService;
    propertyService.setDeviceService(deviceService);
    const auto sampler = std::make_shared<SamplerDevice>("Sampler", std::make_unique<MockAudioFileReader>());
    sampler->loadSample(36, "kick.wav");
    sampler->loadSample(38, "snare.wav");
    deviceService->setDevice(0, sampler);

    // The device's own channel strip, then a target for each loaded pad, in note order, named by
    // the note and what is on it
    const auto targets = propertyService.automationTargets();
    QCOMPARE(targets.size(), 3);
    QVERIFY(targets.at(0).toMap()["group"].toString().isEmpty());
    QVERIFY(!parameterOf(targets.at(0).toMap(), Constants::NahdXml::xmlKeyFader()).isEmpty());
    const auto kick = targets.at(1).toMap();
    const auto snare = targets.at(2).toMap();
    QCOMPARE(kick["ownerId"].toString(), QString::fromStdString(sampler->uuid()));
    QCOMPARE(kick["group"].toString(), QString { "Pad %1 (kick.wav)" }.arg(QString::fromStdString(NoteConverter::midiToString(36))));
    QCOMPARE(snare["group"].toString(), QString { "Pad %1 (snare.wav)" }.arg(QString::fromStdString(NoteConverter::midiToString(38))));

    const auto cutoffKey = QString::fromStdString(SamplerDevice::padAutomationKey(38, Constants::NahdXml::xmlKeyCutoff().toStdString()));
    const auto cutoff = parameterOf(snare, cutoffKey);
    QCOMPARE(cutoff["unit"].toString(), QString { "Hz" });
    QCOMPARE(cutoff["mapping"].toString(), QString { "logFrequency" });

    // And an automation's key leads back to it
    const auto found = propertyService.automationParameter("device", QString::fromStdString(sampler->uuid()), cutoffKey);
    QCOMPARE(found["key"].toString(), cutoffKey);
}

void PropertyServiceTest::test_automationTargets_drumSynth_shouldListEachDrum()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
    PropertyService propertyService;
    propertyService.setDeviceService(deviceService);
    deviceService->setDevice(0, std::make_shared<DrumSynthV2Device>("Drums"));

    // The device's own channel strip, then one target for each drum of the kit, in kit order, with
    // its level, pan and filters at least
    const auto targets = propertyService.automationTargets();
    QCOMPARE(targets.size(), DrumSynthV2::NumVoices + 1);
    QVERIFY(targets.at(0).toMap()["group"].toString().isEmpty());
    QVERIFY(!parameterOf(targets.at(0).toMap(), Constants::NahdXml::xmlKeyFader()).isEmpty());
    for (int voice = 0; voice < DrumSynthV2::NumVoices; voice++) {
        const auto target = targets.at(voice + 1).toMap();
        QCOMPARE(target["group"].toString(), DrumSynthV2::voiceName(voice));
        const auto prefix = QString::fromStdString(DrumSynthV2::voiceId(voice) + "_");
        for (auto && key : { Constants::NahdXml::xmlKeyLevel(), Constants::NahdXml::xmlKeyPan(), Constants::NahdXml::xmlKeyCutoff(), Constants::NahdXml::xmlKeyHpfCutoff(), Constants::NahdXml::xmlKeyAmpDecay() }) {
            QVERIFY2(!parameterOf(target, prefix + key).isEmpty(), qPrintable(prefix + key));
        }
    }
    const auto kickCutoff = parameterOf(targets.at(1).toMap(), QString::fromStdString(DrumSynthV2::voiceId(0) + "_") + Constants::NahdXml::xmlKeyCutoff());
    QCOMPARE(kickCutoff["unit"].toString(), QString { "Hz" });
}

void PropertyServiceTest::test_automationTargets_everySynth_shouldOfferItsControls()
{
    // Every instrument offers its filter, in hertz, and the controls that make it what it is. The
    // register and instrument buttons are switches, and a switch has nothing between its positions.
    const auto check = [](const std::shared_ptr<Device> & device, const QStringList & expectedKeys) {
        const auto audioEngine = std::make_shared<AudioEngine>();
        const auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
        PropertyService propertyService;
        propertyService.setDeviceService(deviceService);
        deviceService->setDevice(0, device);
        const auto targets = propertyService.automationTargets();
        QVERIFY2(targets.size() == 1, device->typeName().c_str());
        const auto target = targets.at(0).toMap();
        const auto cutoff = parameterOf(target, Constants::NahdXml::xmlKeyLpfCutoff());
        QVERIFY2(cutoff["unit"].toString() == "Hz", device->typeName().c_str());
        QVERIFY2(!parameterOf(target, Constants::NahdXml::xmlKeyHpfCutoff()).isEmpty(), device->typeName().c_str());
        for (auto && key : expectedKeys) {
            QVERIFY2(!parameterOf(target, key).isEmpty(), qPrintable(QString::fromStdString(device->typeName()) + ": " + key));
        }
    };
    namespace C = Constants::NahdXml;
    check(std::make_shared<BassSynthDevice>("Bass"), { C::xmlKeyLpfResonance(), C::xmlKeyEnvMod(), C::xmlKeyAccent(), C::xmlKeyPitch(), C::xmlKeyDistDrive() });
    check(std::make_shared<Kick808Device>("Kick"), { C::xmlKeyTune(), C::xmlKeyDecay(), C::xmlKeyPitchDepth() });
    check(std::make_shared<WavetableSynthDevice>("Wavetable"), { C::xmlKeyLpfResonance(), C::xmlKeyOsc1Pos(), C::xmlKeyOsc2Pos(), C::xmlKeyLfoRate(), C::xmlKeyLfo2Intensity(), C::xmlKeyModIntensity() });
    check(std::make_shared<FmSynthDevice>("FM"), { C::xmlKeyLpfResonance(), C::xmlKeyFeedback(), C::xmlKeyOperatorLevel(1), C::xmlKeyOperatorDetune(3), C::xmlKeyLfoRate() });
    check(std::make_shared<PianoSynthDevice>("Piano"), { C::xmlKeyBrightness(), C::xmlKeyHardness() });
    check(std::make_shared<PianoSynthV2Device>("Piano V2"), { C::xmlKeyBrightness(), C::xmlKeyRichness(), C::xmlKeyStretch() });
    check(std::make_shared<PianoSynthV3Device>("Piano V3"), { C::xmlKeyBrightness(), C::xmlKeyRichness(), C::xmlKeyAmpVelocitySensitivity() });
    check(std::make_shared<StringVoiceDevice>("String Voice"), { C::xmlKeyVoiceBalance(), C::xmlKeyStringsLevel8(), C::xmlKeyVibratoDepth() });
    check(std::make_shared<StringVoiceV2Device>("String Voice V2"), { C::xmlKeyVoiceBalance(), C::xmlKeyStringsTone(), C::xmlKeyVelocitySensitivity() });
    check(std::make_shared<StringVoiceV3Device>("String Voice V3"), { C::xmlKeyVoiceBalance(), C::xmlKeyStringsTone(), C::xmlKeyVelocitySensitivity() });
    check(std::make_shared<StringEnsembleDevice>("Ensemble"), { C::xmlKeyVolumeBass(), C::xmlKeyPhaserColor(), C::xmlKeyPhaserRate() });
    check(std::make_shared<SpeechDevice>("Speech"), { C::xmlKeyFormantShift(), C::xmlKeyBreathiness(), C::xmlKeyVibratoDepth() });

    // In their units: a wavetable LFO's rate in hertz, its intensity either way of none
    const auto wavetable = std::make_shared<WavetableSynthDevice>("Wavetable");
    for (auto && entry : wavetable->automatableParameters()) {
        if (entry.key == C::xmlKeyLfoRate().toStdString()) {
            QCOMPARE(entry.parameter.display()->unit, std::string { "Hz" });
        } else if (entry.key == C::xmlKeyLfoIntensity().toStdString()) {
            QCOMPARE(entry.parameter.display()->restPosition(), 0.5);
        }
    }
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::PropertyServiceTest)
