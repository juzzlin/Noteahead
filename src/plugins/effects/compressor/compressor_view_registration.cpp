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

#include "compressor.hpp"

#include "../../../common/constants.hpp"
#include "../../../common/parameter_mapper.hpp"
#include "../../../domain/devices/device.hpp"
#include "../../../domain/effects/effect.hpp"
#include "../../../view/plugins/effect_summary.hpp"
#include "../../../view/plugins/plugin_catalog.hpp"

namespace noteahead {

void registerCompressorEffectView()
{
    PluginCatalog::addEffect({
      .name = "Compressor",
      .typeId = Constants::RackEffectType::compressor().toStdString(),
      .type = Constants::RackEffectType::compressor(),
      .category = PluginCatalog::EffectCategory::Dynamics,
      .dialogFile = "CompressorDialog.qml",
      .summary = [](const std::shared_ptr<Effect> & effect, const PluginCatalog::SummaryContext & context) -> QString {
          const auto attack { effect->parameter(Constants::NahdXml::xmlKeyAttack().toStdString()) };
          const auto ratio { effect->parameter(Constants::NahdXml::xmlKeyRatio().toStdString()) };
          if (attack && ratio) {
              const float attackMs { static_cast<float>(ParameterMapper::mapExponential(attack->get().shownValue(), 0.1, 500.0)) };
              const float ratioValue { static_cast<float>(ratio->get().xmlValue()) / static_cast<float>(ratio->get().xmlScale()) };
              QString scName { EffectSummary::tr("None") };
              if (const auto sourceIndex { effect->sidechainSourceDeviceIndex() }; sourceIndex) {
                  if (const auto sourceDevice { context.device(*sourceIndex) }) {
                      scName = QString::fromStdString(sourceDevice->name());
                  }
              }
              QString modeName { EffectSummary::tr("Peak") };
              if (const auto mode { effect->parameter(Constants::NahdXml::xmlKeyMode().toStdString()) }; mode) {
                  if (mode->get().shownValue() > 0.5f) {
                      modeName = EffectSummary::tr("RMS");
                  }
              }
              return QString { "(%1, attack=%2ms, ratio=%3:1, sidechain=%4)" }
                .arg(modeName)
                .arg(attackMs, 0, 'f', 1)
                .arg(ratioValue, 0, 'g', 3)
                .arg(scName);
          }
          return {};
      },
    });
}

} // namespace noteahead
