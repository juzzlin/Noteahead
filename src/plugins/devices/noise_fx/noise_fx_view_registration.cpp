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

#include "noise_fx_controller.hpp"
#include "noise_fx_device.hpp"

#include "../../../view/plugins/plugin_catalog.hpp"

#include <QQmlEngine>

namespace noteahead {

void registerNoiseFxDeviceView()
{
    PluginCatalog::addDevice({
      .name = "Noise FX",
      .typeId = NoiseFxDevice::typeIdString(),
      .category = PluginCatalog::DeviceCategory::Synths,
      .dialogFile = "NoiseFxDialog.qml",
      .controllerProperty = "noiseFxController",
      .createController = [](const PluginCatalog::DeviceServiceS &) { return std::make_shared<NoiseFxController>(std::make_shared<NoiseFxDevice>("Default Noise FX")); },
      .registerQmlTypes = [] { qmlRegisterType<NoiseFxController>("Noteahead", 1, 0, "NoiseFxController"); },
    });
}

} // namespace noteahead
