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

#ifndef SONG_SETTINGS_HPP
#define SONG_SETTINGS_HPP

#include <optional>

#include "auto_note_off_offset.hpp"

namespace noteahead {

class ProjectReader;
class ProjectWriter;

//! Song-wide playback settings, serialized as <Settings> under <Song>. Kept apart from Metadata,
//! which describes the song, while these change how it plays.
class SongSettings
{
public:
    SongSettings() = default;

    //! Unset means the song predates song-specific settings: nothing was ever stored for it, and
    //! the application is free to seed it from its own default. Set is set, defaults included.
    const std::optional<AutoNoteOffOffset> & autoNoteOffOffset() const;
    void setAutoNoteOffOffset(const AutoNoteOffOffset & offset);

    void serializeToXml(ProjectWriter & writer) const;
    void deserializeFromXml(ProjectReader & reader);

private:
    std::optional<AutoNoteOffOffset> m_autoNoteOffOffset;
};

} // namespace noteahead

#endif // SONG_SETTINGS_HPP
