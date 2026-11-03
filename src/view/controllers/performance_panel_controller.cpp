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

#include "performance_panel_controller.hpp"

#include "../../application/models/performance_panel_model.hpp"
#include "../../application/service/device_service.hpp"
#include "../../common/constants.hpp"
#include "../../domain/devices/xy_pad_device.hpp"
#include "xy_pad_controller.hpp"

#include <QQmlEngine>
#include <QVariantMap>

namespace noteahead {

PerformancePanelController::PerformancePanelController(std::shared_ptr<DeviceService> deviceService, PerformancePanelModel & model, QObject * parent)
  : QObject { parent }
  , m_deviceService { std::move(deviceService) }
  , m_model { model }
{
    if (m_deviceService) {
        connect(m_deviceService.get(), &DeviceService::dataChanged, this, &PerformancePanelController::onDevicesChanged);
    }
}

int PerformancePanelController::revision() const
{
    return m_revision;
}

QString PerformancePanelController::xyPadType() const
{
    return Constants::PanelElementType::xyPad();
}

QString PerformancePanelController::xyPadDeviceTypeId() const
{
    return QString::fromStdString(XyPadDevice::typeIdString());
}

QVariantList PerformancePanelController::availableElements() const
{
    QVariantList elements;
    if (!m_deviceService) {
        return elements;
    }
    for (size_t slot = 0; slot < Constants::deviceRackSize(); slot++) {
        if (const auto device = std::dynamic_pointer_cast<XyPadDevice>(m_deviceService->device(slot))) {
            const auto type = Constants::PanelElementType::xyPad();
            const auto sourceId = QString::fromStdString(device->uuid());
            elements << QVariantMap {
                { "type", type },
                { "sourceId", sourceId },
                { "slot", static_cast<int>(slot) },
                { "onPanel", m_model.contains(type, sourceId) }
            };
        }
    }
    return elements;
}

bool PerformancePanelController::addDevice(int slotIndex)
{
    if (!m_deviceService || slotIndex < 0) {
        return false;
    }
    if (const auto device = std::dynamic_pointer_cast<XyPadDevice>(m_deviceService->device(static_cast<size_t>(slotIndex)))) {
        return m_model.addElement(Constants::PanelElementType::xyPad(), QString::fromStdString(device->uuid()));
    }
    return false;
}

int PerformancePanelController::deviceSlot(const QString & sourceId) const
{
    if (m_deviceService && !sourceId.isEmpty()) {
        const auto uuid = sourceId.toStdString();
        for (size_t slot = 0; slot < Constants::deviceRackSize(); slot++) {
            if (const auto device = m_deviceService->device(slot); device && device->uuid() == uuid) {
                return static_cast<int>(slot);
            }
        }
    }
    return -1;
}

QObject * PerformancePanelController::xyPadController(const QString & sourceId)
{
    if (const auto it = m_xyPadControllers.find(sourceId); it != m_xyPadControllers.end() && it->second) {
        return it->second;
    }
    const auto device = m_deviceService ? std::dynamic_pointer_cast<XyPadDevice>(m_deviceService->findDevice(sourceId.toStdString())) : nullptr;
    if (!device) {
        return nullptr;
    }
    auto * controller = new XyPadController { m_deviceService, this };
    controller->setDevice(device);
    // Handed to QML, which would otherwise collect it as its own: this controller decides its life
    QQmlEngine::setObjectOwnership(controller, QQmlEngine::CppOwnership);
    m_xyPadControllers[sourceId] = controller;
    return controller;
}

void PerformancePanelController::onDevicesChanged()
{
    // A controller outlives its device only until the rack changes: one left playing a removed or
    // replaced device would move parameters of a device nothing hears
    for (auto it = m_xyPadControllers.begin(); it != m_xyPadControllers.end();) {
        const auto device = m_deviceService->findDevice(it->first.toStdString());
        if (!it->second || !device || device != it->second->device()) {
            if (it->second) {
                it->second->deleteLater();
            }
            it = m_xyPadControllers.erase(it);
        } else {
            it++;
        }
    }
    m_revision++;
    emit revisionChanged();
}

} // namespace noteahead
