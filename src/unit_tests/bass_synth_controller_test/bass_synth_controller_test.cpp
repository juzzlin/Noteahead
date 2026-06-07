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

#include "bass_synth_controller_test.hpp"

#include "common/constants.hpp"
#include "domain/devices/bass_synth_device.hpp"
#include "view/controllers/bass_synth_controller.hpp"

#include <QSignalSpy>
#include <QTest>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

namespace noteahead {

void BassSynthControllerTest::test_sampleRateChange_shouldUpdateHzValues()
{
    auto device = std::make_shared<BassSynthDevice>("Test BassSynth");
    BassSynthController controller { device };

    // Initially sample rate should be default
    QCOMPARE(controller.sampleRate(), static_cast<uint32_t>(Constants::defaultSampleRate()));

    device->setLpfCutoff(0.5f);
    const float initialHz = controller.cutoffToHz(controller.lpfCutoff());
    QVERIFY(initialHz > 0.0f);

    QSignalSpy lpfSpy { &controller, &BassSynthController::lpfCutoffChanged };
    QSignalSpy hpfSpy { &controller, &BassSynthController::hpfCutoffChanged };
    QSignalSpy srSpy { &controller, &BassSynthController::sampleRateChanged };

    // Change sample rate to something lower so maxFreq is affected (min(20000, sr*0.49))
    device->setSampleRate(32000);

    QCOMPARE(srSpy.count(), 1);
    QCOMPARE(lpfSpy.count(), 1);
    QCOMPARE(hpfSpy.count(), 1);

    const float newHz = controller.cutoffToHz(controller.lpfCutoff());
    const float expectedHz = initialHz * ((32000.0f * 0.49f) / 20000.0f);
    QVERIFY2(std::abs(newHz - expectedHz) < 1.0f,
             QString("newHz: %1, initialHz: %2, expectedHz: %3").arg(newHz).arg(initialHz).arg(expectedHz).toUtf8().constData());
}

void BassSynthControllerTest::test_deserialization_shouldUpdateHzValues()
{
    auto device = std::make_shared<BassSynthDevice>("Test BassSynth");
    BassSynthController controller { device };

    device->setLpfCutoff(0.1f);

    QString xml;
    QXmlStreamWriter writer { &xml };
    device->serializeToXml(writer);

    device->setLpfCutoff(0.9f);

    QSignalSpy lpfSpy { &controller, &BassSynthController::lpfCutoffChanged };

    QXmlStreamReader reader { xml };
    if (reader.readNextStartElement()) {
        device->deserializeFromXml(reader);
    }

    QCOMPARE(device->lpfCutoff(), 0.1f);
    QVERIFY(lpfSpy.count() >= 1);
}

void BassSynthControllerTest::test_properties_shouldUpdateDeviceAndEmitSignals()
{
    auto device = std::make_shared<BassSynthDevice>("Test BassSynth");
    BassSynthController controller { device };

    // Common properties (DeviceController)
    {
        QSignalSpy spy { &controller, &BassSynthController::volumeChanged };
        controller.setVolume(800);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(controller.volume(), 800);
        QCOMPARE(device->volume(), 0.8f);
    }
    {
        QSignalSpy spy { &controller, &BassSynthController::gainChanged };
        controller.setGain(600);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(controller.gain(), 600);
        QCOMPARE(device->gain(), 0.6f);
    }
    {
        QSignalSpy spy { &controller, &BassSynthController::panChanged };
        controller.setPan(300);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(controller.pan(), 300);
        QCOMPARE(device->pan(), 0.3f);
    }

    // Specific properties
    {
        QSignalSpy spy { &controller, &BassSynthController::waveformChanged };
        controller.setWaveform(1);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(controller.waveform(), 1);
    }
    {
        QSignalSpy spy { &controller, &BassSynthController::tuningChanged };
        controller.setTuning(700);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(controller.tuning(), 700);
    }
    {
        QSignalSpy spy { &controller, &BassSynthController::subLevelChanged };
        controller.setSubLevel(400);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(controller.subLevel(), 400);
    }
    {
        QSignalSpy spy { &controller, &BassSynthController::subOctaveChanged };
        controller.setSubOctave(2);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(controller.subOctave(), 2);
    }
    {
        QSignalSpy spy { &controller, &BassSynthController::lpfCutoffChanged };
        controller.setLpfCutoff(200);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(controller.lpfCutoff(), 200);
    }
    {
        QSignalSpy spy { &controller, &BassSynthController::lpfResonanceChanged };
        controller.setLpfResonance(500);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(controller.lpfResonance(), 500);
    }
    {
        QSignalSpy spy { &controller, &BassSynthController::hpfCutoffChanged };
        controller.setHpfCutoff(100);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(controller.hpfCutoff(), 100);
    }
    {
        QSignalSpy spy { &controller, &BassSynthController::envModChanged };
        controller.setEnvMod(800);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(controller.envMod(), 800);
    }
    {
        QSignalSpy spy { &controller, &BassSynthController::decayChanged };
        controller.setDecay(600);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(controller.decay(), 600);
    }
    {
        QSignalSpy spy { &controller, &BassSynthController::accentChanged };
        controller.setAccent(900);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(controller.accent(), 900);
    }
    {
        QSignalSpy spy { &controller, &BassSynthController::slideChanged };
        controller.setSlide(500);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(controller.slide(), 500);
    }
    {
        QSignalSpy spy { &controller, &BassSynthController::distDriveChanged };
        controller.setDistDrive(750);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(controller.distDrive(), 750);
    }
    {
        QSignalSpy spy { &controller, &BassSynthController::distToneChanged };
        controller.setDistTone(450);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(controller.distTone(), 450);
    }
    {
        QSignalSpy spy { &controller, &BassSynthController::distLevelChanged };
        controller.setDistLevel(350);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(controller.distLevel(), 350);
    }
}

void BassSynthControllerTest::test_reset_shouldRestoreDefaultValues()
{
    auto device = std::make_shared<BassSynthDevice>("Test BassSynth");
    BassSynthController controller { device };

    controller.setVolume(100);
    controller.setLpfCutoff(100);

    QSignalSpy spy { &controller, &BassSynthController::volumeChanged };
    controller.reset();

    QVERIFY(spy.count() >= 1);
    QCOMPARE(controller.volume(), 1000);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::BassSynthControllerTest)
