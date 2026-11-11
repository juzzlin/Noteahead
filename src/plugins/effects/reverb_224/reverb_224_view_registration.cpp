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

#include "reverb_224.hpp"

#include "../../../common/constants.hpp"
#include "../../../domain/effects/effect.hpp"
#include "../../../view/plugins/effect_summary.hpp"
#include "../../../view/plugins/plugin_catalog.hpp"

#include <cmath>

namespace noteahead {

void registerReverb224EffectView()
{
    PluginCatalog::addEffect({
      .name = "Reverb 224",
      .typeId = Reverb224::typeIdString(),
      .type = Constants::RackEffectType::reverb224(),
      .category = PluginCatalog::EffectCategory::Space,
      .dialogFile = "Reverb224Dialog.qml",
      .summary = [](const std::shared_ptr<Effect> & effect, const PluginCatalog::SummaryContext &) -> QString {
          namespace C = Constants::NahdXml;
          const auto valueOf = [&effect](const QString & key) {
              const auto p = effect->parameter(key.toStdString());
              return p ? p->get().value() : 0.0f;
          };
          return QString { "(mid=%1s, bass=%2s, pre=%3ms, mix=%4%)" }
            .arg(Reverb224::decaySeconds(valueOf(C::xmlKeyMidDecay())), 0, 'f', 1)
            .arg(Reverb224::decaySeconds(valueOf(C::xmlKeyBassDecay())), 0, 'f', 1)
            .arg(static_cast<int>(std::round(Reverb224::preDelayMs(valueOf(C::xmlKeyPreDelay())))))
            .arg(static_cast<int>(std::round(valueOf(C::xmlKeyMix()) * 100.0f)));
          return {};
      },
    });
}

} // namespace noteahead
