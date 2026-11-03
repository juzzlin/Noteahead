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

#include "xy_pad_controller_test.hpp"

#include "../../application/service/device_service.hpp"
#include "../../common/constants.hpp"
#include "../../domain/devices/lfo_device.hpp"
#include "../../domain/devices/synth_device.hpp"
#include "../../domain/devices/xy_pad_device.hpp"
#include "../../infra/audio/audio_engine.hpp"
#include "../../infra/data_service.hpp"
#include "../../view/controllers/lfo_controller.hpp"
#include "../../view/controllers/lfo_panel_controller.hpp"
#include "../../view/controllers/xy_pad_controller.hpp"
#include "../../view/controllers/xy_pad_panel_controller.hpp"

#include <QSignalSpy>
#include <QTest>

namespace noteahead {

namespace {

namespace NahdXml = Constants::NahdXml;
using Axis = XyPadDevice::Axis;

XyPadPanelController & panel(XyPadController & controller, int index)
{
    return *qobject_cast<XyPadPanelController *>(controller.pads().at(index).value<QObject *>());
}

} // namespace

void XyPadControllerTest::test_setDevice_otherDevice_shouldBeRefused()
{
    XyPadController controller { nullptr };
    QVERIFY(!controller.setDevice(std::make_shared<SynthDevice>("Synth")));
    QVERIFY(controller.setDevice(std::make_shared<XyPadDevice>("XY")));
    QCOMPARE(controller.pads().size(), static_cast<qsizetype>(XyPadDevice::PadCount));
    QCOMPARE(controller.padCount(), static_cast<int>(XyPadDevice::PadCount));
}

void XyPadControllerTest::test_panel_write_shouldReachThatPadOnly()
{
    XyPadController controller { nullptr };
    const auto device = std::make_shared<XyPadDevice>("XY");
    controller.setDevice(device);

    panel(controller, 2).setReturnTime(0);
    panel(controller, 2).setYDepth(250);

    QCOMPARE(device->padParameterValue(2, NahdXml::xmlKeyReturnTime()), 0.0f);
    QCOMPARE(device->axisParameterValue(2, Axis::Y, NahdXml::xmlKeyIntensity()), 0.25f);
    QCOMPARE(panel(controller, 2).xDepth(), 1000);
    QVERIFY(panel(controller, 0).returnTime() > 0);
}

void XyPadControllerTest::test_panel_target_shouldSetAndClear()
{
    XyPadController controller { nullptr };
    const auto device = std::make_shared<XyPadDevice>("XY");
    controller.setDevice(device);

    panel(controller, 1).setTarget(1, "effect", "delay-id", "feedback");
    QCOMPARE(device->target(1, Axis::Y).ownerId, std::string { "delay-id" });
    QVERIFY(device->target(1, Axis::X).ownerId.empty());
    QCOMPARE(panel(controller, 1).yTargetOwner(), QString { "effect" });
    QCOMPARE(panel(controller, 1).yTargetParameter(), QString { "feedback" });
    QVERIFY(panel(controller, 1).xTargetOwner().isEmpty());

    panel(controller, 1).clearTarget(1);
    QVERIFY(device->target(1, Axis::Y).ownerId.empty());
    QVERIFY(panel(controller, 1).yTargetOwnerId().isEmpty());
}

void XyPadControllerTest::test_panel_targetShared_withAnLfo_shouldWarn()
{
    const auto deviceService = std::make_shared<DeviceService>(std::make_shared<AudioEngine>(), std::make_shared<DataService>());
    const auto pad = std::make_shared<XyPadDevice>("XY");
    const auto lfo = std::make_shared<LfoDevice>("LFO");
    deviceService->setDevice(0, pad);
    deviceService->setDevice(1, lfo);
    XyPadController controller { deviceService };
    controller.setDevice(pad);
    LfoController lfoController { deviceService };
    lfoController.setDevice(lfo);
    auto & lfoPanel = *qobject_cast<LfoPanelController *>(lfoController.lfos().at(0).value<QObject *>());

    panel(controller, 0).setTarget(0, "device", "synth-uuid", "lpfCutoff");
    QVERIFY(!panel(controller, 0).xTargetShared());
    QVERIFY(!panel(controller, 0).yTargetShared());

    lfoPanel.setTarget("device", "synth-uuid", "lpfCutoff");
    QVERIFY(panel(controller, 0).xTargetShared());
    QVERIFY(lfoPanel.targetShared());

    // Another parameter of the same device is not the same parameter
    lfoPanel.setTarget("device", "synth-uuid", "lpfResonance");
    QVERIFY(!panel(controller, 0).xTargetShared());
    QVERIFY(!lfoPanel.targetShared());

    // Two axes of the same pad count too
    panel(controller, 3).setTarget(1, "device", "synth-uuid", "lpfCutoff");
    QVERIFY(panel(controller, 0).xTargetShared());
    QVERIFY(panel(controller, 3).yTargetShared());
}

void XyPadControllerTest::test_press_shouldReachTheDevice()
{
    XyPadController controller { nullptr };
    const auto device = std::make_shared<XyPadDevice>("XY");
    controller.press(0, 0.5, 0.5);
    QVERIFY(!controller.isPressed(0));

    controller.setDevice(device);
    controller.press(2, 0.5, -0.5);
    QVERIFY(controller.isPressed(2));
    QVERIFY(device->isPressed(2));
    QVERIFY(!device->isPressed(0));

    std::vector<ModulationSource::Step> steps;
    device->evaluate(0, 48000, 48000.0, 120.0, steps);
    QVERIFY(std::abs(controller.positionX(2) - 0.5) < 1.0e-3);
    QVERIFY(std::abs(controller.positionY(2) + 0.5) < 1.0e-3);

    controller.move(2, -0.25, 0.0);
    device->evaluate(48000, 48000, 48000.0, 120.0, steps);
    QVERIFY(std::abs(controller.positionX(2) + 0.25) < 1.0e-3);

    controller.release(2);
    QVERIFY(!controller.isPressed(2));
    device->evaluate(96000, 480, 48000.0, 120.0, steps);
    QVERIFY(controller.isMoving(2));
}

void XyPadControllerTest::test_deviceChange_shouldNotifyThePanels()
{
    // A load or automation arrives as a device change rather than through a panel
    XyPadController controller { nullptr };
    const auto device = std::make_shared<XyPadDevice>("XY");
    controller.setDevice(device);
    QSignalSpy spy { &panel(controller, 3), &XyPadPanelController::changed };

    device->updateAxisParameter(3, Axis::X, NahdXml::xmlKeyIntensity(), 0.9f);

    QVERIFY(spy.count() > 0);
    QCOMPARE(panel(controller, 3).xDepth(), 900);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::XyPadControllerTest)
