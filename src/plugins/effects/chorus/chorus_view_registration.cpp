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

#include "chorus.hpp"

#include "../../../common/constants.hpp"
#include "../../../common/parameter_mapper.hpp"
#include "../../../domain/effects/effect.hpp"
#include "../../../view/plugins/effect_summary.hpp"
#include "../../../view/plugins/plugin_catalog.hpp"

#include <cmath>

namespace noteahead {

void registerChorusEffectView()
{
    PluginCatalog::addEffect({
      .name = "Chorus",
      .typeId = Chorus::typeIdString(),
      .type = Constants::RackEffectType::chorus(),
      .category = PluginCatalog::EffectCategory::Modulation,
      .dialogFile = "ChorusDialog.qml",
      .summary = [](const std::shared_ptr<Effect> & effect, const PluginCatalog::SummaryContext &) -> QString {
          const auto rate = effect->parameter(Constants::NahdXml::xmlKeyRate().toStdString());
          const auto mix = effect->parameter(Constants::NahdXml::xmlKeyMix().toStdString());
          if (rate && mix) {
              const float rateHz = static_cast<float>(ParameterMapper::mapExponential(rate->get().shownValue(), 0.1, 10.0));
              return QString { "(rate=%1Hz, mix=%2%)" }
                .arg(rateHz, 0, 'f', 1)
                .arg(static_cast<int>(std::round(mix->get().shownValue() * 100.0f)));
          }
          return {};
      },
    });
}

} // namespace noteahead
