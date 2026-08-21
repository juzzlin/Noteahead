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

const Metadata::TagMap & Metadata::tags() const
{
    return m_tags;
}

const Metadata::TagMap & Metadata::exportTags() const
{
    return m_exportTags;
}

const std::string & Metadata::notes() const
{
    return m_notes;
}

void Metadata::setNotes(const std::string & notes)
{
    m_notes = notes;
}

const RenderSettings & Metadata::renderSettings() const
{
    return m_renderSettings;
}

RenderSettings & Metadata::renderSettings()
{
    return m_renderSettings;
}

void Metadata::setTag(const std::string & name, const std::string & value)
{
    m_tags[name] = value;
}

void Metadata::removeTag(const std::string & name)
{
    m_tags.erase(name);
}

void Metadata::setExportTag(const std::string & name, const std::string & value)
{
    m_exportTags[name] = value;
}

void Metadata::removeExportTag(const std::string & name)
{
    m_exportTags.erase(name);
}

Metadata::TagMap Metadata::effectiveExportTags() const
{
    // Start from the song's own metadata so that a song that has never been exported still tags
    // its files sensibly, then let anything explicitly set for the export win.
    auto effective = m_tags;
    for (const auto & [name, value] : m_exportTags) {
        if (!value.empty()) {
            effective[name] = value;
        }
    }
    std::erase_if(effective, [](const auto & tag) { return tag.second.empty(); });
    return effective;
}

void Metadata::clear()
{
    m_tags.clear();
    m_exportTags.clear();
    m_notes.clear();
    m_renderSettings = RenderSettings {};
}

namespace {

//! Writes a <Tags>/<ExportTags> block, or nothing at all when there is nothing to write, so that
//! a project without export tags serializes exactly as it did before they existed.
void writeTags(ProjectWriter & writer, const QString & elementName, const Metadata::TagMap & tags)
{
    if (tags.empty()) {
        return;
    }

    writer.writeStartElement(elementName);
    for (const auto & [name, value] : tags) {
        writer.writeStartElement(Constants::NahdXml::xmlKeyTag());
        writer.writeAttribute(Constants::NahdXml::xmlKeyName(), QString::fromStdString(name));
        writer.writeAttribute(Constants::NahdXml::xmlKeyValue(), QString::fromStdString(value));
        writer.writeEndElement(); // Tag
    }
    writer.writeEndElement();
}

//! Reads the <Tag> children of the currently open element into the given map.
void readTags(ProjectReader & reader, const QString & elementName, Metadata::TagMap & tags)
{
    while (!(reader.isEndElement() && !reader.name().compare(elementName))) {
        if (reader.isStartElement() && !reader.name().compare(Constants::NahdXml::xmlKeyTag())) {
            const auto name = reader.attribute(Constants::NahdXml::xmlKeyName()).toString().toStdString();
            const auto value = reader.attribute(Constants::NahdXml::xmlKeyValue()).toString().toStdString();
            if (!name.empty()) {
                tags[name] = value;
            }
        }
        reader.readNext();
    }
}

} // namespace

void Metadata::serializeToXml(ProjectWriter & writer) const
{
    writer.writeStartElement(Constants::NahdXml::xmlKeyMetadata());

    writeTags(writer, Constants::NahdXml::xmlKeyTags(), m_tags);
    writeTags(writer, Constants::NahdXml::xmlKeyExportTags(), m_exportTags);

    // Omitted entirely when empty, so a project without notes serializes as it always did.
    if (!m_notes.empty()) {
        writer.writeStartElement(Constants::NahdXml::xmlKeyNotes());
        writer.writeCharacters(QString::fromStdString(m_notes));
        writer.writeEndElement(); // Notes
    }

    m_renderSettings.serializeToXml(writer);

    writer.writeEndElement(); // Metadata
}

void Metadata::deserializeFromXml(ProjectReader & reader)
{
    // Make sure we clear existing data before loading
    clear();

    while (!(reader.isEndElement() && !reader.name().compare(Constants::NahdXml::xmlKeyMetadata()))) {
        if (reader.isStartElement() && !reader.name().compare(Constants::NahdXml::xmlKeyRenderSettings())) {
            m_renderSettings.deserializeFromXml(reader);
        } else if (reader.isStartElement() && !reader.name().compare(Constants::NahdXml::xmlKeyTags())) {
            readTags(reader, Constants::NahdXml::xmlKeyTags(), m_tags);
        } else if (reader.isStartElement() && !reader.name().compare(Constants::NahdXml::xmlKeyExportTags())) {
            readTags(reader, Constants::NahdXml::xmlKeyExportTags(), m_exportTags);
        } else if (reader.isStartElement() && !reader.name().compare(Constants::NahdXml::xmlKeyNotes())) {
            // Element text rather than an attribute, so that newlines survive verbatim.
            // readElementText() leaves the reader on </Notes>, which the readNext() below steps
            // past like any other token.
            m_notes = reader.readElementText().toStdString();
        }
        reader.readNext();
    }
}

} // namespace noteahead
