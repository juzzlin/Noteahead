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

#include "../../application/service/device_service.hpp"
#include "../../application/service/editor_service.hpp"
#include "../../common/constants.hpp"
#include "../../domain/devices/device_factory.hpp"
#include "../../domain/effects/effect_factory.hpp"
#include "../../domain/utility/level_meter.hpp"
#include "../../domain/utility/loudness_meter.hpp"
#include "../../infra/audio/audio_engine.hpp"
#include "../../infra/data_service.hpp"
#include "../../infra/xml/nahd_xml_reader.hpp"
#include "../../infra/xml/nahd_xml_writer.hpp"
#include "../../plugins/devices/bass_synth/bass_synth_device.hpp"
#include "../../plugins/devices/drum_synth/drum_synth_device.hpp"
#include "../../plugins/devices/kick_808/kick_808_device.hpp"
#include "../../plugins/devices/piano_synth/piano_synth_device.hpp"
#include "../../plugins/devices/sampler/sampler_device.hpp"
#include "../../plugins/devices/sampler/sampler_dialog_model.hpp"
#include "../../plugins/devices/snare_synth/snare_synth_device.hpp"
#include "../../plugins/devices/string_voice/string_voice_device.hpp"
#include "../../plugins/devices/sub_mixer/sub_mixer_device.hpp"
#include "../../plugins/devices/synth/synth_device.hpp"
#include "../../plugins/devices/synth/synth_dialog_model.hpp"
#include "../../plugins/effects/gain/gain.hpp"
#include "../../view/controllers/device_meters_controller.hpp"
#include "../../view/controllers/device_mixer_controller.hpp"
#include "../../view/controllers/device_rack_controller.hpp"
#include "../../view/controllers/midi_mapping_controller.hpp"

#include <QBuffer>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QUrl>

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

    void reset() override
    {
    }

    void resetDeviceAudio() override
    {
    }

    void serializeToXml(ProjectWriter &) const override
    {
    }

    void deserializeFromXml(ProjectReader &) override
    {
    }

protected:
    //! Chains to the base so volume, gain, pan and the channel strip settings still land in the
    //! device's cached members; only subclass-specific syncing is what this mock has none of.
    void syncParameters() override
    {
        Device::syncParameters();
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
    const auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
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
    const auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
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

void DeviceRackControllerTest::test_subMixerCandidates_shouldCarryTrackNames()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
    const auto memberName = "Device 1";
    deviceService->setDevice(0, std::make_shared<MockDevice>(memberName));
    deviceService->setDevice(1, std::make_shared<SubMixerDevice>("Sub"));

    const auto editorService = std::make_shared<MockEditorService>();
    editorService->setMockIndices({ 0, 1 });
    editorService->setMockTrackName(0, "Drums");
    editorService->setMockInstrumentPortName(0, QString::fromStdString(memberName));
    editorService->setMockTrackName(1, "Bass");
    editorService->setMockInstrumentPortName(1, QString::fromStdString(memberName));

    DeviceRackController controller { deviceService, {}, editorService };

    const auto candidates = controller.subMixerCandidates(1);
    QCOMPARE(candidates.size(), 1);

    // A slot number says little about what a device actually is; the tracks it plays on do, which
    // is why the Device Rack listing shows them and the member list has to as well.
    const auto entry = candidates.at(0).toMap();
    QCOMPARE(entry["slot"].toInt(), 0);
    QCOMPARE(entry["trackNames"].toString(), QString("Drums, Bass"));
    // Name and type stay separate so the member list can compose the Device Rack's caption.
    QCOMPARE(entry["name"].toString(), QString(memberName));
    QVERIFY(!entry["typeName"].toString().isEmpty());
}

void DeviceRackControllerTest::test_usageSummary_device_shouldReturnTrackNames()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
    const auto deviceName = "Device 1";
    deviceService->setDevice(0, std::make_shared<MockDevice>(deviceName));

    const auto editorService = std::make_shared<MockEditorService>();
    editorService->setMockIndices({ 0 });
    editorService->setMockTrackName(0, "Drums");
    editorService->setMockInstrumentPortName(0, QString::fromStdString(deviceName));

    DeviceRackController controller { deviceService, {}, editorService };

    QCOMPARE(controller.usageSummary(0), QString { "Drums" });
}

void DeviceRackControllerTest::test_usageSummary_subMixer_shouldReturnMemberTrackNames()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
    deviceService->setDevice(0, std::make_shared<MockDevice>("Device 1"));
    deviceService->setDevice(1, std::make_shared<MockDevice>("Device 2"));
    deviceService->setDevice(2, std::make_shared<MockDevice>("Device 3"));
    deviceService->setDevice(3, std::make_shared<SubMixerDevice>("Inner"));
    deviceService->setDevice(4, std::make_shared<SubMixerDevice>("Outer"));

    const auto editorService = std::make_shared<MockEditorService>();
    editorService->setMockIndices({ 0, 1, 2, 3 });
    editorService->setMockTrackName(0, "Drums");
    editorService->setMockInstrumentPortName(0, "Device 1");
    editorService->setMockTrackName(1, "Bass");
    editorService->setMockInstrumentPortName(1, "Device 2");
    editorService->setMockTrackName(2, "Bass, doubled");
    editorService->setMockInstrumentPortName(2, "Device 2");
    editorService->setMockTrackName(3, "Pads");
    editorService->setMockInstrumentPortName(3, "Device 3");

    DeviceRackController controller { deviceService, {}, editorService };

    QCOMPARE(controller.usageSummary(4), QString {});

    QVERIFY(controller.addSubMixerMember(3, 2));
    QVERIFY(controller.addSubMixerMember(4, 0));
    QVERIFY(controller.addSubMixerMember(4, 1));
    QVERIFY(controller.addSubMixerMember(4, 3));
    QCOMPARE(controller.usageSummary(3), QString { "Pads" });
    // A nested SubMixer contributes the tracks of its own members.
    QCOMPARE(controller.usageSummary(4), QString { "Drums, Bass, Bass, doubled, Pads" });
}

void DeviceRackControllerTest::test_deviceGain_shouldDefaultToUnityAndRoundTrip()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
    const auto subMixer = std::make_shared<SubMixerDevice>("Sub");
    deviceService->setDevice(0, subMixer);

    const auto editorService = std::make_shared<MockEditorService>();
    DeviceRackController controller { deviceService, {}, editorService };
    DeviceMixerController mixer { deviceService, editorService };

    // Centre of the -30..+30 dB range, so a fresh Sub Mixer passes its group through untouched.
    QCOMPARE(mixer.deviceGain(0), 500);
    QCOMPARE(subMixer->gain(), 0.5f);

    QSignalSpy revisionSpy { &controller, &DeviceRackController::revisionChanged };

    mixer.setDeviceGain(0, 750);
    QCOMPARE(mixer.deviceGain(0), 750);
    QVERIFY(editorService->isModified());

    // The QML knob reads gain through a plain invokable, so its binding only re-evaluates because
    // the write reaches revisionChanged. Without this the slider snaps back to its stale value the
    // moment the drag ends, which reads as the control being stuck. It arrives on the next turn of
    // the event loop rather than within the call, since changes are caught up with once a frame.
    QTRY_VERIFY(revisionSpy.count() > 0);

    // An empty slot has no gain to report; 0 dB is the sane answer rather than silence or a crash.
    QCOMPARE(mixer.deviceGain(9), 500);
    mixer.setDeviceGain(9, 750);
}

void DeviceRackControllerTest::test_revision_burstOfChanges_shouldRefreshOncePerFrame()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
    deviceService->setDevice(0, std::make_shared<SubMixerDevice>("Sub"));
    const auto editorService = std::make_shared<MockEditorService>();
    DeviceRackController controller { deviceService, {}, editorService };
    DeviceMixerController mixer { deviceService, editorService };
    QSignalSpy revisionSpy { &controller, &DeviceRackController::revisionChanged };

    // As a knob drag does: many changes before the event loop gets a turn. Each refresh re-reads
    // every binding on the revision, so one per change kept the interface behind the hand.
    for (int value = 500; value < 600; value++) {
        mixer.setDeviceGain(0, value);
    }
    QCOMPARE(revisionSpy.count(), 0);
    QTRY_VERIFY(revisionSpy.count() > 0);
    QTest::qWait(3 * AutomationRefreshThrottle::RefreshIntervalMs);

    QVERIFY(revisionSpy.count() <= 2);
    QCOMPARE(mixer.deviceGain(0), 599);
}

void DeviceRackControllerTest::test_setDevice_shouldAddDeviceAndNotify()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
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

void DeviceRackControllerTest::test_replaceDevice_shouldKeepInsertEffectsAndNotify()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
    const auto editorService = std::make_shared<MockEditorService>();

    const auto synth = DeviceFactory::createDevice(SynthDevice::typeIdString(), "TestSynth");
    synth->insertEffectRack().setEffect(0, EffectFactory::createEffect(Gain::typeIdString()));
    synth->setReverbSend(1, 0.25f);
    deviceService->setDevice(0, synth);

    DeviceRackController controller { deviceService, {}, editorService };
    QSignalSpy revisionSpy { &controller, &DeviceRackController::revisionChanged };

    const auto typeId = QString::fromStdString(SamplerDevice::typeIdString());
    controller.replaceDevice(0, typeId);

    QVERIFY(revisionSpy.count() > 0);
    QVERIFY(editorService->isModified());

    const auto replacement = deviceService->device(0);
    QVERIFY(replacement);
    QCOMPARE(QString::fromStdString(replacement->typeId()), typeId);
    // The slot keeps its canonical name, so the tracks pointing at it still find their device.
    QCOMPARE(QString::fromStdString(replacement->name()), Constants::internalDevicePortPrefix() + " 1");
    QVERIFY(replacement->insertEffectRack().effect(0));
    QCOMPARE(replacement->insertEffectRack().effect(0)->typeId(), Gain::typeIdString());
    QCOMPARE(replacement->reverbSend(1), 0.25f);
}

void DeviceRackControllerTest::test_clearDevice_shouldRemoveDeviceAndNotify()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
    const auto editorService = std::make_shared<MockEditorService>();
    deviceService->setDevice(0, std::make_shared<MockDevice>("To be removed"));

    DeviceRackController controller { deviceService, {}, editorService };

    QSignalSpy revisionSpy { &controller, &DeviceRackController::revisionChanged };
    controller.clearDevice(0);

    QVERIFY(revisionSpy.count() > 0);
    QVERIFY(editorService->isModified());
    QVERIFY(deviceService->device(0) == nullptr);
}

void DeviceRackControllerTest::test_availableDevices_shouldReturnCorrectList()
{
    DeviceRackController controller { nullptr, {}, nullptr };
    const auto list = controller.availableDevices();

    // In the order of the names, the devices that register themselves among the rest
    const QStringList expected {
        "Bass Synth",
        "Drum Synth",
        "Drum Synth 909",
        "Drum Synth V2",
        "Electric Bass",
        "FM Drums",
        "FM Synth",
        "JU Synth",
        "JU Synth 08",
        "Kick 808",
        "Kick 909",
        "Kick Synth",
        "LFO",
        "Noise FX",
        "Piano Synth",
        "Piano Synth Acoustic",
        "Piano Synth V2",
        "Piano Synth V3",
        "Sampler",
        "Snare Synth",
        "Speech",
        "String & Voice",
        "String & Voice V2",
        "String & Voice V3",
        "String Ensemble",
        "Sub Mixer",
        "Synth",
        "Synth D",
        "Wavetable Synth",
        "XY Pad",
    };
    QStringList names;
    for (auto && entry : list) {
        names << entry.toMap()["name"].toString();
    }
    QCOMPARE(names, expected);
}

void DeviceRackControllerTest::test_availableDevices_shouldHaveCategories()
{
    DeviceRackController controller { nullptr, {}, nullptr };

    // The gallery shows each device's category after its name
    for (auto && entry : controller.availableDevices()) {
        const auto map = entry.toMap();
        QVERIFY2(!map["category"].toString().isEmpty(), qPrintable(map["name"].toString()));
    }
    const auto categoryOf = [&controller](const QString & name) {
        for (auto && entry : controller.availableDevices()) {
            if (const auto map = entry.toMap(); map["name"].toString() == name) {
                return map["category"].toString();
            }
        }
        return QString {};
    };
    QCOMPARE(categoryOf("Sampler"), QString { "Samplers" });
    QCOMPARE(categoryOf("Synth"), QString { "Synths" });
    // From its PluginCatalog entry
    QCOMPARE(categoryOf("Kick 808"), QString { "Drums" });
}

void DeviceRackControllerTest::test_deviceMeterLevels_shouldReportPreInsertLevel()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
    const auto editorService = std::make_shared<MockEditorService>();
    const auto prefix = Constants::internalDevicePortPrefix().toStdString();
    const auto device = std::make_shared<MockDevice>(prefix + " 1");
    deviceService->setDevice(0, device);

    DeviceRackController controller { deviceService, {}, editorService };

    DeviceMetersController meters { deviceService, editorService };

    // An empty slot has nothing to report, and a device whose meter is off reads the floor.
    QVERIFY(meters.deviceMeterLevels(1).isEmpty());
    QCOMPARE(meters.deviceMeterLevels(0).size(), 2);
    QCOMPARE(meters.deviceMeterLevels(0).at(0).toFloat(), LevelMeter::MinimumDb);

    meters.setMetersActive(true);
    QVERIFY(device->meter().active());

    const std::vector<double> fullScale(64, 1.0);
    device->meter().write(fullScale.data(), 32, 48000);
    QVERIFY(std::abs(meters.deviceMeterLevels(0).at(0).toFloat()) < 0.01f);

    meters.setMetersActive(false);
    QVERIFY(!device->meter().active());
}

void DeviceRackControllerTest::test_setMetersActive_twoViews_shouldStayOnUntilBothTurnThemOff()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
    const auto device = std::make_shared<MockDevice>(Constants::internalDevicePortPrefix().toStdString() + " 1");
    deviceService->setDevice(0, device);
    DeviceMetersController meters { deviceService, std::make_shared<MockEditorService>() };

    // Ctrl+M from the Device Rack: the mixer opens while the rack is still closing, so the rack
    // turns the meters off after the mixer has turned them on
    meters.setMetersActive(true);
    meters.setMetersActive(true);
    meters.setMetersActive(false);
    QVERIFY(device->meter().active());

    meters.setMetersActive(false);
    QVERIFY(!device->meter().active());
}

void DeviceRackControllerTest::test_deviceOutputMeters_shouldCarryEveryReadingOfTheOutputTaps()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
    const auto prefix = Constants::internalDevicePortPrefix().toStdString();
    const auto device = std::make_shared<MockDevice>(prefix + " 1");
    deviceService->setDevice(0, device);

    DeviceRackController controller { deviceService, {}, std::make_shared<MockEditorService>() };

    DeviceMetersController meters { deviceService, std::make_shared<MockEditorService>() };

    // An empty slot has nothing to report. The map is what the mixer's OUT views read, and every one
    // of them has to find its key there whichever view happens to be showing.
    QVERIFY(meters.deviceOutputMeters(1).isEmpty());
    const auto idle = meters.deviceOutputMeters(0);
    QCOMPARE(idle["peakDb"].toFloat(), LevelMeter::MinimumDb);
    QCOMPARE(idle["rmsDb"].toFloat(), LevelMeter::MinimumDb);
    QCOMPARE(idle["integratedLufs"].toFloat(), LoudnessMeter::MinimumLufs);
    QCOMPARE(idle["shortTermLufs"].toFloat(), LoudnessMeter::MinimumLufs);

    meters.setMetersActive(true);
    QVERIFY(device->outputMeter().active());
    QVERIFY(device->outputLoudnessMeter().active());

    const std::vector<double> fullScale(64, 1.0);
    device->outputMeter().write(fullScale.data(), 32, 48000);
    QVERIFY(std::abs(meters.deviceOutputMeters(0)["peakDb"].toFloat()) < 0.01f);

    meters.setMetersActive(false);
    QVERIFY(!device->outputMeter().active());
}

void DeviceRackControllerTest::test_deviceMeterLevels_afterSlotChanged_shouldFollowTheActiveGate()
{
    // The gate is set when the rack goes on screen, but a device dropped into a slot afterwards is
    // born with its taps off, so replacing one used to leave a permanently dead meter.
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
    DeviceRackController controller { deviceService, {}, std::make_shared<MockEditorService>() };
    DeviceMetersController meters { deviceService, std::make_shared<MockEditorService>() };

    meters.setMetersActive(true);
    controller.setDevice(0, QString::fromStdString(Kick808Device::typeIdString()));
    QVERIFY(deviceService->device(0)->meter().active());
    QVERIFY(deviceService->device(0)->loadMeter().active());

    controller.clearDevice(0);
    controller.setDevice(0, QString::fromStdString(SynthDevice::typeIdString()));
    QVERIFY(deviceService->device(0)->meter().active());
    QVERIFY(deviceService->device(0)->loadMeter().active());

    // Off the screen, a device added into the rack must not silently switch the taps back on.
    meters.setMetersActive(false);
    controller.setDevice(1, QString::fromStdString(SynthDevice::typeIdString()));
    QVERIFY(!deviceService->device(1)->meter().active());
    QVERIFY(!deviceService->device(1)->loadMeter().active());
}

namespace {
//! Emits a constant level so a test can put the engine's device path either side of full scale.
class ClippingDevice : public MockDevice
{
public:
    using MockDevice::MockDevice;

    void processAudio(AudioContext & context) override
    {
        for (uint32_t i = 0; i < context.frameCount * 2; i++) {
            context.buffer[i] += m_amplitude;
        }
    }

    bool hasActiveAudio() const override
    {
        return m_active;
    }

    void setAmplitude(double amplitude)
    {
        m_amplitude = amplitude;
    }

    void setActive(bool active)
    {
        m_active = active;
    }

private:
    double m_amplitude { 0.0 };
    bool m_active { true };
};
} // namespace

void DeviceRackControllerTest::test_deviceClipped_afterClearing_shouldStayClearWhileBelowFullScale()
{
    // Drives the real engine rather than poking the detector directly, because the indicator staying
    // lit after it is cleared would come from the engine re-latching it, not from the detector.
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
    const auto device = std::make_shared<ClippingDevice>(Constants::internalDevicePortPrefix().toStdString() + " 1");
    deviceService->setDevice(0, device);
    DeviceRackController controller { deviceService, {}, std::make_shared<MockEditorService>() };
    DeviceMetersController meters { deviceService, std::make_shared<MockEditorService>() };

    std::vector<double> buffer(256 * 2, 0.0);
    const auto render = [&] {
        std::fill(buffer.begin(), buffer.end(), 0.0);
        AudioContext context { std::span(buffer.data(), buffer.size()), 256, 48000 };
        audioEngine->process(context);
    };

    device->setAmplitude(1.5);
    render();
    QVERIFY(meters.deviceClipped(0));

    meters.clearDeviceClip(0);
    QVERIFY(!meters.deviceClipped(0));

    // Gone quiet: the engine clears each device's buffer before rendering, so a silent device must
    // not keep re-latching the indicator off whatever it last played.
    device->setAmplitude(0.0);
    device->setActive(false);
    for (int i = 0; i < 5; i++) {
        render();
    }
    QVERIFY(!meters.deviceClipped(0));

    // Still playing, but comfortably below full scale.
    meters.clearDeviceClip(0);
    device->setAmplitude(0.5);
    device->setActive(true);
    for (int i = 0; i < 5; i++) {
        render();
    }
    QVERIFY(!meters.deviceClipped(0));
}

void DeviceRackControllerTest::test_deviceClipped_shouldLatchUntilCleared()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
    const auto editorService = std::make_shared<MockEditorService>();
    const auto prefix = Constants::internalDevicePortPrefix().toStdString();
    const auto device = std::make_shared<MockDevice>(prefix + " 1");
    deviceService->setDevice(0, device);

    DeviceRackController controller { deviceService, {}, editorService };

    DeviceMetersController meters { deviceService, editorService };

    QVERIFY(!meters.deviceClipped(0));
    // An empty slot cannot clip, and asking must not fall over
    QVERIFY(!meters.deviceClipped(1));

    const std::vector<double> fullScale(64, 1.0);
    // Deliberately without setMetersActive(): the clip indicator has to catch what happened while
    // nothing was on screen, unlike the level meters.
    device->clipDetector().write(fullScale.data(), 32);
    QVERIFY(meters.deviceClipped(0));

    meters.clearDeviceClip(0);
    QVERIFY(!meters.deviceClipped(0));

    device->clipDetector().write(fullScale.data(), 32);
    meters.clearAllDeviceClips();
    QVERIFY(!meters.deviceClipped(0));
}

void DeviceRackControllerTest::test_deviceSettings_shouldRoundTripThroughController()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
    const auto editorService = std::make_shared<MockEditorService>();
    const auto prefix = Constants::internalDevicePortPrefix().toStdString();
    deviceService->setDevice(2, std::make_shared<MockDevice>(prefix + " 3"));

    DeviceRackController controller { deviceService, {}, editorService };

    DeviceMixerController mixer { deviceService, editorService };

    QCOMPARE(controller.slotOfDevice(QString::fromStdString(prefix + " 3")), 2);
    QCOMPARE(controller.slotOfDevice("No Such Device"), -1);

    // Defaults are the legacy chain.
    QCOMPARE(mixer.deviceFaderPosition(2), static_cast<int>(Device::FaderPosition::PreInserts));
    QCOMPARE(mixer.deviceSendTap(2), static_cast<int>(Device::SendTap::PostFader));

    mixer.setDeviceFaderPosition(2, static_cast<int>(Device::FaderPosition::PostInserts));
    mixer.setDeviceSendTap(2, static_cast<int>(Device::SendTap::PreFader));

    QCOMPARE(mixer.deviceFaderPosition(2), static_cast<int>(Device::FaderPosition::PostInserts));
    QCOMPARE(mixer.deviceSendTap(2), static_cast<int>(Device::SendTap::PreFader));
}

void DeviceRackControllerTest::test_deviceMidiMappings_shouldRoundTripThroughController()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
    const auto editorService = std::make_shared<MockEditorService>();
    const auto prefix = Constants::internalDevicePortPrefix().toStdString();
    const auto device = std::make_shared<MockDevice>(prefix + " 1");
    deviceService->setDevice(0, device);

    DeviceRackController controller { deviceService, {}, editorService };

    MidiMappingController midiMappings { deviceService, editorService };
    QVERIFY(midiMappings.deviceMidiMappings(0).isEmpty());

    // Out of range values are brought back into it
    midiMappings.setDeviceMidiMappings(0, { QVariantMap { { "controller", 200 }, { "target", "cutoff" }, { "min", 0.75 }, { "max", 1.5 } } });

    QVERIFY(editorService->isModified());
    const auto expected = MidiMapping { MidiMapping::Kind::Cc, Device::MaxMappableCc, "cutoff", 0.75, 1.0 };
    QVERIFY(device->midiMappings() == Device::MidiMappingList { expected });
    const auto mappings = midiMappings.deviceMidiMappings(0);
    QCOMPARE(mappings.size(), 1);
    QCOMPARE(mappings.at(0).toMap()["controller"].toInt(), static_cast<int>(Device::MaxMappableCc));
    QCOMPARE(mappings.at(0).toMap()["target"].toString(), QString { "cutoff" });
    QCOMPARE(mappings.at(0).toMap()["min"].toDouble(), 0.75);
    QCOMPARE(mappings.at(0).toMap()["max"].toDouble(), 1.0);

    QSignalSpy learnSpy { &midiMappings, &MidiMappingController::controllerCcReceived };
    midiMappings.notifyControllerCcReceived(74);
    QCOMPARE(learnSpy.count(), 1);
    QCOMPARE(learnSpy.at(0).at(0).toInt(), 74);
}

void DeviceRackControllerTest::test_importSettings_matchingType_shouldEmitConfirmationWithoutMismatch()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
    const auto editorService = std::make_shared<MockEditorService>();
    deviceService->setDevice(0, DeviceFactory::createDevice(SynthDevice::typeIdString(), "TestSynth"));

    QByteArray data;
    QBuffer buffer { &data };
    buffer.open(QIODevice::WriteOnly);
    NahdXmlWriter writer { buffer };
    QVERIFY(deviceService->exportDeviceSettings(0, writer));
    buffer.close();

    buffer.open(QIODevice::ReadOnly);
    NahdXmlReader reader { buffer };
    const auto typeInfo = deviceService->peekDeviceTypeInfo(reader);
    buffer.close();

    QCOMPARE(typeInfo.typeId, QString::fromStdString(SynthDevice::typeIdString()));
    QCOMPARE(typeInfo.typeId, QString::fromStdString(deviceService->device(0)->typeId()));
}

void DeviceRackControllerTest::test_importSettings_differentType_shouldEmitConfirmationWithMismatch()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());

    deviceService->setDevice(0, DeviceFactory::createDevice(SynthDevice::typeIdString(), "TestSynth"));

    QByteArray data;
    QBuffer buffer { &data };
    buffer.open(QIODevice::WriteOnly);
    NahdXmlWriter writer { buffer };
    QVERIFY(deviceService->exportDeviceSettings(0, writer));
    buffer.close();

    deviceService->setDevice(0, DeviceFactory::createDevice(SamplerDevice::typeIdString(), "TestSampler"));

    buffer.open(QIODevice::ReadOnly);
    NahdXmlReader reader { buffer };
    const auto typeInfo = deviceService->peekDeviceTypeInfo(reader);
    buffer.close();

    QCOMPARE(typeInfo.typeId, QString::fromStdString(SynthDevice::typeIdString()));
    QVERIFY(typeInfo.typeId != QString::fromStdString(deviceService->device(0)->typeId()));
}

void DeviceRackControllerTest::test_confirmImportSettings_shouldImportAndNotify()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
    const auto editorService = std::make_shared<MockEditorService>();

    const auto synth = std::dynamic_pointer_cast<SynthDevice>(DeviceFactory::createDevice(SynthDevice::typeIdString(), "TestSynth"));
    synth->setVolume(0.42f);
    deviceService->setDevice(0, synth);

    QByteArray data;
    QBuffer buffer { &data };
    buffer.open(QIODevice::WriteOnly);
    NahdXmlWriter writer { buffer };
    QVERIFY(deviceService->exportDeviceSettings(0, writer));
    buffer.close();

    synth->setVolume(1.0f);
    QCOMPARE(synth->volume(), 1.0f);

    buffer.open(QIODevice::ReadOnly);
    NahdXmlReader reader { buffer };
    QVERIFY(deviceService->importDeviceSettings(0, reader));
    buffer.close();

    QCOMPARE(synth->volume(), 0.42f);
}

void DeviceRackControllerTest::test_confirmImportSettings_sameType_shouldReportTheImportedSlot()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
    const auto editorService = std::make_shared<MockEditorService>();

    const auto synth = std::dynamic_pointer_cast<SynthDevice>(DeviceFactory::createDevice(SynthDevice::typeIdString(), "TestSynth"));
    synth->setVolume(0.42f);
    deviceService->setDevice(1, synth);

    DeviceRackController controller { deviceService, {}, editorService };
    QTemporaryDir directory;
    const auto filePath = directory.filePath("device" + Constants::deviceSettingsExtension());
    controller.exportSettings(1, QUrl::fromLocalFile(filePath));

    synth->setVolume(1.0f);

    // A dialog open on the slot has to know the values under it changed: the device object is the
    // one it is already showing, so nothing else tells it.
    QSignalSpy spy { &controller, &DeviceRackController::deviceSettingsImported };
    controller.confirmImportSettings(1, QUrl::fromLocalFile(filePath));

    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).toInt(), 1);
    QCOMPARE(deviceService->device(1), synth);
    QCOMPARE(synth->volume(), 0.42f);
}

void DeviceRackControllerTest::test_confirmImportSettings_differentType_shouldReportTheReplacedSlot()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
    const auto editorService = std::make_shared<MockEditorService>();

    deviceService->setDevice(0, DeviceFactory::createDevice(SynthDevice::typeIdString(), "TestSynth"));

    DeviceRackController controller { deviceService, {}, editorService };
    QTemporaryDir directory;
    const auto filePath = directory.filePath("device" + Constants::deviceSettingsExtension());
    controller.exportSettings(0, QUrl::fromLocalFile(filePath));

    deviceService->setDevice(0, DeviceFactory::createDevice(BassSynthDevice::typeIdString(), "TestBassSynth"));

    QSignalSpy spy { &controller, &DeviceRackController::deviceSettingsImported };
    controller.confirmImportSettings(0, QUrl::fromLocalFile(filePath));

    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).toInt(), 0);
    // The slot holds a different device now, which is what a dialog open on it has to notice
    QCOMPARE(deviceService->device(0)->typeId(), SynthDevice::typeIdString());
}

void DeviceRackControllerTest::test_copyDevice_shouldDuplicateAndNotify()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
    const auto editorService = std::make_shared<MockEditorService>();

    const auto synth = std::dynamic_pointer_cast<SynthDevice>(DeviceFactory::createDevice(SynthDevice::typeIdString(), "TestSynth"));
    synth->setVolume(0.42f);
    deviceService->setDevice(0, synth);

    DeviceRackController controller { deviceService, {}, editorService };
    QSignalSpy revisionSpy { &controller, &DeviceRackController::revisionChanged };

    controller.copyDevice(0, 1);

    QVERIFY(revisionSpy.count() > 0);
    QVERIFY(editorService->isModified());
    const auto copy = std::dynamic_pointer_cast<SynthDevice>(deviceService->device(1));
    QVERIFY(copy != nullptr);
    QCOMPARE(copy->volume(), 0.42f);
}

void DeviceRackControllerTest::test_populatedDevices_shouldReturnOnlyFilledSlots()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
    const auto editorService = std::make_shared<MockEditorService>();

    deviceService->setDevice(0, DeviceFactory::createDevice(SynthDevice::typeIdString(), "TestSynth"));
    deviceService->setDevice(2, DeviceFactory::createDevice(SamplerDevice::typeIdString(), "TestSampler"));

    DeviceRackController controller { deviceService, {}, editorService };

    const auto populated = controller.populatedDevices();
    QCOMPARE(populated.size(), 2);
    QCOMPARE(populated.at(0).toMap()["slotIndex"].toInt(), 0);
    QCOMPARE(populated.at(1).toMap()["slotIndex"].toInt(), 2);
}

void DeviceRackControllerTest::test_openDevice_shouldSnapshotStateForCancel()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
    const auto editorService = std::make_shared<MockEditorService>();

    const auto synth = std::make_shared<SynthDevice>("TestSynth");
    deviceService->setDevice(0, synth);
    synth->setLpfCutoff(0.4f);

    const auto synthController = std::make_shared<SynthDialogModel>(nullptr);
    DeviceRackController controller { deviceService, { synthController }, editorService };

    // Opening the dialog is what has to leave the device restorable, so that a Cancel further on
    // has something to put back
    controller.openDevice(0);

    synth->setLpfCutoff(0.9f);
    synthController->reject();

    QCOMPARE(synth->lpfCutoff(), 0.4f);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::DeviceRackControllerTest)
