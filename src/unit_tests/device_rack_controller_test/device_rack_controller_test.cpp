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

#include "device_rack_controller_test.hpp"

#include "application/service/device_service.hpp"
#include "application/service/editor_service.hpp"
#include "common/constants.hpp"
#include "domain/devices/bass_synth_device.hpp"
#include "domain/devices/device_factory.hpp"
#include "domain/devices/drum_synth_device.hpp"
#include "domain/devices/sampler_device.hpp"
#include "domain/devices/synth_device.hpp"
#include "domain/effects/effect_factory.hpp"
#include "infra/audio/audio_engine.hpp"
#include "view/controllers/device_rack_controller.hpp"
#include "view/controllers/sampler_controller.hpp"
#include "view/controllers/synth_controller.hpp"

#include <QSignalSpy>
#include <QTest>

namespace noteahead {

void DeviceRackControllerTest::initTestCase()
{
    EffectFactory::init();
    DeviceFactory::init();
}

void DeviceRackControllerTest::cleanupTestCase()
{
    EffectFactory::clear();
    DeviceFactory::clear();
}

class MockDevice : public Device
{
public:
    MockDevice(const std::string & name, const std::string & typeId = "mock")
      : m_name { name }
      , m_typeId { typeId }
    {
    }

    std::string name() const override
    {
        return m_name;
    }

    std::string category() const override
    {
        return "Mock Category";
    }

    std::string typeName() const override
    {
        return "Mock Type";
    }

    std::string typeId() const override
    {
        return m_typeId;
    }

    void processMidiNoteOn(uint8_t, uint8_t) override
    {
    }

    void processMidiNoteOff(uint8_t) override
    {
    }

    void processMidiCc(uint8_t, uint8_t, uint8_t) override
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

    void serializeToXml(QXmlStreamWriter &) const override
    {
    }

    void deserializeFromXml(QXmlStreamReader &) override
    {
    }

protected:
    void syncParameters() override
    {
    }

private:
    std::string m_name;
    std::string m_typeId;
};

class MockEditorService : public EditorService
{
public:
    std::vector<quint64> trackIndices() const override
    {
        return m_indices;
    }

    void setMockIndices(const std::vector<quint64> & indices)
    {
        m_indices = indices;
    }

    QString trackName(quint64 trackIndex) const override
    {
        return m_names.at(trackIndex);
    }

    void setMockTrackName(quint64 trackIndex, const QString & name)
    {
        m_names[trackIndex] = name;
    }

    QString instrumentPortName(quint64 trackIndex) const override
    {
        return m_ports.at(trackIndex);
    }

    void setMockInstrumentPortName(quint64 trackIndex, const QString & port)
    {
        m_ports[trackIndex] = port;
    }

    void setIsModified(bool modified) override
    {
        m_modified = modified;
    }

    bool isModified() const override
    {
        return m_modified;
    }

private:
    std::vector<quint64> m_indices;
    std::map<quint64, QString> m_names;
    std::map<quint64, QString> m_ports;
    bool m_modified { false };
};

void DeviceRackControllerTest::test_devices_shouldReturnDeviceNames()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(audioEngine);
    const auto name1 = "Test Device 1";
    const auto name2 = "Test Device 2";
    deviceService->setDevice(0, std::make_shared<MockDevice>(name1));
    deviceService->setDevice(1, std::make_shared<MockDevice>(name2));

    DeviceRackController controller { deviceService, {}, nullptr };

    QCOMPARE(controller.rowCount(), static_cast<int>(Constants::deviceRackSize()));
    QCOMPARE(controller.data(controller.index(0), static_cast<int>(DeviceRackController::DataRole::Name)).toString(), QString::fromStdString(name1));
    QCOMPARE(controller.data(controller.index(1), static_cast<int>(DeviceRackController::DataRole::Name)).toString(), QString::fromStdString(name2));
    QCOMPARE(controller.data(controller.index(2), static_cast<int>(DeviceRackController::DataRole::Name)).toString(), QString(""));
}

void DeviceRackControllerTest::test_trackNames_shouldReturnTrackNamesForDevice()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(audioEngine);
    const auto name1 = "Device 1";
    const auto name2 = "Device 2";
    deviceService->setDevice(0, std::make_shared<MockDevice>(name1));
    deviceService->setDevice(1, std::make_shared<MockDevice>(name2));

    const auto editorService = std::make_shared<MockEditorService>();
    editorService->setMockIndices({ 0, 1, 2, 3 });
    editorService->setMockTrackName(0, "Track 1");
    editorService->setMockInstrumentPortName(0, QString::fromStdString(name1));
    editorService->setMockTrackName(1, "Track 2");
    editorService->setMockInstrumentPortName(1, QString::fromStdString(name2));
    editorService->setMockTrackName(2, "Track 3");
    editorService->setMockInstrumentPortName(2, QString::fromStdString(name1));
    editorService->setMockTrackName(3, "Track 4");
    editorService->setMockInstrumentPortName(3, QString::fromStdString(name2));

    DeviceRackController controller { deviceService, {}, editorService };

    QCOMPARE(controller.data(controller.index(0), static_cast<int>(DeviceRackController::DataRole::TrackNames)).toString(), QString("Track 1, Track 3"));
    QCOMPARE(controller.data(controller.index(1), static_cast<int>(DeviceRackController::DataRole::TrackNames)).toString(), QString("Track 2, Track 4"));
}

void DeviceRackControllerTest::test_setDevice_shouldAddDeviceAndNotify()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(audioEngine);
    const auto editorService = std::make_shared<MockEditorService>();
    DeviceRackController controller { deviceService, {}, editorService };

    QSignalSpy revisionSpy { &controller, &DeviceRackController::revisionChanged };
    QSignalSpy dataChangedSpy { &controller, &DeviceRackController::dataChanged };

    const auto typeId = QString::fromStdString(SynthDevice::typeIdString());
    controller.setDevice(0, typeId);

    QVERIFY(revisionSpy.count() > 0);
    QVERIFY(editorService->isModified());
    QVERIFY(deviceService->device(0) != nullptr);
    QCOMPARE(QString::fromStdString(deviceService->device(0)->typeId()), typeId);
}

void DeviceRackControllerTest::test_clearDevice_shouldRemoveDeviceAndNotify()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(audioEngine);
    const auto editorService = std::make_shared<MockEditorService>();
    deviceService->setDevice(0, std::make_shared<MockDevice>("To be removed"));

    DeviceRackController controller { deviceService, {}, editorService };

    QSignalSpy revisionSpy { &controller, &DeviceRackController::revisionChanged };
    controller.clearDevice(0);

    QVERIFY(revisionSpy.count() > 0);
    QVERIFY(editorService->isModified());
    QVERIFY(deviceService->device(0) == nullptr);
}

void DeviceRackControllerTest::test_addMethods_shouldAddDevicesToFirstEmptySlot()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(audioEngine);
    const auto editorService = std::make_shared<MockEditorService>();
    DeviceRackController controller { deviceService, {}, editorService };

    // Fill first slot
    controller.setDevice(0, QString::fromStdString(SamplerDevice::typeIdString()));

    // addSynth should go to slot 1
    controller.addSynth();
    QVERIFY(deviceService->device(1) != nullptr);
    QCOMPARE(deviceService->device(1)->typeId(), SynthDevice::typeIdString());

    // addBassSynth should go to slot 2
    controller.addBassSynth();
    QVERIFY(deviceService->device(2) != nullptr);
    QCOMPARE(deviceService->device(2)->typeId(), BassSynthDevice::typeIdString());

    // addDrumSynth should go to slot 3
    controller.addDrumSynth();
    QVERIFY(deviceService->device(3) != nullptr);
    QCOMPARE(deviceService->device(3)->typeId(), DrumSynthDevice::typeIdString());

    // addSampler should go to slot 4
    controller.addSampler();
    QVERIFY(deviceService->device(4) != nullptr);
    QCOMPARE(deviceService->device(4)->typeId(), SamplerDevice::typeIdString());
}

void DeviceRackControllerTest::test_availableDevices_shouldReturnCorrectList()
{
    DeviceRackController controller { nullptr, {}, nullptr };
    const auto list = controller.availableDevices();

    QCOMPARE(list.size(), 4);
    QCOMPARE(list.at(0).toMap()["name"].toString(), QString("Sampler"));
    QCOMPARE(list.at(1).toMap()["name"].toString(), QString("Synth"));
    QCOMPARE(list.at(2).toMap()["name"].toString(), QString("Bass Synth"));
    QCOMPARE(list.at(3).toMap()["name"].toString(), QString("Drum Synth"));
}

void DeviceRackControllerTest::test_removeDeviceByName_shouldClearCorrectSlot()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(audioEngine);
    const auto editorService = std::make_shared<MockEditorService>();

    const auto prefix = Constants::internalDevicePortPrefix().toStdString();
    deviceService->setDevice(0, std::make_shared<MockDevice>(prefix + " 1"));
    deviceService->setDevice(2, std::make_shared<MockDevice>(prefix + " 3"));

    DeviceRackController controller { deviceService, {}, editorService };

    controller.removeDevice(QString::fromStdString(prefix + " 3"));
    QVERIFY(deviceService->device(0) != nullptr);
    QVERIFY(deviceService->device(2) == nullptr);

    controller.removeDevice(QString::fromStdString(prefix + " 1"));
    QVERIFY(deviceService->device(0) == nullptr);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::DeviceRackControllerTest)
