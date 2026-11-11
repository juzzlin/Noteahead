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

#include "reference.hpp"
#include "reference_dialog_model.hpp"

#include "../../../common/constants.hpp"
#include "../../../domain/effects/effect.hpp"
#include "../../../view/plugins/effect_summary.hpp"
#include "../../../view/plugins/plugin_catalog.hpp"

#include <QQmlEngine>

namespace noteahead {

void registerReferenceEffectView()
{
    PluginCatalog::addEffect({
      .name = "Reference",
      .typeId = Constants::RackEffectType::reference().toStdString(),
      .type = Constants::RackEffectType::reference(),
      .category = PluginCatalog::EffectCategory::Utility,
      .dialogFile = "ReferenceDialog.qml",
      .summary = [](const std::shared_ptr<Effect> & effect, const PluginCatalog::SummaryContext &) -> QString {
          // The system being checked against, named: like the monitor's fold, a reference left
          // on is a setting that misrepresents everything downstream of it.
          if (const auto reference = std::dynamic_pointer_cast<Reference>(effect); reference) {
              return QString { "(%1)" }.arg(QString::fromStdString(reference->environment().name).toUpper());
          }
          return {};
      },
      .registerQmlTypes = [] { qmlRegisterType<ReferenceDialogModel>("Noteahead", 1, 0, "ReferenceDialogModel"); },
    });
}

} // namespace noteahead
