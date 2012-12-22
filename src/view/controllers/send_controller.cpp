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

#include "send_controller.hpp"

#include "../../application/service/device_service.hpp"
#include "../../application/service/editor_service.hpp"
#include "../../common/constants.hpp"
#include "../../domain/devices/device.hpp"
#include "../../domain/effects/effect.hpp"
#include "../../domain/effects/effect_rack.hpp"

namespace noteahead {

SendController::SendController(DeviceServiceS deviceService, QObject * parent)
  : QObject { parent }
  , m_deviceService { std::move(deviceService) }
{
}

SendController::~SendController() = default;

float SendController::deviceSend(const QString & deviceName, quint32 effectIndex) const
{
    if (const auto device = m_deviceService->device(deviceName.toStdString()); device) {
        return device->reverbSend(effectIndex);
    }
    return 0.0f;
}

void SendController::setDeviceSend(const QString & deviceName, quint32 effectIndex, float send)
{
    if (const auto device = m_deviceService->device(deviceName.toStdString()); device) {
        device->setReverbSend(effectIndex, send);
    }
}

float SendController::partSend(const QString & deviceName, int subIndex, quint32 effectIndex) const
{
    if (subIndex < 0) {
        return deviceSend(deviceName, effectIndex);
    }
    if (const auto device = m_deviceService->device(deviceName.toStdString()); device) {
        return device->sendSourceLevel(static_cast<size_t>(subIndex), effectIndex);
    }
    return 0.0f;
}

void SendController::setPartSend(const QString & deviceName, int subIndex, quint32 effectIndex, float send)
{
    if (subIndex < 0) {
        setDeviceSend(deviceName, effectIndex, send);
        return;
    }
    if (const auto device = m_deviceService->device(deviceName.toStdString()); device) {
        device->setSendSourceLevel(static_cast<size_t>(subIndex), effectIndex, send);
    }
}

bool SendController::hasPartSends(const QString & deviceName) const
{
    if (const auto device = m_deviceService->device(deviceName.toStdString()); device) {
        return device->sendSourceCount() > 0;
    }
    return false;
}

int SendController::masterSendCount() const
{
    return static_cast<int>(Constants::effectRackSize());
}

QString SendController::masterSendEffectType(quint32 busIndex) const
{
    if (const auto effect = m_deviceService->sendEffectRack().effect(busIndex); effect) {
        return QString::fromStdString(effect->type());
    }
    return {};
}

bool SendController::hasMasterSendEffects() const
{
    return m_deviceService->sendEffectRack().hasEffects();
}

} // namespace noteahead
