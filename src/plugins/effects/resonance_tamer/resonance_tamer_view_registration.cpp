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

#include "resonance_tamer.hpp"
#include "resonance_tamer_dialog_model.hpp"

#include "../../../common/constants.hpp"
#include "../../../domain/effects/effect.hpp"
#include "../../../view/plugins/effect_summary.hpp"
#include "../../../view/plugins/plugin_catalog.hpp"

#include <cmath>

#include <QQmlEngine>

namespace noteahead {

void registerResonanceTamerEffectView()
{
    PluginCatalog::addEffect({
      .name = "Resonance Tamer",
      .typeId = ResonanceTamer::typeIdString(),
      .type = QString::fromStdString(ResonanceTamer::typeIdString()),
      .category = PluginCatalog::EffectCategory::EqAndFilter,
      .dialogFile = "ResonanceTamerDialog.qml",
      .summary = [](const std::shared_ptr<Effect> & effect, const PluginCatalog::SummaryContext &) -> QString {
          if (const auto tamer = std::dynamic_pointer_cast<ResonanceTamer>(effect)) {
              const auto valueOf = [&tamer](const QString & key) {
                  const auto p = tamer->parameter(key.toStdString());
                  return p ? p->get().value() : 0.0f;
              };
              namespace C = Constants::NahdXml;
              const auto hz = [](double value) {
                  return value >= 1000.0 ? QString { "%1k" }.arg(value / 1000.0, 0, 'f', 1) : QString::number(static_cast<int>(std::round(value)));
              };
              const auto low = ResonanceTamer::frequencyHz(valueOf(C::xmlKeyLowFreq()));
              const auto high = ResonanceTamer::frequencyHz(valueOf(C::xmlKeyHighFreq()));
              return QString { "(%1 dB, %2=%3 dB, %4-%5 Hz)" }
                .arg(ResonanceTamer::depthDb(valueOf(C::xmlKeyDepth())), 0, 'f', 1)
                .arg(EffectSummary::tr("selectivity"))
                .arg(ResonanceTamer::selectivityDb(valueOf(C::xmlKeySelectivity())), 0, 'f', 1)
                .arg(hz(low), hz(high));
          }
          return {};
      },
      .registerQmlTypes = [] { qmlRegisterType<ResonanceTamerDialogModel>("Noteahead", 1, 0, "ResonanceTamerDialogModel"); },
    });
}

} // namespace noteahead
