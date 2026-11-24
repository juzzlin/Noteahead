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

#include "resonance_tamer_dialog_model.hpp"

#include "resonance_tamer.hpp"

namespace noteahead {

ResonanceTamerDialogModel::ResonanceTamerDialogModel(QObject * parent)
  : EffectDialogModel { parent }
{
}

QVariantList ResonanceTamerDialogModel::reduction(int path, int points) const
{
    QVariantList response;
    if (const auto effect = std::dynamic_pointer_cast<ResonanceTamer>(this->effect()); effect && path >= 0) {
        for (const double db : effect->reductionDbAt(EffectDialogModel::responseFrequencies(points), static_cast<size_t>(path))) {
            response.append(db);
        }
    }
    return response;
}

float ResonanceTamerDialogModel::deepestCutDb() const
{
    const auto effect = std::dynamic_pointer_cast<ResonanceTamer>(this->effect());
    return effect ? static_cast<float>(effect->deepestCutDb()) : 0.0f;
}

} // namespace noteahead
