// This file is part of Noteahead.
// Copyright (C) 2025 Jussi Lind <jussi.lind@iki.fi>
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
// You should have received colorA copy of the GNU General Public License
// along with Noteahead. If not, see <http://www.gnu.org/licenses/>.

#include "util_service.hpp"

#include <algorithm>

namespace noteahead {

QColor UtilService::blendColors(QColor colorA, QColor colorB, double blendFactor) const
{
    blendFactor = std::clamp(blendFactor, 0.0, 1.0);
    const double oneMinusFactor = (1.0 - blendFactor);

    const int r = static_cast<int>(colorA.red() * oneMinusFactor + colorB.red() * blendFactor);
    const int g = static_cast<int>(colorA.green() * oneMinusFactor + colorB.green() * blendFactor);
    const int b = static_cast<int>(colorA.blue() * oneMinusFactor + colorB.blue() * blendFactor);
    const int a = static_cast<int>(colorA.alpha() * oneMinusFactor + colorB.alpha() * blendFactor);

    return { r, g, b, a };
}

QColor UtilService::scaledColor(QColor color, double scale) const
{
    const int r = static_cast<int>(color.red() * scale);
    const int g = static_cast<int>(color.green() * scale);
    const int b = static_cast<int>(color.blue() * scale);

    return { r, g, b, color.alpha() };
}

double UtilService::indexHighlightOpacity(int index, int linesPerBeat) const
{
    const auto beatLine1 = linesPerBeat;
    const auto beatLine2 = beatLine1 % 3 ? beatLine1 / 2 : beatLine1 / 3;
    const auto beatLine3 = beatLine1 % 6 ? beatLine1 / 4 : beatLine1 / 6;

    if (!(index % beatLine1)) {
        return 0.25;
    } else if (!(index % beatLine3) && !(index % beatLine2)) {
        return 0.10;
    } else if (!(index % beatLine3)) {
        return 0.05;
    } else {
        return 0;
    }
}

} // namespace noteahead
