#include "effect_rack_controller_test.hpp"
#include "../../application/service/device_service.hpp"
#include "../../application/service/editor_service.hpp"
#include "../../common/constants.hpp"
#include "../../domain/effects/auto_filter.hpp"
#include "../../domain/effects/auto_panner.hpp"
#include "../../domain/effects/clipper.hpp"
#include "../../domain/effects/compressor.hpp"
#include "../../domain/effects/effect_factory.hpp"
#include "../../domain/effects/effect_rack.hpp"
#include "../../domain/effects/endless_reverb.hpp"
#include "../../domain/effects/eq_8_band_parametric.hpp"
#include "../../domain/effects/panner.hpp"
#include "../../domain/effects/phaser.hpp"
#include "../../domain/effects/reverb.hpp"
#include "../../domain/effects/saturator.hpp"
#include "../../domain/utility/dbtp_meter.hpp"
#include "../../domain/utility/lufs_meter.hpp"
#include "../../infra/audio/audio_engine.hpp"
#include "../../infra/data_service.hpp"
#include "../../infra/xml/nahd_xml_reader.hpp"
#include "../../infra/xml/nahd_xml_writer.hpp"
#include "../../view/controllers/effect_rack_controller.hpp"

#include <QBuffer>
#include <QSignalSpy>
#include <QTest>

#include <cmath>
#include <memory>
#include <numbers>

#include "../../domain/devices/device.hpp"
#include "../../domain/devices/drum_synth_device.hpp"

namespace noteahead {

class MockDevice : public Device
{
public:
    MockDevice(const std::string & name)
      : m_name { name }
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
        return "mock";
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

private:
    std::string m_name;
};

void EffectRackControllerTest::initTestCase()
{
    EffectFactory::init();
}

void EffectRackControllerTest::cleanupTestCase()
{
    EffectFactory::clear();
}

void EffectRackControllerTest::test_effectParametersSummary_reverb_shouldReturnFormattedSummary()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
    const auto editorService = std::make_shared<EditorService>();
    EffectRackController controller { deviceService, editorService };

    controller.setIsInsertRack(true);
    controller.setEffect(0, QString::fromStdString(Reverb::typeIdString()));

    // Default reverb: pre-delay 20ms, decay 1500ms
    const auto summary = controller.effectParametersSummary(0);
    QCOMPARE(summary, QString { "(pre=20ms, decay=1500ms)" });

    // Change parameters
    controller.setParameterValue(0, controller.reverbPreDelayKey(), 50.0f / 500.0f); // 50ms
    controller.setParameterValue(0, controller.reverbDecayKey(), 3000.0f / 10000.0f); // 3000ms

    const auto summary2 = controller.effectParametersSummary(0);
    QCOMPARE(summary2, QString { "(pre=50ms, decay=3000ms)" });
}

void EffectRackControllerTest::test_effectParametersSummary_endlessReverb_shouldReportPreDelay()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
    const auto editorService = std::make_shared<EditorService>();
    EffectRackController controller { deviceService, editorService };

    controller.setIsInsertRack(true);
    controller.setEffect(0, QString::fromStdString(EndlessReverb::typeIdString()));

    // Defaults: pre-delay 20ms, size 70%, not frozen
    const auto summary = controller.effectParametersSummary(0);
    QCOMPARE(summary, QString { "(pre=20ms, size=70%, freeze=off)" });

    controller.setParameterValue(0, controller.endlessPreDelayKey(), 50.0f / 500.0f); // 50ms

    const auto summary2 = controller.effectParametersSummary(0);
    QCOMPARE(summary2, QString { "(pre=50ms, size=70%, freeze=off)" });
}

void EffectRackControllerTest::test_effectParametersSummary_compressor_shouldReturnFormattedSummary()
{
    const auto audioEngine { std::make_shared<AudioEngine>() };
    const auto deviceService { std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>()) };
    const auto editorService { std::make_shared<EditorService>() };
    EffectRackController controller { deviceService, editorService };

    controller.setIsInsertRack(true);
    controller.setEffect(0, QString::fromStdString(Compressor::typeIdString()));

    // Default compressor: detector Peak, attack 0.2 internal -> ~0.5ms, ratio 0.15 internal -> 4:1, sidechain=None
    const auto summary { controller.effectParametersSummary(0) };
    QCOMPARE(summary, QString { "(Peak, attack=0.5ms, ratio=4:1, sidechain=None)" });

    // Change ratio to 10:1 (internal = (10-1)/19 = 0.473...)
    controller.setParameterValue(0, controller.compressorRatioKey(), 9.0f / 19.0f);
    const auto summary2 { controller.effectParametersSummary(0) };
    QCOMPARE(summary2, QString { "(Peak, attack=0.5ms, ratio=10:1, sidechain=None)" });

    // Put a device in slot 1 and set it as sidechain source
    const auto device1 { std::make_shared<MockDevice>("Device 1") };
    deviceService->setDevice(1, device1);

    controller.setParameterValue(0, controller.compressorSideChainSourceDeviceKey(), 1.0f);
    const auto summary3 { controller.effectParametersSummary(0) };
    QCOMPARE(summary3, QString { "(Peak, attack=0.5ms, ratio=10:1, sidechain=Device 1)" });

    controller.setParameterValue(0, controller.compressorModeKey(), 1.0f);
    const auto summary4 { controller.effectParametersSummary(0) };
    QCOMPARE(summary4, QString { "(RMS, attack=0.5ms, ratio=10:1, sidechain=Device 1)" });
}

void EffectRackControllerTest::test_effectParametersSummary_autoPanner_shouldReturnFormattedSummary()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
    const auto editorService = std::make_shared<EditorService>();
    EffectRackController controller { deviceService, editorService };

    controller.setIsInsertRack(true);
    controller.setEffect(0, QString::fromStdString(AutoPanner::typeIdString()));

    // Default auto panner: rate 1.00Hz, intensity 100%
    const auto summary = controller.effectParametersSummary(0);
    QCOMPARE(summary, QString { "(rate=1.00Hz, int=100%)" });
}

void EffectRackControllerTest::test_effectParametersSummary_autoFilter_shouldReturnFormattedSummary()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
    const auto editorService = std::make_shared<EditorService>();
    EffectRackController controller { deviceService, editorService };

    controller.setIsInsertRack(true);
    controller.setEffect(0, QString::fromStdString(AutoFilter::typeIdString()));

    // Default auto filter: low pass at 2.5 kHz swept once a second at a quarter of the range
    const auto summary = controller.effectParametersSummary(0);
    QCOMPARE(summary, QString { "(LPF, 2.5kHz, rate=1.00Hz, int=25%)" });

    controller.setParameterValue(0, controller.autoFilterFilterTypeKey(), 1.0f);
    controller.setParameterValue(0, controller.autoFilterLfoModeKey(), static_cast<float>(Lfo::Mode::BPM));
    controller.setParameterValue(0, controller.autoFilterLfoRateKey(), 0.25f);
    const auto syncedSummary = controller.effectParametersSummary(0);
    QCOMPARE(syncedSummary, QString { "(HPF, 2.5kHz, rate=1/4, int=25%)" });
}

void EffectRackControllerTest::test_effectParametersSummary_phaser_shouldReturnFormattedSummary()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
    const auto editorService = std::make_shared<EditorService>();
    EffectRackController controller { deviceService, editorService };

    controller.setIsInsertRack(true);
    controller.setEffect(0, QString::fromStdString(Phaser::typeIdString()));

    // Default phaser: six stages swept once a second, with no feedback
    const auto summary = controller.effectParametersSummary(0);
    QCOMPARE(summary, QString { "(6 stages, rate=1.00Hz, fb=0%)" });
}

void EffectRackControllerTest::test_effectParametersSummary_panner_shouldReturnFormattedSummary()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
    const auto editorService = std::make_shared<EditorService>();
    EffectRackController controller { deviceService, editorService };

    controller.setIsInsertRack(true);
    controller.setEffect(0, QString::fromStdString(Panner::typeIdString()));

    // Default panner: pan 50%, width 100%
    const auto summary = controller.effectParametersSummary(0);
    QCOMPARE(summary, QString { "(pan=50%, width=100%)" });
}

void EffectRackControllerTest::test_effectParametersSummary_clipper_shouldReturnFormattedSummary()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
    const auto editorService = std::make_shared<EditorService>();
    EffectRackController controller { deviceService, editorService };

    controller.setIsInsertRack(true);
    controller.setEffect(0, QString::fromStdString(Clipper::typeIdString()));

    // Default clipper: threshold 0.0dB
    const auto summary = controller.effectParametersSummary(0);
    QCOMPARE(summary, QString { "(thr=0.0dB)" });
}

void EffectRackControllerTest::test_effectParametersSummary_saturator_shouldReturnFormattedSummary()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
    const auto editorService = std::make_shared<EditorService>();
    EffectRackController controller { deviceService, editorService };

    controller.setIsInsertRack(true);
    controller.setEffect(0, QString::fromStdString(Saturator::typeIdString()));

    // Default saturator: drive 6.0dB, mix 100%
    const auto summary = controller.effectParametersSummary(0);
    QCOMPARE(summary, QString { "(drive=6.0dB, mix=100%)" });
}

void EffectRackControllerTest::test_effectParametersSummary_eq8BandParametric_shouldReturnFormattedSummary()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
    const auto editorService = std::make_shared<EditorService>();
    EffectRackController controller { deviceService, editorService };

    controller.setIsInsertRack(true);
    controller.setEffect(0, QString::fromStdString(Eq8BandParametric::typeIdString()));

    const auto summary = controller.effectParametersSummary(0);
    QCOMPARE(summary, QString { "(Parametric, Mid + Side)" });

    controller.setParameterValue(0, Constants::NahdXml::xmlKeyStereoMode(), static_cast<float>(Eq8BandParametric::StereoMode::Mid));
    QCOMPARE(controller.effectParametersSummary(0), QString { "(Parametric, Mid)" });

    controller.setParameterValue(0, Constants::NahdXml::xmlKeyStereoMode(), static_cast<float>(Eq8BandParametric::StereoMode::Side));
    QCOMPARE(controller.effectParametersSummary(0), QString { "(Parametric, Side)" });
}

void EffectRackControllerTest::test_effectParametersSummary_emptySlot_shouldReturnEmptyString()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
    const auto editorService = std::make_shared<EditorService>();
    EffectRackController controller { deviceService, editorService };

    controller.setIsInsertRack(true);
    QCOMPARE(controller.effectParametersSummary(0), QString { "" });
}

namespace {

//! Drive a rack effect with a stereo sine of the given amplitude, so its meter settles somewhere
//! specific and the summary can be read at a known digit count.
//! The pad character the rack summary lines its numeric fields up with.
const QChar figureSpace { 0x2007 };

void feedSine(const std::shared_ptr<Effect> & effect, double amplitude, double seconds)
{
    static constexpr double sampleRate = 48000.0;
    effect->setSampleRate(sampleRate);
    const auto samples = static_cast<int>(sampleRate * seconds);
    for (int i = 0; i < samples; i++) {
        double l = amplitude * std::sin(2.0 * std::numbers::pi * 1000.0 / sampleRate * i);
        double r = l;
        effect->process(l, r);
    }
}

} // namespace

void EffectRackControllerTest::test_effectParametersSummary_lufsMeter_shouldPadReadingsToConstantWidth()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
    const auto editorService = std::make_shared<EditorService>();
    EffectRackController controller { deviceService, editorService };

    controller.setIsInsertRack(true);
    controller.setEffect(0, QString::fromStdString(LufsMeter::typeIdString()));
    const auto effect = deviceService->insertEffectRack().effect(0);
    QVERIFY(effect);

    // A reading crossing -10 gains a digit, and without padding everything after it in the row would
    // shift along. The summary has to keep the same length whatever the meter says.
    const auto atFloor = controller.effectParametersSummary(0);
    QCOMPARE(atFloor, QString { "(M=%1-∞ S=%1-∞ I=%1-∞ LUFS)" }.arg(QString { figureSpace }.repeated(3)));

    feedSine(effect, 0.385, 4.0); // ≈ -8 LUFS: one digit before the point, so the field is padded
    const auto oneDigit = controller.effectParametersSummary(0);
    QVERIFY(oneDigit.contains(QString { "M=%1-" }.arg(figureSpace)));
    QCOMPARE(oneDigit.length(), atFloor.length());

    feedSine(effect, 0.1, 4.0); // ≈ -21 LUFS: two digits, filling the field
    const auto twoDigits = controller.effectParametersSummary(0);
    QVERIFY(twoDigits.contains("M=-2"));
    QCOMPARE(twoDigits.length(), atFloor.length());
}

void EffectRackControllerTest::test_effectParametersSummary_dbtpMeter_shouldPadReadingsToConstantWidth()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
    const auto editorService = std::make_shared<EditorService>();
    EffectRackController controller { deviceService, editorService };

    controller.setIsInsertRack(true);
    controller.setEffect(0, QString::fromStdString(DbTpMeter::typeIdString()));
    const auto effect = deviceService->insertEffectRack().effect(0);
    QVERIFY(effect);

    const auto atFloor = controller.effectParametersSummary(0);
    QCOMPARE(atFloor, QString { "(L=%1-∞ R=%1-∞ dBTP)" }.arg(QString { figureSpace }.repeated(3)));

    feedSine(effect, 0.35, 0.2); // ≈ -9 dBTP: one digit before the point, so the field is padded
    const auto oneDigit = controller.effectParametersSummary(0);
    QVERIFY(oneDigit.contains(QString { "L=%1-" }.arg(figureSpace)));
    QCOMPARE(oneDigit.length(), atFloor.length());

    // A fresh meter, because the peak hold would otherwise keep showing the loud reading.
    controller.setEffect(0, QString::fromStdString(DbTpMeter::typeIdString()));
    feedSine(deviceService->insertEffectRack().effect(0), 0.035, 0.2); // ≈ -29 dBTP: two digits
    const auto twoDigits = controller.effectParametersSummary(0);
    QVERIFY(twoDigits.contains("L=-2"));
    QCOMPARE(twoDigits.length(), atFloor.length());
}

void EffectRackControllerTest::test_isEffectEnabled_shouldReturnEnabledState()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
    const auto editorService = std::make_shared<EditorService>();
    EffectRackController controller { deviceService, editorService };

    controller.setIsInsertRack(true);
    controller.setEffect(0, QString::fromStdString(Reverb::typeIdString()));

    QVERIFY(controller.isEffectEnabled(0));

    controller.setIsEffectEnabled(0, false);
    QVERIFY(!controller.isEffectEnabled(0));
    QVERIFY(editorService->isModified());

    controller.setIsEffectEnabled(0, true);
    QVERIFY(controller.isEffectEnabled(0));
}

void EffectRackControllerTest::test_currentRack_drumVoiceSubIndex_shouldTargetVoiceRack()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
    const auto editorService = std::make_shared<EditorService>();
    const auto drum = std::make_shared<DrumSynthDevice>("Drum 1");
    deviceService->setDevice(0, drum);

    EffectRackController controller { deviceService, editorService };
    controller.setTargetDeviceName("Drum 1");
    controller.setTargetSubIndex(2);
    controller.setEffect(0, QString::fromStdString(Reverb::typeIdString()));

    // The effect lands on voice 2's rack only.
    QVERIFY(drum->voiceEffectRack(2).hasEffects());
    QVERIFY(!drum->voiceEffectRack(0).hasEffects());
    QVERIFY(!drum->insertEffectRack().hasEffects());

    // Switching back to the whole-device rack targets the device insert rack.
    controller.setTargetSubIndex(-1);
    controller.setEffect(1, QString::fromStdString(Reverb::typeIdString()));
    QVERIFY(drum->insertEffectRack().hasEffects());
}

void EffectRackControllerTest::test_revision_shouldIncrementOnPropertySet()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
    const auto editorService = std::make_shared<EditorService>();
    EffectRackController controller { deviceService, editorService };

    QSignalSpy revisionSpy { &controller, &EffectRackController::revisionChanged };
    const int initialRevision = controller.revision();

    // Setting the same target device name should still increment revision
    controller.setTargetDeviceName(controller.targetDeviceName());
    QCOMPARE(controller.revision(), initialRevision + 1);
    QCOMPARE(revisionSpy.count(), 1);

    // Setting the same isInsertRack value should still increment revision
    controller.setIsInsertRack(controller.isInsertRack());
    QCOMPARE(controller.revision(), initialRevision + 2);
    QCOMPARE(revisionSpy.count(), 2);
}

void EffectRackControllerTest::test_exportSettings_shouldSerializeEffects()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
    const auto editorService = std::make_shared<EditorService>();
    EffectRackController controller { deviceService, editorService };

    controller.setIsInsertRack(true);
    controller.setEffect(0, QString::fromStdString(Reverb::typeIdString()));

    QByteArray data;
    QBuffer buffer { &data };
    buffer.open(QIODevice::WriteOnly);
    NahdXmlWriter writer { buffer };
    QVERIFY(controller.exportSettings(writer));
    buffer.close();

    QVERIFY(!data.isEmpty());
    QVERIFY(data.contains(Reverb::typeIdString().c_str()));
}

void EffectRackControllerTest::test_importSettings_shouldRestoreEffects()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
    const auto editorService = std::make_shared<EditorService>();
    EffectRackController controller { deviceService, editorService };

    controller.setIsInsertRack(true);
    controller.setEffect(0, QString::fromStdString(Reverb::typeIdString()));
    controller.setParameterValue(0, controller.reverbDecayKey(), 0.75f);

    QByteArray data;
    QBuffer buffer { &data };
    buffer.open(QIODevice::WriteOnly);
    NahdXmlWriter writer { buffer };
    QVERIFY(controller.exportSettings(writer));
    buffer.close();

    controller.clearEffect(0);
    QCOMPARE(controller.effectType(0), QString {});

    buffer.open(QIODevice::ReadOnly);
    NahdXmlReader reader { buffer };
    QVERIFY(controller.importSettings(reader));
    buffer.close();

    QCOMPARE(controller.effectType(0), controller.reverbType());
    QCOMPARE(controller.parameterValue(0, controller.reverbDecayKey()), 0.75f);
}

void EffectRackControllerTest::test_importEffectSettings_matchingType_shouldEmitConfirmationWithoutMismatch()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
    const auto editorService = std::make_shared<EditorService>();
    EffectRackController controller { deviceService, editorService };

    controller.setIsInsertRack(true);
    controller.setEffect(0, QString::fromStdString(Reverb::typeIdString()));

    // Create a temporary reverb settings file
    const QString filePath = "test_matching_effect.nahdeff";
    const auto reverb = std::make_shared<Reverb>();
    QFile file { filePath };
    QVERIFY(file.open(QIODevice::WriteOnly));
    NahdXmlWriter writer { file };
    writer.writeStartDocument();
    writer.writeStartElement(Constants::NahdXml::xmlKeySettings());
    writer.writeStartElement(Constants::NahdXml::xmlKeyEffect());
    writer.writeAttribute(Constants::NahdXml::xmlKeyTypeId(), QString::fromStdString(reverb->typeId()));
    writer.writeAttribute(Constants::NahdXml::xmlKeyType(), QString::fromStdString(reverb->type()));
    writer.writeEndElement();
    writer.writeEndElement();
    writer.writeEndDocument();
    file.close();

    QSignalSpy spy { &controller, &EffectRackController::importEffectSettingsConfirmationRequested };
    controller.importEffectSettings(0, QUrl::fromLocalFile(filePath));

    QCOMPARE(spy.count(), 1);
    const auto arguments = spy.at(0);
    QCOMPARE(arguments.at(0).toInt(), 0);
    QCOMPARE(arguments.at(2).toString(), QString::fromStdString(reverb->type()));
    QCOMPARE(arguments.at(3).toString(), QString::fromStdString(reverb->type()));
    QCOMPARE(arguments.at(4).toBool(), false); // No mismatch

    QFile::remove(filePath);
}

void EffectRackControllerTest::test_importEffectSettings_differentType_shouldEmitConfirmationWithMismatch()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
    const auto editorService = std::make_shared<EditorService>();
    EffectRackController controller { deviceService, editorService };

    controller.setIsInsertRack(true);
    controller.setEffect(0, QString::fromStdString(Reverb::typeIdString()));

    // Create a temporary compressor settings file
    const QString filePath = "test_different_effect.nahdeff";
    const auto compressor = std::make_shared<Compressor>();
    QFile file { filePath };
    QVERIFY(file.open(QIODevice::WriteOnly));
    NahdXmlWriter writer { file };
    writer.writeStartDocument();
    writer.writeStartElement(Constants::NahdXml::xmlKeySettings());
    writer.writeStartElement(Constants::NahdXml::xmlKeyEffect());
    writer.writeAttribute(Constants::NahdXml::xmlKeyTypeId(), QString::fromStdString(compressor->typeId()));
    writer.writeAttribute(Constants::NahdXml::xmlKeyType(), QString::fromStdString(compressor->type()));
    writer.writeEndElement();
    writer.writeEndElement();
    writer.writeEndDocument();
    file.close();

    QSignalSpy spy { &controller, &EffectRackController::importEffectSettingsConfirmationRequested };
    controller.importEffectSettings(0, QUrl::fromLocalFile(filePath));

    QCOMPARE(spy.count(), 1);
    const auto arguments = spy.at(0);
    QCOMPARE(arguments.at(4).toBool(), true); // Mismatch

    QFile::remove(filePath);
}

void EffectRackControllerTest::test_confirmImportEffectSettings_shouldImportAndNotify()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
    const auto editorService = std::make_shared<EditorService>();
    EffectRackController controller { deviceService, editorService };

    controller.setIsInsertRack(true);
    controller.setEffect(0, QString::fromStdString(Reverb::typeIdString()));

    const QString filePath = "test_confirm_effect.nahdeff";
    const auto reverb = std::make_shared<Reverb>();
    QFile file { filePath };
    QVERIFY(file.open(QIODevice::WriteOnly));
    NahdXmlWriter writer { file };
    writer.writeStartDocument();
    writer.writeStartElement(Constants::NahdXml::xmlKeySettings());
    writer.writeStartElement(Constants::NahdXml::xmlKeyEffect());
    writer.writeAttribute(Constants::NahdXml::xmlKeyTypeId(), QString::fromStdString(reverb->typeId()));
    writer.writeAttribute(Constants::NahdXml::xmlKeyType(), QString::fromStdString(reverb->type()));
    writer.writeStartElement(Constants::NahdXml::xmlKeyParameter());
    writer.writeAttribute(Constants::NahdXml::xmlKeyName(), "reverbSize");
    writer.writeAttribute(Constants::NahdXml::xmlKeyValue(), "1230");
    writer.writeEndElement();
    writer.writeEndElement();
    writer.writeEndElement();
    writer.writeEndDocument();
    file.close();

    QSignalSpy revisionSpy { &controller, &EffectRackController::revisionChanged };
    controller.confirmImportEffectSettings(0, QUrl::fromLocalFile(filePath));

    QCOMPARE(revisionSpy.count(), 1);
    QCOMPARE(controller.parameterValue(0, controller.reverbSizeKey()), 0.123f);
    QVERIFY(editorService->isModified());

    QFile::remove(filePath);
}

void EffectRackControllerTest::test_copyEffect_shouldDuplicateAndNotify()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
    const auto editorService = std::make_shared<EditorService>();
    EffectRackController controller { deviceService, editorService };

    controller.setIsInsertRack(true);
    controller.setEffect(0, QString::fromStdString(Reverb::typeIdString()));
    controller.setParameterValue(0, controller.reverbSizeKey(), 0.42f);

    QSignalSpy revisionSpy { &controller, &EffectRackController::revisionChanged };
    controller.copyEffect(0, 2);

    QVERIFY(revisionSpy.count() > 0);
    QVERIFY(editorService->isModified());
    QCOMPARE(controller.effectType(2), controller.reverbType());
    QCOMPARE(controller.parameterValue(2, controller.reverbSizeKey()), 0.42f);
}

void EffectRackControllerTest::test_copyRackFrom_device_shouldReplaceTargetRackAndNotify()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
    const auto editorService = std::make_shared<EditorService>();
    const auto device = std::make_shared<MockDevice>("Mock Device");
    deviceService->setDevice(0, device);
    EffectRackController controller { deviceService, editorService };

    // Build the source on the device, and something else on the master rack it will replace.
    controller.setIsInsertRack(true);
    controller.setTargetDeviceName("Mock Device");
    controller.setEffect(1, QString::fromStdString(Reverb::typeIdString()));
    controller.setParameterValue(1, controller.reverbSizeKey(), 0.42f);

    controller.setTargetDeviceName({});
    controller.setEffect(0, QString::fromStdString(Compressor::typeIdString()));

    QSignalSpy revisionSpy { &controller, &EffectRackController::revisionChanged };
    QVERIFY(controller.copyRackFrom("Mock Device", true));

    QVERIFY(revisionSpy.count() > 0);
    QVERIFY(editorService->isModified());
    QCOMPARE(controller.effectType(0), QString {});
    QCOMPARE(controller.effectType(1), controller.reverbType());
    QCOMPARE(controller.parameterValue(1, controller.reverbSizeKey()), 0.42f);

    // The copy is independent: editing it does not reach into the rack it came from.
    controller.setParameterValue(1, controller.reverbSizeKey(), 0.1f);
    controller.setTargetDeviceName("Mock Device");
    QCOMPARE(controller.parameterValue(1, controller.reverbSizeKey()), 0.42f);
}

void EffectRackControllerTest::test_copyRackFrom_sameRack_shouldFail()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
    const auto editorService = std::make_shared<EditorService>();
    EffectRackController controller { deviceService, editorService };

    controller.setIsInsertRack(true);
    controller.setEffect(0, QString::fromStdString(Reverb::typeIdString()));

    QVERIFY(!controller.copyRackFrom({}, true));
    QCOMPARE(controller.effectType(0), controller.reverbType());
}

void EffectRackControllerTest::test_availableRackSources_shouldLeaveOutTheTargetRack()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
    const auto editorService = std::make_shared<EditorService>();
    deviceService->setDevice(0, std::make_shared<MockDevice>("Mock Device"));
    EffectRackController controller { deviceService, editorService };

    const auto names = [](const QVariantList & sources) {
        QStringList list;
        for (const auto & source : sources) {
            list.append(source.toMap()["name"].toString());
        }
        return list;
    };

    // Targeting the master insert rack: the master send rack and the device's rack are left.
    controller.setTargetDeviceName({});
    controller.setIsInsertRack(true);
    auto sources = controller.availableRackSources();
    QCOMPARE(sources.size(), 2);
    QVERIFY(!names(sources).contains("Master Insert Effects"));
    QVERIFY(names(sources).contains("Mock Device"));

    // Targeting the device's rack: both master racks are left.
    controller.setTargetDeviceName("Mock Device");
    sources = controller.availableRackSources();
    QCOMPARE(sources.size(), 2);
    QVERIFY(!names(sources).contains("Mock Device"));

    controller.setEffect(0, QString::fromStdString(Reverb::typeIdString()));
    controller.setTargetDeviceName({});
    for (const auto & source : controller.availableRackSources()) {
        const auto map = source.toMap();
        QCOMPARE(map["effectCount"].toInt(), map["name"].toString() == "Mock Device" ? 1 : 0);
    }
}

void EffectRackControllerTest::test_revertEffect_shouldRestoreSnapshotAndNotify()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
    const auto editorService = std::make_shared<EditorService>();
    EffectRackController controller { deviceService, editorService };

    controller.setIsInsertRack(true);
    controller.setEffect(0, QString::fromStdString(Reverb::typeIdString()));
    controller.setParameterValue(0, controller.reverbSizeKey(), 0.42f);

    controller.snapshotEffect(0);

    controller.setParameterValue(0, controller.reverbSizeKey(), 0.9f);
    controller.setIsEffectEnabled(0, false);
    QCOMPARE(controller.parameterValue(0, controller.reverbSizeKey()), 0.9f);

    QSignalSpy revisionSpy { &controller, &EffectRackController::revisionChanged };
    QSignalSpy parameterSpy { &controller, &EffectRackController::parameterChanged };
    controller.revertEffect(0);

    QCOMPARE(controller.parameterValue(0, controller.reverbSizeKey()), 0.42f);
    QVERIFY(controller.isEffectEnabled(0));
    QCOMPARE(revisionSpy.count(), 1);
    // An empty parameter name is how the dialog is told to re-read all of them
    QCOMPARE(parameterSpy.count(), 1);
    QCOMPARE(parameterSpy.at(0).at(1).toString(), QString {});
}

void EffectRackControllerTest::test_revertEffect_withoutSnapshot_shouldKeepEdits()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
    const auto editorService = std::make_shared<EditorService>();
    EffectRackController controller { deviceService, editorService };

    controller.setIsInsertRack(true);
    controller.setEffect(0, QString::fromStdString(Reverb::typeIdString()));
    controller.setParameterValue(0, controller.reverbSizeKey(), 0.7f);

    controller.revertEffect(0);

    QCOMPARE(controller.parameterValue(0, controller.reverbSizeKey()), 0.7f);
}

void EffectRackControllerTest::test_revertEffect_otherSlot_shouldKeepEdits()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
    const auto editorService = std::make_shared<EditorService>();
    EffectRackController controller { deviceService, editorService };

    controller.setIsInsertRack(true);
    controller.setEffect(0, QString::fromStdString(Reverb::typeIdString()));
    controller.setEffect(1, QString::fromStdString(Reverb::typeIdString()));

    controller.snapshotEffect(0);
    controller.setParameterValue(1, controller.reverbSizeKey(), 0.33f);

    // The snapshot belongs to slot 0, so slot 1 must be left alone
    controller.revertEffect(1);

    QCOMPARE(controller.parameterValue(1, controller.reverbSizeKey()), 0.33f);
}

void EffectRackControllerTest::test_populatedEffects_shouldReturnOnlyFilledSlots()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
    const auto editorService = std::make_shared<EditorService>();
    EffectRackController controller { deviceService, editorService };

    controller.setIsInsertRack(true);
    controller.setEffect(0, QString::fromStdString(Reverb::typeIdString()));
    controller.setEffect(2, QString::fromStdString(Compressor::typeIdString()));

    const auto populated = controller.populatedEffects();
    QCOMPARE(populated.size(), 2);
    QCOMPARE(populated.at(0).toMap()["slotIndex"].toInt(), 0);
    QCOMPARE(populated.at(1).toMap()["slotIndex"].toInt(), 2);
}

void EffectRackControllerTest::test_availableEffects_shouldBeSortedByName()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
    const auto editorService = std::make_shared<EditorService>();
    EffectRackController controller { deviceService, editorService };

    // The gallery is one flat list in this order, and it had silently drifted out of alphabetical
    // as effects were added over time. Sorting is done in availableEffects(); this is what keeps it
    // true for whatever gets added next.
    const auto effects = controller.availableEffects();
    QVERIFY(effects.size() > 1);

    for (qsizetype i = 1; i < effects.size(); i++) {
        const auto previous = effects.at(i - 1).toMap()["name"].toString();
        const auto current = effects.at(i).toMap()["name"].toString();
        QVERIFY2(QString::compare(previous, current, Qt::CaseInsensitive) < 0,
                 qPrintable(QString { "\"%1\" is listed before \"%2\"" }.arg(previous, current)));
    }
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::EffectRackControllerTest)
