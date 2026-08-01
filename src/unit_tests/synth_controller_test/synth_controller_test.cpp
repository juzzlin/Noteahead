// This file is part of Noteahead.
// Copyright (C) 2026 Jussi Lind <jussi.lind@iki.fi>
//
#include "synth_controller_test.hpp"
#include "../../common/constants.hpp"
#include "../../domain/devices/device.hpp"
#include "../../domain/devices/synth_device.hpp"
#include "../../view/controllers/synth_controller.hpp"

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

void SynthControllerTest::test_voiceModes()
{
    const auto synth = std::make_shared<SynthDevice>("Test Synth");
    SynthController controller { synth };
    const auto modes = controller.voiceModes();
    QCOMPARE(modes.size(), 5);
    // The order is the serialized VoiceMode ordinal, so it is append-only: reordering these would
    // change the voice mode of every project saved before the change.
    QCOMPARE(modes.at(0), QString("Poly"));
    QCOMPARE(modes.at(1), QString("Unison"));
    QCOMPARE(modes.at(2), QString("Dual"));
    QCOMPARE(modes.at(3), QString("Supersaw"));
    QCOMPARE(modes.at(4), QString("Drift"));
}

void SynthControllerTest::test_scopeActive_shouldFollowShownInstance()
{
    const auto synthA = std::make_shared<SynthDevice>("Synth A");
    const auto synthB = std::make_shared<SynthDevice>("Synth B");
    SynthController controller { synthA };

    // Nothing captures until the scope is shown.
    QVERIFY(!synthA->scope().active());

    // Showing the scope enables capture on the current instance only.
    controller.setScopeActive(true);
    QVERIFY(synthA->scope().active());
    QVERIFY(!synthB->scope().active());

    // Switching instance while the scope is shown must move capture to the new instance and stop
    // it on the old one, so a device that is no longer displayed never keeps capturing.
    controller.setDevice(synthB);
    QVERIFY(!synthA->scope().active());
    QVERIFY(synthB->scope().active());

    // Hiding the scope stops all capture.
    controller.setScopeActive(false);
    QVERIFY(!synthB->scope().active());
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::SynthControllerTest)
