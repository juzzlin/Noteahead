// This file is part of Cacophony.
// Copyright (C) 2024 Jussi Lind <jussi.lind@iki.fi>
//
// Cacophony is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// Cacophony is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Cacophony. If not, see <http://www.gnu.org/licenses/>.

#ifndef PATTERN_HPP
#define PATTERN_HPP

#include <memory>
#include <vector>

class QXmlStreamWriter;

namespace cacophony {

class Event;
class Track;
class NoteData;
struct Position;

class Pattern
{
public:
    Pattern(uint32_t length, uint32_t trackCount);

    uint32_t columnCount(uint32_t trackId) const;

    uint32_t lineCount() const;

    uint32_t trackCount() const;

    std::string trackName(uint32_t trackId) const;

    void setTrackName(uint32_t trackId, std::string name);

    using NoteDataS = std::shared_ptr<NoteData>;

    NoteDataS noteDataAtPosition(const Position & position) const;

    void setNoteDataAtPosition(const NoteData & noteData, const Position & position) const;

    using EventS = std::shared_ptr<Event>;
    using EventList = std::vector<EventS>;
    EventList renderToEvents(size_t startTick, size_t ticksPerLine) const;

    void serializeToXml(QXmlStreamWriter & writer) const;

private:
    void initialize(uint32_t length, uint32_t trackCount);

    std::vector<std::shared_ptr<Track>> m_tracks;
};

} // namespace cacophony

#endif // PATTERN_HPP
