// This file is part of Noteahead.
// Copyright (C) 2024 Jussi Lind <jussi.lind@iki.fi>
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

#ifndef COLUMN_HPP
#define COLUMN_HPP

#include <memory>
#include <vector>

#include "mixer_unit.hpp"

class QXmlStreamWriter;

namespace noteahead {

class Event;
class InstrumentSettings;
class Line;
class NoteData;
struct Position;

class Column : public MixerUnit
{
public:
    Column(size_t index, size_t length);

    bool hasData() const;
    bool hasPosition(const Position & position) const;

    size_t lineCount() const;
    void setLineCount(size_t lineCount);

    using LineS = std::shared_ptr<Line>;
    void addOrReplaceLine(LineS line);

    Position nextNoteDataOnSameColumn(const Position & position) const;
    Position prevNoteDataOnSameColumn(const Position & position) const;

    using NoteDataS = std::shared_ptr<NoteData>;
    NoteDataS noteDataAtPosition(const Position & position) const;
    void setNoteDataAtPosition(const NoteData & noteData, const Position & position);

    using PositionList = std::vector<Position>;
    PositionList deleteNoteDataAtPosition(const Position & position);
    PositionList insertNoteDataAtPosition(const NoteData & noteData, const Position & position);

    PositionList transposeColumn(const Position & position, int semitones);

    using EventS = std::shared_ptr<Event>;
    using EventList = std::vector<EventS>;
    EventList renderToEvents(size_t startTick, size_t ticksPerLine) const;

    using InstrumentSettingsS = std::shared_ptr<InstrumentSettings>;
    InstrumentSettingsS instrumentSettings(const Position & position) const;
    void setInstrumentSettings(const Position & position, InstrumentSettingsS instrumentSettings);

    void serializeToXml(QXmlStreamWriter & writer) const;

private:
    void initialize(size_t length);

    size_t m_virtualLineCount = 0;

    std::vector<LineS> m_lines;
};

} // namespace noteahead

#endif // COLUMN_HPP
