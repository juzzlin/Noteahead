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

#include "kick_808_controller.hpp"
#include "kick_808_device.hpp"

#include "../../../view/plugins/plugin_catalog.hpp"

#include <QQmlEngine>

namespace noteahead {

void registerKick808DeviceView()
{
    PluginCatalog::addDevice({
      .name = "Kick 808",
      .typeId = Kick808Device::typeIdString(),
      .category = PluginCatalog::DeviceCategory::Drums,
      .dialogFile = "Kick808Dialog.qml",
      .controllerProperty = "kick808Controller",
      .createController = [] { return std::make_shared<Kick808Controller>(std::make_shared<Kick808Device>("Default Kick808")); },
      .registerQmlTypes = [] { qmlRegisterType<Kick808Controller>("Noteahead", 1, 0, "Kick808Controller"); },
    });
}

} // namespace noteahead
