// This file is part of Noteahead.
// Copyright (C) 2026 Jussi Lind <jussi.lind@iki.fi>
//
#include "drum_synth_controller_test.hpp"
#include "../../application/service/device_service.hpp"
#include "../../common/constants.hpp"
#include "../../domain/devices/drum_synth_device.hpp"
#include "../../domain/devices/drum_synth_v2_device.hpp"
#include "../../infra/audio/audio_engine.hpp"
#include "../../infra/data_service.hpp"
#include "../../view/controllers/drum_synth_controller.hpp"
#include "../../view/controllers/drum_synth_v2_controller.hpp"

#include <QSignalSpy>
#include <QTest>

namespace noteahead {

void DrumSynthControllerTest::test_waveform_rapidChanges_shouldRenderOnce()
{
    // Rendering a drum voice costs tens of milliseconds, and a knob drag emits a change per pixel
    // of travel. Without the wait every one of those would pay for a render; with it a whole drag
    // collapses into the single redraw the user is actually waiting to see.
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
    DrumSynthV2Controller controller { deviceService };

    const auto device = std::make_shared<DrumSynthV2Device>(Constants::drumSynthV2DeviceName().toStdString());
    deviceService->setDevice(0, device);
    controller.setDevice(Constants::drumSynthV2DeviceName());
    controller.setWaveformRequest(64, true);

    QSignalSpy spy { &controller, &DrumSynthV2Controller::waveformChanged };
    QVERIFY(spy.wait(2000));
    spy.clear();

    // A drag's worth of changes, far faster than the wait.
    for (int step = 0; step < 20; step++) {
        controller.setVoiceDecay(step * 100);
    }
    QCOMPARE(spy.count(), 0);

    QVERIFY(spy.wait(2000));
    QCOMPARE(spy.count(), 1);
    QVERIFY(!controller.waveformData().isEmpty());
}

void DrumSynthControllerTest::test_waveform_whileHidden_shouldNotRender()
{
    // A knob turned with the dialog shut costs nothing at all.
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
    DrumSynthV2Controller controller { deviceService };

    const auto device = std::make_shared<DrumSynthV2Device>(Constants::drumSynthV2DeviceName().toStdString());
    deviceService->setDevice(0, device);
    controller.setDevice(Constants::drumSynthV2DeviceName());
    controller.setWaveformRequest(64, false);

    QSignalSpy spy { &controller, &DrumSynthV2Controller::waveformChanged };
    controller.setVoiceDecay(5000);
    QTest::qWait(600);
    QCOMPARE(spy.count(), 0);
    QVERIFY(controller.waveformData().isEmpty());
}

void DrumSynthControllerTest::test_sampleRateChange_shouldUpdateHzValues()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
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

void DrumSynthControllerTest::test_properties_shouldUpdateDeviceAndEmitSignals()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
    DrumSynthController controller { deviceService };

    const auto device = std::make_shared<DrumSynthDevice>(Constants::drumSynthDeviceName().toStdString());
    deviceService->setDevice(0, device);
    controller.setDevice(Constants::drumSynthDeviceName());

    // Common properties
    {
        QSignalSpy spy { &controller, &DrumSynthController::volumeChanged };
        controller.setVolume(800);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(controller.volume(), 800);
    }

    // Voice specific
    controller.setSelectedVoice(0); // Kick
    {
        QSignalSpy spy { &controller, &DrumSynthController::voiceLevelChanged };
        controller.setVoiceLevel(700);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(controller.voiceLevel(), 700);
    }
}

void DrumSynthControllerTest::test_reset_shouldRestoreDefaultValues()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
    DrumSynthController controller { deviceService };

    const auto device = std::make_shared<DrumSynthDevice>(Constants::drumSynthDeviceName().toStdString());
    deviceService->setDevice(0, device);
    controller.setDevice(Constants::drumSynthDeviceName());

    controller.setVolume(100);
    QSignalSpy spy { &controller, &DrumSynthController::volumeChanged };
    controller.reset();
    QVERIFY(spy.count() >= 1);
    QCOMPARE(controller.volume(), static_cast<int>(std::round(Constants::faderUnityPosition() * Constants::uiInternalScaling())));
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::DrumSynthControllerTest)
