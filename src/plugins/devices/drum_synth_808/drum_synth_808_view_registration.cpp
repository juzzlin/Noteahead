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

#include "drum_synth_808_device.hpp"
#include "drum_synth_808_dialog_model.hpp"

#include "../../../application/service/device_service.hpp"
#include "../../../view/plugins/plugin_catalog.hpp"

#include <QQmlEngine>

namespace noteahead {

void registerDrumSynth808DeviceView()
{
    PluginCatalog::addDevice({
      .name = "Drum Synth 808",
      .typeId = DrumSynth808Device::typeIdString(),
      .category = PluginCatalog::DeviceCategory::Drums,
      .dialogFile = "DrumSynth808Dialog.qml",
      .controllerProperty = "drumSynth808Controller",
      .createDialogModel = [](const PluginCatalog::DeviceServiceS & deviceService) { return std::make_shared<DrumSynth808DialogModel>(deviceService); },
      .registerQmlTypes = [] { qmlRegisterType<DrumSynth808DialogModel>("Noteahead", 1, 0, "DrumSynth808DialogModel"); },
    });
}

} // namespace noteahead
