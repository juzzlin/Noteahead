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

Parameter::Parameter(const std::string & name, float internalValue, int xmlMin, int xmlMax, int xmlDefault, int xmlScale, Type type, LegacyNameList legacyNames, LegacyValueConverter legacyValueConverter)
  : m_name { name }
  , m_xmlMin { xmlMin }
  , m_xmlMax { xmlMax }
  , m_xmlDefault { xmlDefault }
  , m_xmlScale { xmlScale }
  , m_type { type }
  , m_legacyNames { std::move(legacyNames) }
  , m_legacyValueConverter { std::move(legacyValueConverter) }
{
    setValue(internalValue);
}

const std::string & Parameter::name() const
{
    return m_name;
}

const LegacyNameList & Parameter::legacyNames() const
{
    return m_legacyNames;
}

const LegacyValueConverter & Parameter::legacyValueConverter() const
{
    return m_legacyValueConverter;
}

float Parameter::clampToRange(float val) const
{
    if (m_type == Type::Discrete) {
        return std::clamp(val, static_cast<float>(std::min(m_xmlMin, m_xmlMax)), static_cast<float>(std::max(m_xmlMin, m_xmlMax)));
    }
    if (m_type == Type::Boolean) {
        return val > 0.5f ? 1.0f : 0.0f;
    }
    return std::clamp(val, 0.0f, 1.0f);
}

float Parameter::value() const
{
    return m_value;
}

float Parameter::authoredValue() const
{
    return m_authoredValue;
}

bool Parameter::isAutomated() const
{
    return m_automated;
}

void Parameter::setValue(float val)
{
    m_value = clampToRange(val);
    m_authoredValue = m_value;
    // Whatever automation had written here is gone: the user just said what this value is.
    m_automated = false;
}

void Parameter::setAutomationValue(float val)
{
    m_value = clampToRange(val);
    m_automated = true;
}

void Parameter::clearAutomation()
{
    if (m_automated) {
        m_value = m_authoredValue;
        m_automated = false;
    }
}

bool Parameter::update(float val)
{
    const float oldValue = m_value;
    setValue(val);
    return std::abs(m_value - oldValue) > 0.0001f;
}

int Parameter::xmlValueOf(float value) const
{
    if (m_type == Type::Discrete || m_type == Type::Boolean) {
        return static_cast<int>(std::round(value));
    }
    return internalToXmlValue(value, m_xmlMin, m_xmlMax);
}

int Parameter::xmlValue() const
{
    return xmlValueOf(m_value);
}

int Parameter::xmlAuthoredValue() const
{
    return xmlValueOf(m_authoredValue);
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

float Parameter::defaultValue() const
{
    if (m_type == Type::Discrete || m_type == Type::Boolean) {
        return clampToRange(static_cast<float>(m_xmlDefault));
    }
    return xmlValueToInternal(m_xmlDefault, m_xmlMin, m_xmlMax);
}

int Parameter::xmlScale() const
{
    return m_xmlScale;
}

Parameter::Type Parameter::type() const
{
    return m_type;
}

bool Parameter::isDiscrete() const
{
    return m_type == Type::Discrete;
}

bool Parameter::isBoolean() const
{
    return m_type == Type::Boolean;
}

void Parameter::setFromXml(int xmlVal, std::optional<int> xmlMin, std::optional<int> xmlMax)
{
    const int min = xmlMin.value_or(m_xmlMin);
    const int max = xmlMax.value_or(m_xmlMax);
    if (m_type == Type::Discrete || m_type == Type::Boolean) {
        m_value = static_cast<float>(std::clamp(xmlVal, std::min(min, max), std::max(min, max)));
    } else {
        m_value = xmlValueToInternal(xmlVal, min, max);
    }
    // Loading a project authors every value it carries.
    m_authoredValue = m_value;
    m_automated = false;
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
