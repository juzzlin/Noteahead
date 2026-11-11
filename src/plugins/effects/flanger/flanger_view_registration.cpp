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

#include "flanger.hpp"

#include "../../../common/constants.hpp"
#include "../../../common/parameter_mapper.hpp"
#include "../../../domain/dsp/lfo.hpp"
#include "../../../domain/effects/effect.hpp"
#include "../../../view/controllers/knob_controller.hpp"
#include "../../../view/plugins/effect_summary.hpp"
#include "../../../view/plugins/plugin_catalog.hpp"

namespace noteahead {

void registerFlangerEffectView()
{
    PluginCatalog::addEffect({
      .name = "Flanger",
      .typeId = Flanger::typeIdString(),
      .type = Constants::RackEffectType::flanger(),
      .category = PluginCatalog::EffectCategory::Modulation,
      .dialogFile = "FlangerDialog.qml",
      .summary = [](const std::shared_ptr<Effect> & effect, const PluginCatalog::SummaryContext &) -> QString {
          const auto rate = effect->parameter(Constants::NahdXml::xmlKeyLfoRate().toStdString());
          const auto mode = effect->parameter(Constants::NahdXml::xmlKeyLfoMode().toStdString());
          const auto divider = effect->parameter(Constants::NahdXml::xmlKeyRateDivider().toStdString());
          const auto feedback = effect->parameter(Constants::NahdXml::xmlKeyFeedback().toStdString());
          if (rate && mode && divider && feedback) {
              const auto rateDivider = std::max(1, divider->get().xmlValue());
              QString rateStr;
              if (static_cast<Lfo::Mode>(mode->get().xmlValue()) == Lfo::Mode::BPM) {
                  KnobController knobController;
                  rateStr = knobController.syncLabel(knobController.syncIndex(rate->get().shownValue() * Constants::uiInternalScaling()));
                  if (rateDivider > 1) {
                      rateStr += QString { "/%1" }.arg(rateDivider);
                  }
              } else {
                  rateStr = QString { "%1Hz" }.arg(ParameterMapper::mapLfoFrequency(rate->get().shownValue(), 0.05, 20.0) / rateDivider, 0, 'f', 2);
              }
              // Signed, because the two polarities of the feedback voice the comb quite
              // differently and the sign is what says which one is in use.
              const double signedFeedback = (static_cast<double>(feedback->get().shownValue()) * 2.0 - 1.0) * 100.0;
              return QString { "(rate=%1, fb=%2%)" }.arg(rateStr).arg(signedFeedback, 0, 'f', 0);
          }
          return {};
      },
    });
}

} // namespace noteahead
