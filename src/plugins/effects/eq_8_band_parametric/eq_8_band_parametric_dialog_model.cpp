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

#include "eq_8_band_parametric_dialog_model.hpp"

#include "eq_8_band_parametric.hpp"

#include "../../../common/constants.hpp"

#include <cmath>

namespace noteahead {

Eq8BandParametricDialogModel::Eq8BandParametricDialogModel(QObject * parent)
  : EffectDialogModel { parent }
{
}

QVariantList Eq8BandParametricDialogModel::response(int points) const
{
    const auto equalizer = std::dynamic_pointer_cast<Eq8BandParametric>(this->effect());
    if (!equalizer) {
        return {};
    }
    QVariantList response;
    const auto path = equalizer->isPathBypassed(Eq8BandParametric::Path::Mid) ? Eq8BandParametric::Path::Side : Eq8BandParametric::Path::Mid;
    for (const double db : equalizer->magnitudesDbAt(responseFrequencies(points), path)) {
        response.append(db);
    }
    return response;
}

QVariantList Eq8BandParametricDialogModel::passThroughResponse(int points) const
{
    const auto equalizer = std::dynamic_pointer_cast<Eq8BandParametric>(this->effect());
    if (!equalizer) {
        return {};
    }

    // Nothing to draw when the equalizer shapes both paths: a second curve lying exactly on the
    // first says only that there is a second curve.
    const auto bypassed = equalizer->isPathBypassed(Eq8BandParametric::Path::Mid)
      ? Eq8BandParametric::Path::Mid
      : (equalizer->isPathBypassed(Eq8BandParametric::Path::Side) ? Eq8BandParametric::Path::Side : std::optional<Eq8BandParametric::Path> {});
    if (!bypassed) {
        return {};
    }

    return sampleResponse(points, [&equalizer, path = *bypassed](double hz) { return equalizer->magnitudeDbAt(hz, path); });
}

void Eq8BandParametricDialogModel::setBandType(quint32 bandIndex, int bandType)
{
    const auto typeKey = Constants::NahdXml::xmlKeyBandType(bandIndex);
    if (static_cast<int>(std::round(parameterValue(typeKey))) == bandType) {
        return;
    }

    setParameterValue(typeKey, static_cast<float>(bandType));

    if (const auto q = Eq8BandParametric::defaultQParameterValue(static_cast<SvfFilter::Type>(bandType)); q) {
        setParameterValue(Constants::NahdXml::xmlKeyBandQ(bandIndex), *q);
    }
}

} // namespace noteahead
