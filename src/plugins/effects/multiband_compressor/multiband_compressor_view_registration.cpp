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

#include "multiband_compressor.hpp"

#include "../../../common/constants.hpp"
#include "../../../common/parameter_mapper.hpp"
#include "../../../domain/devices/device.hpp"
#include "../../../domain/effects/effect.hpp"
#include "../../../view/plugins/effect_summary.hpp"
#include "../../../view/plugins/plugin_catalog.hpp"

#include <cmath>

namespace noteahead {

void registerMultibandCompressorEffectView()
{
    PluginCatalog::addEffect({
      .name = "Multiband Compressor",
      .typeId = Constants::RackEffectType::multibandCompressor().toStdString(),
      .type = Constants::RackEffectType::multibandCompressor(),
      .category = PluginCatalog::EffectCategory::Dynamics,
      .dialogFile = "MultibandCompressorDialog.qml",
      .summary = [](const std::shared_ptr<Effect> & effect, const PluginCatalog::SummaryContext & context) -> QString {
          const auto lowerCrossover { effect->parameter(Constants::NahdXml::xmlKeyCrossoverFreq(0).toStdString()) };
          const auto upperCrossover { effect->parameter(Constants::NahdXml::xmlKeyCrossoverFreq(1).toStdString()) };
          if (lowerCrossover && upperCrossover) {
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
              return QString { "(%1, %2/%3Hz, sidechain=%4)" }
                .arg(modeName)
                .arg(static_cast<int>(std::round(ParameterMapper::mapLogFrequency(lowerCrossover->get().shownValue(), 20.0, 20000.0))))
                .arg(static_cast<int>(std::round(ParameterMapper::mapLogFrequency(upperCrossover->get().shownValue(), 20.0, 20000.0))))
                .arg(scName);
          }
          return {};
      },
    });
}

} // namespace noteahead
