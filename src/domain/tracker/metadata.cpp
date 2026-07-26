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

#include "metadata.hpp"

#include "../../common/constants.hpp"
#include "../../common/xml/project_reader.hpp"
#include "../../common/xml/project_writer.hpp"

#include <QVariant>

namespace noteahead {

const std::map<std::string, std::string> & Metadata::tags() const
{
    return m_tags;
}

void Metadata::setTag(const std::string & name, const std::string & value)
{
    m_tags[name] = value;
}

void Metadata::removeTag(const std::string & name)
{
    m_tags.erase(name);
}

void Metadata::clear()
{
    m_tags.clear();
}

void Metadata::serializeToXml(ProjectWriter & writer) const
{
    writer.writeStartElement(Constants::NahdXml::xmlKeyMetadata());

    if (!m_tags.empty()) {
        writer.writeStartElement(Constants::NahdXml::xmlKeyTags());
        for (const auto & [name, value] : m_tags) {
            writer.writeStartElement(Constants::NahdXml::xmlKeyTag());
            writer.writeAttribute(Constants::NahdXml::xmlKeyName(), QString::fromStdString(name));
            writer.writeAttribute(Constants::NahdXml::xmlKeyValue(), QString::fromStdString(value));
            writer.writeEndElement(); // Tag
        }
        writer.writeEndElement(); // Tags
    }

    writer.writeEndElement(); // Metadata
}

void Metadata::deserializeFromXml(ProjectReader & reader)
{
    // Make sure we clear existing data before loading
    clear();

    while (!(reader.isEndElement() && !reader.name().compare(Constants::NahdXml::xmlKeyMetadata()))) {
        if (reader.isStartElement() && !reader.name().compare(Constants::NahdXml::xmlKeyTags())) {
            // Read Tags
            while (!(reader.isEndElement() && !reader.name().compare(Constants::NahdXml::xmlKeyTags()))) {
                if (reader.isStartElement() && !reader.name().compare(Constants::NahdXml::xmlKeyTag())) {
                    const auto name = reader.attribute(Constants::NahdXml::xmlKeyName()).toString().toStdString();
                    const auto value = reader.attribute(Constants::NahdXml::xmlKeyValue()).toString().toStdString();
                    if (!name.empty()) {
                        setTag(name, value);
                    }
                }
                reader.readNext();
            }
        }
        reader.readNext();
    }
}

} // namespace noteahead
