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

#include "gate.hpp"

#include "../../../common/constants.hpp"
#include "../../../domain/effects/effect.hpp"
#include "../../../view/plugins/effect_summary.hpp"
#include "../../../view/plugins/plugin_catalog.hpp"

namespace noteahead {

void registerGateEffectView()
{
    PluginCatalog::addEffect({
      .name = "Gate",
      .typeId = Constants::RackEffectType::gate().toStdString(),
      .type = Constants::RackEffectType::gate(),
      .category = PluginCatalog::EffectCategory::Dynamics,
      .dialogFile = "GateDialog.qml",
      .summary = [](const std::shared_ptr<Effect> & effect, const PluginCatalog::SummaryContext &) -> QString {
          const auto threshold = effect->parameter(Constants::NahdXml::xmlKeyThreshold().toStdString());
          const auto ratio = effect->parameter(Constants::NahdXml::xmlKeyRatio().toStdString());
          const auto range = effect->parameter(Constants::NahdXml::xmlKeyRange().toStdString());
          if (threshold && ratio && range) {
              const double thresholdDb = -60.0 + static_cast<double>(threshold->get().shownValue()) * 60.0;
              return QString { "(%1dB, %2:1, %3dB)" }
                .arg(thresholdDb, 0, 'f', 1)
                .arg(static_cast<double>(ratio->get().xmlValue()) / ratio->get().xmlScale(), 0, 'f', 1)
                .arg(-static_cast<double>(range->get().xmlValue()) / range->get().xmlScale(), 0, 'f', 0);
          }
          return {};
      },
    });
}

} // namespace noteahead
