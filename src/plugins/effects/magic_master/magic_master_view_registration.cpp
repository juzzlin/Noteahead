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

#include <algorithm>
#include <array>
#include <cmath>

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
          const auto threshold { effect->parameter(C::xmlKeyThreshold().toStdString()) };
          const auto ceiling { effect->parameter(C::xmlKeyCeiling().toStdString()) };
          if (!threshold || !ceiling) {
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
          const auto eqProfileParam { effect->parameter(C::xmlKeyEqProfile().toStdString()) };
          const auto eqEnabledParam { effect->parameter(C::xmlKeyEqEnabled().toStdString()) };
          const bool eqEnabled { !eqEnabledParam || eqEnabledParam->get().shownValue() > 0.5f };
          static constexpr std::array<const char *, MagicMaster::EqProfileCount> profileNames {
              "EDM", "Club Bass", "Festival", "House Warm", "Techno Punch", "Trance Air", "Synthwave", "Flat (Pink)"
          };
          const size_t profileIdx { eqProfileParam
                                      ? std::clamp(static_cast<size_t>(std::lround(eqProfileParam->get().shownValue())), size_t { 0 }, profileNames.size() - 1)
                                      : size_t { 0 } };
          const QString base { (eqEnabled && profileIdx > 0)
                                 ? QString { "(%1, thresh=%2 dB, ceil=%3 dBTP)" }
                                     .arg(QString::fromLatin1(profileNames[profileIdx]))
                                     .arg(static_cast<double>(threshold->get().xmlValue()) / 100.0, 0, 'f', 1)
                                     .arg(static_cast<double>(ceiling->get().xmlValue()) / 100.0, 0, 'f', 1)
                                 : QString { "(thresh=%1 dB, ceil=%2 dBTP)" }
                                     .arg(static_cast<double>(threshold->get().xmlValue()) / 100.0, 0, 'f', 1)
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
