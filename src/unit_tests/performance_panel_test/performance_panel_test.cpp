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

#include "performance_panel_test.hpp"

#include "../../application/models/performance_panel_model.hpp"
#include "../../application/service/device_service.hpp"
#include "../../common/constants.hpp"
#include "../../domain/devices/lfo_device.hpp"
#include "../../domain/devices/xy_pad_device.hpp"
#include "../../domain/effects/panner.hpp"
#include "../../domain/utility/rta.hpp"
#include "../../infra/audio/audio_engine.hpp"
#include "../../infra/data_service.hpp"
#include "../../view/controllers/performance_panel_controller.hpp"
#include "../../view/controllers/xy_pad_controller.hpp"

#include <QSignalSpy>
#include <QTest>

namespace noteahead {

namespace {
const auto xyPad = Constants::PanelElementType::xyPad();
const auto rtaType = Constants::PanelElementType::rta();

std::shared_ptr<DeviceService> createDeviceService()
{
    return std::make_shared<DeviceService>(std::make_shared<AudioEngine>(), std::make_shared<DataService>());
}
} // namespace

void PerformancePanelTest::test_addElement_shouldAppendAndMarkModified()
{
    PerformancePanelModel model;
    QSignalSpy modifiedSpy { &model, &PerformancePanelModel::modified };
    QVERIFY(model.addElement(xyPad, "a"));
    QVERIFY(model.addElement(xyPad, "b"));
    QCOMPARE(model.rowCount(), 2);
    QCOMPARE(model.data(model.index(1), static_cast<int>(PerformancePanelModel::Roles::SourceIdRole)).toString(), QString { "b" });
    QCOMPARE(modifiedSpy.count(), 2);
}

void PerformancePanelTest::test_addElement_twice_shouldBeRefused()
{
    PerformancePanelModel model;
    QVERIFY(model.addElement(xyPad, "a"));
    QVERIFY(!model.addElement(xyPad, "a"));
    QVERIFY(!model.addElement("", "b"));
    QCOMPARE(model.rowCount(), 1);
}

void PerformancePanelTest::test_removeElement_shouldDropThatOneAndMarkModified()
{
    PerformancePanelModel model;
    model.addElement(xyPad, "a");
    model.addElement(xyPad, "b");
    QSignalSpy modifiedSpy { &model, &PerformancePanelModel::modified };
    model.removeElement(0);
    model.removeElement(5);
    QCOMPARE(model.elements(), (PerformancePanelModel::ElementList { { xyPad, "b" } }));
    QCOMPARE(modifiedSpy.count(), 1);
}

void PerformancePanelTest::test_clear_shouldNotMarkModified()
{
    PerformancePanelModel model;
    model.addElement(xyPad, "a");
    QSignalSpy modifiedSpy { &model, &PerformancePanelModel::modified };
    model.clear();
    QCOMPARE(model.rowCount(), 0);
    QCOMPARE(modifiedSpy.count(), 0);
}

void PerformancePanelTest::test_availableElements_shouldListXyPadDevicesOnly()
{
    const auto deviceService = createDeviceService();
    const auto pad = std::make_shared<XyPadDevice>("XY");
    deviceService->setDevice(2, pad);
    deviceService->setDevice(4, std::make_shared<LfoDevice>("LFO"));
    PerformancePanelModel model;
    PerformancePanelController controller { deviceService, model };

    auto elements = controller.availableElements();
    QCOMPARE(elements.size(), 1);
    QCOMPARE(elements.at(0).toMap()["slot"].toInt(), 2);
    QCOMPARE(elements.at(0).toMap()["sourceId"].toString(), QString::fromStdString(pad->uuid()));
    QVERIFY(!elements.at(0).toMap()["onPanel"].toBool());

    QVERIFY(controller.addDevice(2));
    elements = controller.availableElements();
    QVERIFY(elements.at(0).toMap()["onPanel"].toBool());
}

void PerformancePanelTest::test_availableElements_rta_shouldListItWithItsLocation()
{
    const auto deviceService = createDeviceService();
    const auto rta = std::make_shared<Rta>();
    deviceService->insertEffectRack().setEffect(1, std::make_shared<Panner>());
    deviceService->insertEffectRack().setEffect(3, rta);
    PerformancePanelModel model;
    PerformancePanelController controller { deviceService, model };

    const auto elements = controller.availableElements();
    QCOMPARE(elements.size(), 1);
    const auto element = elements.at(0).toMap();
    QCOMPARE(element["type"].toString(), rtaType);
    QCOMPARE(element["sourceId"].toString(), QString::fromStdString(rta->id()));
    const auto location = element["location"].toMap();
    QCOMPARE(location["rack"].toInt(), static_cast<int>(DeviceService::EffectLocation::Rack::MasterInsert));
    QCOMPARE(location["effectSlot"].toInt(), 3);
}

void PerformancePanelTest::test_addEffect_otherEffect_shouldBeRefused()
{
    const auto deviceService = createDeviceService();
    const auto panner = std::make_shared<Panner>();
    const auto rta = std::make_shared<Rta>();
    deviceService->insertEffectRack().setEffect(0, panner);
    deviceService->insertEffectRack().setEffect(1, rta);
    PerformancePanelModel model;
    PerformancePanelController controller { deviceService, model };

    QVERIFY(!controller.addEffect(QString::fromStdString(panner->id())));
    QVERIFY(!controller.addEffect("gone"));
    QVERIFY(controller.addEffect(QString::fromStdString(rta->id())));
    QCOMPARE(model.elements(), (PerformancePanelModel::ElementList { { rtaType, QString::fromStdString(rta->id()) } }));
}

void PerformancePanelTest::test_effectLocation_effectRemoved_shouldBeEmpty()
{
    const auto deviceService = createDeviceService();
    const auto rta = std::make_shared<Rta>();
    deviceService->insertEffectRack().setEffect(0, rta);
    PerformancePanelModel model;
    PerformancePanelController controller { deviceService, model };
    const auto id = QString::fromStdString(rta->id());
    QVERIFY(!controller.effectLocation(id).isEmpty());

    deviceService->insertEffectRack().setEffect(0, nullptr);

    QVERIFY(controller.effectLocation(id).isEmpty());
}

void PerformancePanelTest::test_rtaSetActive_twice_shouldCountAsOneViewer()
{
    const auto deviceService = createDeviceService();
    const auto rta = std::make_shared<Rta>();
    deviceService->insertEffectRack().setEffect(0, rta);
    PerformancePanelModel model;
    PerformancePanelController controller { deviceService, model };
    const auto id = QString::fromStdString(rta->id());

    controller.rtaSetActive(id, true);
    controller.rtaSetActive(id, true);
    QVERIFY(!rta->isSettled());

    controller.rtaSetActive(id, false);
    QVERIFY(rta->isSettled());
}

void PerformancePanelTest::test_rtaSetActive_effectRemoved_shouldStillLetGoOfIt()
{
    // The rack moves on before the element is gone: the RTA taken must be the one let go of
    const auto deviceService = createDeviceService();
    const auto rta = std::make_shared<Rta>();
    deviceService->insertEffectRack().setEffect(0, rta);
    PerformancePanelModel model;
    PerformancePanelController controller { deviceService, model };
    const auto id = QString::fromStdString(rta->id());
    controller.rtaSetActive(id, true);

    deviceService->insertEffectRack().setEffect(0, nullptr);
    controller.rtaSetActive(id, false);

    QVERIFY(rta->isSettled());
}

void PerformancePanelTest::test_addDevice_otherDevice_shouldBeRefused()
{
    const auto deviceService = createDeviceService();
    deviceService->setDevice(4, std::make_shared<LfoDevice>("LFO"));
    PerformancePanelModel model;
    PerformancePanelController controller { deviceService, model };
    QVERIFY(!controller.addDevice(4));
    QVERIFY(!controller.addDevice(5));
    QVERIFY(!controller.addDevice(-1));
    QCOMPARE(model.rowCount(), 0);
}

void PerformancePanelTest::test_deviceSlot_shouldFollowTheDevice()
{
    const auto deviceService = createDeviceService();
    const auto pad = std::make_shared<XyPadDevice>("XY");
    deviceService->setDevice(7, pad);
    PerformancePanelModel model;
    PerformancePanelController controller { deviceService, model };
    const auto uuid = QString::fromStdString(pad->uuid());
    QCOMPARE(controller.deviceSlot(uuid), 7);
    QCOMPARE(controller.deviceSlot("nothing-like-it"), -1);
    QCOMPARE(controller.deviceSlot(""), -1);
}

void PerformancePanelTest::test_xyPadController_shouldPlayThatDevice()
{
    const auto deviceService = createDeviceService();
    const auto pad = std::make_shared<XyPadDevice>("XY");
    deviceService->setDevice(1, pad);
    PerformancePanelModel model;
    PerformancePanelController controller { deviceService, model };
    const auto uuid = QString::fromStdString(pad->uuid());

    auto * xyPadController = qobject_cast<XyPadController *>(controller.xyPadController(uuid));
    QVERIFY(xyPadController);
    QCOMPARE(controller.xyPadController(uuid), xyPadController);
    xyPadController->press(1, 0.5, 0.5);
    QVERIFY(pad->isPressed(1));
    QVERIFY(!controller.xyPadController("nothing-like-it"));
}

void PerformancePanelTest::test_xyPadController_deviceRemoved_shouldBeDropped()
{
    const auto deviceService = createDeviceService();
    const auto pad = std::make_shared<XyPadDevice>("XY");
    deviceService->setDevice(1, pad);
    PerformancePanelModel model;
    PerformancePanelController controller { deviceService, model };
    const auto uuid = QString::fromStdString(pad->uuid());
    QPointer<QObject> xyPadController = controller.xyPadController(uuid);
    QSignalSpy revisionSpy { &controller, &PerformancePanelController::revisionChanged };

    deviceService->clearDevice(1);
    QVERIFY(revisionSpy.count() > 0);
    QVERIFY(!controller.xyPadController(uuid));
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QVERIFY(!xyPadController);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::PerformancePanelTest)
