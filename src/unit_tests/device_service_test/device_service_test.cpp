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

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::DeviceServiceTest)
