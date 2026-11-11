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

#include "piano_synth_controller.hpp"
#include "piano_synth_device.hpp"

#include "../../../view/plugins/plugin_catalog.hpp"

#include <QQmlEngine>

namespace noteahead {

void registerPianoSynthDeviceView()
{
    PluginCatalog::addDevice({
      .name = "Piano Synth",
      .typeId = PianoSynthDevice::typeIdString(),
      .category = PluginCatalog::DeviceCategory::Keys,
      .dialogFile = "PianoSynthDialog.qml",
      .controllerProperty = "pianoSynthController",
      .createController = [](const PluginCatalog::DeviceServiceS &) { return std::make_shared<PianoSynthController>(std::make_shared<PianoSynthDevice>("Default PianoSynth")); },
      .registerQmlTypes = [] { qmlRegisterType<PianoSynthController>("Noteahead", 1, 0, "PianoSynthController"); },
    });
}

} // namespace noteahead
