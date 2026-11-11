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

#include "air_band_eq.hpp"
#include "air_band_eq_dialog_model.hpp"

#include "../../../common/constants.hpp"
#include "../../../domain/effects/effect.hpp"
#include "../../../view/plugins/effect_summary.hpp"
#include "../../../view/plugins/plugin_catalog.hpp"

#include <QQmlEngine>

namespace noteahead {

void registerAirBandEqEffectView()
{
    PluginCatalog::addEffect({
      .name = "Air Band EQ",
      .typeId = Constants::RackEffectType::airBandEq().toStdString(),
      .type = Constants::RackEffectType::airBandEq(),
      .category = PluginCatalog::EffectCategory::EqAndFilter,
      .dialogFile = "AirBandEqDialog.qml",
      .summary = [](const std::shared_ptr<Effect> &, const PluginCatalog::SummaryContext &) -> QString {
          return "(Air)";
      },
      .registerQmlTypes = [] { qmlRegisterType<AirBandEqDialogModel>("Noteahead", 1, 0, "AirBandEqDialogModel"); },
    });
}

} // namespace noteahead
