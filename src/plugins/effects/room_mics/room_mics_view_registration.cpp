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

#include "room_mics.hpp"
#include "room_mics_dialog_model.hpp"
#include "room_view_renderer.hpp"

#include "../../../common/constants.hpp"
#include "../../../domain/effects/effect.hpp"
#include "../../../view/plugins/plugin_catalog.hpp"

#include <QQmlEngine>

namespace noteahead {

void registerRoomMicsEffectView()
{
    PluginCatalog::addEffect({
      .name = "Room Mics",
      .typeId = RoomMics::typeIdString(),
      .type = Constants::RackEffectType::roomMics(),
      .category = PluginCatalog::EffectCategory::Space,
      .dialogFile = "RoomMicsDialog.qml",
      .summary = [](const std::shared_ptr<Effect> & effect, const PluginCatalog::SummaryContext &) -> QString {
          const auto roomMics = std::dynamic_pointer_cast<RoomMics>(effect);
          if (!roomMics) {
              return {};
          }
          const auto settings = roomMics->settings();
          return QString { "(%1x%2x%3 m, RT60 %4 s)" }
            .arg(settings.widthM, 0, 'f', 1)
            .arg(settings.depthM, 0, 'f', 1)
            .arg(settings.heightM, 0, 'f', 1)
            .arg(RoomAcoustics::midReverbTime(RoomAcoustics::reverbTimes(settings)), 0, 'f', 2);
      },
      .registerQmlTypes = [] {
          qmlRegisterType<RoomMicsDialogModel>("Noteahead", 1, 0, "RoomMicsDialogModel");
          qmlRegisterType<RoomViewRenderer>("Noteahead", 1, 0, "RoomViewRenderer"); },
    });
}

} // namespace noteahead
