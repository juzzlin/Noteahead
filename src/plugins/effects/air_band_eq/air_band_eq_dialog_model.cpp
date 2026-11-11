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

#include "air_band_eq_dialog_model.hpp"

#include "air_band_eq.hpp"

namespace noteahead {

AirBandEqDialogModel::AirBandEqDialogModel(QObject * parent)
  : EffectDialogModel { parent }
{
}

QVariantList AirBandEqDialogModel::response(int points) const
{
    const auto effect = std::dynamic_pointer_cast<AirBandEq>(this->effect());
    if (!effect) {
        return {};
    }
    QVariantList response;
    for (const double db : effect->magnitudesDbAt(EffectDialogModel::responseFrequencies(points))) {
        response.append(db);
    }
    return response;
}

} // namespace noteahead
