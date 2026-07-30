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

    const std::map<std::string, std::string> & tags() const;

    //! How this song renders to audio. Kept here so that everything about a song that is not notes
    //! travels together, and so that Song needs no separate member for it.
    const RenderSettings & renderSettings() const;
    RenderSettings & renderSettings();
    void setTag(const std::string & name, const std::string & value);
    void removeTag(const std::string & name);
    void clear();

    void serializeToXml(ProjectWriter & writer) const;
    void deserializeFromXml(ProjectReader & reader);

private:
    std::map<std::string, std::string> m_tags;
    RenderSettings m_renderSettings;
};

} // namespace noteahead

#endif // METADATA_HPP
