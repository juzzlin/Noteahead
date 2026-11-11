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

#include "compressor_2a.hpp"
#include "compressor_2a_dialog_model.hpp"

#include "../../../common/constants.hpp"
#include "../../../domain/effects/effect.hpp"
#include "../../../view/plugins/effect_summary.hpp"
#include "../../../view/plugins/plugin_catalog.hpp"

#include <cmath>

#include <QQmlEngine>

namespace noteahead {

void registerCompressor2aEffectView()
{
    PluginCatalog::addEffect({
      .name = "Compressor 2A",
      .typeId = Constants::RackEffectType::compressor2A().toStdString(),
      .type = Constants::RackEffectType::compressor2A(),
      .category = PluginCatalog::EffectCategory::Dynamics,
      .dialogFile = "Compressor2ADialog.qml",
      .summary = [](const std::shared_ptr<Effect> & effect, const PluginCatalog::SummaryContext &) -> QString {
          const auto peakReduction { effect->parameter(Constants::NahdXml::xmlKeyPeakReduction().toStdString()) };
          const auto mode { effect->parameter(Constants::NahdXml::xmlKeyMode().toStdString()) };
          if (peakReduction && mode) {
              return QString { "(peak=%1, %2)" }
                .arg(static_cast<int>(std::round(peakReduction->get().shownValue() * 100.0f)))
                .arg(mode->get().xmlValue() == static_cast<int>(Compressor2A::Mode::Limit) ? QString { "Limit" } : QString { "Compress" });
          }
          return {};
      },
      .registerQmlTypes = [] { qmlRegisterType<Compressor2ADialogModel>("Noteahead", 1, 0, "Compressor2ADialogModel"); },
    });
}

} // namespace noteahead
