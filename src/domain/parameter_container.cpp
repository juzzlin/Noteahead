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

#include "parameter_container.hpp"
#include "../common/constants.hpp"

#include <QXmlStreamReader>
#include <QXmlStreamWriter>

namespace noteahead {

ParameterContainer::~ParameterContainer() = default;

void ParameterContainer::addParameter(Parameter parameter)
{
    const auto name = parameter.name();
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

void ParameterContainer::serializeParametersToXml(QXmlStreamWriter & writer) const
{
    for (const auto & [name, p] : m_parameters) {
        writer.writeStartElement(Constants::NahdXml::xmlKeyParameter());
        writer.writeAttribute(Constants::NahdXml::xmlKeyName(), QString::fromStdString(name));
        writer.writeAttribute(Constants::NahdXml::xmlKeyValue(), QString::number(p.xmlValue()));
        writer.writeAttribute(Constants::NahdXml::xmlKeyMin(), QString::number(p.xmlMin()));
        writer.writeAttribute(Constants::NahdXml::xmlKeyMax(), QString::number(p.xmlMax()));
        writer.writeAttribute(Constants::NahdXml::xmlKeyDefault(), QString::number(p.xmlDefault()));
        writer.writeAttribute(Constants::NahdXml::xmlKeyScale(), QString::number(p.xmlScale()));
        writer.writeEndElement();
    }
}

void ParameterContainer::deserializeParametersFromXml(QXmlStreamReader & reader)
{
    while (reader.readNextStartElement()) {
        if (reader.name() == Constants::NahdXml::xmlKeyParameter()) {
            const auto name = reader.attributes().value(Constants::NahdXml::xmlKeyName()).toString().toStdString();
            const auto value = reader.attributes().value(Constants::NahdXml::xmlKeyValue()).toInt();
            if (auto p = parameter(name); p) {
                p->get().setFromXml(value);
            }
            reader.skipCurrentElement();
        } else {
            reader.skipCurrentElement();
        }
    }
}

} // namespace noteahead
