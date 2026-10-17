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

#include "lfo_controller_test.hpp"

#include "../../common/constants.hpp"
#include "../../domain/devices/lfo_device.hpp"
#include "../../domain/devices/synth_device.hpp"
#include "../../view/controllers/lfo_controller.hpp"
#include "../../view/controllers/lfo_panel_controller.hpp"

#include <QSignalSpy>
#include <QTest>

namespace noteahead {

namespace {

namespace NahdXml = Constants::NahdXml;

LfoPanelController & panel(LfoController & controller, int index)
{
    return *qobject_cast<LfoPanelController *>(controller.lfos().at(index).value<QObject *>());
}

} // namespace

void LfoControllerTest::test_setDevice_otherDevice_shouldBeRefused()
{
    LfoController controller { nullptr };
    QVERIFY(!controller.setDevice(std::make_shared<SynthDevice>("Synth")));
    QVERIFY(controller.setDevice(std::make_shared<LfoDevice>("LFO")));
    QCOMPARE(controller.lfos().size(), static_cast<qsizetype>(LfoDevice::LfoCount));
}

void LfoControllerTest::test_panel_write_shouldReachThatLfoOnly()
{
    LfoController controller { nullptr };
    const auto device = std::make_shared<LfoDevice>("LFO");
    controller.setDevice(device);

    panel(controller, 2).setRateDivider(16);
    panel(controller, 2).setDepth(250);
    panel(controller, 2).setModulationMode(2);
    panel(controller, 2).setRetrigger(true);

    QCOMPARE(device->lfoParameterValue(2, NahdXml::xmlKeyRateDivider()), 16.0f);
    QCOMPARE(device->lfoParameterValue(2, NahdXml::xmlKeyIntensity()), 0.25f);
    QCOMPARE(panel(controller, 2).modulationMode(), 2);
    QVERIFY(panel(controller, 2).retrigger());
    QCOMPARE(panel(controller, 0).rateDivider(), 1);
    QVERIFY(!panel(controller, 0).retrigger());
}

void LfoControllerTest::test_panel_target_shouldSetAndClear()
{
    LfoController controller { nullptr };
    const auto device = std::make_shared<LfoDevice>("LFO");
    controller.setDevice(device);

    panel(controller, 1).setTarget("device", "synth-uuid", "lpfCutoff");
    QCOMPARE(device->target(1).ownerId, std::string { "synth-uuid" });
    QCOMPARE(panel(controller, 1).targetOwner(), QString { "device" });
    QCOMPARE(panel(controller, 1).targetParameter(), QString { "lpfCutoff" });

    panel(controller, 1).clearTarget();
    QVERIFY(device->target(1).ownerId.empty());
    QVERIFY(panel(controller, 1).targetOwner().isEmpty());
}

void LfoControllerTest::test_deviceChange_shouldNotifyThePanels()
{
    // A load, a preset or automation all arrive as a device change rather than through a panel
    LfoController controller { nullptr };
    const auto device = std::make_shared<LfoDevice>("LFO");
    controller.setDevice(device);
    QSignalSpy spy { &panel(controller, 3), &LfoPanelController::changed };

    device->updateLfoParameter(3, NahdXml::xmlKeyIntensity(), 0.9f);

    QVERIFY(spy.count() > 0);
    QCOMPARE(panel(controller, 3).depth(), 900);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::LfoControllerTest)
