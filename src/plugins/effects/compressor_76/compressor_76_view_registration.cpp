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

#include "compressor_76.hpp"
#include "compressor_76_dialog_model.hpp"

#include "../../../common/constants.hpp"
#include "../../../domain/effects/effect.hpp"
#include "../../../view/plugins/plugin_catalog.hpp"

#include <QQmlEngine>

#include <algorithm>
#include <cmath>

namespace noteahead {

void registerCompressor76EffectView()
{
    PluginCatalog::addEffect({
      .name = "Compressor 76",
      .typeId = Compressor76::typeIdString(),
      .type = Constants::RackEffectType::compressor76(),
      .category = PluginCatalog::EffectCategory::Dynamics,
      .dialogFile = "Compressor76Dialog.qml",
      .summary = [](const std::shared_ptr<Effect> & effect, const PluginCatalog::SummaryContext &) -> QString {
          const auto ratioParam { effect->parameter(Constants::NahdXml::xmlKeyRatio().toStdString()) };
          const auto modeParam { effect->parameter(Constants::NahdXml::xmlKeyMode().toStdString()) };
          const auto inputParam { effect->parameter(Constants::NahdXml::xmlKeyInputGain().toStdString()) };
          if (ratioParam && modeParam && inputParam) {
              const auto ratioIndex = std::clamp(static_cast<int>(std::lround(ratioParam->get().shownValue())), 0, static_cast<int>(Compressor76::Ratio::All));
              const auto ratioText = QString::fromStdString(Compressor76::ratioLabel(static_cast<Compressor76::Ratio>(ratioIndex)));
              const auto character = static_cast<int>(std::lround(modeParam->get().shownValue())) == static_cast<int>(Compressor76::Character::BlueStripe)
                ? QString { "Blue Stripe" }
                : QString { "Blackface" };
              const double inDb = Compressor76::inputGainDb(inputParam->get().shownValue());
              const double shownDb = std::abs(inDb) < 0.05 ? 0.0 : inDb;
              const QString sign = shownDb >= 0.0 ? "+" : "";
              return QString { "(%1, %2, in=%3%4dB)" }
                .arg(ratioText)
                .arg(character)
                .arg(sign)
                .arg(shownDb, 0, 'f', 1);
          }
          return {};
      },
      .registerQmlTypes = [] { qmlRegisterType<Compressor76DialogModel>("Noteahead", 1, 0, "Compressor76DialogModel"); },
    });
}

} // namespace noteahead
