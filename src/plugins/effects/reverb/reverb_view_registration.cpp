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

#include "reverb.hpp"

#include "../../../common/constants.hpp"
#include "../../../domain/effects/effect.hpp"
#include "../../../view/plugins/effect_summary.hpp"
#include "../../../view/plugins/plugin_catalog.hpp"

namespace noteahead {

void registerReverbEffectView()
{
    PluginCatalog::addEffect({
      .name = "Reverb",
      .typeId = Constants::RackEffectType::reverb().toStdString(),
      .type = Constants::RackEffectType::reverb(),
      .category = PluginCatalog::EffectCategory::Space,
      .dialogFile = "ReverbDialog.qml",
      .summary = [](const std::shared_ptr<Effect> & effect, const PluginCatalog::SummaryContext &) -> QString {
          const auto preDelay = effect->parameter(Constants::NahdXml::xmlKeyPreDelay().toStdString());
          const auto decay = effect->parameter(Constants::NahdXml::xmlKeyDecay().toStdString());
          if (preDelay && decay) {
              return QString { "(pre=%1ms, decay=%2ms)" }
                .arg(preDelay->get().xmlValue() / preDelay->get().xmlScale())
                .arg(decay->get().xmlValue() / decay->get().xmlScale());
          }
          return {};
      },
    });
}

} // namespace noteahead
