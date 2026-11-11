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

#include "bass_synth_dialog_model_test.hpp"

#include "../../../../../common/constants.hpp"
#include "../../../../../infra/xml/nahd_xml_reader.hpp"
#include "../../../../../infra/xml/nahd_xml_writer.hpp"
#include "../../bass_synth_device.hpp"
#include "../../bass_synth_dialog_model.hpp"

#include <QSignalSpy>
#include <QTest>

namespace noteahead {

void BassSynthDialogModelTest::test_sampleRateChange_shouldUpdateHzValues()
{
    auto device = std::make_shared<BassSynthDevice>("Test BassSynth");
    BassSynthDialogModel controller { device };

    // Initially sample rate should be default
    QCOMPARE(controller.sampleRate(), static_cast<uint32_t>(Constants::defaultSampleRate()));

    device->setLpfCutoff(0.5f);
    const float initialHz = controller.cutoffToHz(controller.lpfCutoff());
    QVERIFY(initialHz > 0.0f);

    QSignalSpy lpfSpy { &controller, &BassSynthDialogModel::lpfCutoffChanged };
    QSignalSpy hpfSpy { &controller, &BassSynthDialogModel::hpfCutoffChanged };
    QSignalSpy srSpy { &controller, &BassSynthDialogModel::sampleRateChanged };

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

void BassSynthDialogModelTest::test_deserialization_shouldUpdateHzValues()
{
    auto device = std::make_shared<BassSynthDevice>("Test BassSynth");
    BassSynthDialogModel controller { device };

    device->setLpfCutoff(0.1f);

    QString xml;
    NahdXmlWriter writer { xml };
    device->serializeToXml(writer);

    device->setLpfCutoff(0.9f);

    QSignalSpy lpfSpy { &controller, &BassSynthDialogModel::lpfCutoffChanged };

    NahdXmlReader reader { xml };
    if (reader.readNextStartElement()) {
        device->deserializeFromXml(reader);
    }

    QCOMPARE(device->lpfCutoff(), 0.1f);
    QVERIFY(lpfSpy.count() >= 1);
}

void BassSynthDialogModelTest::test_properties_shouldUpdateDeviceAndEmitSignals()
{
    auto device = std::make_shared<BassSynthDevice>("Test BassSynth");
    BassSynthDialogModel controller { device };

    // Common properties (DeviceDialogModel)
    {
        QSignalSpy spy { &controller, &BassSynthDialogModel::volumeChanged };
        controller.setVolume(800);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(controller.volume(), 800);
        QCOMPARE(device->volume(), 0.8f);
    }
    {
        QSignalSpy spy { &controller, &BassSynthDialogModel::gainChanged };
        controller.setGain(600);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(controller.gain(), 600);
        QCOMPARE(device->gain(), 0.6f);
    }
    {
        QSignalSpy spy { &controller, &BassSynthDialogModel::panChanged };
        controller.setPan(300);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(controller.pan(), 300);
        QCOMPARE(device->pan(), 0.3f);
    }

    // Specific properties
    {
        QSignalSpy spy { &controller, &BassSynthDialogModel::waveformChanged };
        controller.setWaveform(1);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(controller.waveform(), 1);
    }
    {
        QSignalSpy spy { &controller, &BassSynthDialogModel::tuningChanged };
        controller.setTuning(700);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(controller.tuning(), 700);
    }
    {
        QSignalSpy spy { &controller, &BassSynthDialogModel::subLevelChanged };
        controller.setSubLevel(400);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(controller.subLevel(), 400);
    }
    {
        QSignalSpy spy { &controller, &BassSynthDialogModel::subOctaveChanged };
        controller.setSubOctave(2);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(controller.subOctave(), 2);
    }
    {
        QSignalSpy spy { &controller, &BassSynthDialogModel::lpfCutoffChanged };
        controller.setLpfCutoff(200);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(controller.lpfCutoff(), 200);
    }
    {
        QSignalSpy spy { &controller, &BassSynthDialogModel::lpfResonanceChanged };
        controller.setLpfResonance(500);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(controller.lpfResonance(), 500);
    }
    {
        QSignalSpy spy { &controller, &BassSynthDialogModel::hpfCutoffChanged };
        controller.setHpfCutoff(100);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(controller.hpfCutoff(), 100);
    }
    {
        QSignalSpy spy { &controller, &BassSynthDialogModel::envModChanged };
        controller.setEnvMod(800);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(controller.envMod(), 800);
    }
    {
        QSignalSpy spy { &controller, &BassSynthDialogModel::decayChanged };
        controller.setDecay(600);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(controller.decay(), 600);
    }
    {
        QSignalSpy spy { &controller, &BassSynthDialogModel::accentChanged };
        controller.setAccent(900);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(controller.accent(), 900);
    }
    {
        QSignalSpy spy { &controller, &BassSynthDialogModel::slideChanged };
        controller.setSlide(500);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(controller.slide(), 500);
    }
    {
        QSignalSpy spy { &controller, &BassSynthDialogModel::pitchBendRangeChanged };
        controller.setPitchBendRange(12);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(controller.pitchBendRange(), 12);
        QCOMPARE(device->pitchBendRange(), 12);
    }
    {
        QSignalSpy spy { &controller, &BassSynthDialogModel::distDriveChanged };
        controller.setDistDrive(750);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(controller.distDrive(), 750);
    }
    {
        QSignalSpy spy { &controller, &BassSynthDialogModel::distToneChanged };
        controller.setDistTone(450);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(controller.distTone(), 450);
    }
    {
        QSignalSpy spy { &controller, &BassSynthDialogModel::distLevelChanged };
        controller.setDistLevel(350);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(controller.distLevel(), 350);
    }
}

void BassSynthDialogModelTest::test_reset_shouldRestoreDefaultValues()
{
    auto device = std::make_shared<BassSynthDevice>("Test BassSynth");
    BassSynthDialogModel controller { device };

    controller.setVolume(100);
    controller.setLpfCutoff(100);

    QSignalSpy spy { &controller, &BassSynthDialogModel::volumeChanged };
    controller.reset();

    QVERIFY(spy.count() >= 1);
    QCOMPARE(controller.volume(), static_cast<int>(std::round(Constants::faderUnityPosition() * Constants::uiInternalScaling())));
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::BassSynthDialogModelTest)
