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

#include "eq_32_band_graphical_dialog_model.hpp"

#include "eq_32_band_graphical.hpp"

#include "../../../common/constants.hpp"

namespace noteahead {

Eq32BandGraphicalDialogModel::Eq32BandGraphicalDialogModel(QObject * parent)
  : EffectDialogModel { parent }
{
}

QVariantList Eq32BandGraphicalDialogModel::response(int points) const
{
    const auto effect = std::dynamic_pointer_cast<Eq32BandGraphical>(this->effect());
    if (!effect) {
        return {};
    }
    // All points at once: the equalizer designs its bands from the parameters for every call, and
    // that is the expensive part
    QVariantList response;
    for (const double db : effect->magnitudesDbAt(EffectDialogModel::responseFrequencies(points))) {
        response.append(db);
    }
    return response;
}

int Eq32BandGraphicalDialogModel::bandCount() const
{
    return static_cast<int>(Eq32BandGraphical::BandCount);
}

QString Eq32BandGraphicalDialogModel::bandLabel(quint32 bandIndex) const
{
    return QString::fromStdString(Eq32BandGraphical::nominalLabel(bandIndex));
}

double Eq32BandGraphicalDialogModel::linearPhaseLatencyMs() const
{
    const auto effect = this->effect();
    if (!effect || effect->sampleRate() <= 0.0) {
        return 0.0;
    }
    return static_cast<double>(Eq32BandGraphical::linearPhaseLatencyFrames(effect->sampleRate())) * 1000.0 / effect->sampleRate();
}

void Eq32BandGraphicalDialogModel::flatten()
{
    if (!std::dynamic_pointer_cast<Eq32BandGraphical>(this->effect())) {
        return;
    }
    std::vector<std::pair<QString, float>> gains;
    for (size_t i = 0; i < Eq32BandGraphical::BandCount; i++) {
        gains.emplace_back(Constants::NahdXml::xmlKeyBandGain(i), Eq32BandGraphical::sliderParameterValue(0.0));
    }
    setParameterValues(gains);
}

} // namespace noteahead
