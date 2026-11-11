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

#include "auto_filter.hpp"

#include "../../../common/constants.hpp"
#include "../../../common/parameter_mapper.hpp"
#include "../../../domain/dsp/lfo.hpp"
#include "../../../domain/effects/effect.hpp"
#include "../../../view/controllers/knob_controller.hpp"
#include "../../../view/plugins/effect_summary.hpp"
#include "../../../view/plugins/plugin_catalog.hpp"

#include <cmath>

namespace noteahead {

void registerAutoFilterEffectView()
{
    PluginCatalog::addEffect({
      .name = "Auto Filter",
      .typeId = Constants::RackEffectType::autoFilter().toStdString(),
      .type = Constants::RackEffectType::autoFilter(),
      .category = PluginCatalog::EffectCategory::Modulation,
      .dialogFile = "AutoFilterDialog.qml",
      .summary = [](const std::shared_ptr<Effect> & effect, const PluginCatalog::SummaryContext &) -> QString {
          const auto filterType = effect->parameter(Constants::NahdXml::xmlKeyFilterType().toStdString());
          const auto cutoff = effect->parameter(Constants::NahdXml::xmlKeyCutoff().toStdString());
          const auto mode = effect->parameter(Constants::NahdXml::xmlKeyLfoMode().toStdString());
          const auto rate = effect->parameter(Constants::NahdXml::xmlKeyLfoRate().toStdString());
          const auto intensity = effect->parameter(Constants::NahdXml::xmlKeyLfoIntensity().toStdString());
          if (filterType && cutoff && mode && rate && intensity) {
              const QStringList typeNames { "LPF", "HPF", "BPF", "Notch" };
              QString rateStr;
              if (static_cast<Lfo::Mode>(mode->get().xmlValue()) == Lfo::Mode::BPM) {
                  KnobController knobController;
                  rateStr = knobController.syncLabel(knobController.syncIndex(rate->get().shownValue() * Constants::uiInternalScaling()));
              } else {
                  rateStr = QString { "%1Hz" }.arg(ParameterMapper::mapLfoFrequency(rate->get().shownValue(), 0.05, 20.0), 0, 'f', 2);
              }
              // The intensity readout carries the same cubic taper as the knob it came from.
              const auto intensityPercent = ParameterMapper::mapCubicCentered((intensity->get().shownValue() - 0.5f) * 2.0f, -100.0, 100.0);
              return QString { "(%1, %2, rate=%3, int=%4%)" }
                .arg(typeNames.value(std::clamp(filterType->get().xmlValue(), 0, 3)))
                .arg(EffectSummary::formatFrequency(ParameterMapper::mapExponential(cutoff->get().shownValue(), 20.0, 20000.0)))
                .arg(rateStr)
                .arg(static_cast<int>(std::round(intensityPercent)));
          }
          return {};
      },
    });
}

} // namespace noteahead
