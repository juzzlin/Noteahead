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

#include "delay.hpp"

#include "../../../common/constants.hpp"
#include "../../../domain/effects/effect.hpp"
#include "../../../view/controllers/knob_controller.hpp"
#include "../../../view/plugins/effect_summary.hpp"
#include "../../../view/plugins/plugin_catalog.hpp"

#include <cmath>

namespace noteahead {

void registerDelayEffectView()
{
    PluginCatalog::addEffect({
      .name = "Delay",
      .typeId = Constants::RackEffectType::delay().toStdString(),
      .type = Constants::RackEffectType::delay(),
      .category = PluginCatalog::EffectCategory::Space,
      .dialogFile = "DelayDialog.qml",
      .summary = [](const std::shared_ptr<Effect> & effect, const PluginCatalog::SummaryContext &) -> QString {
          const auto sync = effect->parameter(Constants::NahdXml::xmlKeyDelaySync().toStdString());
          const auto feedback = effect->parameter(Constants::NahdXml::xmlKeyDelayFeedback().toStdString());
          if (sync && feedback) {
              QString timeStr;
              if (sync->get().shownValue() > 0.5f) {
                  const auto division = effect->parameter(Constants::NahdXml::xmlKeyDelaySyncDivision().toStdString());
                  KnobController knobController;
                  timeStr = knobController.syncLabel(knobController.syncIndex(division->get().shownValue() * Constants::uiInternalScaling()));
              } else {
                  const auto time = effect->parameter(Constants::NahdXml::xmlKeyDelayTime().toStdString());
                  timeStr = QString { "%1ms" }.arg(static_cast<int>(std::round(time->get().shownValue() * 10000.0f)));
              }
              return QString { "(time=%1, fb=%2%)" }
                .arg(timeStr)
                .arg(static_cast<int>(std::round(feedback->get().shownValue() * 100.0f)));
          }
          return {};
      },
    });
}

} // namespace noteahead
