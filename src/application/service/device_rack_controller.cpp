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

#include "device_rack_controller.hpp"
#include "device_service.hpp"
#include "sampler_controller.hpp"
#include "../../domain/devices/sampler_device.hpp"

namespace noteahead {

DeviceRackController::DeviceRackController(DeviceServiceS deviceService, SamplerControllerS samplerController, QObject * parent)
  : QObject { parent }
  , m_deviceService { std::move(deviceService) }
  , m_samplerController { std::move(samplerController) }
{
    connect(m_deviceService.get(), &DeviceService::dataChanged, this, &DeviceRackController::devicesChanged);
}

DeviceRackController::~DeviceRackController() = default;

QStringList DeviceRackController::devices() const
{
    return m_deviceService->internalDeviceNamesQt();
}

void DeviceRackController::openDevice(const QString & name)
{
    if (const auto sampler = std::dynamic_pointer_cast<SamplerDevice>(m_deviceService->device(name.toStdString()))) {
        m_samplerController->setSampler(sampler);
        emit samplerDialogRequested();
    }
}

} // namespace noteahead
