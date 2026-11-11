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

#include "ju_synth_controller.hpp"
#include "ju_synth_device.hpp"

#include "../../../view/plugins/plugin_catalog.hpp"

#include <QQmlEngine>

namespace noteahead {

void registerJuSynthDeviceView()
{
    PluginCatalog::addDevice({
      .name = "JU Synth",
      .typeId = JuSynthDevice::typeIdString(),
      .category = PluginCatalog::DeviceCategory::Synths,
      .dialogFile = "JuSynthDialog.qml",
      .controllerProperty = "juSynthController",
      .createController = [](const PluginCatalog::DeviceServiceS &) { return std::make_shared<JuSynthController>(std::make_shared<JuSynthDevice>("Default JuSynth")); },
      .registerQmlTypes = [] { qmlRegisterType<JuSynthController>("Noteahead", 1, 0, "JuSynthController"); },
    });
}

} // namespace noteahead
