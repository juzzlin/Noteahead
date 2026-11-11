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

#include "dynamic_eq_dialog_model.hpp"

#include "dynamic_eq.hpp"

namespace noteahead {

namespace {

QVariantList dynamicEqResponseAt(const std::shared_ptr<DynamicEq> & effect, int points, double gainDb)
{
    QVariantList response;
    if (effect) {
        for (const double db : effect->magnitudesDbAt(EffectDialogModel::responseFrequencies(points), gainDb)) {
            response.append(db);
        }
    }
    return response;
}

} // namespace

DynamicEqDialogModel::DynamicEqDialogModel(QObject * parent)
  : EffectDialogModel { parent }
{
}

QVariantList DynamicEqDialogModel::response(int points) const
{
    const auto effect = std::dynamic_pointer_cast<DynamicEq>(this->effect());
    return dynamicEqResponseAt(effect, points, effect ? effect->currentGainDb() : 0.0);
}

QVariantList DynamicEqDialogModel::rangeResponse(int points) const
{
    const auto effect = std::dynamic_pointer_cast<DynamicEq>(this->effect());
    return dynamicEqResponseAt(effect, points, effect ? effect->rangeDb() : 0.0);
}

float DynamicEqDialogModel::gainDb() const
{
    const auto effect = std::dynamic_pointer_cast<DynamicEq>(this->effect());
    return effect ? effect->currentGainDb() : 0.0f;
}

float DynamicEqDialogModel::rangeDb() const
{
    const auto effect = std::dynamic_pointer_cast<DynamicEq>(this->effect());
    return effect ? static_cast<float>(effect->rangeDb()) : 0.0f;
}

float DynamicEqDialogModel::detectorLevelDb() const
{
    const auto effect = std::dynamic_pointer_cast<DynamicEq>(this->effect());
    return effect ? effect->detectorLevelDb() : -120.0f;
}

QVariantList DynamicEqDialogModel::gainHistory() const
{
    QVariantList list;
    if (const auto effect = std::dynamic_pointer_cast<DynamicEq>(this->effect()); effect) {
        for (const float db : effect->gainHistory()) {
            list.append(db);
        }
    }
    return list;
}

void DynamicEqDialogModel::setActive(bool active)
{
    if (const auto effect = std::dynamic_pointer_cast<DynamicEq>(this->effect()); effect) {
        effect->setAnalysisEnabled(active);
    }
}

} // namespace noteahead
