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

#ifndef DEVICE_FACTORY_HPP
#define DEVICE_FACTORY_HPP

#include <functional>
#include <memory>
#include <string>

namespace noteahead {

class Device;

class DeviceFactory
{
public:
    using Creator = std::function<std::shared_ptr<Device>(const std::string & name)>;

    static void registerDevice(const std::string & typeId, Creator creator);
    static std::shared_ptr<Device> createDevice(const std::string & typeId, const std::string & name);
    static void init();
    static void clear();
};

} // namespace noteahead

#endif // DEVICE_FACTORY_HPP
