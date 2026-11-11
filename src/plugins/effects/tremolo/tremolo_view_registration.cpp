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

#include "tremolo.hpp"

#include "../../../common/constants.hpp"
#include "../../../common/parameter_mapper.hpp"
#include "../../../domain/effects/effect.hpp"
#include "../../../view/controllers/knob_controller.hpp"
#include "../../../view/plugins/effect_summary.hpp"
#include "../../../view/plugins/plugin_catalog.hpp"

#include <cmath>

namespace noteahead {

void registerTremoloEffectView()
{
    PluginCatalog::addEffect({
      .name = "Tremolo",
      .typeId = Constants::RackEffectType::tremolo().toStdString(),
      .type = Constants::RackEffectType::tremolo(),
      .category = PluginCatalog::EffectCategory::Modulation,
      .dialogFile = "TremoloDialog.qml",
      .summary = [](const std::shared_ptr<Effect> & effect, const PluginCatalog::SummaryContext &) -> QString {
          const auto sync = effect->parameter(Constants::NahdXml::xmlKeySync().toStdString());
          const auto intensity = effect->parameter(Constants::NahdXml::xmlKeyIntensity().toStdString());
          const auto divider = effect->parameter(Constants::NahdXml::xmlKeyRateDivider().toStdString());
          if (sync && intensity && divider) {
              const auto rateDivider = std::max(1, divider->get().xmlValue());
              QString rateStr;
              if (sync->get().shownValue() > 0.5f) {
                  const auto division = effect->parameter(Constants::NahdXml::xmlKeyDelaySyncDivision().toStdString());
                  KnobController knobController;
                  rateStr = knobController.syncLabel(knobController.syncIndex(division->get().shownValue() * Constants::uiInternalScaling()));
                  if (rateDivider > 1) {
                      rateStr += QString { "/%1" }.arg(rateDivider);
                  }
              } else {
                  const auto rate = effect->parameter(Constants::NahdXml::xmlKeyRate().toStdString());
                  rateStr = QString { "%1Hz" }.arg(ParameterMapper::mapLfoFrequency(rate->get().shownValue(), 0.05, 20.0) / rateDivider, 0, 'f', 2);
              }
              return QString { "(rate=%1, depth=%2%)" }
                .arg(rateStr)
                .arg(static_cast<int>(std::lround(intensity->get().shownValue() * 100.0)));
          }
          return {};
      },
    });
}

} // namespace noteahead
