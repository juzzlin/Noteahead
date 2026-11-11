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

#include "eq_32_band_graphical.hpp"

#include "../../../common/constants.hpp"
#include "../../../domain/effects/effect.hpp"
#include "../../../view/plugins/effect_summary.hpp"
#include "../../../view/plugins/plugin_catalog.hpp"

#include <cmath>

namespace noteahead {

void registerEq32BandGraphicalEffectView()
{
    PluginCatalog::addEffect({
      .name = "EQ 32-Band Graphical",
      .typeId = Constants::RackEffectType::eq32BandGraphical().toStdString(),
      .type = Constants::RackEffectType::eq32BandGraphical(),
      .category = PluginCatalog::EffectCategory::EqAndFilter,
      .dialogFile = "Eq32BandGraphicalDialog.qml",
      .summary = [](const std::shared_ptr<Effect> & effect, const PluginCatalog::SummaryContext &) -> QString {
          QString modeName { EffectSummary::tr("Stereo") };
          if (const auto stereoMode { effect->parameter(Constants::NahdXml::xmlKeyStereoMode().toStdString()) }; stereoMode) {
              switch (static_cast<Eq32BandGraphical::StereoMode>(std::clamp(static_cast<int>(std::round(stereoMode->get().shownValue())), 0, 2))) {
              case Eq32BandGraphical::StereoMode::Mid:
                  modeName = EffectSummary::tr("Mid");
                  break;
              case Eq32BandGraphical::StereoMode::Side:
                  modeName = EffectSummary::tr("Side");
                  break;
              case Eq32BandGraphical::StereoMode::Stereo:
                  break;
              }
          }
          if (effect->latencyFrames(effect->sampleRate()) > 0) {
              return QString { "(Graphical, %1, %2)" }.arg(modeName, EffectSummary::tr("Linear Phase"));
          }
          return QString { "(Graphical, %1)" }.arg(modeName);
      },
    });
}

} // namespace noteahead
