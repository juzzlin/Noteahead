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

#include "all_pass_filter.hpp"

#include "../../../common/constants.hpp"
#include "../../../common/parameter_mapper.hpp"
#include "../../../domain/effects/effect.hpp"
#include "../../../view/plugins/effect_summary.hpp"
#include "../../../view/plugins/plugin_catalog.hpp"

#include <cmath>

namespace noteahead {

void registerAllPassFilterEffectView()
{
    PluginCatalog::addEffect({
      .name = "All-Pass Filter",
      .typeId = AllPassFilter::typeIdString(),
      .type = QString::fromStdString(AllPassFilter::typeIdString()),
      .category = PluginCatalog::EffectCategory::EqAndFilter,
      .dialogFile = "AllPassFilterDialog.qml",
      .summary = [](const std::shared_ptr<Effect> & effect, const PluginCatalog::SummaryContext &) -> QString {
          const auto freq = effect->parameter(Constants::NahdXml::xmlKeyFrequency().toStdString());
          const auto stages = effect->parameter(Constants::NahdXml::xmlKeyStages().toStdString());
          if (freq && stages) {
              // In hertz, as the filter runs at it: the stored number is the knob's position
              // spread linearly over 20 to 20000, not a frequency
              const auto hz = ParameterMapper::mapLogFrequency(static_cast<double>(freq->get().shownValue()), 20.0, 20000.0);
              return QString { "(freq=%1Hz, stages=%2)" }
                .arg(static_cast<int>(std::lround(hz)))
                .arg(stages->get().xmlValue());
          }
          return {};
      },
    });
}

} // namespace noteahead
