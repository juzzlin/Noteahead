// This fi777le is part of Cacophony.
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

#include "song.hpp"

#include "../application/position.hpp"
#include "../common/constants.hpp"
#include "../contrib/SimpleLogger/src/simple_logger.hpp"
#include "../domain/event.hpp"
#include "../domain/note_data.hpp"
#include "column.hpp"
#include "line.hpp"
#include "pattern.hpp"
#include "track.hpp"

#include <QXmlStreamReader>
#include <QXmlStreamWriter>

namespace cacophony {

static const auto TAG = "Song";

Song::Song()
{
    initialize();
}

uint32_t Song::columnCount(uint32_t trackId) const
{
    return m_patterns.at(0)->columnCount(trackId);
}

uint32_t Song::lineCount(uint32_t patternId) const
{
    return m_patterns.at(patternId)->lineCount();
}

uint32_t Song::patternCount() const
{
    return static_cast<uint32_t>(m_patterns.size());
}

uint32_t Song::trackCount() const
{
    return m_patterns.at(0)->trackCount();
}

bool Song::hasData() const
{
    return std::find_if(m_patterns.begin(), m_patterns.end(), [](auto && pattern) {
               return pattern->hasData();
           })
      != m_patterns.end();
}

std::string Song::trackName(uint32_t trackId) const
{
    return m_patterns.at(0)->trackName(trackId);
}

void Song::setTrackName(uint32_t trackId, std::string name)
{
    m_patterns.at(0)->setTrackName(trackId, name);
}

std::string Song::fileName() const
{
    return m_fileName;
}

void Song::setFileName(std::string fileName)
{
    m_fileName = fileName;
}

Song::NoteDataS Song::noteDataAtPosition(const Position & position) const
{
    return m_patterns.at(position.pattern)->noteDataAtPosition(position);
}

void Song::setNoteDataAtPosition(const NoteData & noteData, const Position & position)
{
    juzzlin::L(TAG).trace() << "Set note data at position: " << noteData.toString() << " @ " << position.toString();
    m_patterns.at(position.pattern)->setNoteDataAtPosition(noteData, position);
}

void Song::initialize()
{
    m_patterns.clear();
    m_patterns.push_back(std::make_shared<Pattern>(0, 64, 8));
}

uint32_t Song::linesPerBeat() const
{
    return m_linesPerBeat;
}

void Song::setLinesPerBeat(uint32_t lpb)
{
    m_linesPerBeat = lpb;
}

uint32_t Song::ticksPerLine() const
{
    return m_ticksPerLine;
}

Song::PatternAndLineOpt Song::patternAndLineByTick(uint32_t tick) const
{
    if (const auto iter = m_tickToPatternAndLineMap.find(tick); iter != m_tickToPatternAndLineMap.end()) {
        return iter->second;
    } else {
        return {};
    }
}

uint32_t Song::beatsPerMinute() const
{
    return m_beatsPerMinute;
}

void Song::setBeatsPerMinute(uint32_t bpm)
{
    m_beatsPerMinute = bpm;
}

Song::EventList Song::introduceNoteOffs(const EventList & events) const
{
    Song::EventList processedEvents;
    std::map<std::pair<int, int>, int> activeNotes; // Tracks active notes (key: {track, column}, value: note)

    for (const auto & event : events) {
        if (const auto noteData = event->noteData(); noteData) {
            const auto trackColumn = std::make_pair(noteData->track(), noteData->column());
            if (noteData->type() == NoteData::Type::NoteOn) {
                if (activeNotes.contains(trackColumn)) {
                    const auto activeNote = activeNotes[trackColumn];
                    const auto noteData = std::make_shared<NoteData>(trackColumn.first, trackColumn.second);
                    noteData->setAsNoteOff(static_cast<uint8_t>(activeNote));
                    processedEvents.push_back(std::make_shared<Event>(event->tick() - 1, noteData));
                }
                activeNotes[trackColumn] = noteData->note();
            } else if (noteData->type() == NoteData::Type::NoteOff) {
                activeNotes.erase(trackColumn);
            }
        }

        processedEvents.push_back(event);
    }

    for (const auto & [trackColumn, note] : activeNotes) {
        const auto noteData = std::make_shared<NoteData>(trackColumn.first, trackColumn.second);
        noteData->setAsNoteOff(static_cast<uint8_t>(note));
        processedEvents.push_back(std::make_shared<Event>(events.back()->tick() + 1, noteData));
    }

    return processedEvents;
}

void Song::updateTickToPatternAndLineMapping(size_t tick, size_t patternIndex, size_t patternLineCount)
{
    for (size_t lineIndex = 0; lineIndex < patternLineCount; lineIndex++) {
        m_tickToPatternAndLineMap[tick + lineIndex * m_ticksPerLine] = std::make_pair(patternIndex, lineIndex);
    }
}

Song::EventList Song::renderToEvents()
{
    m_tickToPatternAndLineMap.clear();

    Song::EventList eventList;
    size_t tick = 0;

    const auto startOfSongEvent = std::make_shared<Event>(tick);
    startOfSongEvent->setAsStartOfSong();
    eventList.push_back(startOfSongEvent);

    for (size_t patternIndex = 0; patternIndex < m_patterns.size(); patternIndex++) {
        const auto & pattern = m_patterns[patternIndex];
        const auto patternEventList = pattern->renderToEvents(tick, m_ticksPerLine);
        std::copy(patternEventList.begin(), patternEventList.end(), std::back_inserter(eventList));
        updateTickToPatternAndLineMapping(tick, patternIndex, pattern->lineCount());
        tick += pattern->lineCount() * m_ticksPerLine;
    }

    const auto endOfSongEvent = std::make_shared<Event>(tick);
    endOfSongEvent->setAsEndOfSong();
    eventList.push_back(endOfSongEvent);

    eventList = introduceNoteOffs(eventList);

    return eventList;
}

void Song::serializeToXml(QXmlStreamWriter & writer) const
{
    writer.writeStartElement(Constants::xmlKeySong());

    writer.writeAttribute(Constants::xmlKeyBeatsPerMinute(), QString::number(m_beatsPerMinute));
    writer.writeAttribute(Constants::xmlKeyLinesPerBeat(), QString::number(m_linesPerBeat));

    writer.writeStartElement(Constants::xmlKeyPatterns());

    for (const auto & pattern : m_patterns) {
        if (pattern) {
            pattern->serializeToXml(writer);
        }
    }

    writer.writeEndElement(); // Patterns
    writer.writeEndElement(); // Song
}

uint32_t readUIntAttribute(QXmlStreamReader & reader, QString name)
{
    if (!reader.attributes().hasAttribute(name)) {
        throw std::runtime_error { "Attribute " + name.toStdString() + " not found!" };
    } else {
        return reader.attributes().value(name).toUInt();
    }
}

QString readStringAttribute(QXmlStreamReader & reader, QString name)
{
    if (!reader.attributes().hasAttribute(name)) {
        throw std::runtime_error { "Attribute " + name.toStdString() + " not found!" };
    } else {
        return reader.attributes().value(name).toString();
    }
}

void Song::deserializeFromXml(QXmlStreamReader & reader)
{
    setBeatsPerMinute(readUIntAttribute(reader, Constants::xmlKeyBeatsPerMinute()));
    setLinesPerBeat(readUIntAttribute(reader, Constants::xmlKeyLinesPerBeat()));

    juzzlin::L(TAG).trace() << "Reading Song started";
    while (!(reader.isEndElement() && !reader.name().compare(Constants::xmlKeySong()))) {
        juzzlin::L(TAG).trace() << "Current element: " << reader.name().toString().toStdString();
        if (reader.isStartElement() && !reader.name().compare(Constants::xmlKeyPatterns())) {
            deserializePatterns(reader);
        }
        reader.readNext();
    }
    juzzlin::L(TAG).trace() << "Reading Song ended";
}

void Song::deserializePatterns(QXmlStreamReader & reader)
{
    juzzlin::L(TAG).trace() << "Reading Patterns started";
    while (!(reader.isEndElement() && !reader.name().compare(Constants::xmlKeyPatterns()))) {
        juzzlin::L(TAG).trace() << "Patterns: Current element: " << reader.name().toString().toStdString();
        if (reader.isStartElement() && !reader.name().compare(Constants::xmlKeyPattern())) {
            const auto pattern = deserializePattern(reader);
            m_patterns[pattern->index()] = pattern;
        }
        reader.readNext();
    }
    juzzlin::L(TAG).trace() << "Reading Patterns ended";
}

Song::PatternS Song::deserializePattern(QXmlStreamReader & reader)
{
    juzzlin::L(TAG).trace() << "Reading Pattern started";
    const auto index = readUIntAttribute(reader, Constants::xmlKeyIndex());
    const auto lineCount = readUIntAttribute(reader, Constants::xmlKeyLineCount());
    const auto trackCount = readUIntAttribute(reader, Constants::xmlKeyTrackCount());
    const auto pattern = std::make_shared<Pattern>(index, lineCount, trackCount);
    while (!(reader.isEndElement() && !reader.name().compare(Constants::xmlKeyPattern()))) {
        juzzlin::L(TAG).trace() << "Pattern: Current element: " << reader.name().toString().toStdString();
        if (reader.isStartElement() && !reader.name().compare(Constants::xmlKeyTracks())) {
            deserializeTracks(reader, pattern);
        }
        reader.readNext();
    }
    juzzlin::L(TAG).trace() << "Reading Pattern ended";
    return pattern;
}

void Song::deserializeTracks(QXmlStreamReader & reader, PatternS pattern)
{
    juzzlin::L(TAG).trace() << "Reading Tracks started";
    while (!(reader.isEndElement() && !reader.name().compare(Constants::xmlKeyTracks()))) {
        juzzlin::L(TAG).trace() << "Tracks: Current element: " << reader.name().toString().toStdString();
        if (reader.isStartElement() && !reader.name().compare(Constants::xmlKeyTrack())) {
            const auto track = deserializeTrack(reader);
            pattern->addOrReplaceTrack(track);
        }
        reader.readNext();
    }
    juzzlin::L(TAG).trace() << "Reading Tracks ended";
}

Song::TrackS Song::deserializeTrack(QXmlStreamReader & reader)
{
    juzzlin::L(TAG).trace() << "Reading Track started";
    const auto index = readUIntAttribute(reader, Constants::xmlKeyIndex());
    const auto name = readStringAttribute(reader, Constants::xmlKeyName());
    const auto columnCount = readUIntAttribute(reader, Constants::xmlKeyColumnCount());
    const auto lineCount = readUIntAttribute(reader, Constants::xmlKeyLineCount());
    const auto track = std::make_shared<Track>(index, name.toStdString(), lineCount, columnCount);
    while (!(reader.isEndElement() && !reader.name().compare(Constants::xmlKeyTrack()))) {
        juzzlin::L(TAG).trace() << "Track: Current element: " << reader.name().toString().toStdString();
        if (reader.isStartElement() && !reader.name().compare(Constants::xmlKeyColumns())) {
            deserializeColumns(reader, track);
            return track;
        }
        reader.readNext();
    }
    juzzlin::L(TAG).trace() << "Reading Track ended";
    return track;
}

void Song::deserializeColumns(QXmlStreamReader & reader, TrackS track)
{
    juzzlin::L(TAG).trace() << "Reading Columns started";
    while (!(reader.isEndElement() && !reader.name().compare(Constants::xmlKeyColumns()))) {
        juzzlin::L(TAG).trace() << "Columns: Current element: " << reader.name().toString().toStdString();
        if (reader.isStartElement() && !reader.name().compare(Constants::xmlKeyColumn())) {
            const auto column = deserializeColumn(reader);
            track->addOrReplaceColumn(column);
        }
        reader.readNext();
    }
    juzzlin::L(TAG).trace() << "Reading Columns ended";
}

Song::ColumnS Song::deserializeColumn(QXmlStreamReader & reader)
{
    juzzlin::L(TAG).trace() << "Reading Column started";
    const auto index = readUIntAttribute(reader, Constants::xmlKeyIndex());
    const auto lineCount = readUIntAttribute(reader, Constants::xmlKeyLineCount());
    const auto column = std::make_shared<Column>(index, lineCount);
    while (!(reader.isEndElement() && !reader.name().compare(Constants::xmlKeyColumn()))) {
        juzzlin::L(TAG).trace() << "Current element: " << reader.name().toString().toStdString();
        if (reader.isStartElement() && !reader.name().compare(Constants::xmlKeyLines())) {
            deserializeLines(reader, column);
        }
        reader.readNext();
    }
    juzzlin::L(TAG).trace() << "Reading Column ended";
    return column;
}

void Song::deserializeLines(QXmlStreamReader & reader, ColumnS column)
{
    juzzlin::L(TAG).trace() << "Reading Lines started";
    while (!(reader.isEndElement() && !reader.name().compare(Constants::xmlKeyLines()))) {
        juzzlin::L(TAG).trace() << "Deserializing Line: " << reader.name().toString().toStdString();
        if (reader.isStartElement() && !reader.name().compare(Constants::xmlKeyLine())) {
            const auto line = deserializeLine(reader);
            column->addOrReplaceLine(line);
        }
        reader.readNext();
    }
    juzzlin::L(TAG).trace() << "Reading Lines ended";
}

Song::LineS Song::deserializeLine(QXmlStreamReader & reader)
{
    juzzlin::L(TAG).trace() << "Reading Line started";
    const auto index = readUIntAttribute(reader, Constants::xmlKeyIndex());
    const auto line = std::make_shared<Line>(index);
    while (!(reader.isEndElement() && !reader.name().compare(Constants::xmlKeyLine()))) {
        juzzlin::L(TAG).trace() << "Current element: " << reader.name().toString().toStdString();
        if (reader.isStartElement() && !reader.name().compare(Constants::xmlKeyNoteData())) {
            const auto noteData = deserializeNoteData(reader);
            line->setNoteData(*noteData);
        }
        reader.readNext();
    }
    juzzlin::L(TAG).trace() << "Reading Line ended";
    return line;
}

Song::NoteDataS Song::deserializeNoteData(QXmlStreamReader & reader)
{
    juzzlin::L(TAG).trace() << "Reading NoteData";
    const auto typeString = readStringAttribute(reader, Constants::xmlKeyType());
    const auto type = typeString == Constants::xmlKeyNoteOn() ? NoteData::Type::NoteOn : NoteData::Type::NoteOff;
    const auto noteData = std::make_shared<NoteData>();
    if (type == NoteData::Type::NoteOn) {
        const auto note = readUIntAttribute(reader, Constants::xmlKeyNote());
        const auto velocity = readUIntAttribute(reader, Constants::xmlKeyVelocity());
        noteData->setAsNoteOn(static_cast<uint8_t>(note), static_cast<uint8_t>(velocity));
    } else {
        const auto note = readUIntAttribute(reader, Constants::xmlKeyNote());
        noteData->setAsNoteOff(static_cast<uint8_t>(note));
    }
    return noteData;
}

} // namespace cacophony
