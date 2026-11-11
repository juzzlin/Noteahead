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

#include "auto_panner.hpp"

#include "../../../common/constants.hpp"
#include "../../../common/parameter_mapper.hpp"
#include "../../../domain/effects/effect.hpp"
#include "../../../view/controllers/knob_controller.hpp"
#include "../../../view/plugins/effect_summary.hpp"
#include "../../../view/plugins/plugin_catalog.hpp"

#include <cmath>

namespace noteahead {

void registerAutoPannerEffectView()
{
    PluginCatalog::addEffect({
      .name = "Auto Panner",
      .typeId = Constants::RackEffectType::autoPanner().toStdString(),
      .type = Constants::RackEffectType::autoPanner(),
      .category = PluginCatalog::EffectCategory::Modulation,
      .dialogFile = "AutoPannerDialog.qml",
      .summary = [](const std::shared_ptr<Effect> & effect, const PluginCatalog::SummaryContext &) -> QString {
          const auto sync = effect->parameter(Constants::NahdXml::xmlKeySync().toStdString());
          const auto intensity = effect->parameter(Constants::NahdXml::xmlKeyIntensity().toStdString());
          const auto divider = effect->parameter(Constants::NahdXml::xmlKeyRateDivider().toStdString());
          if (sync && intensity) {
              const auto rateDivider = divider ? std::max(1, divider->get().xmlValue()) : 1;
              QString rateStr;
              if (sync->get().shownValue() > 0.5f) {
                  const auto division = effect->parameter(Constants::NahdXml::xmlKeyDelaySyncDivision().toStdString());
                  KnobController knobController;
                  // A division cannot be divided into a division that has a name, so the
                  // divider is reported as itself.
                  rateStr = knobController.syncLabel(knobController.syncIndex(division->get().shownValue() * Constants::uiInternalScaling()));
                  if (rateDivider > 1) {
                      rateStr += QString { "/%1" }.arg(rateDivider);
                  }
              } else {
                  const auto rate = effect->parameter(Constants::NahdXml::xmlKeyRate().toStdString());
                  const float rateHz = static_cast<float>(ParameterMapper::mapLfoFrequency(rate->get().shownValue(), 0.05, 20.0)) / static_cast<float>(rateDivider);
                  rateStr = QString { "%1Hz" }.arg(rateHz, 0, 'f', 2);
              }
              return QString { "(rate=%1, int=%2%)" }
                .arg(rateStr)
                .arg(static_cast<int>(std::round(intensity->get().shownValue() * 100.0f)));
          }
          return {};
      },
    });
}

} // namespace noteahead
