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

#include "phase_scope.hpp"

#include "../../../common/constants.hpp"
#include "../../../domain/effects/effect.hpp"
#include "../../../view/plugins/effect_summary.hpp"
#include "../../../view/plugins/plugin_catalog.hpp"

#include <cmath>

namespace noteahead {

void registerPhaseScopeEffectView()
{
    PluginCatalog::addEffect({
      .name = "Phase Scope",
      .typeId = Constants::RackEffectType::phaseScope().toStdString(),
      .type = QString::fromStdString(PhaseScope::typeIdString()),
      .category = PluginCatalog::EffectCategory::Meters,
      .dialogFile = "PhaseScopeDialog.qml",
      .summary = [](const std::shared_ptr<Effect> & effect, const PluginCatalog::SummaryContext &) -> QString {
          // Only what is set away from neutral, which is what tells two of them apart
          namespace C = Constants::NahdXml;
          const auto valueOf = [&effect](const QString & key) {
              const auto p = effect->parameter(key.toStdString());
              return p ? p->get().shownValue() : 0.0f;
          };
          QStringList items;
          if (valueOf(C::xmlKeyInvertLeft()) > 0.5f || valueOf(C::xmlKeyInvertRight()) > 0.5f) {
              items << QString { "\u00d8 %1%2" }.arg(valueOf(C::xmlKeyInvertLeft()) > 0.5f ? "L" : "").arg(valueOf(C::xmlKeyInvertRight()) > 0.5f ? "R" : "");
          }
          const QStringList modes { {}, EffectSummary::tr("Left"), EffectSummary::tr("Right"), EffectSummary::tr("Swap"), EffectSummary::tr("Mono") };
          if (const auto mode = static_cast<int>(std::lround(valueOf(C::xmlKeyChannelMode()))); mode > 0 && mode < modes.size()) {
              items << modes.at(mode);
          }
          if (const auto width = static_cast<int>(std::lround(valueOf(C::xmlKeyWidth()) * 200.0f)); width != 100) {
              items << QString { "%1 %2%" }.arg(EffectSummary::tr("width")).arg(width);
          }
          if (valueOf(C::xmlKeyMonoBass()) > 0.5f) {
              items << EffectSummary::tr("bass mono");
          }
          if (const auto ms = valueOf(C::xmlKeyDelayTime()) * PhaseScope::MaxDelayMs; ms > 0.0) {
              items << QString { "%1 %2 ms" }.arg(EffectSummary::tr("delay")).arg(ms, 0, 'f', 2);
          }
          if (const auto db = (valueOf(C::xmlKeyGain()) * 2.0 - 1.0) * PhaseScope::MaxGainDb; std::abs(db) >= 0.05) {
              items << QString { "%1%2 dB" }.arg(db > 0 ? "+" : "").arg(db, 0, 'f', 1);
          }
          return items.isEmpty() ? QString { "(%1)" }.arg(EffectSummary::tr("neutral")) : QString { "(%1)" }.arg(items.join(", "));
      },
    });
}

} // namespace noteahead
