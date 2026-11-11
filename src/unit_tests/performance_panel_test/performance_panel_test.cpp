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
#include "../../domain/utility/dbtp_meter.hpp"
#include "../../domain/utility/lufs_meter.hpp"
#include "../../domain/utility/rta.hpp"
#include "../../domain/utility/stereo_field_meter.hpp"
#include "../../infra/audio/audio_engine.hpp"
#include "../../infra/data_service.hpp"
#include "../../plugins/devices/lfo/lfo_device.hpp"
#include "../../plugins/devices/xy_pad/xy_pad_device.hpp"
#include "../../plugins/devices/xy_pad/xy_pad_dialog_model.hpp"
#include "../../plugins/effects/panner/panner.hpp"
#include "../../view/controllers/performance_panel_controller.hpp"
#include "../../view/qml/Components/level_meter_renderer.hpp"

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

void PerformancePanelTest::test_moveElement_right_shouldReorderAndMarkModified()
{
    PerformancePanelModel model;
    model.addElement(xyPad, "a");
    model.addElement(xyPad, "b");
    model.addElement(xyPad, "c");
    QSignalSpy modifiedSpy { &model, &PerformancePanelModel::modified };
    QSignalSpy movedSpy { &model, &QAbstractItemModel::rowsMoved };
    QVERIFY(model.moveElement(0, 2));
    QCOMPARE(model.elements(), (PerformancePanelModel::ElementList { { xyPad, "b" }, { xyPad, "c" }, { xyPad, "a" } }));
    QCOMPARE(modifiedSpy.count(), 1);
    QCOMPARE(movedSpy.count(), 1);
}

void PerformancePanelTest::test_moveElement_left_shouldReorderAsOneMove()
{
    PerformancePanelModel model;
    model.addElement(xyPad, "a");
    model.addElement(xyPad, "b");
    model.addElement(xyPad, "c");
    QSignalSpy movedSpy { &model, &QAbstractItemModel::rowsMoved };
    QSignalSpy removedSpy { &model, &QAbstractItemModel::rowsRemoved };
    QVERIFY(model.moveElement(2, 0));
    QCOMPARE(model.elements(), (PerformancePanelModel::ElementList { { xyPad, "c" }, { xyPad, "a" }, { xyPad, "b" } }));
    QCOMPARE(movedSpy.count(), 1);
    // The view keeps its delegates only if nothing is removed
    QCOMPARE(removedSpy.count(), 0);
    QCOMPARE(model.data(model.index(0), static_cast<int>(PerformancePanelModel::Roles::SourceIdRole)).toString(), QString { "c" });
}

void PerformancePanelTest::test_moveElement_sameOrOutOfRange_shouldDoNothing()
{
    PerformancePanelModel model;
    model.addElement(xyPad, "a");
    model.addElement(xyPad, "b");
    QSignalSpy modifiedSpy { &model, &PerformancePanelModel::modified };
    QVERIFY(!model.moveElement(1, 1));
    QVERIFY(!model.moveElement(-1, 0));
    QVERIFY(!model.moveElement(0, 2));
    QCOMPARE(model.elements(), (PerformancePanelModel::ElementList { { xyPad, "a" }, { xyPad, "b" } }));
    QCOMPARE(modifiedSpy.count(), 0);
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

void PerformancePanelTest::test_availableElements_meters_shouldListThemByType()
{
    const auto deviceService = createDeviceService();
    deviceService->insertEffectRack().setEffect(0, std::make_shared<LufsMeter>());
    deviceService->insertEffectRack().setEffect(1, std::make_shared<DbTpMeter>());
    deviceService->insertEffectRack().setEffect(2, std::make_shared<StereoFieldMeter>());
    PerformancePanelModel model;
    PerformancePanelController controller { deviceService, model };

    QStringList types;
    for (auto && element : controller.availableElements()) {
        types << element.toMap()["type"].toString();
    }
    QCOMPARE(types, (QStringList { controller.lufsMeterType(), controller.dbtpMeterType(), controller.stereoFieldMeterType() }));
    for (auto && type : types) {
        QVERIFY(controller.isEffectType(type));
    }
    QVERIFY(!controller.isEffectType(controller.xyPadType()));
}

void PerformancePanelTest::test_addEffect_meter_shouldAddItByType()
{
    const auto deviceService = createDeviceService();
    const auto meter = std::make_shared<DbTpMeter>();
    deviceService->insertEffectRack().setEffect(0, meter);
    PerformancePanelModel model;
    PerformancePanelController controller { deviceService, model };

    QVERIFY(controller.addEffect(QString::fromStdString(meter->id())));
    QCOMPARE(model.elements(), (PerformancePanelModel::ElementList { { controller.dbtpMeterType(), QString::fromStdString(meter->id()) } }));
}

void PerformancePanelTest::test_setAnalysisActive_stereoFieldMeter_shouldStartAndStopIt()
{
    const auto deviceService = createDeviceService();
    const auto meter = std::make_shared<StereoFieldMeter>();
    deviceService->insertEffectRack().setEffect(0, meter);
    PerformancePanelModel model;
    PerformancePanelController controller { deviceService, model };
    const auto id = QString::fromStdString(meter->id());

    // Its dialog showing it too: the panel letting go must leave the dialog's analysis running
    meter->addAnalysisViewer();
    controller.setAnalysisActive(id, true);
    controller.setAnalysisActive(id, true);
    controller.setAnalysisActive(id, false);
    QVERIFY(!meter->isSettled());

    meter->removeAnalysisViewer();
    QVERIFY(meter->isSettled());
}

void PerformancePanelTest::test_updateRenderer_lufsMeter_shouldShowItsThreeReadings()
{
    const auto deviceService = createDeviceService();
    const auto meter = std::make_shared<LufsMeter>();
    deviceService->insertEffectRack().setEffect(0, meter);
    PerformancePanelModel model;
    PerformancePanelController controller { deviceService, model };
    LevelMeterRenderer renderer;

    controller.updateRenderer(QString::fromStdString(meter->id()), &renderer);

    QCOMPARE(renderer.bars().size(), size_t { 3 });
    QCOMPARE(renderer.bars().at(0).label, QString { "M" });
    QCOMPARE(renderer.bars().at(0).value, static_cast<double>(meter->momentaryLufs()));
    QCOMPARE(renderer.bars().at(2).label, QString { "I" });
    QVERIFY(!renderer.bars().at(0).hold);
    QCOMPARE(renderer.scale().unit, QString { "LUFS" });
}

void PerformancePanelTest::test_updateRenderer_dbtpMeter_shouldShowBothChannelsWithHolds()
{
    const auto deviceService = createDeviceService();
    const auto meter = std::make_shared<DbTpMeter>();
    deviceService->insertEffectRack().setEffect(0, meter);
    PerformancePanelModel model;
    PerformancePanelController controller { deviceService, model };
    LevelMeterRenderer renderer;

    controller.updateRenderer(QString::fromStdString(meter->id()), &renderer);

    QCOMPARE(renderer.bars().size(), size_t { 2 });
    QCOMPARE(renderer.bars().at(1).label, QString { "R" });
    QVERIFY(renderer.bars().at(1).hold);
    QCOMPARE(renderer.scale().unit, QString { "dBTP" });
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

void PerformancePanelTest::test_setAnalysisActive_rtaTwice_shouldCountAsOneViewer()
{
    const auto deviceService = createDeviceService();
    const auto rta = std::make_shared<Rta>();
    deviceService->insertEffectRack().setEffect(0, rta);
    PerformancePanelModel model;
    PerformancePanelController controller { deviceService, model };
    const auto id = QString::fromStdString(rta->id());

    controller.setAnalysisActive(id, true);
    controller.setAnalysisActive(id, true);
    QVERIFY(!rta->isSettled());

    controller.setAnalysisActive(id, false);
    QVERIFY(rta->isSettled());
}

void PerformancePanelTest::test_setAnalysisActive_effectRemoved_shouldStillLetGoOfIt()
{
    // The rack moves on before the element is gone: the RTA taken must be the one let go of
    const auto deviceService = createDeviceService();
    const auto rta = std::make_shared<Rta>();
    deviceService->insertEffectRack().setEffect(0, rta);
    PerformancePanelModel model;
    PerformancePanelController controller { deviceService, model };
    const auto id = QString::fromStdString(rta->id());
    controller.setAnalysisActive(id, true);

    deviceService->insertEffectRack().setEffect(0, nullptr);
    controller.setAnalysisActive(id, false);

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

    auto * xyPadController = qobject_cast<XyPadDialogModel *>(controller.xyPadController(uuid));
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
