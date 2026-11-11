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

#include "air_band_eq_dialog_model_test.hpp"

#include "../../../eq_8_band_parametric/eq_8_band_parametric.hpp"
#include "../../air_band_eq.hpp"
#include "../../air_band_eq_dialog_model.hpp"

#include "../../../../../application/service/device_service.hpp"
#include "../../../../../application/service/editor_service.hpp"
#include "../../../../../common/constants.hpp"
#include "../../../../../domain/effects/effect_factory.hpp"
#include "../../../../../infra/audio/audio_engine.hpp"
#include "../../../../../infra/data_service.hpp"
#include "../../../../../view/controllers/effect_rack_controller.hpp"

#include <QTest>

#include <algorithm>
#include <cmath>

namespace noteahead {

namespace {

//! Puts @p typeId in slot 0 of @p controller's insert rack, which is what every curve here reads.
void setUpInsertRackWith(EffectRackController & controller, const std::string & typeId)
{
    controller.setIsInsertRack(true);
    controller.setEffect(0, QString::fromStdString(typeId));
}

} // namespace

void AirBandEqDialogModelTest::initTestCase()
{
    EffectFactory::init();
}

void AirBandEqDialogModelTest::test_response_shouldFollowTheBandThatIsBoosted()
{
    const auto engine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(engine, std::make_shared<DataService>());
    const auto editorService = std::make_shared<EditorService>();
    EffectRackController controller { deviceService, editorService };
    AirBandEqDialogModel model;
    model.setRack(&controller);
    model.setEffectIndex(0);
    setUpInsertRackWith(controller, AirBandEq::typeIdString());

    // The 650 Hz band pass all the way up, which the curve has to show around 650 Hz and nowhere
    // near the ends of the range.
    controller.setParameterValue(0, Constants::NahdXml::xmlKeyBandGain(3), 1.0f);

    const auto response = model.response(64);
    QCOMPARE(response.size(), 64);

    // 20 Hz to 20 kHz over 64 points, evenly spaced by octave: 650 Hz lands a little past the middle.
    const auto at = [&response](double hz) {
        const int i = static_cast<int>(std::round(std::log(hz / 20.0) / std::log(1000.0) * 63.0));
        return response.at(std::clamp(i, 0, 63)).toDouble();
    };
    QVERIFY2(at(650.0) > 9.0, qPrintable(QString("%1 dB at 650 Hz").arg(at(650.0))));
    QVERIFY2(at(650.0) > at(20000.0) + 6.0, qPrintable(QString("%1 dB at 650 Hz, %2 at the top").arg(at(650.0)).arg(at(20000.0))));
    QVERIFY2(at(650.0) > at(20.0) + 6.0, qPrintable(QString("%1 dB at 650 Hz, %2 at the bottom").arg(at(650.0)).arg(at(20.0))));
}

void AirBandEqDialogModelTest::test_response_flatBands_shouldBeFlat()
{
    // Every band knob starts at its flat detent and the air band starts off, so an equalizer nobody
    // has touched draws a straight line.
    const auto engine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(engine, std::make_shared<DataService>());
    const auto editorService = std::make_shared<EditorService>();
    EffectRackController controller { deviceService, editorService };
    AirBandEqDialogModel model;
    model.setRack(&controller);
    model.setEffectIndex(0);
    setUpInsertRackWith(controller, AirBandEq::typeIdString());

    const auto response = model.response(32);
    QCOMPARE(response.size(), 32);
    for (auto && point : response) {
        QVERIFY2(std::abs(point.toDouble()) < 0.01, qPrintable(QString::number(point.toDouble())));
    }
}

void AirBandEqDialogModelTest::test_response_wrongEffect_shouldBeEmpty()
{
    // The dialog asks by index, and an index is a thing that can be stale.
    const auto engine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(engine, std::make_shared<DataService>());
    const auto editorService = std::make_shared<EditorService>();
    EffectRackController controller { deviceService, editorService };
    AirBandEqDialogModel model;
    model.setRack(&controller);
    model.setEffectIndex(0);
    setUpInsertRackWith(controller, Eq8BandParametric::typeIdString());

    QVERIFY(model.response(32).isEmpty());
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::AirBandEqDialogModelTest)
