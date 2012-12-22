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

#include "device_mixer_controller.hpp"

#include "../../application/service/device_service.hpp"
#include "../../application/service/editor_service.hpp"
#include "../../common/constants.hpp"
#include "../../domain/devices/device.hpp"

namespace noteahead {

DeviceMixerController::DeviceMixerController(DeviceServiceS deviceService, EditorServiceS editorService, QObject * parent)
  : QObject { parent }
  , m_deviceService { std::move(deviceService) }
  , m_editorService { std::move(editorService) }
{
}

DeviceMixerController::~DeviceMixerController() = default;

int DeviceMixerController::deviceVolume(int slotIndex) const
{
    const auto device = m_deviceService->device(static_cast<size_t>(slotIndex));
    // The unity point of the throw, matching the parameter default, so an absent device reads as unity.
    return device ? static_cast<int>(std::round(device->volume() * Constants::uiInternalScaling())) : static_cast<int>(std::round(Constants::faderUnityPosition() * Constants::uiInternalScaling()));
}

void DeviceMixerController::setDeviceVolume(int slotIndex, int value)
{
    if (const auto device = m_deviceService->device(static_cast<size_t>(slotIndex))) {
        device->setVolume(static_cast<float>(value) / Constants::uiInternalScaling());
        m_editorService->setIsModified(true);
    }
}

int DeviceMixerController::deviceGain(int slotIndex) const
{
    const auto device = m_deviceService->device(static_cast<size_t>(slotIndex));
    // Centre of the range is 0 dB, which is also what an absent device should read as.
    return device ? static_cast<int>(std::round(device->gain() * Constants::uiInternalScaling())) : 500;
}

void DeviceMixerController::setDeviceGain(int slotIndex, int value)
{
    if (const auto device = m_deviceService->device(static_cast<size_t>(slotIndex))) {
        device->setGain(static_cast<float>(value) / Constants::uiInternalScaling());
        m_editorService->setIsModified(true);
    }
}

int DeviceMixerController::devicePan(int slotIndex) const
{
    const auto device = m_deviceService->device(static_cast<size_t>(slotIndex));
    // Centre, which is also the sane answer for an absent device.
    return device ? static_cast<int>(std::round(device->pan() * Constants::uiInternalScaling())) : 500;
}

void DeviceMixerController::setDevicePan(int slotIndex, int value)
{
    if (const auto device = m_deviceService->device(static_cast<size_t>(slotIndex))) {
        device->setPan(static_cast<float>(value) / Constants::uiInternalScaling());
        m_editorService->setIsModified(true);
    }
}

int DeviceMixerController::deviceFaderPosition(int slotIndex) const
{
    const auto device = m_deviceService->device(static_cast<size_t>(slotIndex));
    return device ? static_cast<int>(device->faderPosition()) : static_cast<int>(Device::FaderPosition::PreInserts);
}

void DeviceMixerController::setDeviceFaderPosition(int slotIndex, int value)
{
    if (const auto device = m_deviceService->device(static_cast<size_t>(slotIndex))) {
        device->setFaderPosition(static_cast<Device::FaderPosition>(value));
        m_editorService->setIsModified(true);
    }
}

int DeviceMixerController::deviceSendTap(int slotIndex) const
{
    const auto device = m_deviceService->device(static_cast<size_t>(slotIndex));
    return device ? static_cast<int>(device->sendTap()) : static_cast<int>(Device::SendTap::PostFader);
}

void DeviceMixerController::setDeviceSendTap(int slotIndex, int value)
{
    if (const auto device = m_deviceService->device(static_cast<size_t>(slotIndex))) {
        device->setSendTap(static_cast<Device::SendTap>(value));
        m_editorService->setIsModified(true);
    }
}

} // namespace noteahead
