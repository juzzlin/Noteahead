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

#include "drum_synth_v2_controller.hpp"
#include "drum_synth_v2_device.hpp"

#include "../../../application/service/device_service.hpp"
#include "../../../view/plugins/plugin_catalog.hpp"

#include <QQmlEngine>

namespace noteahead {

void registerDrumSynthV2DeviceView()
{
    PluginCatalog::addDevice({
      .name = "Drum Synth V2",
      .typeId = DrumSynthV2Device::typeIdString(),
      .category = PluginCatalog::DeviceCategory::Drums,
      .dialogFile = "DrumSynthV2Dialog.qml",
      .controllerProperty = "drumSynthV2Controller",
      .createController = [](const PluginCatalog::DeviceServiceS & deviceService) { return std::make_shared<DrumSynthV2Controller>(deviceService); },
      .registerQmlTypes = [] { qmlRegisterType<DrumSynthV2Controller>("Noteahead", 1, 0, "DrumSynthV2Controller"); },
    });
}

} // namespace noteahead
