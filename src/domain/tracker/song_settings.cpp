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

#include "song_settings.hpp"

#include "../../common/constants.hpp"
#include "../../common/xml/project_writer.hpp"

namespace noteahead {

const std::optional<AutoNoteOffOffset> & SongSettings::autoNoteOffOffset() const
{
    return m_autoNoteOffOffset;
}

void SongSettings::setAutoNoteOffOffset(const AutoNoteOffOffset & offset)
{
    m_autoNoteOffOffset = offset;
}

void SongSettings::serializeToXml(ProjectWriter & writer) const
{
    if (!m_autoNoteOffOffset.has_value()) {
        return;
    }

    writer.writeStartElement(Constants::NahdXml::xmlKeySongSettings());

    m_autoNoteOffOffset->serializeToXmlAttributes(writer);

    writer.writeEndElement(); // SongSettings
}

void SongSettings::deserializeFromXml(ProjectReader & reader)
{
    m_autoNoteOffOffset = AutoNoteOffOffset::deserializeFromXmlAttributes(reader);
}

} // namespace noteahead
