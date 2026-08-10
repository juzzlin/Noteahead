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
// You should have received a copy of the GNU General Public License
// along with Noteahead. If not, see <http://www.gnu.org/licenses/>.

#ifndef INTERPOLATOR_HPP
#define INTERPOLATOR_HPP

#include <QString>

#include <cstddef>

namespace noteahead {

class Interpolator
{
public:
    //! The shape of the ramp between the start and end value. Serialized by name, so the order is free.
    enum class CurveType
    {
        Linear,
        Exponential,
        Logarithmic,
        EaseIn,
        EaseOut,
        EaseInOut
    };

    Interpolator(size_t startLine, size_t endLine, double startValue, double endValue);
    Interpolator(size_t startLine, size_t endLine, double startValue, double endValue, CurveType curve);

    double getValue(size_t line) const;

    CurveType curve() const;

    //! Maps t in [0, 1] to the shaped position on the ramp.
    static double applyCurve(CurveType curve, double t);

    static QString curveToXmlValue(CurveType curve);
    static CurveType curveFromXmlValue(QString value);

private:
    size_t m_startLine = 0;
    size_t m_endLine = 0;

    double m_startValue = 0;
    double m_endValue = 0;

    CurveType m_curve = CurveType::Linear;
};

} // namespace noteahead

#endif // INTERPOLATOR_HPP
