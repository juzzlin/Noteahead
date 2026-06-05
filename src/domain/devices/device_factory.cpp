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

#include <map>

namespace noteahead {

namespace {
std::map<std::string, DeviceFactory::Creator> & registry()
{
    static std::map<std::string, DeviceFactory::Creator> instance;
    return instance;
}
}

void DeviceFactory::registerDevice(const std::string & typeId, Creator creator)
{
    registry()[typeId] = std::move(creator);
}

std::shared_ptr<Device> DeviceFactory::createDevice(const std::string & typeId, const std::string & name)
{
    const auto it = registry().find(typeId);
    if (it != registry().end()) {
        return it->second(name);
    }
    return nullptr;
}

void DeviceFactory::clear()
{
    registry().clear();
}

} // namespace noteahead
