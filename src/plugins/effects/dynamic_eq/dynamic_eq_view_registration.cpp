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

#include "dynamic_eq.hpp"

#include "../../../common/constants.hpp"
#include "../../../domain/devices/device.hpp"
#include "../../../domain/effects/effect.hpp"
#include "../../../view/plugins/effect_summary.hpp"
#include "../../../view/plugins/plugin_catalog.hpp"

#include <cmath>

namespace noteahead {

void registerDynamicEqEffectView()
{
    PluginCatalog::addEffect({
      .name = "Dynamic EQ",
      .typeId = DynamicEq::typeIdString(),
      .type = QString::fromStdString(DynamicEq::typeIdString()),
      .category = PluginCatalog::EffectCategory::EqAndFilter,
      .dialogFile = "DynamicEqDialog.qml",
      .summary = [](const std::shared_ptr<Effect> & effect, const PluginCatalog::SummaryContext & context) -> QString {
          if (const auto eq = std::dynamic_pointer_cast<DynamicEq>(effect)) {
              const auto valueOf = [&eq](const QString & key) {
                  const auto p = eq->parameter(key.toStdString());
                  return p ? p->get().shownValue() : 0.0f;
              };
              namespace C = Constants::NahdXml;
              const QStringList types { EffectSummary::tr("Bell"), EffectSummary::tr("Low Shelf"), EffectSummary::tr("High Shelf"), EffectSummary::tr("Flat") };
              const auto bandType = std::clamp(static_cast<int>(std::lround(valueOf(C::xmlKeyFilterType()))), 0, 3);
              QStringList items { types.at(bandType) };
              if (bandType != static_cast<int>(DynamicEq::BandType::Flat)) {
                  const auto hz = DynamicEq::frequencyHz(valueOf(C::xmlKeyFrequency()));
                  items << (hz >= 1000.0 ? QString { "%1 kHz" }.arg(hz / 1000.0, 0, 'f', 1) : QString { "%1 Hz" }.arg(static_cast<int>(std::round(hz))));
              }
              const auto range = eq->rangeDb();
              items << QString { "%1%2 dB" }.arg(range > 0 ? "+" : "").arg(range, 0, 'f', 1);
              if (valueOf(C::xmlKeyDetectorSource()) > 0.5f) {
                  QString scName { EffectSummary::tr("None") };
                  if (const auto sourceIndex { eq->sidechainSourceDeviceIndex() }; sourceIndex) {
                      if (const auto sourceDevice { context.device(*sourceIndex) }) {
                          scName = QString::fromStdString(sourceDevice->name());
                      }
                  }
                  items << QString { "%1=%2" }.arg(EffectSummary::tr("sidechain"), scName);
              }
              return QString { "(%1)" }.arg(items.join(", "));
          }
          return {};
      },
    });
}

} // namespace noteahead
