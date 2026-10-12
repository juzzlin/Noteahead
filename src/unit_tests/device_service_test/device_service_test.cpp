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

#include "device_service_test.hpp"

#include "../../application/service/device_service.hpp"
#include "../../common/constants.hpp"
#include "../../domain/devices/device_factory.hpp"
#include "../../domain/devices/drum_synth_constants.hpp"
#include "../../domain/devices/drum_synth_device.hpp"
#include "../../domain/devices/sampler_device.hpp"
#include "../../domain/devices/sub_mixer_device.hpp"
#include "../../domain/devices/synth_device.hpp"
#include "../../domain/effects/auto_filter.hpp"
#include "../../domain/effects/effect_factory.hpp"
#include "../../domain/effects/gain.hpp"
#include "../../domain/tracker/parameter_data.hpp"
#include "../../infra/audio/audio_engine.hpp"
#include "../../infra/data_service.hpp"
#include "../../infra/xml/nahd_xml_reader.hpp"
#include "../../infra/xml/nahd_xml_writer.hpp"

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

#include <QBuffer>
#include <QFile>
#include <QSignalSpy>
#include <QTemporaryFile>
#include <QTest>
#include <QVariant>

namespace noteahead {

class MockAudioFileReader : public AudioFileReader
{
public:
    bool open(const std::string &, Mode mode, Info & info) override
    {
        m_isOpen = true;
        m_mode = mode;
        if (mode == Mode::Read) {
            info.frames = 100;
            info.channels = 2;
            info.samplerate = 44100;
            m_info = info;
        }
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

    bool isOpen() const override
    {
        return m_isOpen;
    }

    Info info() const override
    {
        return m_info;
    }

    int64_t writeFloat(std::span<const float> data) override
    {
        m_writtenData.append(reinterpret_cast<const char *>(data.data()), static_cast<qsizetype>(data.size_bytes()));
        return static_cast<int64_t>(data.size() / 2);
    }

    int64_t readFloat(std::span<float> data) override
    {
        if (m_mode == Mode::Read && !m_writtenData.isEmpty()) {
            const size_t toRead = std::min(data.size_bytes(), static_cast<size_t>(m_writtenData.size()));
            std::memcpy(data.data(), m_writtenData.data(), toRead);
            return static_cast<int64_t>(toRead / sizeof(float) / 2);
        }
        std::fill(data.begin(), data.end(), 0.0f);
        return static_cast<int64_t>(data.size() / 2);
    }

    int64_t readDouble(std::span<double>) override
    {
        return 0;
    }

    int64_t readInt(std::span<int32_t>) override
    {
        return 0;
    }

    int64_t writeInt(std::span<const int32_t>) override
    {
        return 0;
    }

    bool seek(int64_t, int) override
    {
        return true;
    }

private:
    bool m_isOpen = false;
    Info m_info {};
    Mode m_mode = Mode::Read;
    QByteArray m_writtenData;
};

// Simulates a real reader (e.g. SndFileReader) that cannot open an unresolved embedded path.
// Used to prove that embedded data is extracted before samples are loaded during import.
class NahdFailingMockAudioFileReader : public MockAudioFileReader
{
public:
    bool open(const std::string & path, Mode mode, Info & info) override
    {
        if (QString::fromStdString(path).startsWith(Constants::NahdXml::embeddedDataPathPrefix())) {
            return false;
        }
        return MockAudioFileReader::open(path, mode, info);
    }
};

void DeviceServiceTest::initTestCase()
{
    EffectFactory::init();
    DeviceFactory::init();
}

void DeviceServiceTest::cleanupTestCase()
{
    EffectFactory::clear();
    DeviceFactory::clear();
}

void DeviceServiceTest::test_midiCc_shouldNotEmitDataChanged()
{
    // Automation traffic arrives many times per beat. DeviceService::dataChanged resets the Device
    // Rack model, rebuilds the MIDI port lists and marks the song modified, so routing MIDI CC
    // through it made playing an automated song cost far more than playing the same song without.
    const auto audioEngine = std::make_shared<AudioEngine>();
    DeviceService deviceService { audioEngine, std::make_shared<DataService>() };
    const auto device = std::make_shared<SynthDevice>("Synth 1");
    deviceService.setDevice(0, device);

    QSignalSpy structuralSpy { &deviceService, &DeviceService::dataChanged };
    QSignalSpy parameterSpy { device.get(), &Device::parametersChanged };

    device->processMidiCc(7, 64, 0);

    QCOMPARE(structuralSpy.count(), 0);
    QCOMPARE(parameterSpy.count(), 1);

    // Stopping the transport sends all-notes-off to every device, so that is transport traffic too:
    // routing it through dataChanged() marked the project modified just for playing it.
    device->processMidiAllNotesOff();
    QCOMPARE(structuralSpy.count(), 0);

    // A real edit still has to reach the rest of the application
    device->setVolume(0.1f);
    QVERIFY(structuralSpy.count() > 0);
}

void DeviceServiceTest::test_midiCc_drumSynthVoice_shouldNotEmitDataChanged()
{
    // A voice CC used to announce a project edit. Besides marking the song modified on plain
    // automation traffic, it did so while holding the device mutex: the receivers read back from
    // the audio engine, whose callback holds the engine mutex and then waits for this very device
    // mutex, so applying track settings during playback deadlocked the two threads.
    const auto audioEngine = std::make_shared<AudioEngine>();
    DeviceService deviceService { audioEngine, std::make_shared<DataService>() };
    const auto device = std::make_shared<DrumSynthDevice>("Drum Synth 1");
    deviceService.setDevice(0, device);

    QSignalSpy structuralSpy { &deviceService, &DeviceService::dataChanged };
    QSignalSpy parameterSpy { device.get(), &Device::parametersChanged };

    // Kick HPF cutoff
    device->processMidiCc(DrumSynth::CcStartRange1 + 2, 64, 0);

    QCOMPARE(structuralSpy.count(), 0);
    QCOMPARE(parameterSpy.count(), 1);

    // The dialog's own writes are edits and still have to say so
    device->updateVoiceParameter(static_cast<int>(DrumSynth::VoiceIndex::Kick), Constants::NahdXml::xmlKeyHpfCutoff().toStdString(), 0.25f);
    QVERIFY(structuralSpy.count() > 0);
}

void DeviceServiceTest::test_notifyParameterAutomation_device_shouldAnnounceOnTheDevice()
{
    // The device's own dialog listens to the device, so the announcement has to reach it there,
    // carrying how long until the change lands
    const auto audioEngine = std::make_shared<AudioEngine>();
    DeviceService deviceService { audioEngine, std::make_shared<DataService>() };
    const auto device = std::make_shared<SynthDevice>("Synth 1");
    const auto other = std::make_shared<SynthDevice>("Synth 2");
    deviceService.setDevice(0, device);
    deviceService.setDevice(1, other);

    QSignalSpy deviceSpy { device.get(), &Device::parameterAutomationScheduled };
    QSignalSpy otherSpy { other.get(), &Device::parameterAutomationScheduled };
    QSignalSpy serviceSpy { &deviceService, &DeviceService::parameterAutomationScheduled };
    const ParameterData data { 0, 0, std::static_pointer_cast<ParameterAutomationTarget>(device), Constants::NahdXml::xmlKeyHpfCutoff().toStdString(), ParameterAutomation::Mode::Absolute, 0.5 };
    deviceService.notifyParameterAutomation(data, std::chrono::milliseconds { 42 });

    QCOMPARE(deviceSpy.count(), 1);
    QCOMPARE(deviceSpy.at(0).at(0).toLongLong(), 42);
    QCOMPARE(otherSpy.count(), 0);
    QCOMPARE(serviceSpy.count(), 1);
    QCOMPARE(serviceSpy.at(0).at(0).value<quintptr>(), reinterpret_cast<quintptr>(static_cast<ParameterAutomationTarget *>(device.get())));
}

void DeviceServiceTest::test_clearAutomation_shouldAnnounceOnEveryDevice()
{
    // Stopping lets go of every automation, and every dialog has to show the set values again
    const auto audioEngine = std::make_shared<AudioEngine>();
    DeviceService deviceService { audioEngine, std::make_shared<DataService>() };
    const auto device = std::make_shared<SynthDevice>("Synth 1");
    deviceService.setDevice(0, device);
    QSignalSpy deviceSpy { device.get(), &Device::parameterAutomationScheduled };

    deviceService.clearAutomation();
    QCOMPARE(deviceSpy.count(), 1);
}

void DeviceServiceTest::test_clearAutomation_shouldRestoreEveryDevice()
{
    // The sweep the transport runs when playback stops. Devices also restore themselves on CC 121,
    // but only where that message goes; this is what covers the rest.
    const auto audioEngine = std::make_shared<AudioEngine>();
    DeviceService deviceService { audioEngine, std::make_shared<DataService>() };

    const auto synth = std::make_shared<SynthDevice>("Synth 1");
    synth->setLpfCutoff(0.25f);
    deviceService.setDevice(0, synth);

    const auto drum = std::make_shared<DrumSynthDevice>("Drum Synth 1");
    const auto kickHpfKey = DrumSynth::voiceId(static_cast<int>(DrumSynth::VoiceIndex::Kick)) + "_" + Constants::NahdXml::xmlKeyHpfCutoff().toStdString();
    drum->updateVoiceParameter(static_cast<int>(DrumSynth::VoiceIndex::Kick), Constants::NahdXml::xmlKeyHpfCutoff().toStdString(), 0.25f);
    deviceService.setDevice(1, drum);

    synth->processMidiCc(74, 127, 0);
    drum->processMidiCc(16, 127, 0);
    QCOMPARE(synth->lpfCutoff(), 1.0f);
    QCOMPARE(drum->parameter(kickHpfKey)->get().value(), 1.0f);

    QSignalSpy structuralSpy { &deviceService, &DeviceService::dataChanged };

    deviceService.clearAutomation();

    QCOMPARE(synth->lpfCutoff(), 0.25f);
    QCOMPARE(drum->parameter(kickHpfKey)->get().value(), 0.25f);

    // Handing values back is not an edit: it undoes what playback did, it does not change the song.
    QCOMPARE(structuralSpy.count(), 0);
}

void DeviceServiceTest::test_allNotesOff_sampler_shouldNotEmitDataChanged()
{
    // The Sampler is the one that restores its manual values on all-notes-off, and it was the one
    // still announcing that as a data change on every stop.
    const auto audioEngine = std::make_shared<AudioEngine>();
    DeviceService deviceService { audioEngine, std::make_shared<DataService>() };
    const auto device = std::make_shared<SamplerDevice>("Sampler 1", std::make_unique<MockAudioFileReader>());
    deviceService.setDevice(0, device);

    QSignalSpy structuralSpy { &deviceService, &DeviceService::dataChanged };
    QSignalSpy parameterSpy { device.get(), &Device::parametersChanged };

    device->processMidiAllNotesOff();

    QCOMPARE(structuralSpy.count(), 0);
    QCOMPARE(parameterSpy.count(), 1);
}

void DeviceServiceTest::test_exportDeviceSettings_shouldGenerateCorrectXml()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto dataService = std::make_shared<DataService>();
    DeviceService service { audioEngine, dataService };

    service.setDevice(0, DeviceFactory::createDevice(SynthDevice::typeIdString(), "TestSynth"));

    QTemporaryFile tempFile;
    QVERIFY(tempFile.open());
    const auto filePath = tempFile.fileName();
    tempFile.close();

    QVERIFY(service.exportDeviceSettings(0, filePath));

    QFile file { filePath };
    QVERIFY(file.open(QIODevice::ReadOnly));
    NahdXmlReader reader { file };

    QVERIFY(reader.readNextStartElement());
    QCOMPARE(reader.name(), Constants::NahdXml::xmlKeySettings());
    QCOMPARE(reader.attribute(Constants::NahdXml::xmlKeyFileFormatVersion()), Constants::fileFormatVersion());

    QVERIFY(reader.readNextStartElement());
    QCOMPARE(reader.name(), Constants::NahdXml::xmlKeyDevice());
    QCOMPARE(reader.attribute(Constants::NahdXml::xmlKeyTypeId()), QString::fromStdString(SynthDevice::typeIdString()));
}

void DeviceServiceTest::test_importDeviceSettings_shouldRestoreParameters()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto dataService = std::make_shared<DataService>();
    DeviceService service { audioEngine, dataService };

    const auto synth = std::dynamic_pointer_cast<SynthDevice>(DeviceFactory::createDevice(SynthDevice::typeIdString(), "TestSynth"));
    service.setDevice(0, synth);

    // Set a custom value
    synth->setVolume(0.75f);

    QTemporaryFile tempFile;
    QVERIFY(tempFile.open());
    const auto filePath = tempFile.fileName();
    tempFile.close();

    QVERIFY(service.exportDeviceSettings(0, filePath));

    // Reset volume
    synth->setVolume(1.0f);
    QCOMPARE(synth->volume(), 1.0f);

    QVERIFY(service.importDeviceSettings(0, filePath));
    QCOMPARE(synth->volume(), 0.75f);
}

void DeviceServiceTest::test_importDeviceSettings_shouldReplaceDeviceIfTypeDiffers()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto dataService = std::make_shared<DataService>();
    DeviceService service { audioEngine, dataService };

    service.setDevice(0, DeviceFactory::createDevice(SynthDevice::typeIdString(), "TestSynth"));

    // Create a sampler settings file
    QTemporaryFile tempFile;
    QVERIFY(tempFile.open());
    const auto filePath = tempFile.fileName();
    tempFile.close();

    {
        DeviceService service2 { std::make_shared<AudioEngine>(), std::make_shared<DataService>() };
        service2.setDevice(0, DeviceFactory::createDevice(SamplerDevice::typeIdString(), "TestSampler"));
        QVERIFY(service2.exportDeviceSettings(0, filePath));
    }

    QCOMPARE(service.device(0)->typeId(), SynthDevice::typeIdString());
    QVERIFY(service.importDeviceSettings(0, filePath));
    QCOMPARE(service.device(0)->typeId(), SamplerDevice::typeIdString());
}

void DeviceServiceTest::test_importDeviceSettings_emptySlot_shouldCreateDeviceFromFile()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto dataService = std::make_shared<DataService>();
    DeviceService service { audioEngine, dataService };

    // Export a synth from another service instance
    QTemporaryFile tempFile;
    QVERIFY(tempFile.open());
    const auto filePath = tempFile.fileName();
    tempFile.close();

    {
        DeviceService service2 { std::make_shared<AudioEngine>(), std::make_shared<DataService>() };
        const auto synth = std::dynamic_pointer_cast<SynthDevice>(DeviceFactory::createDevice(SynthDevice::typeIdString(), "TestSynth"));
        synth->setVolume(0.75f);
        service2.setDevice(0, synth);
        QVERIFY(service2.exportDeviceSettings(0, filePath));
    }

    // The target slot is empty; import should create the device from the file's type
    QVERIFY(!service.device(0));
    QVERIFY(service.importDeviceSettings(0, filePath));
    QVERIFY(service.device(0));
    QCOMPARE(service.device(0)->typeId(), SynthDevice::typeIdString());
    const auto synth = std::dynamic_pointer_cast<SynthDevice>(service.device(0));
    QVERIFY(synth);
    QCOMPARE(synth->volume(), 0.75f);
}

void DeviceServiceTest::test_replaceDevice_shouldKeepChannelStrip()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto dataService = std::make_shared<DataService>();
    DeviceService service { audioEngine, dataService };

    const auto synth = DeviceFactory::createDevice(SynthDevice::typeIdString(), "TestSynth");
    service.setDevice(0, synth);

    const auto gain = EffectFactory::createEffect(Gain::typeIdString());
    QVERIFY(gain);
    gain->parameter(Constants::NahdXml::xmlKeyGain().toStdString())->get().setValue(0.75f);
    synth->insertEffectRack().setEffect(1, gain);
    synth->setVolume(0.6f);
    synth->setGain(0.4f);
    synth->setPan(0.3f);
    synth->setFaderPosition(Device::FaderPosition::PostInserts);
    synth->setSendTap(Device::SendTap::PreFader);
    synth->setReverbSend(2, 0.5f);

    service.replaceDevice(0, DeviceFactory::createDevice(SamplerDevice::typeIdString(), "TestSynth"));

    const auto replacement = service.device(0);
    QVERIFY(replacement);
    QCOMPARE(replacement->typeId(), SamplerDevice::typeIdString());

    const auto adopted = replacement->insertEffectRack().effect(1);
    QVERIFY(adopted);
    QCOMPARE(adopted->typeId(), Gain::typeIdString());
    QCOMPARE(adopted->parameter(Constants::NahdXml::xmlKeyGain().toStdString())->get().value(), 0.75f);
    // A clone, so editing the new rack cannot reach into the old device's.
    QVERIFY(adopted != gain);

    QCOMPARE(replacement->volume(), 0.6f);
    QCOMPARE(replacement->gain(), 0.4f);
    QCOMPARE(replacement->pan(), 0.3f);
    QCOMPARE(replacement->faderPosition(), Device::FaderPosition::PostInserts);
    QCOMPARE(replacement->sendTap(), Device::SendTap::PreFader);
    QCOMPARE(replacement->reverbSend(2), 0.5f);
    QCOMPARE(replacement->reverbSend(0), 0.0f);
}

void DeviceServiceTest::test_replaceDevice_shouldKeepEffectIds()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto dataService = std::make_shared<DataService>();
    DeviceService service { audioEngine, dataService };

    const auto synth = DeviceFactory::createDevice(SynthDevice::typeIdString(), "TestSynth");
    synth->insertEffectRack().setEffect(1, EffectFactory::createEffect(Gain::typeIdString()));
    service.setDevice(0, synth);
    const auto id = synth->insertEffectRack().effect(1)->id();

    service.replaceDevice(0, DeviceFactory::createDevice(SamplerDevice::typeIdString(), "TestSynth"));

    // Swapping the instrument keeps the strip, and with it whatever automation was aimed at it.
    QCOMPARE(service.device(0)->insertEffectRack().effect(1)->id(), id);
}

void DeviceServiceTest::test_replaceDevice_shouldGiveTheNewDeviceItsOwnUuid()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    DeviceService service { audioEngine, std::make_shared<DataService>() };
    const auto synth = DeviceFactory::createDevice(SynthDevice::typeIdString(), "TestSynth");
    service.setDevice(0, synth);
    const auto replacement = DeviceFactory::createDevice(SamplerDevice::typeIdString(), "TestSynth");
    const auto replacementUuid = replacement->uuid();

    service.replaceDevice(0, replacement);

    // Unlike the strip's effects, the instrument itself is new: an automation written for the old
    // one's parameters must not carry on driving whatever the new one calls by the same name.
    QCOMPARE(service.device(0)->uuid(), replacementUuid);
    QVERIFY(service.device(0)->uuid() != synth->uuid());
}

void DeviceServiceTest::test_replaceDevice_emptySlot_shouldJustSetTheDevice()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto dataService = std::make_shared<DataService>();
    DeviceService service { audioEngine, dataService };

    QVERIFY(!service.device(0));

    service.replaceDevice(0, DeviceFactory::createDevice(SynthDevice::typeIdString(), "TestSynth"));

    const auto device = service.device(0);
    QVERIFY(device);
    QCOMPARE(device->typeId(), SynthDevice::typeIdString());
    QCOMPARE(device->volume(), Constants::faderUnityPosition());
}

void DeviceServiceTest::test_replaceDevice_subMixerMember_shouldKeepMembership()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto dataService = std::make_shared<DataService>();
    DeviceService service { audioEngine, dataService };

    service.setDevice(0, DeviceFactory::createDevice(SubMixerDevice::typeIdString(), "TestSubMixer"));
    service.setDevice(1, DeviceFactory::createDevice(SynthDevice::typeIdString(), "TestSynth"));
    QVERIFY(service.addSubMixerMember(0, 1));

    // The slot never falls empty, so nothing prunes the membership out from under the SubMixer.
    service.replaceDevice(1, DeviceFactory::createDevice(SamplerDevice::typeIdString(), "TestSynth"));

    QCOMPARE(service.subMixerOwningSlot(1), 0);
}

void DeviceServiceTest::test_copyDevice_shouldDuplicateParametersIntoTargetSlot()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto dataService = std::make_shared<DataService>();
    DeviceService service { audioEngine, dataService };

    const auto synth = std::dynamic_pointer_cast<SynthDevice>(DeviceFactory::createDevice(SynthDevice::typeIdString(), "TestSynth"));
    synth->setVolume(0.75f);
    service.setDevice(0, synth);

    QVERIFY(!service.device(1));
    QVERIFY(service.copyDevice(0, 1));

    const auto copy = std::dynamic_pointer_cast<SynthDevice>(service.device(1));
    QVERIFY(copy);
    QCOMPARE(copy->typeId(), SynthDevice::typeIdString());
    QCOMPARE(copy->volume(), 0.75f);
    // The copy is an independent instance, not the same shared pointer.
    QVERIFY(copy != synth);
    // The source is left untouched.
    QCOMPARE(service.device(0), synth);
}

void DeviceServiceTest::test_copyDevice_shouldRenewEffectIds()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto dataService = std::make_shared<DataService>();
    DeviceService service { audioEngine, dataService };

    // A drum synth, so that the per-voice racks are covered as well as the insert rack.
    const auto drums = std::make_shared<DrumSynthDevice>("TestDrums");
    drums->insertEffectRack().setEffect(0, EffectFactory::createEffect(Gain::typeIdString()));
    drums->voiceEffectRack(0).setEffect(0, EffectFactory::createEffect(Gain::typeIdString()));
    service.setDevice(0, drums);

    QVERIFY(service.copyDevice(0, 1));

    const auto copy = std::dynamic_pointer_cast<DrumSynthDevice>(service.device(1));
    QVERIFY(copy);
    QVERIFY(copy->insertEffectRack().effect(0));
    QVERIFY(copy->voiceEffectRack(0).effect(0));
    // Two effects sharing an id would leave an automation unable to tell them apart.
    QVERIFY(copy->insertEffectRack().effect(0)->id() != drums->insertEffectRack().effect(0)->id());
    QVERIFY(copy->voiceEffectRack(0).effect(0)->id() != drums->voiceEffectRack(0).effect(0)->id());
}

void DeviceServiceTest::test_findEffect_shouldSearchEveryRack()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    DeviceService service { audioEngine, std::make_shared<DataService>() };

    const auto drums = std::make_shared<DrumSynthDevice>("TestDrums");
    const auto inDeviceInsert = std::make_shared<Gain>();
    const auto inVoiceRack = std::make_shared<Gain>();
    drums->insertEffectRack().setEffect(2, inDeviceInsert);
    drums->voiceEffectRack(3).setEffect(0, inVoiceRack);
    service.setDevice(0, drums);

    const auto inMasterInsert = std::make_shared<Gain>();
    const auto inSend = std::make_shared<Gain>();
    const auto inSendChain = std::make_shared<Gain>();
    service.insertEffectRack().setEffect(1, inMasterInsert);
    service.sendEffectRack().setEffect(0, inSend);
    service.sendChainRack(1).setEffect(0, inSendChain);

    for (auto && effect : std::vector<std::shared_ptr<Effect>> { inDeviceInsert, inVoiceRack, inMasterInsert, inSend, inSendChain }) {
        QCOMPARE(service.findEffect(effect->id()), effect);
    }
    QVERIFY(!service.findEffect("no-such-effect"));
    QVERIFY(!service.findEffect({}));
}

void DeviceServiceTest::test_findDevice_shouldFindItByUuid()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    DeviceService service { audioEngine, std::make_shared<DataService>() };
    const auto synth = DeviceFactory::createDevice(SynthDevice::typeIdString(), "TestSynth");
    const auto sampler = DeviceFactory::createDevice(SamplerDevice::typeIdString(), "TestSampler");
    service.setDevice(1, synth);
    service.setDevice(4, sampler);

    QCOMPARE(service.findDevice(synth->uuid()), synth);
    QCOMPARE(service.findDevice(sampler->uuid()), sampler);
    QVERIFY(!service.findDevice("no-such-device"));
    QVERIFY(!service.findDevice({}));
}

void DeviceServiceTest::test_deviceUuidForPort_shouldNameTheDeviceBehindThePort()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    DeviceService service { audioEngine, std::make_shared<DataService>() };
    const auto synth = DeviceFactory::createDevice(SynthDevice::typeIdString(), "TestSynth");
    service.setDevice(0, synth);
    const auto port = QString::fromStdString(service.internalDeviceNames().at(0));

    QCOMPARE(service.deviceUuidForPort(port), QString::fromStdString(synth->uuid()));
    // An external port has no device behind it
    QVERIFY(service.deviceUuidForPort("Some MIDI Port").isEmpty());
}

void DeviceServiceTest::test_scheduleParameterChange_device_shouldApplyInTheBlockOfItsFrame()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    DeviceService service { audioEngine, std::make_shared<DataService>() };
    const auto synth = std::make_shared<SynthDevice>("TestSynth");
    service.setDevice(0, synth);
    const auto key = Constants::NahdXml::xmlKeyLpfCutoff().toStdString();
    synth->setLpfCutoff(0.8f);

    service.scheduleParameterChange(ParameterData { 0, 0, synth, key, ParameterAutomation::Mode::Absolute, 0.25 }, 1000);
    service.scheduleParameterChange(ParameterData { 0, 0, synth, key, ParameterAutomation::Mode::Relative, 0.5 }, 1600);

    std::vector<double> buffer(512 * 2, 0.0);
    AudioContext context { std::span(buffer.data(), buffer.size()), 512, 48000 };
    audioEngine->process(context); // Frames 0..511: not yet
    QVERIFY(std::abs(synth->lpfCutoff() - 0.8f) < 1.0e-6f);
    audioEngine->process(context); // 512..1023 holds frame 1000
    QVERIFY(std::abs(synth->lpfCutoff() - 0.25f) < 1.0e-6f);
    audioEngine->process(context); // 1024..1535: nothing due
    QVERIFY(std::abs(synth->lpfCutoff() - 0.25f) < 1.0e-6f);
    audioEngine->process(context); // 1536..2047 holds frame 1600: half of the knob
    QVERIFY(std::abs(synth->parameter(key)->get().value() - 0.4f) < 1.0e-6f);
    // Relative leaves the knob at the value it scales
    QVERIFY(std::abs(synth->lpfCutoff() - 0.8f) < 1.0e-6f);
    QCOMPARE(synth->parameter(key)->get().authoredValue(), 0.8f);
}

void DeviceServiceTest::test_clearAutomation_shouldRestoreAutomatedDevices()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    DeviceService service { audioEngine, std::make_shared<DataService>() };
    const auto synth = std::make_shared<SynthDevice>("TestSynth");
    service.setDevice(0, synth);
    synth->setHpfCutoff(0.1f);

    service.scheduleParameterChange(ParameterData { 0, 0, synth, Constants::NahdXml::xmlKeyHpfCutoff().toStdString(), ParameterAutomation::Mode::Absolute, 0.7 }, std::nullopt);
    std::vector<double> buffer(256 * 2, 0.0);
    AudioContext context { std::span(buffer.data(), buffer.size()), 256, 48000 };
    audioEngine->process(context);
    QVERIFY(std::abs(synth->hpfCutoff() - 0.7f) < 1.0e-6f);

    service.clearAutomation();

    QVERIFY(std::abs(synth->hpfCutoff() - 0.1f) < 1.0e-6f);
    QVERIFY(!synth->isAutomated());
}

void DeviceServiceTest::test_scheduleParameterChange_shouldApplyInTheBlockOfItsFrame()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    DeviceService service { audioEngine, std::make_shared<DataService>() };
    const auto filter = std::make_shared<AutoFilter>();
    service.insertEffectRack().setEffect(0, filter);
    const auto cutoffKey = Constants::NahdXml::xmlKeyCutoff().toStdString();
    const auto authored = filter->parameter(cutoffKey)->get().authoredValue();

    service.scheduleParameterChange(ParameterData { 0, 0, filter, cutoffKey, ParameterAutomation::Mode::Absolute, 0.25 }, 1000);

    std::vector<double> buffer(512 * 2, 0.0);
    AudioContext context { std::span(buffer.data(), buffer.size()), 512, 48000 };
    audioEngine->process(context); // Frames 0..511: not yet
    QCOMPARE(filter->parameter(cutoffKey)->get().value(), authored);
    audioEngine->process(context); // Frames 512..1023: the block that holds frame 1000
    QCOMPARE(filter->parameter(cutoffKey)->get().value(), 0.25f);
    // The live layer only: the project keeps the knob
    QCOMPARE(filter->parameter(cutoffKey)->get().authoredValue(), authored);
}

void DeviceServiceTest::test_scheduleParameterChange_relative_shouldScaleTheKnob()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    DeviceService service { audioEngine, std::make_shared<DataService>() };
    const auto filter = std::make_shared<AutoFilter>();
    service.insertEffectRack().setEffect(0, filter);
    const auto cutoffKey = Constants::NahdXml::xmlKeyCutoff().toStdString();
    filter->parameter(cutoffKey)->get().setValue(0.8f);

    service.scheduleParameterChange(ParameterData { 0, 0, filter, cutoffKey, ParameterAutomation::Mode::Relative, 0.5 }, std::nullopt);
    std::vector<double> buffer(256 * 2, 0.0);
    AudioContext context { std::span(buffer.data(), buffer.size()), 256, 48000 };
    audioEngine->process(context);

    QVERIFY(std::abs(filter->parameter(cutoffKey)->get().value() - 0.4f) < 1.0e-6f);
}

void DeviceServiceTest::test_clearAutomation_shouldRestoreAutomatedEffects()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    DeviceService service { audioEngine, std::make_shared<DataService>() };
    const auto filter = std::make_shared<AutoFilter>();
    service.insertEffectRack().setEffect(0, filter);
    const auto cutoffKey = Constants::NahdXml::xmlKeyCutoff().toStdString();
    const auto authored = filter->parameter(cutoffKey)->get().authoredValue();

    service.scheduleParameterChange(ParameterData { 0, 0, filter, cutoffKey, ParameterAutomation::Mode::Absolute, 0.1 }, std::nullopt);
    // Still queued when the transport stops: must not land afterwards either
    service.scheduleParameterChange(ParameterData { 0, 0, filter, cutoffKey, ParameterAutomation::Mode::Absolute, 0.9 }, 1'000'000);
    std::vector<double> buffer(256 * 2, 0.0);
    AudioContext context { std::span(buffer.data(), buffer.size()), 256, 48000 };
    audioEngine->process(context);
    QCOMPARE(filter->parameter(cutoffKey)->get().value(), 0.1f);

    service.clearAutomation();
    QCOMPARE(filter->parameter(cutoffKey)->get().value(), authored);
    QVERIFY(!filter->isAutomated());

    audioEngine->process(context);
    QCOMPARE(filter->parameter(cutoffKey)->get().value(), authored);
}

void DeviceServiceTest::test_copyDevice_shouldRenewTheDeviceUuid()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    DeviceService service { audioEngine, std::make_shared<DataService>() };
    const auto synth = DeviceFactory::createDevice(SynthDevice::typeIdString(), "TestSynth");
    service.setDevice(0, synth);

    QVERIFY(service.copyDevice(0, 1));

    // Two devices sharing one would leave an automation unable to tell them apart
    QVERIFY(service.device(1));
    QVERIFY(!service.device(1)->uuid().empty());
    QVERIFY(service.device(1)->uuid() != synth->uuid());
    QCOMPARE(service.device(0)->uuid(), synth->uuid());
}

void DeviceServiceTest::test_deviceUuid_newDevices_shouldBeUnique()
{
    const auto a = DeviceFactory::createDevice(SynthDevice::typeIdString(), "A");
    const auto b = DeviceFactory::createDevice(SynthDevice::typeIdString(), "B");
    QVERIFY(!a->uuid().empty());
    QVERIFY(a->uuid() != b->uuid());
}

void DeviceServiceTest::test_copyDevice_differentType_shouldReplaceTargetDevice()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto dataService = std::make_shared<DataService>();
    DeviceService service { audioEngine, dataService };

    service.setDevice(0, DeviceFactory::createDevice(SynthDevice::typeIdString(), "TestSynth"));
    service.setDevice(1, DeviceFactory::createDevice(SamplerDevice::typeIdString(), "TestSampler"));

    QCOMPARE(service.device(1)->typeId(), SamplerDevice::typeIdString());
    QVERIFY(service.copyDevice(0, 1));
    QCOMPARE(service.device(1)->typeId(), SynthDevice::typeIdString());
}

void DeviceServiceTest::test_copyDevice_emptySource_shouldFail()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto dataService = std::make_shared<DataService>();
    DeviceService service { audioEngine, dataService };

    QVERIFY(!service.device(0));
    QVERIFY(!service.copyDevice(0, 1));
    QVERIFY(!service.device(1));
}

void DeviceServiceTest::test_copyDevice_sameSlot_shouldFail()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto dataService = std::make_shared<DataService>();
    DeviceService service { audioEngine, dataService };

    service.setDevice(0, DeviceFactory::createDevice(SynthDevice::typeIdString(), "TestSynth"));
    QVERIFY(!service.copyDevice(0, 0));
}

void DeviceServiceTest::test_exportImport_withEmbeddedData_shouldWork()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto dataService = std::make_shared<DataService>();
    DeviceService service { audioEngine, dataService };

    service.setSamplerAudioFileReaderFactory([]() {
        return std::make_unique<MockAudioFileReader>();
    });

    // Create a mock reader and populate it with data (simulating a file on disk)
    auto mockReader = std::make_unique<MockAudioFileReader>();
    AudioFileReader::Info info {};
    mockReader->open("", AudioFileReader::Mode::Write, info);
    const std::vector<float> dummyData(200, 0.5f);
    mockReader->writeFloat(dummyData);
    mockReader->close();

    auto sampler = std::make_shared<SamplerDevice>("TestSampler", std::move(mockReader));
    service.setDevice(0, sampler);

    // Create a dummy sample file path (doesn't need to be a real valid WAV because we use MockAudioFileReader)
    QTemporaryFile sampleFile { "test.wav" };
    QVERIFY(sampleFile.open());
    const auto samplePath = sampleFile.fileName();
    sampleFile.write(QByteArray { 800, 0 }); // Write 800 bytes to disk
    sampleFile.close();

    sampler->loadSample(60, samplePath.toStdString());
    sampler->setEmbedWaveData(true);

    QTemporaryFile settingsFile;
    QVERIFY(settingsFile.open());
    const auto settingsPath = settingsFile.fileName();
    settingsFile.close();

    QVERIFY(service.exportDeviceSettings(0, settingsPath));

    // Clear and re-import into a fresh service/engine
    const auto audioEngine2 = std::make_shared<AudioEngine>();
    const auto dataService2 = std::make_shared<DataService>();
    DeviceService service2 { audioEngine2, dataService2 };
    service2.setSamplerAudioFileReaderFactory([]() {
        return std::make_unique<MockAudioFileReader>();
    });
    service2.setDevice(0, DeviceFactory::createDevice(SynthDevice::typeIdString(), "InitialSynth"));

    QVERIFY(service2.importDeviceSettings(0, settingsPath));

    const auto importedSampler = std::dynamic_pointer_cast<SamplerDevice>(service2.device(0));
    QVERIFY(importedSampler);

    // Check if the sample path is now a nahd:// path and it resolves via dataService2
    QVERIFY(importedSampler->sample(60));
    const auto importedPath = QString::fromStdString(importedSampler->sample(60)->filePath);
    QVERIFY(importedPath.startsWith(Constants::NahdXml::embeddedDataPathPrefix()));

    const auto resolvedPath = dataService2->resolvePath(importedPath);
    QVERIFY(resolvedPath != importedPath);
    QVERIFY(QFile::exists(resolvedPath));

    QFile resolvedFile;
    resolvedFile.setFileName(resolvedPath);
    QVERIFY(resolvedFile.open(QIODevice::ReadOnly));
    // MockAudioFileReader returns 100 frames of 2 channels (800 bytes for float)
    QCOMPARE(resolvedFile.size(), 100 * 2 * sizeof(float));
}

void DeviceServiceTest::test_importDeviceSettings_embeddedData_emptySlot_shouldExtractDataBeforeLoadingSamples()
{
    // Export a Sampler with an embedded sample.
    QTemporaryFile settingsFile;
    QVERIFY(settingsFile.open());
    const auto settingsPath = settingsFile.fileName();
    settingsFile.close();

    {
        DeviceService service { std::make_shared<AudioEngine>(), std::make_shared<DataService>() };
        auto sampler = std::make_shared<SamplerDevice>("TestSampler", std::make_unique<MockAudioFileReader>());
        service.setDevice(0, sampler);

        QTemporaryFile sampleFile { "test.wav" };
        QVERIFY(sampleFile.open());
        const auto samplePath = sampleFile.fileName();
        sampleFile.write(QByteArray { 800, 0 });
        sampleFile.close();

        sampler->loadSample(60, samplePath.toStdString());
        sampler->setEmbedWaveData(true);
        QVERIFY(service.exportDeviceSettings(0, settingsPath));
    }

    // Import into a fresh service with an empty device rack (i.e. no project loaded first).
    // The reader fails to open unresolved nahd:// paths, so if the embedded data were not
    // extracted before the device is deserialized, the sample load would throw and import
    // would fail. This reproduces the crash reported when importing without a loaded project.
    const auto dataService = std::make_shared<DataService>();
    DeviceService service { std::make_shared<AudioEngine>(), dataService };
    service.setSamplerAudioFileReaderFactory([]() {
        return std::make_unique<NahdFailingMockAudioFileReader>();
    });

    QVERIFY(!service.device(0)); // Empty slot
    QVERIFY(service.importDeviceSettings(0, settingsPath));

    const auto importedSampler = std::dynamic_pointer_cast<SamplerDevice>(service.device(0));
    QVERIFY(importedSampler);
    QVERIFY(importedSampler->sample(60));

    const auto importedPath = QString::fromStdString(importedSampler->sample(60)->filePath);
    QVERIFY(importedPath.startsWith(Constants::NahdXml::embeddedDataPathPrefix()));
    QVERIFY(dataService->resolvePath(importedPath) != importedPath);
}

void DeviceServiceTest::test_peekDeviceTypeInfo_synth_shouldReturnCorrectTypeInfo()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto dataService = std::make_shared<DataService>();
    DeviceService service { audioEngine, dataService };

    service.setDevice(0, DeviceFactory::createDevice(SynthDevice::typeIdString(), "TestSynth"));

    QByteArray data;
    QBuffer buffer { &data };
    buffer.open(QIODevice::WriteOnly);
    NahdXmlWriter writer { buffer };
    writer.setAutoFormatting(true);
    QVERIFY(service.exportDeviceSettings(0, writer));
    buffer.close();

    buffer.open(QIODevice::ReadOnly);
    NahdXmlReader reader { buffer };
    const auto info = service.peekDeviceTypeInfo(reader);
    buffer.close();

    QCOMPARE(info.typeId, QString::fromStdString(SynthDevice::typeIdString()));
    QVERIFY(!info.typeName.isEmpty());
}

namespace {

//! A Devices element holding one device this build knows and one it does not, which is what a
//! project saved by a newer version looks like from here.
QString devicesXmlWithAnUnknownDevice()
{
    return QString {
        "<Devices>"
        " <Device slot=\"0\" name=\"Noteahead Internal Device 1\" typeName=\"Noteahead Synth\" category=\"Synths\" typeId=\"%1\">"
        "  <Parameters/>"
        " </Device>"
        " <Device slot=\"1\" name=\"Noteahead Internal Device 2\" typeName=\"Noteahead Fictional\" category=\"Synths\" typeId=\"not-a-registered-type\">"
        "  <Parameters>"
        "   <Parameter name=\"somethingOnlyTheNewerVersionKnows\" valueType=\"int\" value=\"7\"/>"
        "  </Parameters>"
        " </Device>"
        "</Devices>"
    }
      .arg(QString::fromStdString(SynthDevice::typeIdString()));
}

QString serializedDevices(DeviceService & service)
{
    QString xml;
    NahdXmlWriter writer { xml };
    service.serializeToXml(writer);
    return xml;
}

void deserializeDevices(DeviceService & service, const QString & xml)
{
    NahdXmlReader reader { xml };
    reader.readNextStartElement();
    service.deserializeFromXml(reader);
}

} // namespace

void DeviceServiceTest::test_deserialize_unknownDevice_shouldNotDropTheKnownOnes()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    DeviceService service { audioEngine, std::make_shared<DataService>() };

    deserializeDevices(service, devicesXmlWithAnUnknownDevice());

    QVERIFY(service.device(static_cast<size_t>(0)));
    // Nothing here can build it, so its slot reads as empty
    QVERIFY(!service.device(static_cast<size_t>(1)));
}

void DeviceServiceTest::test_deserialize_unknownDevice_shouldReportIt()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    DeviceService service { audioEngine, std::make_shared<DataService>() };

    QSignalSpy spy { &service, &DeviceService::unknownDevicesFound };

    deserializeDevices(service, devicesXmlWithAnUnknownDevice());

    QCOMPARE(spy.count(), 1);
    const auto descriptions = spy.at(0).at(0).toStringList();
    QCOMPARE(descriptions.size(), 1);
    QVERIFY(descriptions.at(0).contains("Noteahead Fictional"));
}

void DeviceServiceTest::test_serialize_unknownDevice_shouldWriteItBackUnchanged()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    DeviceService service { audioEngine, std::make_shared<DataService>() };

    deserializeDevices(service, devicesXmlWithAnUnknownDevice());

    const auto xml = serializedDevices(service);

    // Opening a project in a version that cannot show one of its devices, and saving it, used to
    // be what deleted that device for good
    QVERIFY(xml.contains("not-a-registered-type"));
    QVERIFY(xml.contains("somethingOnlyTheNewerVersionKnows"));
    QVERIFY(xml.contains("Noteahead Fictional"));
}

void DeviceServiceTest::test_serialize_unknownDevice_slotTakenByARealDevice_shouldBeForgotten()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    DeviceService service { audioEngine, std::make_shared<DataService>() };

    deserializeDevices(service, devicesXmlWithAnUnknownDevice());
    // The user put something of their own in the slot, which is the one thing that may replace it
    service.setDevice(1, DeviceFactory::createDevice(SynthDevice::typeIdString(), "Noteahead Internal Device 2"));

    const auto xml = serializedDevices(service);

    QVERIFY(!xml.contains("not-a-registered-type"));
    QVERIFY(!xml.contains("somethingOnlyTheNewerVersionKnows"));
}

void DeviceServiceTest::test_deserialize_unknownDevice_loadedAgain_shouldNotAccumulate()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    DeviceService service { audioEngine, std::make_shared<DataService>() };

    deserializeDevices(service, devicesXmlWithAnUnknownDevice());
    deserializeDevices(service, "<Devices/>");

    // The previous project's leftovers must not follow the user into the next one
    QVERIFY(!serializedDevices(service).contains("not-a-registered-type"));
}

void DeviceServiceTest::test_peekDeviceTypeInfo_nonexistentFile_shouldReturnEmpty()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto dataService = std::make_shared<DataService>();
    DeviceService service { audioEngine, dataService };

    const auto info = service.peekDeviceTypeInfo("/nonexistent/path/file.nahddev");
    QVERIFY(info.typeId.isEmpty());
    QVERIFY(info.typeName.isEmpty());
}

void DeviceServiceTest::test_reverbSends_shouldSaveAndLoadCorrectly()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto dataService = std::make_shared<DataService>();
    DeviceService service { audioEngine, dataService };

    const auto dev = DeviceFactory::createDevice(SynthDevice::typeIdString(), "TestSynth");
    service.setDevice(0, dev);

    // Initial state check
    QCOMPARE(dev->reverbSendCount(), Constants::effectRackSize());
    for (size_t i = 0; i < dev->reverbSendCount(); i++) {
        QCOMPARE(dev->reverbSend(i), 0.0f);
    }

    // Set values
    dev->setReverbSend(0, 0.5f);
    dev->setReverbSend(2, 0.75f);

    QCOMPARE(dev->reverbSend(0), 0.5f);
    QCOMPARE(dev->reverbSend(1), 0.0f);
    QCOMPARE(dev->reverbSend(2), 0.75f);

    // Serialize
    QString xml;
    NahdXmlWriter writer { xml };
    service.serializeToXml(writer);

    // Deserialize into another engine / service
    const auto audioEngine2 = std::make_shared<AudioEngine>();
    const auto dataService2 = std::make_shared<DataService>();
    DeviceService service2 { audioEngine2, dataService2 };

    const auto dev2 = DeviceFactory::createDevice(SynthDevice::typeIdString(), "TestSynth");
    service2.setDevice(0, dev2);

    NahdXmlReader reader { xml };
    QVERIFY(reader.readNextStartElement());
    QCOMPARE(reader.name(), Constants::NahdXml::xmlKeyDevices());
    service2.deserializeFromXml(reader);

    QCOMPARE(dev2->reverbSend(0), 0.5f);
    QCOMPARE(dev2->reverbSend(1), 0.0f);
    QCOMPARE(dev2->reverbSend(2), 0.75f);
}

void DeviceServiceTest::test_partSends_shouldSaveAndLoadCorrectly()
{
    // A pad's route is the pad's, so it has to survive a save with the pad it belongs to rather than
    // being folded into whatever the device itself sends.
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto dataService = std::make_shared<DataService>();
    DeviceService service { audioEngine, dataService };

    DeviceFactory::init();
    const auto dev = DeviceFactory::createDevice(DrumSynthDevice::typeIdString(), "TestDrums");
    service.setDevice(0, dev);

    dev->setReverbSend(0, 0.25f); // The device's own, which must stay its own
    dev->setSendSourceLevel(1, 0, 0.5f);
    dev->setSendSourceLevel(3, 2, 0.75f);

    QString xml;
    NahdXmlWriter writer { xml };
    service.serializeToXml(writer);

    const auto audioEngine2 = std::make_shared<AudioEngine>();
    const auto dataService2 = std::make_shared<DataService>();
    DeviceService service2 { audioEngine2, dataService2 };
    const auto dev2 = DeviceFactory::createDevice(DrumSynthDevice::typeIdString(), "TestDrums");
    service2.setDevice(0, dev2);

    NahdXmlReader reader { xml };
    QVERIFY(reader.readNextStartElement());
    QCOMPARE(reader.name(), Constants::NahdXml::xmlKeyDevices());
    service2.deserializeFromXml(reader);

    QCOMPARE(dev2->reverbSend(0), 0.25f);
    QCOMPARE(dev2->sendSourceLevel(1, 0), 0.5f);
    QCOMPARE(dev2->sendSourceLevel(3, 2), 0.75f);
    // And nothing else was routed by the reading of it.
    QCOMPARE(dev2->sendSourceLevel(0, 0), 0.0f);
    QCOMPARE(dev2->sendSourceLevel(1, 1), 0.0f);
}

void DeviceServiceTest::test_partSends_projectWithoutThem_shouldLoadAsUnrouted()
{
    // Every project saved until now: its sends carry no sub-index, so they are the device's own and
    // every part reads back routed nowhere.
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto dataService = std::make_shared<DataService>();
    DeviceService service { audioEngine, dataService };

    DeviceFactory::init();
    const auto dev = DeviceFactory::createDevice(DrumSynthDevice::typeIdString(), "TestDrums");
    service.setDevice(0, dev);
    dev->setReverbSend(1, 0.6f);

    QString xml;
    NahdXmlWriter writer { xml };
    service.serializeToXml(writer);
    QVERIFY2(!xml.contains(Constants::NahdXml::xmlKeySubIndex()), "a device with no part sends wrote one anyway");

    const auto audioEngine2 = std::make_shared<AudioEngine>();
    const auto dataService2 = std::make_shared<DataService>();
    DeviceService service2 { audioEngine2, dataService2 };
    const auto dev2 = DeviceFactory::createDevice(DrumSynthDevice::typeIdString(), "TestDrums");
    service2.setDevice(0, dev2);

    NahdXmlReader reader { xml };
    QVERIFY(reader.readNextStartElement());
    service2.deserializeFromXml(reader);

    QCOMPARE(dev2->reverbSend(1), 0.6f);
    for (size_t source = 0; source < dev2->sendSourceCount(); source++) {
        QCOMPARE(dev2->sendSourceLevel(source, 1), 0.0f);
    }
}

void DeviceServiceTest::test_masterRackEnabled_shouldSaveAndLoadCorrectly()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto dataService = std::make_shared<DataService>();
    DeviceService service { audioEngine, dataService };

    // Both master racks are enabled by default.
    QVERIFY(service.insertEffectRack().enabled());
    QVERIFY(service.sendEffectRack().enabled());

    service.insertEffectRack().setEnabled(false);
    service.sendEffectRack().setEnabled(false);

    QString xml;
    NahdXmlWriter writer { xml };
    service.serializeToXml(writer);

    const auto audioEngine2 = std::make_shared<AudioEngine>();
    const auto dataService2 = std::make_shared<DataService>();
    DeviceService service2 { audioEngine2, dataService2 };

    NahdXmlReader reader { xml };
    QVERIFY(reader.readNextStartElement());
    QCOMPARE(reader.name(), Constants::NahdXml::xmlKeyDevices());
    service2.deserializeFromXml(reader);

    QVERIFY(!service2.insertEffectRack().enabled());
    QVERIFY(!service2.sendEffectRack().enabled());
}

namespace {

//! Parks inside processAudio() until released, so a test can hold the engine in its callback -- and
//! with it AudioEngine's mutex -- for as long as it needs to.
class BlockingDevice : public Device
{
public:
    std::string name() const override
    {
        return "Noteahead Internal Device 1";
    }

    std::string category() const override
    {
        return "Test";
    }

    std::string typeName() const override
    {
        return "Blocking";
    }

    std::string typeId() const override
    {
        return "blocking";
    }

    void processDeviceMidiNoteOn(uint8_t note, uint8_t velocity) override
    {
        m_note = note;
        m_velocity = velocity;
        m_noteCount++;
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
        std::unique_lock<std::mutex> lock { m_mutex };
        m_isProcessing = true;
        m_entered.notify_all();
        m_released.wait(lock, [this] { return m_release; });
    }

    //! Blocks until the device is inside processAudio(), which is when the engine holds its mutex.
    void waitUntilProcessing()
    {
        std::unique_lock<std::mutex> lock { m_mutex };
        m_entered.wait(lock, [this] { return m_isProcessing; });
    }

    void release()
    {
        {
            std::lock_guard<std::mutex> lock { m_mutex };
            m_release = true;
        }
        m_released.notify_all();
    }

    int noteCount() const
    {
        return m_noteCount;
    }

    uint8_t note() const
    {
        return m_note;
    }

    uint8_t velocity() const
    {
        return m_velocity;
    }

private:
    std::mutex m_mutex;
    std::condition_variable m_entered;
    std::condition_variable m_released;
    bool m_isProcessing = false;
    bool m_release = false;
    std::atomic_int m_noteCount { 0 };
    std::atomic<uint8_t> m_note { 0 };
    std::atomic<uint8_t> m_velocity { 0 };
};

} // namespace

void DeviceServiceTest::test_device_bySlotAndByName_shouldResolveWithoutTheEngine()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    DeviceService service { audioEngine, std::make_shared<DataService>() };

    const auto device = std::make_shared<SynthDevice>("Noteahead Internal Device 3");
    service.setDevice(2, device);

    QCOMPARE(service.device(2), device);
    QCOMPARE(service.device("Noteahead Internal Device 3"), device);
    QVERIFY(!service.device(3));

    service.clearDevice(2);

    QVERIFY(!service.device(2));
    QVERIFY(!service.device("Noteahead Internal Device 3"));
}

void DeviceServiceTest::test_processMidiNoteOn_whileEngineIsProcessing_shouldNotBlock()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    DeviceService service { audioEngine, std::make_shared<DataService>() };

    const auto device = std::make_shared<BlockingDevice>();
    service.setDevice(0, device);

    std::vector<double> buffer(256, 0.0);
    AudioContext context { std::span(buffer.data(), buffer.size()), 128, 48000 };

    // The engine now sits in the callback holding its mutex, exactly as it does for the whole of a
    // real audio buffer.
    std::thread engineThread { [&] { audioEngine->process(context); } };
    device->waitUntilProcessing();

    // Resolving the device used to go through AudioEngine::device(), which wants that same mutex,
    // so this call would not return until the callback had finished. It has to arrive now.
    service.processMidiNoteOn(QString::fromStdString(device->name()), 64, 100);

    QCOMPARE(device->noteCount(), 1);
    QCOMPARE(device->note(), 64);
    QCOMPARE(device->velocity(), 100);

    device->release();
    engineThread.join();
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::DeviceServiceTest)
