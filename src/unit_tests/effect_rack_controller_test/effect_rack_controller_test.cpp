#include "effect_rack_controller_test.hpp"
#include "../../application/service/device_service.hpp"
#include "../../application/service/editor_service.hpp"
#include "../../application/service/effect_rack_controller.hpp"
#include "../../common/constants.hpp"
#include "../../domain/devices/effect_rack.hpp"
#include "../../domain/dsp/compressor_effect.hpp"
#include "../../domain/dsp/reverb_effect.hpp"
#include "../../infra/audio/audio_engine.hpp"

#include <QTest>
#include <memory>

namespace noteahead {

void EffectRackControllerTest::test_effectParametersSummary_reverb()
{
    auto audioEngine = std::make_shared<AudioEngine>();
    auto deviceService = std::make_shared<DeviceService>(audioEngine);
    auto editorService = std::make_shared<EditorService>();
    EffectRackController controller { deviceService, editorService };

    controller.setIsInsertRack(true);
    controller.setEffect(0, QString::fromStdString(ReverbEffect::typeIdString()));

    // Default reverb: pre-delay 20ms, decay 1500ms
    QString summary = controller.effectParametersSummary(0);
    QCOMPARE(summary, QString { "(pre=20ms, decay=1500ms)" });

    // Change parameters
    controller.setParameterValue(0, controller.reverbPreDelayKey(), 50.0f / 500.0f); // 50ms
    controller.setParameterValue(0, controller.reverbDecayKey(), 3000.0f / 10000.0f); // 3000ms

    summary = controller.effectParametersSummary(0);
    QCOMPARE(summary, QString { "(pre=50ms, decay=3000ms)" });
}

void EffectRackControllerTest::test_effectParametersSummary_compressor()
{
    auto audioEngine = std::make_shared<AudioEngine>();
    auto deviceService = std::make_shared<DeviceService>(audioEngine);
    auto editorService = std::make_shared<EditorService>();
    EffectRackController controller { deviceService, editorService };

    controller.setIsInsertRack(true);
    controller.setEffect(0, QString::fromStdString(CompressorEffect::typeIdString()));

    // Default compressor: attack 0.2 internal -> ~0.5ms, ratio 0.15 internal -> 4:1
    QString summary = controller.effectParametersSummary(0);
    QCOMPARE(summary, QString { "(attack=0.5ms, ratio=4:1)" });

    // Change ratio to 10:1 (internal = (10-1)/19 = 0.473...)
    controller.setParameterValue(0, controller.compressorRatioKey(), 9.0f / 19.0f);
    summary = controller.effectParametersSummary(0);
    QCOMPARE(summary, QString { "(attack=0.5ms, ratio=10:1)" });
}

void EffectRackControllerTest::test_effectParametersSummary_emptySlot()
{
    auto audioEngine = std::make_shared<AudioEngine>();
    auto deviceService = std::make_shared<DeviceService>(audioEngine);
    auto editorService = std::make_shared<EditorService>();
    EffectRackController controller { deviceService, editorService };

    controller.setIsInsertRack(true);
    QCOMPARE(controller.effectParametersSummary(0), QString { "" });
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::EffectRackControllerTest)
