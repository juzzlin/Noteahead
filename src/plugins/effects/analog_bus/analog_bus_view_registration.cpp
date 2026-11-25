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

#include "analog_bus.hpp"
#include "analog_bus_dialog_model.hpp"

#include "../../../common/constants.hpp"
#include "../../../domain/effects/effect.hpp"
#include "../../../view/plugins/effect_summary.hpp"
#include "../../../view/plugins/plugin_catalog.hpp"

#include <QQmlEngine>

#include <algorithm>
#include <array>
#include <cmath>

namespace noteahead {

void registerAnalogBusEffectView()
{
    PluginCatalog::addEffect({
      .name = "Analog Bus",
      .typeId = AnalogBus::typeIdString(),
      .type = Constants::RackEffectType::analogBus(),
      .category = PluginCatalog::EffectCategory::Distortion,
      .dialogFile = "AnalogBusDialog.qml",
      .summary = [](const std::shared_ptr<Effect> & effect, const PluginCatalog::SummaryContext &) -> QString {
          namespace C = Constants::NahdXml;
          const auto value = [&effect](const QString & key) {
              const auto p = effect->parameter(key.toStdString());
              return p ? p->get().shownValue() : 0.0f;
          };
          const auto routing = static_cast<AnalogBus::Routing>(std::clamp(static_cast<int>(std::lround(value(C::xmlKeyRouting()))), 0, static_cast<int>(AnalogBus::Routing::NToA)));
          static constexpr std::array<const char *, 5> colorNames { "Off", "Bus 4000", "Tape", "Vinyl", "Lo-Fi 12" };
          const auto colorIdx = std::clamp(static_cast<size_t>(std::lround(value(C::xmlKeyColorMode()))), size_t { 0 }, colorNames.size() - 1);
          const double driveA = AnalogBus::driveDb(value(C::xmlKeyDriveA()));
          const double driveN = AnalogBus::driveDb(value(C::xmlKeyDriveN()));
          const int mixPct = static_cast<int>(std::lround(value(C::xmlKeyMix()) * 100.0f));
          if (colorIdx > 0) {
              return QString { "(%1, %2, A=+%3dB, N=+%4dB, mix=%5%)" }
                .arg(QString::fromStdString(AnalogBus::routingLabel(routing)))
                .arg(QString::fromLatin1(colorNames[colorIdx]))
                .arg(driveA, 0, 'f', 1)
                .arg(driveN, 0, 'f', 1)
                .arg(mixPct);
          }
          return QString { "(%1, A=+%2dB, N=+%3dB, mix=%4%)" }
            .arg(QString::fromStdString(AnalogBus::routingLabel(routing)))
            .arg(driveA, 0, 'f', 1)
            .arg(driveN, 0, 'f', 1)
            .arg(mixPct);
      },
      .registerQmlTypes = [] { qmlRegisterType<AnalogBusDialogModel>("Noteahead", 1, 0, "AnalogBusDialogModel"); },
    });
}

} // namespace noteahead
