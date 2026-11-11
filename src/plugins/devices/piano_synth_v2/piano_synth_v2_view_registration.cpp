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

#include "piano_synth_v2_controller.hpp"
#include "piano_synth_v2_device.hpp"

#include "../../../view/plugins/plugin_catalog.hpp"

#include <QQmlEngine>

namespace noteahead {

void registerPianoSynthV2DeviceView()
{
    PluginCatalog::addDevice({
      .name = "Piano Synth V2",
      .typeId = PianoSynthV2Device::typeIdString(),
      .category = PluginCatalog::DeviceCategory::Keys,
      .dialogFile = "PianoSynthV2Dialog.qml",
      .controllerProperty = "pianoSynthV2Controller",
      .createController = [](const PluginCatalog::DeviceServiceS &) { return std::make_shared<PianoSynthV2Controller>(std::make_shared<PianoSynthV2Device>("Default PianoSynth V2")); },
      .registerQmlTypes = [] { qmlRegisterType<PianoSynthV2Controller>("Noteahead", 1, 0, "PianoSynthV2Controller"); },
    });
}

} // namespace noteahead
