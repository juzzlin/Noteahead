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

#include "effect_dialog_model.hpp"

#include "effect_rack_controller.hpp"

#include <cmath>

namespace noteahead {

EffectDialogModel::EffectDialogModel(QObject * parent)
  : QObject { parent }
{
}

EffectDialogModel::~EffectDialogModel() = default;

QObject * EffectDialogModel::rack() const
{
    return m_rack;
}

void EffectDialogModel::setRack(QObject * rack)
{
    const auto controller = qobject_cast<EffectRackController *>(rack);
    if (m_rack != controller) {
        m_rack = controller;
        emit rackChanged();
    }
}

int EffectDialogModel::effectIndex() const
{
    return m_effectIndex;
}

void EffectDialogModel::setEffectIndex(int effectIndex)
{
    if (m_effectIndex != effectIndex) {
        m_effectIndex = effectIndex;
        emit effectIndexChanged();
    }
}

std::vector<double> EffectDialogModel::responseFrequencies(int points)
{
    std::vector<double> frequencies;
    if (points <= 1) {
        return frequencies;
    }
    frequencies.reserve(static_cast<size_t>(points));
    for (int i = 0; i < points; i++) {
        const double t = static_cast<double>(i) / static_cast<double>(points - 1);
        frequencies.push_back(20.0 * std::pow(1000.0, t));
    }
    return frequencies;
}

QVariantList EffectDialogModel::sampleResponse(int points, const std::function<double(double)> & magnitudeDbAt)
{
    QVariantList response;
    for (const double hz : responseFrequencies(points)) {
        response.append(magnitudeDbAt(hz));
    }
    return response;
}

std::shared_ptr<Effect> EffectDialogModel::effect() const
{
    return m_rack && m_effectIndex >= 0 ? m_rack->effectAt(static_cast<quint32>(m_effectIndex)) : nullptr;
}

float EffectDialogModel::parameterValue(const QString & key) const
{
    return m_rack && m_effectIndex >= 0 ? m_rack->parameterValue(static_cast<quint32>(m_effectIndex), key) : 0.0f;
}

void EffectDialogModel::setParameterValue(const QString & key, float value)
{
    if (m_rack && m_effectIndex >= 0) {
        m_rack->setParameterValue(static_cast<quint32>(m_effectIndex), key, value);
    }
}

} // namespace noteahead
