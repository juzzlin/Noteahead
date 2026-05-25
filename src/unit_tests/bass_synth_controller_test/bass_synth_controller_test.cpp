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
#include "../../application/service/bass_synth_controller.hpp"
#include "../../common/constants.hpp"
#include "../../domain/devices/bass_synth_device.hpp"

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
    // Device::serializeToXml already writes its own start element
    device->serializeToXml(writer);

    // Change value in device so we can see it restored
    device->setLpfCutoff(0.9f);

    QSignalSpy lpfSpy { &controller, &BassSynthController::lpfCutoffChanged };

    QXmlStreamReader reader { xml };
    if (reader.readNextStartElement()) {
        device->deserializeFromXml(reader);
    }

    QCOMPARE(device->lpfCutoff(), 0.1f);
    // dataChanged signal from Device should have triggered requestSettings in controller
    QVERIFY(lpfSpy.count() >= 1);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::BassSynthControllerTest)
