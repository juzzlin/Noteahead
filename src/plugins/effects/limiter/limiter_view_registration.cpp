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

#include "limiter.hpp"
#include "limiter_dialog_model.hpp"

#include "../../../common/constants.hpp"
#include "../../../domain/effects/effect.hpp"
#include "../../../view/plugins/effect_summary.hpp"
#include "../../../view/plugins/plugin_catalog.hpp"

#include <QQmlEngine>

namespace noteahead {

void registerLimiterEffectView()
{
    PluginCatalog::addEffect({
      .name = "Limiter",
      .typeId = Constants::RackEffectType::limiter().toStdString(),
      .type = Constants::RackEffectType::limiter(),
      .category = PluginCatalog::EffectCategory::Dynamics,
      .dialogFile = "LimiterDialog.qml",
      .summary = [](const std::shared_ptr<Effect> & effect, const PluginCatalog::SummaryContext &) -> QString {
          const auto ceiling = effect->parameter(Constants::NahdXml::xmlKeyCeiling().toStdString());
          const auto boost = effect->parameter(Constants::NahdXml::xmlKeyBoost().toStdString());
          if (ceiling && boost) {
              return QString { "(ceil=%1dB, boost=%2)" }
                .arg(ceiling->get().xmlValue() / 100.0f, 0, 'f', 1)
                .arg(boost->get().shownValue() > 0.5f ? EffectSummary::tr("on") : EffectSummary::tr("off"));
          }
          return {};
      },
      .registerQmlTypes = [] { qmlRegisterType<LimiterDialogModel>("Noteahead", 1, 0, "LimiterDialogModel"); },
    });
}

} // namespace noteahead
