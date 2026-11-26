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

#include "magic_master.hpp"
#include "magic_master_dialog_model.hpp"

#include "../../../common/constants.hpp"
#include "../../../domain/effects/effect.hpp"
#include "../../../view/plugins/plugin_catalog.hpp"

#include <QQmlEngine>

#include <array>

namespace noteahead {

void registerMagicMasterEffectView()
{
    PluginCatalog::addEffect({
      .name = "Magic Master",
      .typeId = MagicMaster::typeIdString(),
      .type = Constants::RackEffectType::magicMaster(),
      .category = PluginCatalog::EffectCategory::Dynamics,
      .dialogFile = "MagicMasterDialog.qml",
      .summary = [](const std::shared_ptr<Effect> & effect, const PluginCatalog::SummaryContext &) -> QString {
          namespace C = Constants::NahdXml;
          const auto target { effect->parameter(C::xmlKeyTargetLufs().toStdString()) };
          const auto ceiling { effect->parameter(C::xmlKeyCeiling().toStdString()) };
          if (!target || !ceiling) {
              return {};
          }
          const std::array<QString, 6> stageKeys {
              C::xmlKeySubCleanEnabled(),
              C::xmlKeyEqEnabled(),
              C::xmlKeyMultibandEnabled(),
              C::xmlKeyStereoEnabled(),
              C::xmlKeyClipperEnabled(),
              C::xmlKeyLimiterEnabled(),
          };
          int activeStages { 0 };
          for (const auto & key : stageKeys) {
              if (const auto p { effect->parameter(key.toStdString()) }; p && p->get().shownValue() > 0.5f) {
                  activeStages++;
              }
          }
          const auto base { QString { "(target=%1 LUFS, peak=%2 dBTP)" }
                              .arg(static_cast<double>(target->get().xmlValue()) / 100.0, 0, 'f', 1)
                              .arg(static_cast<double>(ceiling->get().xmlValue()) / 100.0, 0, 'f', 1) };
          if (activeStages < 6) {
              return QString { "%1 [%2/6]" }.arg(base).arg(activeStages);
          }
          return base;
      },
      .registerQmlTypes = [] { qmlRegisterType<MagicMasterDialogModel>("Noteahead", 1, 0, "MagicMasterDialogModel"); },
    });
}

} // namespace noteahead
