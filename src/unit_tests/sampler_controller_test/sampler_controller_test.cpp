// This file is part of Noteahead.
// Copyright (C) 2026 Jussi Lind <jussi.lind@iki.fi>
//
#include "sampler_controller_test.hpp"
#include "common/constants.hpp"
#include "domain/devices/sampler_device.hpp"
#include "view/controllers/sampler_controller.hpp"

#include <QSignalSpy>
#include <QTest>

namespace noteahead {

void SamplerControllerTest::test_sampleRateChange_shouldUpdateHzValues()
{
    const auto sampler = std::make_shared<SamplerDevice>("Test Sampler");
    SamplerController controller { sampler };

    // Initially sample rate should be default
    QCOMPARE(controller.sampleRate(), static_cast<uint32_t>(Constants::defaultSampleRate()));

    controller.setSelectedPad(0);
    controller.setSelectedPadCutoff(0.5);
    const auto initialHz = controller.cutoffToHz(controller.selectedPadCutoff());
    QVERIFY(initialHz > 0.0f);

    QSignalSpy cutoffSpy { &controller, &SamplerController::selectedPadCutoffChanged };
    QSignalSpy srSpy { &controller, &SamplerController::sampleRateChanged };

    // Change sample rate to something lower so maxFreq is affected (min(20000, sr*0.49))
    sampler->setSampleRate(32000);

    QCOMPARE(srSpy.count(), 1);
    QCOMPARE(cutoffSpy.count(), 1);

    const auto newHz = controller.cutoffToHz(controller.selectedPadCutoff());
    const auto expectedHz = initialHz * ((32000.0f * 0.49f) / 20000.0f);
    QVERIFY2(std::abs(newHz - expectedHz) < 1.0f,
             QString("newHz: %1, initialHz: %2, expectedHz: %3").arg(newHz).arg(initialHz).arg(expectedHz).toUtf8().constData());
}

void SamplerControllerTest::test_properties_shouldUpdateDeviceAndEmitSignals()
{
    const auto sampler = std::make_shared<SamplerDevice>("Test Sampler");
    SamplerController controller { sampler };

    // Common properties (now scaled ints)
    {
        QSignalSpy spy { &controller, &SamplerController::volumeChanged };
        controller.setVolume(800);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(controller.volume(), 800);
        QCOMPARE(sampler->volume(), 0.8f);
    }

    // Controller specific
    {
        QSignalSpy spy { &controller, &SamplerController::selectedPadChanged };
        controller.setSelectedPad(2);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(controller.selectedPad(), 2);
    }
}

void SamplerControllerTest::test_reset_shouldRestoreDefaultValues()
{
    const auto sampler = std::make_shared<SamplerDevice>("Test Sampler");
    SamplerController controller { sampler };

    controller.setVolume(100);
    QSignalSpy spy { &controller, &SamplerController::volumeChanged };
    controller.reset();
    QVERIFY(spy.count() >= 1);
    QCOMPARE(controller.volume(), 1000);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::SamplerControllerTest)
