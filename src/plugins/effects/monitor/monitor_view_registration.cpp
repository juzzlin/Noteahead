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

#include "monitor.hpp"

#include "../../../common/constants.hpp"
#include "../../../domain/effects/effect.hpp"
#include "../../../view/plugins/effect_summary.hpp"
#include "../../../view/plugins/plugin_catalog.hpp"

namespace noteahead {

void registerMonitorEffectView()
{
    PluginCatalog::addEffect({
      .name = "Monitor",
      .typeId = Constants::RackEffectType::monitor().toStdString(),
      .type = Constants::RackEffectType::monitor(),
      .category = PluginCatalog::EffectCategory::Utility,
      .dialogFile = "MonitorDialog.qml",
      .summary = [](const std::shared_ptr<Effect> & effect, const PluginCatalog::SummaryContext &) -> QString {
          // Named in full and in capitals: a monitor left folded is the one setting in the rack
          // that quietly misrepresents everything downstream of it, so it has to be readable
          // without opening anything.
          if (const auto mode = effect->parameter(Constants::NahdXml::xmlKeyMode().toStdString()); mode) {
              switch (static_cast<Monitor::Mode>(mode->get().xmlValue())) {
              case Monitor::Mode::Stereo:
                  return QString { "(%1)" }.arg(EffectSummary::tr("STEREO"));
              case Monitor::Mode::Mono:
                  return QString { "(%1)" }.arg(EffectSummary::tr("MONO"));
              case Monitor::Mode::Left:
                  return QString { "(%1)" }.arg(EffectSummary::tr("LEFT"));
              case Monitor::Mode::Right:
                  return QString { "(%1)" }.arg(EffectSummary::tr("RIGHT"));
              case Monitor::Mode::Side:
                  return QString { "(%1)" }.arg(EffectSummary::tr("SIDE"));
              }
          }
          return {};
      },
    });
}

} // namespace noteahead
