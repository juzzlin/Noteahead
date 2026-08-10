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

#include "interpolator.hpp"

#include "../../common/constants.hpp"

#include <map>

namespace noteahead {

Interpolator::Interpolator(size_t startLine, size_t endLine, double startValue, double endValue)
  : Interpolator { startLine, endLine, startValue, endValue, CurveType::Linear }
{
}

Interpolator::Interpolator(size_t startLine, size_t endLine, double startValue, double endValue, CurveType curve)
  : m_startLine { startLine }
  , m_endLine { endLine }
  , m_startValue { startValue }
  , m_endValue { endValue }
  , m_curve { curve }
{
}

Interpolator::CurveType Interpolator::curve() const
{
    return m_curve;
}

double Interpolator::applyCurve(CurveType curve, double t)
{
    switch (curve) {
    case CurveType::Exponential:
        return t * t;
    case CurveType::Logarithmic:
        return t * (2 - t);
    case CurveType::EaseIn:
        return t * t * t;
    case CurveType::EaseOut: {
        const double u = 1 - t;
        return 1 - u * u * u;
    }
    case CurveType::EaseInOut:
        return t * t * (3 - 2 * t);
    case CurveType::Linear:
        break;
    }
    return t;
}

QString Interpolator::curveToXmlValue(CurveType curve)
{
    switch (curve) {
    case CurveType::Exponential:
        return Constants::NahdXml::xmlValueExponential();
    case CurveType::Logarithmic:
        return Constants::NahdXml::xmlValueLogarithmic();
    case CurveType::EaseIn:
        return Constants::NahdXml::xmlValueEaseIn();
    case CurveType::EaseOut:
        return Constants::NahdXml::xmlValueEaseOut();
    case CurveType::EaseInOut:
        return Constants::NahdXml::xmlValueEaseInOut();
    case CurveType::Linear:
        break;
    }
    return Constants::NahdXml::xmlValueLinear();
}

Interpolator::CurveType Interpolator::curveFromXmlValue(QString value)
{
    static const std::map<QString, CurveType> curveByName = {
        { Constants::NahdXml::xmlValueLinear(), CurveType::Linear },
        { Constants::NahdXml::xmlValueExponential(), CurveType::Exponential },
        { Constants::NahdXml::xmlValueLogarithmic(), CurveType::Logarithmic },
        { Constants::NahdXml::xmlValueEaseIn(), CurveType::EaseIn },
        { Constants::NahdXml::xmlValueEaseOut(), CurveType::EaseOut },
        { Constants::NahdXml::xmlValueEaseInOut(), CurveType::EaseInOut }
    };
    if (const auto iter = curveByName.find(value); iter != curveByName.end()) {
        return iter->second;
    }
    return CurveType::Linear;
}

double Interpolator::getValue(size_t line) const
{
    if (line <= m_startLine) {
        return m_startValue;
    }

    if (line >= m_endLine) {
        return m_endValue;
    }

    if (m_startLine == m_endLine) {
        return m_startValue;
    }

    const double t = static_cast<double>(line - m_startLine) / static_cast<double>(m_endLine - m_startLine);
    const double d = m_endValue - m_startValue;
    return m_startValue + applyCurve(m_curve, t) * d;
}

} // namespace noteahead
