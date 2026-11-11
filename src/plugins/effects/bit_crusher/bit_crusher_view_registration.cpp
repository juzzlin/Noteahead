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

#include "bit_crusher.hpp"

#include "../../../common/constants.hpp"
#include "../../../common/parameter_mapper.hpp"
#include "../../../domain/effects/effect.hpp"
#include "../../../view/plugins/effect_summary.hpp"
#include "../../../view/plugins/plugin_catalog.hpp"

namespace noteahead {

void registerBitCrusherEffectView()
{
    PluginCatalog::addEffect({
      .name = "Bit Crusher",
      .typeId = Constants::RackEffectType::bitCrusher().toStdString(),
      .type = Constants::RackEffectType::bitCrusher(),
      .category = PluginCatalog::EffectCategory::Distortion,
      .dialogFile = "BitCrusherDialog.qml",
      .summary = [](const std::shared_ptr<Effect> & effect, const PluginCatalog::SummaryContext &) -> QString {
          const auto bits = effect->parameter(Constants::NahdXml::xmlKeyBitDepth().toStdString());
          const auto rate = effect->parameter(Constants::NahdXml::xmlKeySampleRate().toStdString());
          if (bits && rate) {
              const double hz = ParameterMapper::mapExponential(rate->get().shownValue(), 100.0, 48000.0);
              return QString { "(%1 %2, %3 kHz)" }
                .arg(bits->get().xmlValue())
                .arg(EffectSummary::tr("bits"))
                .arg(hz / 1000.0, 0, 'f', 1);
          }
          return {};
      },
    });
}

} // namespace noteahead
