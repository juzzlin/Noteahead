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
#include "../../view/qml/Components/xy_pad_renderer.hpp"

#include <QQuickWindow>
#include <QSignalSpy>
#include <QTest>

#include <cmath>

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

void XyPadControllerTest::test_panel_chooseTargetOwner_shouldTakeTheNextFreePair()
{
    const auto deviceService = std::make_shared<DeviceService>(std::make_shared<AudioEngine>(), std::make_shared<DataService>());
    const auto pad = std::make_shared<XyPadDevice>("XY");
    const auto lfo = std::make_shared<LfoDevice>("LFO");
    const auto synth = std::make_shared<SynthDevice>("Synth");
    deviceService->setDevice(0, pad);
    deviceService->setDevice(1, lfo);
    deviceService->setDevice(2, synth);
    XyPadController controller { deviceService };
    controller.setDevice(pad);
    LfoController lfoController { deviceService };
    lfoController.setDevice(lfo);
    auto & lfoPanel = *qobject_cast<LfoPanelController *>(lfoController.lfos().at(0).value<QObject *>());

    QStringList parameters;
    for (auto && automatable : synth->automatableParameters()) {
        parameters.push_back(QString::fromStdString(automatable.key));
    }
    const auto uuid = QString::fromStdString(synth->uuid());
    const auto priority = synth->modulationPriority();

    panel(controller, 0).chooseTargetOwner(0, "device", uuid, parameters);
    QCOMPARE(panel(controller, 0).xTargetParameter().toStdString(), priority.at(0));
    QCOMPARE(panel(controller, 0).yTargetParameter().toStdString(), priority.at(1));
    QCOMPARE(panel(controller, 0).yTargetOwnerId(), uuid);

    // The next pad takes the next pair rather than the same one again
    panel(controller, 1).chooseTargetOwner(0, "device", uuid, parameters);
    QCOMPARE(panel(controller, 1).xTargetParameter().toStdString(), priority.at(2));
    QCOMPARE(panel(controller, 1).yTargetParameter().toStdString(), priority.at(3));

    // Choosing the same device again keeps what the axis had: it is not taken by itself
    panel(controller, 0).chooseTargetOwner(0, "device", uuid, parameters);
    QCOMPARE(panel(controller, 0).xTargetParameter().toStdString(), priority.at(0));

    // An LFO counts the pads' targets as taken too
    lfoPanel.chooseTargetOwner("device", uuid, parameters);
    QCOMPARE(lfoPanel.targetParameter().toStdString(), priority.at(4));
    QVERIFY(!lfoPanel.targetShared());
}

void XyPadControllerTest::test_panel_chooseTargetOwner_otherAxisSet_shouldLeaveItAlone()
{
    const auto deviceService = std::make_shared<DeviceService>(std::make_shared<AudioEngine>(), std::make_shared<DataService>());
    const auto pad = std::make_shared<XyPadDevice>("XY");
    const auto synth = std::make_shared<SynthDevice>("Synth");
    deviceService->setDevice(0, pad);
    deviceService->setDevice(1, synth);
    XyPadController controller { deviceService };
    controller.setDevice(pad);
    const auto uuid = QString::fromStdString(synth->uuid());
    const QStringList parameters { "lpfCutoff", "lpfResonance", "hpfCutoff" };

    panel(controller, 0).setTarget(1, "effect", "some-effect", "mix");
    panel(controller, 0).chooseTargetOwner(0, "device", uuid, parameters);
    QCOMPARE(panel(controller, 0).xTargetParameter(), QString { "lpfCutoff" });
    QCOMPARE(panel(controller, 0).yTargetOwnerId(), QString { "some-effect" });
    QCOMPARE(panel(controller, 0).yTargetParameter(), QString { "mix" });

    // Choosing for Y alone takes the first free one
    panel(controller, 1).chooseTargetOwner(1, "device", uuid, parameters);
    QCOMPARE(panel(controller, 1).yTargetParameter(), QString { "lpfResonance" });
    QCOMPARE(panel(controller, 1).xTargetOwnerId(), uuid);
    QCOMPARE(panel(controller, 1).xTargetParameter(), QString { "hpfCutoff" });
}

void XyPadControllerTest::test_panel_chooseTargetOwner_midi_shouldNeverPickThePitchBend()
{
    const auto deviceService = std::make_shared<DeviceService>(std::make_shared<AudioEngine>(), std::make_shared<DataService>());
    const auto pad = std::make_shared<XyPadDevice>("XY");
    deviceService->setDevice(0, pad);
    XyPadController controller { deviceService };
    controller.setDevice(pad);
    // Listed first, as the picker lists it, and with no standard controller among the rest
    const QStringList parameters { "0:pb", "0:20", "0:21" };

    panel(controller, 0).chooseTargetOwner(0, "midi", "External", parameters);
    QCOMPARE(panel(controller, 0).xTargetParameter(), QString { "0:20" });
    QCOMPARE(panel(controller, 0).yTargetParameter(), QString { "0:21" });
    QVERIFY(!panel(controller, 0).xPitchBend());

    // Picked by hand it is a pitch bend, over the whole of its range
    panel(controller, 0).setTarget(0, "midi", "External", "0:pb");
    QVERIFY(panel(controller, 0).xPitchBend());
    QCOMPARE(panel(controller, 0).xMidiMin(), -100);
    QCOMPARE(panel(controller, 0).xMidiMax(), 100);
    panel(controller, 0).setXMidiMin(-500);
    QCOMPARE(panel(controller, 0).xMidiMin(), -100);
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

void XyPadControllerTest::test_press_otherController_shouldNotifyThisOne()
{
    // The dialog and the performance panel each have a controller of the same device
    const auto device = std::make_shared<XyPadDevice>("XY");
    XyPadController played { nullptr };
    played.setDevice(device);
    XyPadController watching { nullptr };
    watching.setDevice(device);
    QSignalSpy spy { &watching, &XyPadController::padPressed };

    played.press(2, 0.5, 0.5);

    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).toInt(), 2);
}

namespace {
//! A 100 x 100 pad 0 in a window of its own, so that the mouse reaches it as it does in the app.
struct RendererFixture
{
    RendererFixture()
    {
        controller.setDevice(device);
        window.resize(200, 200);
        renderer.setParentItem(window.contentItem());
        renderer.setSize({ 100, 100 });
        renderer.setController(&controller);
        window.show();
    }

    //! Where the audio thread has pad 0 once it has had a second to get there.
    QPointF settledPosition()
    {
        std::vector<ModulationSource::Step> steps;
        device->evaluate(frame, 48000, 48000.0, 120.0, steps);
        frame += 48000;
        return { controller.positionX(0), controller.positionY(0) };
    }

    XyPadController controller { nullptr };
    std::shared_ptr<XyPadDevice> device = std::make_shared<XyPadDevice>("XY");
    QQuickWindow window;
    XyPadRenderer renderer;
    uint64_t frame = 0;
};

bool near(double actual, double expected)
{
    return std::abs(actual - expected) < 1.0e-3;
}
} // namespace

void XyPadControllerTest::test_renderer_press_shouldPutTheHandThere()
{
    RendererFixture fixture;
    QTest::mousePress(&fixture.window, Qt::LeftButton, {}, { 75, 25 });
    QVERIFY(fixture.device->isPressed(0));
    const auto position = fixture.settledPosition();
    QVERIFY(near(position.x(), 0.5));
    QVERIFY(near(position.y(), 0.5));
}

void XyPadControllerTest::test_renderer_dragPastTheEdge_shouldPinThePadToTheEdge()
{
    RendererFixture fixture;
    QTest::mousePress(&fixture.window, Qt::LeftButton, {}, { 50, 50 });
    QTest::mouseMove(&fixture.window, { 180, 150 });
    QVERIFY(fixture.device->isPressed(0));
    const auto position = fixture.settledPosition();
    QVERIFY(near(position.x(), 1.0));
    QVERIFY(near(position.y(), -1.0));
}

void XyPadControllerTest::test_renderer_release_shouldLetGo()
{
    RendererFixture fixture;
    QTest::mousePress(&fixture.window, Qt::LeftButton, {}, { 75, 25 });
    QTest::mouseRelease(&fixture.window, Qt::LeftButton, {}, { 75, 25 });
    QVERIFY(!fixture.device->isPressed(0));
}

} // namespace noteahead

QTEST_MAIN(noteahead::XyPadControllerTest)
