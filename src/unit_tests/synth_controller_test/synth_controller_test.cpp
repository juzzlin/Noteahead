// This file is part of Noteahead.
// Copyright (C) 2026 Jussi Lind <jussi.lind@iki.fi>
//
#include "synth_controller_test.hpp"
#include "common/constants.hpp"
#include "domain/devices/synth_device.hpp"
#include "view/controllers/synth_controller.hpp"

#include <QSignalSpy>
#include <QTest>

namespace noteahead {

void SynthControllerTest::test_sampleRateChange_shouldUpdateHzValues()
{
    const auto synth = std::make_shared<SynthDevice>("Test Synth");
    SynthController controller { synth };

    // Initially sample rate should be default
    QCOMPARE(controller.sampleRate(), static_cast<uint32_t>(Constants::defaultSampleRate()));

    synth->setLpfCutoff(0.5f);
    const auto initialHz = controller.cutoffToHz(controller.lpfCutoff());
    QVERIFY(initialHz > 0.0f);

    QSignalSpy cutoffSpy { &controller, &SynthController::lpfCutoffChanged };
    QSignalSpy srSpy { &controller, &SynthController::sampleRateChanged };

    // Change sample rate to something lower so maxFreq is affected (min(20000, sr*0.49))
    synth->setSampleRate(32000);

    QCOMPARE(srSpy.count(), 1);
    QCOMPARE(cutoffSpy.count(), 1);

    const auto newHz = controller.cutoffToHz(controller.lpfCutoff());
    const auto expectedHz = initialHz * ((32000.0f * 0.49f) / 20000.0f);
    QVERIFY2(std::abs(newHz - expectedHz) < 1.0f,
             QString("newHz: %1, initialHz: %2, expectedHz: %3").arg(newHz).arg(initialHz).arg(expectedHz).toUtf8().constData());
}

void SynthControllerTest::test_properties_shouldUpdateDeviceAndEmitSignals()
{
    const auto synth = std::make_shared<SynthDevice>("Test Synth");
    SynthController controller { synth };

    // Common properties
    {
        QSignalSpy spy { &controller, &SynthController::volumeChanged };
        controller.setVolume(800);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(controller.volume(), 800);
    }

    // VCO1
    {
        QSignalSpy spy { &controller, &SynthController::vco1WaveformChanged };
        controller.setVco1Waveform(1);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(controller.vco1Waveform(), 1);
    }
    {
        QSignalSpy spy { &controller, &SynthController::vco1OctaveChanged };
        controller.setVco1Octave(1);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(controller.vco1Octave(), 1);
    }

    // Filter
    {
        QSignalSpy spy { &controller, &SynthController::lpfCutoffChanged };
        controller.setLpfCutoff(600);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(controller.lpfCutoff(), 600);
    }

    // LFO
    {
        QSignalSpy spy { &controller, &SynthController::lfoRateChanged };
        controller.setLfoRate(400);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(controller.lfoRate(), 400);
    }
}

void SynthControllerTest::test_reset_shouldRestoreDefaultValues()
{
    const auto synth = std::make_shared<SynthDevice>("Test Synth");
    SynthController controller { synth };

    controller.setVolume(100);
    QSignalSpy spy { &controller, &SynthController::volumeChanged };
    controller.reset();
    QVERIFY(spy.count() >= 1);
    QCOMPARE(controller.volume(), 1000);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::SynthControllerTest)
