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

#include "snare_synth_controller_test.hpp"

#include "../../common/constants.hpp"
#include "../../domain/devices/bass_synth_device.hpp"
#include "../../domain/devices/snare_synth_device.hpp"
#include "../../domain/devices/snare_synth_presets.hpp"
#include "../../view/controllers/snare_synth_controller.hpp"

#include <QMetaProperty>
#include <QSignalSpy>
#include <QTest>

#include <memory>
#include <vector>

namespace noteahead {

void SnareSynthControllerTest::test_requestSettings_shouldNotifyEveryProperty()
{
    SnareSynthController controller { std::make_shared<SnareSynthDevice>("Snare") };
    const auto * meta = controller.metaObject();
    std::vector<std::unique_ptr<QSignalSpy>> spies;
    std::vector<QString> names;
    for (int i = meta->propertyOffset(); i < meta->propertyCount(); i++) {
        const auto property = meta->property(i);
        if (!property.hasNotifySignal() || !property.isWritable()) {
            continue;
        }
        spies.push_back(std::make_unique<QSignalSpy>(&controller, property.notifySignal()));
        names.push_back(QString::fromLatin1(property.name()));
    }
    QVERIFY(spies.size() >= 18);

    controller.requestSettings();

    for (size_t i = 0; i < spies.size(); i++) {
        QVERIFY2(spies.at(i)->count() > 0, qPrintable(names.at(i) + " is never notified"));
    }
}

void SnareSynthControllerTest::test_properties_shouldReachTheDevice()
{
    const auto snare = std::make_shared<SnareSynthDevice>("Snare");
    SnareSynthController controller { snare };
    const auto scaling = Constants::uiInternalScaling();

    controller.setTune(static_cast<int>(0.65 * scaling));
    controller.setTone(static_cast<int>(0.45 * scaling));
    controller.setDecay(static_cast<int>(0.75 * scaling));
    controller.setSnappy(static_cast<int>(0.85 * scaling));
    controller.setPanSpread(static_cast<int>(0.90 * scaling));
    controller.setDrive(static_cast<int>(0.30 * scaling));
    controller.setChromatic(true);

    QVERIFY(std::abs(snare->tune() - 0.65f) < 0.01f);
    QVERIFY(std::abs(snare->tone() - 0.45f) < 0.01f);
    QVERIFY(std::abs(snare->decay() - 0.75f) < 0.01f);
    QVERIFY(std::abs(snare->snappy() - 0.85f) < 0.01f);
    QVERIFY(std::abs(snare->panSpread() - 0.90f) < 0.01f);
    QVERIFY(std::abs(snare->drive() - 0.30f) < 0.01f);
    QVERIFY(snare->chromatic());
}

void SnareSynthControllerTest::test_factoryPresetNames_shouldListThePresets()
{
    SnareSynthController controller { std::make_shared<SnareSynthDevice>("Snare") };
    const auto names = controller.factoryPresetNames();
    QCOMPARE(names.size(), static_cast<qsizetype>(SnareSynthPresets::presets().size()));
    QCOMPARE(names.front(), QString { "Init" });
}

void SnareSynthControllerTest::test_setDevice_otherType_shouldRefuse()
{
    SnareSynthController controller { std::make_shared<SnareSynthDevice>("Snare") };
    QVERIFY(!controller.setDevice(std::static_pointer_cast<Device>(std::make_shared<BassSynthDevice>("Bass"))));
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::SnareSynthControllerTest)
