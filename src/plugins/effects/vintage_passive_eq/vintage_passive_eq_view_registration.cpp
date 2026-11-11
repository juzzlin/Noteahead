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

#include "vintage_passive_eq.hpp"

#include "../../../common/constants.hpp"
#include "../../../domain/effects/effect.hpp"
#include "../../../view/plugins/effect_summary.hpp"
#include "../../../view/plugins/plugin_catalog.hpp"

namespace noteahead {

void registerVintagePassiveEqEffectView()
{
    PluginCatalog::addEffect({
      .name = "Vintage Passive EQ",
      .typeId = Constants::RackEffectType::vintagePassiveEq().toStdString(),
      .type = Constants::RackEffectType::vintagePassiveEq(),
      .category = PluginCatalog::EffectCategory::EqAndFilter,
      .dialogFile = "VintagePassiveEqDialog.qml",
      .summary = [](const std::shared_ptr<Effect> &, const PluginCatalog::SummaryContext &) -> QString {
          return "(Passive)";
      },
    });
}

} // namespace noteahead
