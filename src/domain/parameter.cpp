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

#include "parameter.hpp"

#include <algorithm>
#include <cmath>

namespace noteahead {

Parameter::Parameter(const std::string & name, float internalValue, int xmlMin, int xmlMax, int xmlDefault, int xmlScale)
  : m_name { name }
  , m_value { std::clamp(internalValue, 0.0f, 1.0f) }
  , m_xmlMin { xmlMin }
  , m_xmlMax { xmlMax }
  , m_xmlDefault { xmlDefault }
  , m_xmlScale { xmlScale }
{
}

const std::string & Parameter::name() const
{
    return m_name;
}

float Parameter::value() const
{
    return m_value;
}

void Parameter::setValue(float val)
{
    m_value = std::clamp(val, 0.0f, 1.0f);
}

int Parameter::xmlValue() const
{
    return static_cast<int>(std::round(m_value * static_cast<float>(m_xmlMax - m_xmlMin))) + m_xmlMin;
}

int Parameter::xmlMin() const
{
    return m_xmlMin;
}

int Parameter::xmlMax() const
{
    return m_xmlMax;
}

int Parameter::xmlDefault() const
{
    return m_xmlDefault;
}

int Parameter::xmlScale() const
{
    return m_xmlScale;
}

void Parameter::setFromXml(int xmlVal)
{
    const int clamped = std::clamp(xmlVal, std::min(m_xmlMin, m_xmlMax), std::max(m_xmlMin, m_xmlMax));
    if (const int range = m_xmlMax - m_xmlMin; range != 0) {
        m_value = static_cast<float>(clamped - m_xmlMin) / static_cast<float>(range);
    } else {
        m_value = 0.0f;
    }
}

void Parameter::reset()
{
    setFromXml(m_xmlDefault);
}

} // namespace noteahead
