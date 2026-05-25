// This file is part of Noteahead.
// Copyright (C) 2026 Jussi Lind <jussi.lind@iki.fi>
//
#include "drum_synth_controller_test.hpp"
#include "../../application/service/device_service.hpp"
#include "../../application/service/drum_synth_controller.hpp"
#include "../../common/constants.hpp"
#include "../../domain/devices/drum_synth_device.hpp"
#include "../../infra/audio/audio_engine.hpp"

#include <QSignalSpy>
#include <QTest>

namespace noteahead {

void DrumSynthControllerTest::test_sampleRateChange_shouldUpdateHzValues()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(audioEngine);
    DrumSynthController controller { deviceService };

    const auto device = std::make_shared<DrumSynthDevice>(Constants::drumSynthDeviceName().toStdString());
    deviceService->setDevice(0, device);

    controller.setDevice(Constants::drumSynthDeviceName());

    // Initially sample rate should be default
    QCOMPARE(controller.sampleRate(), static_cast<uint32_t>(Constants::defaultSampleRate()));

    controller.setSelectedVoice(0);
    controller.setVoiceLpfCutoff(500);
    const auto initialHz = controller.cutoffToHz(controller.voiceLpfCutoff());
    QVERIFY(initialHz > 0.0f);

    QSignalSpy propSpy { &controller, &DrumSynthController::voiceLpfCutoffChanged };
    QSignalSpy srSpy { &controller, &DrumSynthController::sampleRateChanged };

    // Change sample rate to something lower so maxFreq is affected (min(20000, sr*0.49))
    device->setSampleRate(32000);

    QCOMPARE(srSpy.count(), 1);
    QVERIFY(propSpy.count() >= 1);

    const auto newHz = controller.cutoffToHz(controller.voiceLpfCutoff());
    const auto expectedHz = initialHz * ((32000.0f * 0.49f) / 20000.0f);
    QVERIFY2(std::abs(newHz - expectedHz) < 1.0f,
             QString("newHz: %1, initialHz: %2, expectedHz: %3").arg(newHz).arg(initialHz).arg(expectedHz).toUtf8().constData());
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::DrumSynthControllerTest)
