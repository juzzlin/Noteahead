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

#include "device_rack.hpp"
#include "device_service.hpp"
#include "../../domain/devices/sampler_device.hpp"

namespace noteahead {

DeviceRack::DeviceRack(DeviceServiceS deviceService)
  : m_deviceService { std::move(deviceService) }
{
}

DeviceRack::~DeviceRack() = default;

void DeviceRack::initialize()
{
    for (size_t i = 1; i <= 4; i++) {
        const auto sampler = std::make_shared<SamplerDevice>("Sampler " + std::to_string(i));
        sampler->setId(i);
        m_deviceService->registerDevice(sampler);
    }
}

} // namespace noteahead
