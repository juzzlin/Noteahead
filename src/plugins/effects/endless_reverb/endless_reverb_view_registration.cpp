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

#include "endless_reverb.hpp"

#include "../../../common/constants.hpp"
#include "../../../domain/effects/effect.hpp"
#include "../../../view/plugins/effect_summary.hpp"
#include "../../../view/plugins/plugin_catalog.hpp"

#include <cmath>

namespace noteahead {

void registerEndlessReverbEffectView()
{
    PluginCatalog::addEffect({
      .name = "Endless Reverb",
      .typeId = Constants::RackEffectType::endless().toStdString(),
      .type = Constants::RackEffectType::endless(),
      .category = PluginCatalog::EffectCategory::Space,
      .dialogFile = "EndlessReverbDialog.qml",
      .summary = [](const std::shared_ptr<Effect> & effect, const PluginCatalog::SummaryContext &) -> QString {
          const auto preDelay = effect->parameter(Constants::NahdXml::xmlKeyPreDelay().toStdString());
          const auto size = effect->parameter(Constants::NahdXml::xmlKeySize().toStdString());
          const auto freeze = effect->parameter(Constants::NahdXml::xmlKeyFreeze().toStdString());
          if (preDelay && size && freeze) {
              // Pre-delay leads, as it does on the plain Reverb: the two read the same way in
              // a rack listing, and the parameter has the same 0-500 ms range on both.
              return QString { "(pre=%1ms, size=%2%, freeze=%3)" }
                .arg(preDelay->get().xmlValue() / preDelay->get().xmlScale())
                .arg(static_cast<int>(std::round(size->get().shownValue() * 100.0f)))
                .arg(freeze->get().shownValue() > 0.5f ? EffectSummary::tr("on") : EffectSummary::tr("off"));
          }
          return {};
      },
    });
}

} // namespace noteahead
