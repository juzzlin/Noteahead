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

#include "auto_ducker.hpp"
#include "auto_ducker_dialog_model.hpp"

#include "../../../common/constants.hpp"
#include "../../../domain/devices/device.hpp"
#include "../../../domain/effects/effect.hpp"
#include "../../../view/plugins/effect_summary.hpp"
#include "../../../view/plugins/plugin_catalog.hpp"

#include <QQmlEngine>

namespace noteahead {

void registerAutoDuckerEffectView()
{
    PluginCatalog::addEffect({
      .name = "Auto Ducker",
      .typeId = Constants::RackEffectType::autoDucker().toStdString(),
      .type = Constants::RackEffectType::autoDucker(),
      .category = PluginCatalog::EffectCategory::Dynamics,
      .dialogFile = "AutoDuckerDialog.qml",
      .summary = [](const std::shared_ptr<Effect> & effect, const PluginCatalog::SummaryContext & context) -> QString {
          const auto amount { effect->parameter(Constants::NahdXml::xmlKeyAmount().toStdString()) };
          const auto threshold { effect->parameter(Constants::NahdXml::xmlKeyThreshold().toStdString()) };
          if (amount && threshold) {
              QString scName { EffectSummary::tr("None") };
              if (const auto sourceIndex { effect->sidechainSourceDeviceIndex() }; sourceIndex) {
                  if (const auto sourceDevice { context.device(*sourceIndex) }) {
                      scName = QString::fromStdString(sourceDevice->name());
                  }
              }
              return QString { "(amount=%1dB, thr=%2dB, sidechain=%3)" }
                .arg(amount->get().xmlValue() / 100.0f, 0, 'f', 1)
                .arg(threshold->get().xmlValue() / 100.0f, 0, 'f', 1)
                .arg(scName);
          }
          return {};
      },
      .registerQmlTypes = [] { qmlRegisterType<AutoDuckerDialogModel>("Noteahead", 1, 0, "AutoDuckerDialogModel"); },
    });
}

} // namespace noteahead
