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

#include "ju_synth_08_controller_test.hpp"
#include "../../domain/devices/ju_synth_08_device.hpp"
#include "../../view/controllers/ju_synth_08_controller.hpp"

#include <QSignalSpy>
#include <QTest>

using namespace noteahead;

void JuSynth08ControllerTest::test_deviceBindingAndSettings()
{
    auto device = std::make_shared<JuSynth08Device>("JU Synth 08");
    JuSynth08Controller controller { device };

    QCOMPARE(controller.device(), device);
    QCOMPARE(controller.property("displayName").toString(), QString("JU Synth 08"));
    QVERIFY(!controller.factoryPresetNames().isEmpty());
}

void JuSynth08ControllerTest::test_presetLoading()
{
    auto device = std::make_shared<JuSynth08Device>("JU Synth 08");
    JuSynth08Controller controller { device };

    QSignalSpy spy { &controller, &JuSynth08Controller::crossModChanged };

    const auto presetNames = controller.factoryPresetNames();
    QVERIFY(presetNames.size() > 1);

    controller.loadPreset(0);
    controller.loadPreset(1);

    QVERIFY(spy.count() >= 1);
}

void JuSynth08ControllerTest::test_propertiesRoundtrip()
{
    auto device = std::make_shared<JuSynth08Device>("JU Synth 08");
    JuSynth08Controller controller { device };

    controller.setVcfSlope(0);
    QCOMPARE(controller.vcfSlope(), 0);

    controller.setCrossMod(750);
    QCOMPARE(controller.crossMod(), 750);

    controller.setUnisonDetune(350);
    QCOMPARE(controller.unisonDetune(), 350);

    controller.setSync(true);
    QVERIFY(controller.sync());
}

QTEST_GUILESS_MAIN(JuSynth08ControllerTest)
