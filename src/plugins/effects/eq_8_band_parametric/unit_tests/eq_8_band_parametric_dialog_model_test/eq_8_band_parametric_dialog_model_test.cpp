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

#include "eq_8_band_parametric_dialog_model_test.hpp"

#include "../../eq_8_band_parametric.hpp"
#include "../../eq_8_band_parametric_dialog_model.hpp"

#include "../../../../../application/service/device_service.hpp"
#include "../../../../../application/service/editor_service.hpp"
#include "../../../../../common/constants.hpp"
#include "../../../../../domain/dsp/svf_filter.hpp"
#include "../../../../../domain/effects/effect_factory.hpp"
#include "../../../../../infra/audio/audio_engine.hpp"
#include "../../../../../infra/data_service.hpp"
#include "../../../../../view/controllers/effect_rack_controller.hpp"

#include <QTest>

#include <algorithm>
#include <cmath>
#include <ranges>

namespace noteahead {

void Eq8BandParametricDialogModelTest::initTestCase()
{
    EffectFactory::init();
}

namespace {

//! Puts @p typeId in slot 0 of @p controller's insert rack, which is what every curve here reads.
void setUpInsertRackWith(EffectRackController & controller, const std::string & typeId)
{
    controller.setIsInsertRack(true);
    controller.setEffect(0, QString::fromStdString(typeId));
}

} // namespace

void Eq8BandParametricDialogModelTest::test_eq8BandParametricResponse_shouldSpanTheAudibleRange()
{
    const auto engine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(engine, std::make_shared<DataService>());
    const auto editorService = std::make_shared<EditorService>();
    EffectRackController controller { deviceService, editorService };
    Eq8BandParametricDialogModel model;
    model.setRack(&controller);
    model.setEffectIndex(0);
    controller.setIsInsertRack(true);
    controller.setEffect(0, QString::fromStdString(Eq8BandParametric::typeIdString()));

    // A bell at the top of the range, to say which end of the list is which.
    controller.setParameterValue(0, Constants::NahdXml::xmlKeyBandType(0), static_cast<float>(SvfFilter::Type::Bell));
    controller.setParameterValue(0, Constants::NahdXml::xmlKeyBandFreq(0), 1.0f);
    controller.setParameterValue(0, Constants::NahdXml::xmlKeyBandGain(0), 1.0f); // +24 dB

    const auto response = model.response(64);
    QCOMPARE(response.size(), 64);
    QVERIFY2(response.last().toDouble() > response.first().toDouble() + 6.0,
             qPrintable(QString("%1 dB at the bottom, %2 at the top").arg(response.first().toDouble()).arg(response.last().toDouble())));
}

void Eq8BandParametricDialogModelTest::test_eq8BandParametricResponse_flatBands_shouldBeFlat()
{
    const auto engine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(engine, std::make_shared<DataService>());
    const auto editorService = std::make_shared<EditorService>();
    EffectRackController controller { deviceService, editorService };
    Eq8BandParametricDialogModel model;
    model.setRack(&controller);
    model.setEffectIndex(0);
    controller.setIsInsertRack(true);
    controller.setEffect(0, QString::fromStdString(Eq8BandParametric::typeIdString()));

    // Every band starts bypassed, so an equalizer nobody has touched draws a straight line.
    const auto response = model.response(32);
    QCOMPARE(response.size(), 32);
    for (auto && point : response) {
        QVERIFY2(std::abs(point.toDouble()) < 0.01, qPrintable(QString::number(point.toDouble())));
    }
}

void Eq8BandParametricDialogModelTest::test_eq8BandParametricResponse_wrongEffect_shouldBeEmpty()
{
    // The dialog asks by index, and an index is a thing that can be stale. Anything but an
    // equalizer there has no response to give rather than a wrong one.
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
    const auto editorService = std::make_shared<EditorService>();
    EffectRackController controller { deviceService, editorService };
    Eq8BandParametricDialogModel model;
    model.setRack(&controller);
    model.setEffectIndex(0);
    controller.setIsInsertRack(true);

    QVERIFY(model.response(32).isEmpty());
}

void Eq8BandParametricDialogModelTest::test_eq8BandParametricSetBandType_sameType_shouldKeepTheQ()
{
    // A combo box reports the user picking the item that was already current, so opening the type
    // list on a band and closing it again used to overwrite a Q that had been set by hand. Projects
    // lost their band widths just by being looked at.
    const auto engine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(engine, std::make_shared<DataService>());
    const auto editorService = std::make_shared<EditorService>();
    EffectRackController controller { deviceService, editorService };
    Eq8BandParametricDialogModel model;
    model.setRack(&controller);
    model.setEffectIndex(0);
    setUpInsertRackWith(controller, Eq8BandParametric::typeIdString());

    const auto qKey = Constants::NahdXml::xmlKeyBandQ(0);
    model.setBandType(0, static_cast<int>(SvfFilter::Type::Bell));
    controller.setParameterValue(0, qKey, 0.5051f); // a width of the user's own choosing
    const auto authored = controller.parameterValue(0, qKey);

    model.setBandType(0, static_cast<int>(SvfFilter::Type::Bell));

    QCOMPARE(controller.parameterValue(0, qKey), authored);
}

void Eq8BandParametricDialogModelTest::test_eq8BandParametricSetBandType_newType_shouldOpenAtThatTypesQ()
{
    const auto engine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(engine, std::make_shared<DataService>());
    const auto editorService = std::make_shared<EditorService>();
    EffectRackController controller { deviceService, editorService };
    Eq8BandParametricDialogModel model;
    model.setRack(&controller);
    model.setEffectIndex(0);
    setUpInsertRackWith(controller, Eq8BandParametric::typeIdString());

    const auto qKey = Constants::NahdXml::xmlKeyBandQ(0);
    const auto typeKey = Constants::NahdXml::xmlKeyBandType(0);
    model.setBandType(0, static_cast<int>(SvfFilter::Type::Bell));
    controller.setParameterValue(0, qKey, 0.5051f);

    // Changing the type is the move the default is for: Q means a different thing to a shelf.
    model.setBandType(0, static_cast<int>(SvfFilter::Type::LowShelf));

    QCOMPARE(static_cast<int>(std::round(controller.parameterValue(0, typeKey))), static_cast<int>(SvfFilter::Type::LowShelf));
    const auto expected = Eq8BandParametric::defaultQParameterValue(SvfFilter::Type::LowShelf);
    QVERIFY(expected.has_value());
    QCOMPARE(controller.parameterValue(0, qKey), *expected);
}

void Eq8BandParametricDialogModelTest::test_eq8BandParametricSetBandType_typeWithNoOpinion_shouldKeepTheQ()
{
    const auto engine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(engine, std::make_shared<DataService>());
    const auto editorService = std::make_shared<EditorService>();
    EffectRackController controller { deviceService, editorService };
    Eq8BandParametricDialogModel model;
    model.setRack(&controller);
    model.setEffectIndex(0);
    setUpInsertRackWith(controller, Eq8BandParametric::typeIdString());

    const auto qKey = Constants::NahdXml::xmlKeyBandQ(0);
    model.setBandType(0, static_cast<int>(SvfFilter::Type::Bell));
    controller.setParameterValue(0, qKey, 0.5051f);
    const auto authored = controller.parameterValue(0, qKey);

    // Bypassing a band shapes nothing, so there is no width to have an opinion about -- and the one
    // the band had is what it should still have when it is switched back on.
    model.setBandType(0, static_cast<int>(SvfFilter::Type::Bypass));

    QCOMPARE(controller.parameterValue(0, qKey), authored);
}

void Eq8BandParametricDialogModelTest::test_eq8BandParametricPassThroughResponse_bothPathsShaped_shouldBeEmpty()
{
    // In Mid + Side mode there is no second curve to draw: it would lie exactly on the first and say
    // only that there is a second curve.
    const auto engine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(engine, std::make_shared<DataService>());
    const auto editorService = std::make_shared<EditorService>();
    EffectRackController controller { deviceService, editorService };
    Eq8BandParametricDialogModel model;
    model.setRack(&controller);
    model.setEffectIndex(0);
    setUpInsertRackWith(controller, Eq8BandParametric::typeIdString());
    controller.setParameterValue(0, Constants::NahdXml::xmlKeyStereoMode(), static_cast<float>(Eq8BandParametric::StereoMode::MidSide));

    QVERIFY(model.passThroughResponse(32).isEmpty());
}

void Eq8BandParametricDialogModelTest::test_eq8BandParametricPassThroughResponse_onePathBypassed_shouldBeFlat()
{
    const auto engine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(engine, std::make_shared<DataService>());
    const auto editorService = std::make_shared<EditorService>();
    EffectRackController controller { deviceService, editorService };
    Eq8BandParametricDialogModel model;
    model.setRack(&controller);
    model.setEffectIndex(0);
    setUpInsertRackWith(controller, Eq8BandParametric::typeIdString());
    controller.setParameterValue(0, Constants::NahdXml::xmlKeyBandType(0), static_cast<float>(SvfFilter::Type::Bell));
    controller.setParameterValue(0, Constants::NahdXml::xmlKeyBandGain(0), 0.0f); // -24 dB
    controller.setParameterValue(0, Constants::NahdXml::xmlKeyStereoMode(), static_cast<float>(Eq8BandParametric::StereoMode::Side));

    // The mid path takes none of that cut, and the curve for it is the flat line that says so.
    const auto passThrough = model.passThroughResponse(32);
    QCOMPARE(passThrough.size(), 32);
    for (auto && point : passThrough) {
        QVERIFY2(std::abs(point.toDouble()) < 0.01, qPrintable(QString::number(point.toDouble())));
    }

    // While the curve drawn boldest is the side path, which does take it.
    const auto shaped = model.response(32);
    const auto deepest = std::ranges::min(shaped | std::views::transform([](auto && p) { return p.toDouble(); }));
    QVERIFY2(deepest < -12.0, qPrintable(QString("deepest point of the shaped path was %1 dB").arg(deepest)));
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::Eq8BandParametricDialogModelTest)
