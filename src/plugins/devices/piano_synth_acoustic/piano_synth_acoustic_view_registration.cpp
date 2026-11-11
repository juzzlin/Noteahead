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

#include "piano_synth_acoustic_device.hpp"
#include "piano_synth_acoustic_dialog_model.hpp"

#include "../../../view/plugins/plugin_catalog.hpp"

#include <QQmlEngine>

namespace noteahead {

void registerPianoSynthAcousticDeviceView()
{
    PluginCatalog::addDevice({
      .name = "Piano Synth Acoustic",
      .typeId = PianoSynthAcousticDevice::typeIdString(),
      .category = PluginCatalog::DeviceCategory::Keys,
      .dialogFile = "PianoSynthAcousticDialog.qml",
      .controllerProperty = "pianoSynthAcousticController",
      .createDialogModel = [](const PluginCatalog::DeviceServiceS &) { return std::make_shared<PianoSynthAcousticDialogModel>(std::make_shared<PianoSynthAcousticDevice>("Default PianoSynth Acoustic")); },
      .registerQmlTypes = [] { qmlRegisterType<PianoSynthAcousticDialogModel>("Noteahead", 1, 0, "PianoSynthAcousticDialogModel"); },
    });
}

} // namespace noteahead
