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
#include "../../domain/tracker/parameter_automation.hpp"
#include "../../domain/utility/rta.hpp"
#include "../qml/Dialogs/rta_renderer.hpp"
#include "xy_pad_controller.hpp"

#include <QQmlEngine>
#include <QVariantMap>

#include <algorithm>
#include <array>
#include <cmath>

namespace noteahead {

namespace {
//! The dB ranges the RTA's range setting picks from, as its dialog shows them.
const std::array<int, 4> rtaDbRanges { 30, 45, 60, 80 };

float parameterValue(const Effect & effect, const QString & key)
{
    const auto parameter = effect.parameter(key.toStdString());
    return parameter ? parameter->get().shownValue() : 0.0f;
}
} // namespace

PerformancePanelController::PerformancePanelController(std::shared_ptr<DeviceService> deviceService, PerformancePanelModel & model, QObject * parent)
  : QObject { parent }
  , m_deviceService { std::move(deviceService) }
  , m_model { model }
{
    if (m_deviceService) {
        connect(m_deviceService.get(), &DeviceService::dataChanged, this, &PerformancePanelController::refresh);
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

QString PerformancePanelController::rtaType() const
{
    return Constants::PanelElementType::rta();
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
    for (auto && location : m_deviceService->effectLocations()) {
        if (std::dynamic_pointer_cast<Rta>(location.effect)) {
            const auto type = Constants::PanelElementType::rta();
            const auto sourceId = QString::fromStdString(location.effect->id());
            elements << QVariantMap {
                { "type", type },
                { "sourceId", sourceId },
                { "location", effectLocation(sourceId) },
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

bool PerformancePanelController::addEffect(const QString & effectId)
{
    if (m_deviceService && std::dynamic_pointer_cast<Rta>(m_deviceService->findEffect(effectId.toStdString()))) {
        return m_model.addElement(Constants::PanelElementType::rta(), effectId);
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

QVariantMap PerformancePanelController::effectLocation(const QString & sourceId) const
{
    if (!m_deviceService || sourceId.isEmpty()) {
        return {};
    }
    const auto id = sourceId.toStdString();
    for (auto && location : m_deviceService->effectLocations()) {
        if (location.effect->id() == id) {
            const bool onDevice = location.rack == DeviceService::EffectLocation::Rack::DeviceInsert || location.rack == DeviceService::EffectLocation::Rack::DeviceVoice;
            const auto device = onDevice ? m_deviceService->device(location.deviceSlot) : nullptr;
            return {
                { "owner", ParameterAutomation::Target::ownerToString(ParameterAutomation::Target::Owner::Effect) },
                { "ownerId", sourceId },
                { "type", QString::fromStdString(location.effect->type()) },
                { "rack", static_cast<int>(location.rack) },
                { "deviceSlot", static_cast<int>(location.deviceSlot) },
                { "deviceTypeName", QString::fromStdString(location.deviceTypeName) },
                { "deviceName", device ? QString::fromStdString(device->name()) : QString {} },
                { "rackIndex", static_cast<int>(location.rackIndex) },
                { "effectSlot", static_cast<int>(location.effectSlot) }
            };
        }
    }
    return {};
}

void PerformancePanelController::rtaSetActive(const QString & sourceId, bool active)
{
    if (active) {
        if (const auto it = m_activeRtas.find(sourceId); it != m_activeRtas.end() && !it->second.expired()) {
            return;
        }
        if (const auto rta = m_deviceService ? std::dynamic_pointer_cast<Rta>(m_deviceService->findEffect(sourceId.toStdString())) : nullptr) {
            rta->addAnalysisViewer();
            m_activeRtas[sourceId] = rta;
        }
    } else if (const auto it = m_activeRtas.find(sourceId); it != m_activeRtas.end()) {
        if (const auto rta = it->second.lock()) {
            rta->removeAnalysisViewer();
        }
        m_activeRtas.erase(it);
    }
}

void PerformancePanelController::rtaUpdateRenderer(const QString & sourceId, QObject * rendererObject) const
{
    const auto renderer = qobject_cast<RtaRenderer *>(rendererObject);
    if (!renderer) {
        return;
    }
    if (const auto it = m_activeRtas.find(sourceId); it != m_activeRtas.end()) {
        if (const auto rta = it->second.lock()) {
            // Read every frame, so that what the RTA's own dialog changes shows here at once
            const auto range = static_cast<size_t>(std::clamp(static_cast<int>(std::lround(parameterValue(*rta, Constants::NahdXml::xmlKeyDbRange()))), 0, static_cast<int>(rtaDbRanges.size()) - 1));
            renderer->setDbRange(rtaDbRanges.at(range));
            renderer->setShowPinkNoise(parameterValue(*rta, Constants::NahdXml::xmlKeyShowPinkNoise()) >= 0.5f);
            renderer->setPinkNoiseLevel(parameterValue(*rta, Constants::NahdXml::xmlKeyPinkNoiseLevel()));
            // Layout first: levels of a new band count drawn against the old positions would put
            // every bar in the wrong place
            if (const auto generation = rta->layoutGeneration(); generation != renderer->layoutGeneration()) {
                renderer->setBandLayout(rta->bandLogPositions(), generation);
            }
            renderer->setBandLevels(rta->bandMagnitudesDb());
        }
    }
}

void PerformancePanelController::refresh()
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
