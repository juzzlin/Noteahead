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

#include "stereo_enhancer.hpp"

#include "../../../common/constants.hpp"
#include "../../../domain/effects/effect.hpp"
#include "../../../view/plugins/effect_summary.hpp"
#include "../../../view/plugins/plugin_catalog.hpp"

#include <cmath>

namespace noteahead {

void registerStereoEnhancerEffectView()
{
    PluginCatalog::addEffect({
      .name = "Stereo Enhancer",
      .typeId = Constants::RackEffectType::stereoEnhancer().toStdString(),
      .type = Constants::RackEffectType::stereoEnhancer(),
      .category = PluginCatalog::EffectCategory::Stereo,
      .dialogFile = "StereoEnhancerDialog.qml",
      .summary = [](const std::shared_ptr<Effect> & effect, const PluginCatalog::SummaryContext &) -> QString {
          const auto bass = effect->parameter(Constants::NahdXml::xmlKeyBassGain().toStdString());
          const auto high = effect->parameter(Constants::NahdXml::xmlKeyHighGain().toStdString());
          const auto spread = effect->parameter(Constants::NahdXml::xmlKeySpread().toStdString());
          if (bass && high && spread) {
              return QString { "(bass=%1%, hi=%2%, spread=%3%)" }
                .arg(static_cast<int>(std::round(bass->get().shownValue() * 100.0f)))
                .arg(static_cast<int>(std::round(high->get().shownValue() * 100.0f)))
                .arg(static_cast<int>(std::round(spread->get().shownValue() * 100.0f)));
          }
          return {};
      },
    });
}

} // namespace noteahead
