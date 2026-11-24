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

#include "speech_v2_device.hpp"
#include "speech_v2_dialog_model.hpp"

#include "../../../view/plugins/plugin_catalog.hpp"

#include <QQmlEngine>

namespace noteahead {

void registerSpeechV2DeviceView()
{
    PluginCatalog::addDevice({
      .name = "Speech V2",
      .typeId = SpeechV2Device::typeIdString(),
      .category = PluginCatalog::DeviceCategory::StringsAndVoice,
      .dialogFile = "SpeechV2Dialog.qml",
      .controllerProperty = "speechV2Controller",
      .createDialogModel = [](const PluginCatalog::DeviceServiceS &) { return std::make_shared<SpeechV2DialogModel>(std::make_shared<SpeechV2Device>("Default Speech V2")); },
      .registerQmlTypes = [] { qmlRegisterType<SpeechV2DialogModel>("Noteahead", 1, 0, "SpeechV2DialogModel"); },
    });
}

} // namespace noteahead
