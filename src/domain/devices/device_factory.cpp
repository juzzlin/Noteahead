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

#include "device_factory.hpp"

#include "../../contrib/SimpleLogger/src/simple_logger.hpp"

#include <format>
#include <map>

namespace noteahead {

static const auto TAG = "DeviceFactory";

namespace {
std::map<std::string, DeviceFactory::Creator> & registry()
{
    static std::map<std::string, DeviceFactory::Creator> instance;
    return instance;
}
} // namespace

void DeviceFactory::registerDevice(const std::string & typeId, Creator creator)
{
    registry()[typeId] = std::move(creator);
}

std::shared_ptr<Device> DeviceFactory::createDevice(const std::string & typeId, const std::string & name)
{
    juzzlin::L(TAG).info() << std::format("Creating a new device {}: {}", typeId, name);

    if (const auto it = registry().find(typeId); it != registry().end()) {
        return it->second(name);
    }

    return nullptr;
}

void DeviceFactory::clear()
{
    registry().clear();
}

} // namespace noteahead
