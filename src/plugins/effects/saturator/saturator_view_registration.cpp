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

#include "saturator.hpp"

#include "../../../common/constants.hpp"
#include "../../../domain/effects/effect.hpp"
#include "../../../view/plugins/effect_summary.hpp"
#include "../../../view/plugins/plugin_catalog.hpp"

#include <cmath>

namespace noteahead {

void registerSaturatorEffectView()
{
    PluginCatalog::addEffect({
      .name = "Saturator",
      .typeId = Constants::RackEffectType::saturator().toStdString(),
      .type = Constants::RackEffectType::saturator(),
      .category = PluginCatalog::EffectCategory::Distortion,
      .dialogFile = "SaturatorDialog.qml",
      .summary = [](const std::shared_ptr<Effect> & effect, const PluginCatalog::SummaryContext &) -> QString {
          const auto drive = effect->parameter(Constants::NahdXml::xmlKeyDriveDb().toStdString());
          const auto mix = effect->parameter(Constants::NahdXml::xmlKeyMix().toStdString());
          if (drive && mix) {
              return QString { "(drive=%1dB, mix=%2%)" }
                .arg(drive->get().shownValue() * 40.0f, 0, 'f', 1)
                .arg(static_cast<int>(std::round(mix->get().shownValue() * 100.0f)));
          }
          return {};
      },
    });
}

} // namespace noteahead
