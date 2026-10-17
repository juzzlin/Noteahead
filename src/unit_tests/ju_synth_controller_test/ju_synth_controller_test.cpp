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

#include "ju_synth_controller_test.hpp"

#include "../../common/constants.hpp"
#include "../../domain/devices/bass_synth_device.hpp"
#include "../../domain/devices/ju_synth_device.hpp"
#include "../../domain/devices/ju_synth_presets.hpp"
#include "../../view/controllers/ju_synth_controller.hpp"

#include <QMetaProperty>
#include <QSignalSpy>
#include <QTest>

#include <memory>
#include <vector>

namespace noteahead {

void JuSynthControllerTest::test_requestSettings_shouldNotifyEveryProperty()
{
    // Stated over the metaobject, so a property added without its line in requestSettings() fails
    // here instead of leaving a knob that never refreshes.
    JuSynthController controller { std::make_shared<JuSynthDevice>("JU") };
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
    QVERIFY(spies.size() > 20);

    controller.requestSettings();

    for (size_t i = 0; i < spies.size(); i++) {
        QVERIFY2(spies.at(i)->count() > 0, qPrintable(names.at(i) + " is never notified"));
    }
}

void JuSynthControllerTest::test_properties_shouldReachTheDevice()
{
    const auto synth = std::make_shared<JuSynthDevice>("JU");
    JuSynthController controller { synth };
    const auto scaling = Constants::uiInternalScaling();

    controller.setLpfCutoff(static_cast<int>(0.4 * scaling));
    controller.setPulseWidth(static_cast<int>(0.7 * scaling));
    controller.setRange(0);
    controller.setPwmSource(static_cast<int>(JuSynthDevice::PwmSource::Envelope));
    controller.setChorusMode(static_cast<int>(JunoChorus::Mode::Both));
    controller.setPulseEnabled(true);
    controller.setEnvInverted(true);

    QVERIFY(std::abs(synth->lpfCutoff() - 0.4f) < 0.001f);
    QVERIFY(std::abs(synth->pulseWidth() - 0.7f) < 0.001f);
    QCOMPARE(synth->range(), 0);
    QCOMPARE(synth->pwmSource(), JuSynthDevice::PwmSource::Envelope);
    QCOMPARE(synth->chorusMode(), JunoChorus::Mode::Both);
    QVERIFY(synth->pulseEnabled());
    QVERIFY(synth->envInverted());
    QCOMPARE(controller.chorusMode(), static_cast<int>(JunoChorus::Mode::Both));
}

void JuSynthControllerTest::test_factoryPresetNames_shouldListThePresets()
{
    JuSynthController controller { std::make_shared<JuSynthDevice>("JU") };
    const auto names = controller.factoryPresetNames();
    QCOMPARE(names.size(), static_cast<qsizetype>(JuSynthPresets::presets().size()));
    QCOMPARE(names.front(), QString { "Init" });
}

void JuSynthControllerTest::test_setDevice_otherType_shouldRefuse()
{
    JuSynthController controller { std::make_shared<JuSynthDevice>("JU") };
    QVERIFY(!controller.setDevice(std::static_pointer_cast<Device>(std::make_shared<BassSynthDevice>("Bass"))));
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::JuSynthControllerTest)
