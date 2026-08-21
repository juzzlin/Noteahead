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

#ifndef METADATA_HPP
#define METADATA_HPP

#include "render_settings.hpp"

#include <map>
#include <string>

namespace noteahead {

class ProjectReader;
class ProjectWriter;

class Metadata
{
public:
    Metadata() = default;

    using TagMap = std::map<std::string, std::string>;

    //! What the song is: title, composer, artist and so on. Edited in the song metadata dialog.
    const TagMap & tags() const;

    //! What the rendered audio file should claim. Separate from the song's own metadata so that
    //! a file can be tagged differently from the piece, and empty by default: an unset export tag
    //! falls back on the song's, which is what effectiveExportTags() resolves.
    const TagMap & exportTags() const;

    //! How this song renders to audio. Kept here so that everything about a song that is not notes
    //! travels together, and so that Song needs no separate member for it.
    const RenderSettings & renderSettings() const;
    RenderSettings & renderSettings();
    void setTag(const std::string & name, const std::string & value);
    void removeTag(const std::string & name);

    void setExportTag(const std::string & name, const std::string & value);
    void removeExportTag(const std::string & name);

    //! The export tag where one is set, the song's own tag otherwise. This is what reaches the
    //! audio file, and the only place the fallback rule lives.
    TagMap effectiveExportTags() const;

    void clear();

    void serializeToXml(ProjectWriter & writer) const;
    void deserializeFromXml(ProjectReader & reader);

private:
    TagMap m_tags;
    TagMap m_exportTags;
    RenderSettings m_renderSettings;
};

} // namespace noteahead

#endif // METADATA_HPP
