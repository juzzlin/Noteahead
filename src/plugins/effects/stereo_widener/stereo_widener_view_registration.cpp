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

#include "stereo_widener.hpp"
#include "stereo_widener_dialog_model.hpp"

#include "../../../common/constants.hpp"
#include "../../../common/parameter_mapper.hpp"
#include "../../../domain/effects/effect.hpp"
#include "../../../view/plugins/effect_summary.hpp"
#include "../../../view/plugins/plugin_catalog.hpp"

#include <cmath>

#include <QQmlEngine>

namespace noteahead {

void registerStereoWidenerEffectView()
{
    PluginCatalog::addEffect({
      .name = "Stereo Widener",
      .typeId = Constants::RackEffectType::stereoWidener().toStdString(),
      .type = Constants::RackEffectType::stereoWidener(),
      .category = PluginCatalog::EffectCategory::Stereo,
      .dialogFile = "StereoWidenerDialog.qml",
      .summary = [](const std::shared_ptr<Effect> & effect, const PluginCatalog::SummaryContext &) -> QString {
          const auto lowWidth { effect->parameter(Constants::NahdXml::xmlKeyBandWidth(0).toStdString()) };
          const auto midWidth { effect->parameter(Constants::NahdXml::xmlKeyBandWidth(1).toStdString()) };
          const auto highWidth { effect->parameter(Constants::NahdXml::xmlKeyBandWidth(2).toStdString()) };
          if (lowWidth && midWidth && highWidth) {
              QString monoName { EffectSummary::tr("off") };
              if (const auto monoBass { effect->parameter(Constants::NahdXml::xmlKeyMonoBass().toStdString()) }; monoBass && monoBass->get().shownValue() > 0.5f) {
                  if (const auto monoFreq { effect->parameter(Constants::NahdXml::xmlKeyMonoFreq().toStdString()) }; monoFreq) {
                      monoName = QString { "<%1Hz" }.arg(static_cast<int>(std::round(ParameterMapper::mapLogFrequency(monoFreq->get().shownValue(), 20.0, 300.0))));
                  }
              }
              const auto widthPercent = [](const auto & parameter) {
                  return static_cast<int>(std::round(parameter->get().shownValue() * 200.0f));
              };
              return QString { "(lo=%1%, mid=%2%, hi=%3%, mono=%4)" }
                .arg(widthPercent(lowWidth))
                .arg(widthPercent(midWidth))
                .arg(widthPercent(highWidth))
                .arg(monoName);
          }
          return {};
      },
      .registerQmlTypes = [] { qmlRegisterType<StereoWidenerDialogModel>("Noteahead", 1, 0, "StereoWidenerDialogModel"); },
    });
}

} // namespace noteahead
