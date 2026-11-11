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

#include "wave_designer.hpp"
#include "wave_designer_dialog_model.hpp"

#include "../../../common/constants.hpp"
#include "../../../domain/effects/effect.hpp"
#include "../../../view/plugins/effect_summary.hpp"
#include "../../../view/plugins/plugin_catalog.hpp"

#include <QQmlEngine>

namespace noteahead {

void registerWaveDesignerEffectView()
{
    PluginCatalog::addEffect({
      .name = "Wave Designer",
      .typeId = Constants::RackEffectType::waveDesigner().toStdString(),
      .type = Constants::RackEffectType::waveDesigner(),
      .category = PluginCatalog::EffectCategory::Dynamics,
      .dialogFile = "WaveDesignerDialog.qml",
      .summary = [](const std::shared_ptr<Effect> & effect, const PluginCatalog::SummaryContext &) -> QString {
          const auto attack = effect->parameter(Constants::NahdXml::xmlKeyAttack().toStdString());
          const auto sustain = effect->parameter(Constants::NahdXml::xmlKeySustain().toStdString());
          if (attack && sustain) {
              return QString { "(attack=%1, sustain=%2)" }
                .arg((attack->get().shownValue() - 0.5f) * 2.0f, 0, 'f', 2)
                .arg((sustain->get().shownValue() - 0.5f) * 2.0f, 0, 'f', 2);
          }
          return {};
      },
      .registerQmlTypes = [] { qmlRegisterType<WaveDesignerDialogModel>("Noteahead", 1, 0, "WaveDesignerDialogModel"); },
    });
}

} // namespace noteahead
