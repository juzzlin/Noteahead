#include "effect_rack_controller_test.hpp"
#include "application/service/device_service.hpp"
#include "application/service/editor_service.hpp"
#include "common/constants.hpp"
#include "domain/dsp/clipper_effect.hpp"
#include "domain/dsp/compressor_effect.hpp"
#include "domain/dsp/eq_8_band_parametric_effect.hpp"
#include "domain/dsp/reverb_effect.hpp"
#include "domain/effects/auto_panner_effect.hpp"
#include "domain/effects/effect_factory.hpp"
#include "domain/effects/effect_rack.hpp"
#include "domain/effects/panner_effect.hpp"
#include "infra/audio/audio_engine.hpp"
#include "infra/data_service.hpp"
#include "infra/xml/nahd_xml_reader.hpp"
#include "infra/xml/nahd_xml_writer.hpp"
#include "view/controllers/effect_rack_controller.hpp"

#include <QBuffer>
#include <QSignalSpy>
#include <QTest>
#include <memory>

namespace noteahead {

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
    controller.setEffect(0, QString::fromStdString(ReverbEffect::typeIdString()));

    // Default reverb: pre-delay 20ms, decay 1500ms
    const auto summary = controller.effectParametersSummary(0);
    QCOMPARE(summary, QString { "(pre=20ms, decay=1500ms)" });

    // Change parameters
    controller.setParameterValue(0, controller.reverbPreDelayKey(), 50.0f / 500.0f); // 50ms
    controller.setParameterValue(0, controller.reverbDecayKey(), 3000.0f / 10000.0f); // 3000ms

    const auto summary2 = controller.effectParametersSummary(0);
    QCOMPARE(summary2, QString { "(pre=50ms, decay=3000ms)" });
}

void EffectRackControllerTest::test_effectParametersSummary_compressor_shouldReturnFormattedSummary()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
    const auto editorService = std::make_shared<EditorService>();
    EffectRackController controller { deviceService, editorService };

    controller.setIsInsertRack(true);
    controller.setEffect(0, QString::fromStdString(CompressorEffect::typeIdString()));

    // Default compressor: attack 0.2 internal -> ~0.5ms, ratio 0.15 internal -> 4:1
    const auto summary = controller.effectParametersSummary(0);
    QCOMPARE(summary, QString { "(attack=0.5ms, ratio=4:1)" });

    // Change ratio to 10:1 (internal = (10-1)/19 = 0.473...)
    controller.setParameterValue(0, controller.compressorRatioKey(), 9.0f / 19.0f);
    const auto summary2 = controller.effectParametersSummary(0);
    QCOMPARE(summary2, QString { "(attack=0.5ms, ratio=10:1)" });
}

void EffectRackControllerTest::test_effectParametersSummary_autoPanner_shouldReturnFormattedSummary()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
    const auto editorService = std::make_shared<EditorService>();
    EffectRackController controller { deviceService, editorService };

    controller.setIsInsertRack(true);
    controller.setEffect(0, QString::fromStdString(AutoPannerEffect::typeIdString()));

    // Default auto panner: rate 1.00Hz, intensity 100%
    const auto summary = controller.effectParametersSummary(0);
    QCOMPARE(summary, QString { "(rate=1.00Hz, int=100%)" });
}

void EffectRackControllerTest::test_effectParametersSummary_panner_shouldReturnFormattedSummary()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
    const auto editorService = std::make_shared<EditorService>();
    EffectRackController controller { deviceService, editorService };

    controller.setIsInsertRack(true);
    controller.setEffect(0, QString::fromStdString(PannerEffect::typeIdString()));

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
    controller.setEffect(0, QString::fromStdString(ClipperEffect::typeIdString()));

    // Default clipper: threshold 0.0dB
    const auto summary = controller.effectParametersSummary(0);
    QCOMPARE(summary, QString { "(thr=0.0dB)" });
}

void EffectRackControllerTest::test_effectParametersSummary_eq8BandParametric_shouldReturnFormattedSummary()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
    const auto editorService = std::make_shared<EditorService>();
    EffectRackController controller { deviceService, editorService };

    controller.setIsInsertRack(true);
    controller.setEffect(0, QString::fromStdString(Eq8BandParametricEffect::typeIdString()));

    const auto summary = controller.effectParametersSummary(0);
    QCOMPARE(summary, QString { "(Parametric)" });
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

void EffectRackControllerTest::test_isEffectEnabled_shouldReturnEnabledState()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
    const auto editorService = std::make_shared<EditorService>();
    EffectRackController controller { deviceService, editorService };

    controller.setIsInsertRack(true);
    controller.setEffect(0, QString::fromStdString(ReverbEffect::typeIdString()));

    QVERIFY(controller.isEffectEnabled(0));

    controller.setIsEffectEnabled(0, false);
    QVERIFY(!controller.isEffectEnabled(0));
    QVERIFY(editorService->isModified());

    controller.setIsEffectEnabled(0, true);
    QVERIFY(controller.isEffectEnabled(0));
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
    controller.setEffect(0, QString::fromStdString(ReverbEffect::typeIdString()));

    QByteArray data;
    QBuffer buffer { &data };
    buffer.open(QIODevice::WriteOnly);
    NahdXmlWriter writer { buffer };
    QVERIFY(controller.exportSettings(writer));
    buffer.close();

    QVERIFY(!data.isEmpty());
    QVERIFY(data.contains(ReverbEffect::typeIdString().c_str()));
}

void EffectRackControllerTest::test_importSettings_shouldRestoreEffects()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
    const auto editorService = std::make_shared<EditorService>();
    EffectRackController controller { deviceService, editorService };

    controller.setIsInsertRack(true);
    controller.setEffect(0, QString::fromStdString(ReverbEffect::typeIdString()));
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
    controller.setEffect(0, QString::fromStdString(ReverbEffect::typeIdString()));

    // Create a temporary reverb settings file
    const QString filePath = "test_matching_effect.nahdeff";
    const auto reverb = std::make_shared<ReverbEffect>();
    QFile file { filePath };
    file.open(QIODevice::WriteOnly);
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
    controller.setEffect(0, QString::fromStdString(ReverbEffect::typeIdString()));

    // Create a temporary compressor settings file
    const QString filePath = "test_different_effect.nahdeff";
    const auto compressor = std::make_shared<CompressorEffect>();
    QFile file { filePath };
    file.open(QIODevice::WriteOnly);
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
    controller.setEffect(0, QString::fromStdString(ReverbEffect::typeIdString()));

    const QString filePath = "test_confirm_effect.nahdeff";
    const auto reverb = std::make_shared<ReverbEffect>();
    QFile file { filePath };
    file.open(QIODevice::WriteOnly);
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

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::EffectRackControllerTest)
