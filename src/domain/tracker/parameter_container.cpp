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

#include "domain/tracker/parameter_container.hpp"

#include "common/constants.hpp"
#include "common/xml/project_reader.hpp"
#include "common/xml/project_writer.hpp"
#include "contrib/SimpleLogger/src/simple_logger.hpp"

#include <iomanip>

#include <QVariant>

namespace noteahead {

ParameterContainer::~ParameterContainer() = default;

void ParameterContainer::addParameter(Parameter parameter)
{
    const auto name = parameter.name();
    for (const auto & legacyName : parameter.legacyNames()) {
        m_legacyNameMap[legacyName] = name;
    }
    m_parameters.emplace(name, std::move(parameter));
}

ParameterContainer::ParameterOpt ParameterContainer::parameter(const std::string & name)
{
    if (const auto it = m_parameters.find(name); it != m_parameters.end()) {
        return std::ref(it->second);
    }
    return std::nullopt;
}

ParameterContainer::ConstParameterOpt ParameterContainer::parameter(const std::string & name) const
{
    if (const auto it = m_parameters.find(name); it != m_parameters.end()) {
        return std::cref(it->second);
    }
    return std::nullopt;
}

std::map<std::string, Parameter> & ParameterContainer::parameters()
{
    return m_parameters;
}

const std::map<std::string, Parameter> & ParameterContainer::parameters() const
{
    return m_parameters;
}

void ParameterContainer::reset()
{
    for (auto && [name, p] : m_parameters) {
        p.reset();
    }
}

void ParameterContainer::serializeParametersToXml(ProjectWriter & writer) const
{
    for (const auto & [name, p] : m_parameters) {
        writer.writeStartElement(Constants::NahdXml::xmlKeyParameter());
        writer.writeAttribute(Constants::NahdXml::xmlKeyName(), QString::fromStdString(name));

        if (p.type() == Parameter::Type::Continuous) {
            writer.writeAttribute(Constants::NahdXml::xmlKeyParameterValueType(), Constants::NahdXml::xmlValueFloat());
            writer.writeAttribute(Constants::NahdXml::xmlKeyValue(), QString::number(p.xmlValue()));
            writer.writeAttribute(Constants::NahdXml::xmlKeyMin(), QString::number(p.xmlMin()));
            writer.writeAttribute(Constants::NahdXml::xmlKeyMax(), QString::number(p.xmlMax()));
            writer.writeAttribute(Constants::NahdXml::xmlKeyDefault(), QString::number(p.xmlDefault()));
            writer.writeAttribute(Constants::NahdXml::xmlKeyScale(), QString::number(p.xmlScale()));
        } else if (p.type() == Parameter::Type::Discrete) {
            writer.writeAttribute(Constants::NahdXml::xmlKeyParameterValueType(), Constants::NahdXml::xmlValueInt());
            writer.writeAttribute(Constants::NahdXml::xmlKeyValue(), QString::number(p.xmlValue()));
        } else if (p.type() == Parameter::Type::Boolean) {
            writer.writeAttribute(Constants::NahdXml::xmlKeyParameterValueType(), Constants::NahdXml::xmlValueBool());
            writer.writeAttribute(Constants::NahdXml::xmlKeyValue(), p.value() > 0.5f ? Constants::NahdXml::xmlValueTrue() : Constants::NahdXml::xmlValueFalse());
        }

        writer.writeEndElement();
    }
}

void ParameterContainer::deserializeParametersFromXml(ProjectReader & reader)
{
    while (reader.readNextStartElement()) {
        if (reader.name() == Constants::NahdXml::xmlKeyParameter()) {
            deserializeParameter(reader);
        } else {
            reader.skipCurrentElement();
        }
    }
}

void ParameterContainer::deserializeParameter(ProjectReader & reader)
{
    static const std::string TAG { "ParameterContainer" };
    auto name = reader.attribute(Constants::NahdXml::xmlKeyName()).toString().toStdString();

    if (const auto it = m_legacyNameMap.find(name); it != m_legacyNameMap.end()) {
        juzzlin::L(TAG).warning() << "Mapping legacy parameter name " << std::quoted(name) << " to " << std::quoted(it->second);
        name = it->second;
    }

    const auto valueType = reader.attribute(Constants::NahdXml::xmlKeyParameterValueType()).toString();
    const auto xmlValueStr = reader.attribute(Constants::NahdXml::xmlKeyValue()).toString();

    std::optional<int> xmlMin;
    if (const auto minAttr = reader.attribute(Constants::NahdXml::xmlKeyMin()); minAttr.isValid()) {
        xmlMin = minAttr.toInt();
    }
    std::optional<int> xmlMax;
    if (const auto maxAttr = reader.attribute(Constants::NahdXml::xmlKeyMax()); maxAttr.isValid()) {
        xmlMax = maxAttr.toInt();
    }

    if (auto p = parameter(name); p) {
        if (valueType == Constants::NahdXml::xmlValueInt() || valueType == Constants::NahdXml::xmlValueFloat()) {
            p->get().setFromXml(xmlValueStr.toInt(), xmlMin, xmlMax);
        } else if (valueType == Constants::NahdXml::xmlValueBool()) {
            p->get().setValue((xmlValueStr == Constants::NahdXml::xmlValueTrue() || xmlValueStr == "1") ? 1.0f : 0.0f);
        } else {
            // Fallback for older files
            p->get().setFromXml(xmlValueStr.toInt(), xmlMin, xmlMax);
        }
    }
    reader.skipCurrentElement();
}

} // namespace noteahead
