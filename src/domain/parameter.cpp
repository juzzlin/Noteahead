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

Parameter::Parameter(const std::string & name, float internalValue, int xmlMin, int xmlMax, int xmlDefault, int xmlScale, bool discrete)
  : m_name { name }
  , m_xmlMin { xmlMin }
  , m_xmlMax { xmlMax }
  , m_xmlDefault { xmlDefault }
  , m_xmlScale { xmlScale }
  , m_discrete { discrete }
{
    setValue(internalValue);
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
    if (m_discrete) {
        m_value = std::clamp(val, static_cast<float>(std::min(m_xmlMin, m_xmlMax)), static_cast<float>(std::max(m_xmlMin, m_xmlMax)));
    } else {
        m_value = std::clamp(val, 0.0f, 1.0f);
    }
}

int Parameter::xmlValue() const
{
    if (m_discrete) {
        return static_cast<int>(std::round(m_value));
    }
    return internalToXmlValue(m_value, m_xmlMin, m_xmlMax);
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

bool Parameter::isDiscrete() const
{
    return m_discrete;
}

void Parameter::setFromXml(int xmlVal)
{
    if (m_discrete) {
        m_value = static_cast<float>(std::clamp(xmlVal, std::min(m_xmlMin, m_xmlMax), std::max(m_xmlMin, m_xmlMax)));
    } else {
        m_value = xmlValueToInternal(xmlVal, m_xmlMin, m_xmlMax);
    }
}

void Parameter::reset()
{
    setFromXml(m_xmlDefault);
}

float Parameter::xmlValueToInternal(int xmlVal, int xmlMin, int xmlMax)
{
    const int clamped = std::clamp(xmlVal, std::min(xmlMin, xmlMax), std::max(xmlMin, xmlMax));
    if (const int range = xmlMax - xmlMin; range != 0) {
        return static_cast<float>(clamped - xmlMin) / static_cast<float>(range);
    }
    return 0.0f;
}

int Parameter::internalToXmlValue(float value, int xmlMin, int xmlMax)
{
    return static_cast<int>(std::round(std::clamp(value, 0.0f, 1.0f) * static_cast<float>(xmlMax - xmlMin))) + xmlMin;
}

} // namespace noteahead
