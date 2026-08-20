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
#include "../../domain/devices/synth_device.hpp"
#include "../../domain/effects/effect_factory.hpp"
#include "../../infra/audio/audio_engine.hpp"
#include "../../infra/data_service.hpp"
#include "../../infra/xml/nahd_xml_reader.hpp"
#include "../../infra/xml/nahd_xml_writer.hpp"

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

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::DeviceServiceTest)
