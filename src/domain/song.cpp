// This fi777le is part of Noteahead.
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

#include "song.hpp"

#include "../application/position.hpp"
#include "../common/constants.hpp"
#include "../contrib/SimpleLogger/src/simple_logger.hpp"
#include "../domain/event.hpp"
#include "../domain/instrument.hpp"
#include "../domain/note_data.hpp"
#include "column.hpp"
#include "line.hpp"
#include "pattern.hpp"
#include "play_order.hpp"
#include "track.hpp"

#include <ranges>

#include <QXmlStreamReader>
#include <QXmlStreamWriter>

namespace noteahead {

static const auto TAG = "Song";

Song::Song()
  : m_playOrder { std::make_unique<PlayOrder>() }
{
    initialize();
}

void Song::createPattern(uint32_t patternIndex)
{
    if (m_patterns.empty()) {
        initialize();
    } else {
        const auto previousPattern = m_patterns.at(m_patterns.rbegin()->first);
        m_patterns[patternIndex] = previousPattern->copyWithoutData(patternIndex);
    }
}

bool Song::hasPattern(uint32_t patternIndex) const
{
    return m_patterns.contains(patternIndex);
}

void Song::addColumn(uint32_t trackIndex)
{
    std::ranges::for_each(m_patterns, [=](const auto & pattern) {
        pattern.second->addColumn(trackIndex);
    });
}

bool Song::deleteColumn(uint32_t trackIndex)
{
    if (columnCount(trackIndex) > 1) {
        std::ranges::for_each(m_patterns, [=](const auto & pattern) {
            pattern.second->deleteColumn(trackIndex);
        });
        return true;
    }
    return false;
}

uint32_t Song::columnCount(uint32_t trackIndex) const
{
    return m_patterns.at(0)->columnCount(trackIndex);
}

uint32_t Song::columnCount(uint32_t patternIndex, uint32_t trackIndex) const
{
    return m_patterns.at(patternIndex)->columnCount(trackIndex);
}

uint32_t Song::lineCount(uint32_t patternIndex) const
{
    return m_patterns.at(patternIndex)->lineCount();
}

void Song::setLineCount(uint32_t patternIndex, uint32_t lineCount)
{
    m_patterns.at(patternIndex)->setLineCount(lineCount);
}

uint32_t Song::patternCount() const
{
    return static_cast<uint32_t>(m_patterns.size());
}

uint32_t Song::patternAtSongPosition(uint32_t position) const
{
    return m_playOrder->positionToPattern(position);
}

void Song::setPatternAtSongPosition(uint32_t position, uint32_t pattern)
{
    m_playOrder->setPatternAtPosition(position, pattern);
}

uint32_t Song::trackCount() const
{
    return m_patterns.at(0)->trackCount();
}

uint32_t Song::trackCount(uint32_t patternIndex) const
{
    return m_patterns.at(patternIndex)->trackCount();
}

bool Song::hasData() const
{
    return std::ranges::find_if(m_patterns, [](auto && pattern) {
               return pattern.second->hasData();
           })
      != m_patterns.end();
}

bool Song::hasData(uint32_t pattern, uint32_t track, uint32_t column) const
{
    return m_patterns.at(pattern)->hasData(track, column);
}

std::string Song::trackName(uint32_t trackIndex) const
{
    return m_patterns.at(0)->trackName(trackIndex);
}

void Song::setTrackName(uint32_t trackIndex, std::string name)
{
    m_patterns.at(0)->setTrackName(trackIndex, name);
}

Song::InstrumentS Song::instrument(uint32_t trackIndex) const
{
    return m_patterns.at(0)->instrument(trackIndex);
}

void Song::setInstrument(uint32_t trackIndex, InstrumentS instrument)
{
    m_patterns.at(0)->setInstrument(trackIndex, instrument);
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

Position Song::nextNoteDataOnSameColumn(const Position & position) const
{
    return m_patterns.at(position.pattern)->nextNoteDataOnSameColumn(position);
}

Position Song::prevNoteDataOnSameColumn(const Position & position) const
{
    return m_patterns.at(position.pattern)->prevNoteDataOnSameColumn(position);
}

void Song::initialize()
{
    m_patterns.clear();
    m_patterns[0] = std::make_shared<Pattern>(0, 64, 8);
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

Song::SongPositionOpt Song::songPositionByTick(uint32_t tick) const
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

uint32_t Song::autoNoteOffTickOffset() const
{
    return static_cast<uint32_t>(250 * m_beatsPerMinute * m_linesPerBeat / 60 / 1000);
}

Song::EventList Song::introduceNoteOffs(const EventList & events) const
{
    Song::EventList processedEvents;
    using TrackAndColumn = std::pair<int, int>;
    std::map<TrackAndColumn, uint8_t> activeNotes; // Tracks active notes (key: {track, column}, value: note)

    const auto autoNoteOffTickOffset = this->autoNoteOffTickOffset();
    juzzlin::L(TAG).debug() << "Auto note-off tick offset: " << autoNoteOffTickOffset;

    for (const auto & event : events) {
        if (const auto noteData = event->noteData(); noteData) {
            const auto trackColumn = std::make_pair(noteData->track(), noteData->column());
            if (noteData->type() == NoteData::Type::NoteOn) {
                if (activeNotes.contains(trackColumn)) {
                    const auto activeNote = activeNotes[trackColumn];
                    const auto noteData = std::make_shared<NoteData>(trackColumn.first, trackColumn.second);
                    noteData->setAsNoteOff(static_cast<uint8_t>(activeNote));
                    processedEvents.push_back(std::make_shared<Event>(event->tick() - autoNoteOffTickOffset, noteData));
                }
                activeNotes[trackColumn] = *noteData->note();
            } else if (noteData->type() == NoteData::Type::NoteOff) {
                if (activeNotes.contains(trackColumn)) {
                    // Map anonymous note-off to the playing note
                    event->noteData()->setAsNoteOff(activeNotes[trackColumn]);
                    activeNotes.erase(trackColumn);
                }
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

void Song::updateTickToSongPositionMapping(size_t patternStartTick, uint32_t playOrderSongPosition, uint32_t patternIndex, uint32_t lineCount)
{
    for (uint32_t lineIndex = 0; lineIndex < lineCount; lineIndex++) {
        m_tickToPatternAndLineMap[patternStartTick + lineIndex * m_ticksPerLine] = { playOrderSongPosition, patternIndex, lineIndex };
    }
}

void Song::assignInstruments(const EventList & events) const
{
    std::ranges::for_each(events, [this](const auto & event) {
        if (const auto noteData = event->noteData(); noteData) {
            event->setInstrument(instrument(noteData->track()));
        }
    });
}

Song::EventList Song::renderStartOfSong(size_t tick) const
{
    Song::EventList eventList;
    const auto startOfSongEvent = std::make_shared<Event>(tick);
    startOfSongEvent->setAsStartOfSong();
    eventList.push_back(startOfSongEvent);
    return eventList;
}

Song::EventList Song::renderEndOfSong(Song::EventList eventList, size_t tick) const
{
    const auto endOfSongEvent = std::make_shared<Event>(tick);
    endOfSongEvent->setAsEndOfSong();
    eventList.push_back(endOfSongEvent);
    return eventList;
}

std::pair<Song::EventList, size_t> Song::renderPatterns(Song::EventList eventList, size_t tick)
{
    m_tickToPatternAndLineMap.clear();

    for (uint32_t playOrderSongPosition = 0; playOrderSongPosition < m_playOrder->length(); playOrderSongPosition++) {
        const auto patternIndex = m_playOrder->positionToPattern(playOrderSongPosition);
        juzzlin::L(TAG).debug() << "Rendering position " << playOrderSongPosition << " as pattern " << patternIndex;
        const auto & pattern = m_patterns[patternIndex];
        const auto patternEventList = pattern->renderToEvents(tick, m_ticksPerLine);
        std::copy(patternEventList.begin(), patternEventList.end(), std::back_inserter(eventList));
        updateTickToSongPositionMapping(tick, playOrderSongPosition, patternIndex, pattern->lineCount());
        tick += pattern->lineCount() * m_ticksPerLine;
    }

    return { eventList, tick };
}

Song::EventList Song::renderContent()
{
    size_t tick = 0;

    auto eventList = renderStartOfSong(tick);

    std::tie(eventList, tick) = renderPatterns(eventList, tick);

    eventList = renderEndOfSong(eventList, tick);

    eventList = introduceNoteOffs(eventList);

    return eventList;
}

Song::EventList Song::renderToEvents()
{
    const auto & eventList = renderContent();

    assignInstruments(eventList);

    return eventList;
}

void Song::serializeToXml(QXmlStreamWriter & writer) const
{
    writer.writeStartElement(Constants::xmlKeySong());

    writer.writeAttribute(Constants::xmlKeyBeatsPerMinute(), QString::number(m_beatsPerMinute));
    writer.writeAttribute(Constants::xmlKeyLinesPerBeat(), QString::number(m_linesPerBeat));

    m_playOrder->serializeToXml(writer);

    writer.writeStartElement(Constants::xmlKeyPatterns());

    std::ranges::for_each(m_patterns, [&writer](const auto & pattern) {
        if (pattern.second) {
            pattern.second->serializeToXml(writer);
        }
    });

    writer.writeEndElement(); // Patterns
    writer.writeEndElement(); // Song
}

bool readBoolAttribute(QXmlStreamReader & reader, QString name)
{
    if (!reader.attributes().hasAttribute(name)) {
        throw std::runtime_error { "Attribute " + name.toStdString() + " not found!" };
    } else {
        return reader.attributes().value(name).toString() == "true";
    }
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
        if (reader.isStartElement()) {
            if (!reader.name().compare(Constants::xmlKeyPatterns())) {
                deserializePatterns(reader);
            } else if (!reader.name().compare(Constants::xmlKeyPlayOrder())) {
                deserializePlayOrder(reader);
            }
        }
        reader.readNext();
    }
    juzzlin::L(TAG).trace() << "Reading Song ended";
}

void Song::deserializePlayOrder(QXmlStreamReader & reader)
{
    juzzlin::L(TAG).trace() << "Reading PlayOrder started";
    while (!(reader.isEndElement() && !reader.name().compare(Constants::xmlKeyPlayOrder()))) {
        juzzlin::L(TAG).trace() << "PlayOrder: Current element: " << reader.name().toString().toStdString();
        if (reader.isStartElement() && !reader.name().compare(Constants::xmlKeyPosition())) {
            deserializePosition(reader);
        }
        reader.readNext();
    }
    juzzlin::L(TAG).trace() << "Reading PlayOrder ended";
}

void Song::deserializePosition(QXmlStreamReader & reader)
{
    juzzlin::L(TAG).trace() << "Reading Position started";
    const auto index = readUIntAttribute(reader, Constants::xmlKeyIndex());
    const auto pattern = readUIntAttribute(reader, Constants::xmlKeyPatternAttr());
    setPatternAtSongPosition(index, pattern);
    juzzlin::L(TAG).trace() << "Reading Position ended";
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
        } else if (reader.isStartElement() && !reader.name().compare(Constants::xmlKeyInstrument())) {
            if (const auto instrument = deserializeInstrument(reader); instrument) {
                track->setInstrument(instrument);
            }
        }
        reader.readNext();
    }
    juzzlin::L(TAG).trace() << "Reading Track ended";
    return track;
}

Song::InstrumentS Song::deserializeInstrument(QXmlStreamReader & reader)
{
    juzzlin::L(TAG).trace() << "Reading Instrument";

    // Read mandatory properties
    const auto portName = readStringAttribute(reader, Constants::xmlKeyPortName());
    const auto channel = readUIntAttribute(reader, Constants::xmlKeyChannel());
    const auto instrument = std::make_shared<Instrument>(portName);
    instrument->channel = static_cast<uint8_t>(channel);

    // Read optional properties
    const auto patchEnabled = readBoolAttribute(reader, Constants::xmlKeyPatchEnabled());
    if (patchEnabled) {
        const auto patch = readUIntAttribute(reader, Constants::xmlKeyPatch());
        instrument->patch = patch;
    }

    const auto bankEnabled = readBoolAttribute(reader, Constants::xmlKeyBankEnabled());
    if (bankEnabled) {
        const auto bankLsb = static_cast<uint8_t>(readUIntAttribute(reader, Constants::xmlKeyBankLsb()));
        const auto bankMsb = static_cast<uint8_t>(readUIntAttribute(reader, Constants::xmlKeyBankMsb()));
        const auto bankByteOrderSwapped = readBoolAttribute(reader, Constants::xmlKeyBankByteOrderSwapped());
        instrument->bank = { bankLsb, bankMsb, bankByteOrderSwapped };
    }

    // Ensure we reach the end of the Instrument element
    while (!(reader.isEndElement() && reader.name() == Constants::xmlKeyInstrument())) {
        reader.readNext();
    }

    return instrument;
}

void Song::deserializeColumns(QXmlStreamReader & reader, TrackS track)
{
    juzzlin::L(TAG).trace() << "Reading Columns started";
    while (!(reader.isEndElement() && !reader.name().compare(Constants::xmlKeyColumns()))) {
        juzzlin::L(TAG).trace() << "Columns: Current element: " << reader.name().toString().toStdString();
        if (reader.isStartElement() && !reader.name().compare(Constants::xmlKeyColumn())) {
            const auto column = deserializeColumn(reader, track->index());
            track->setColumn(column);
        }
        reader.readNext();
    }
    juzzlin::L(TAG).trace() << "Reading Columns ended";
}

Song::ColumnS Song::deserializeColumn(QXmlStreamReader & reader, uint32_t trackIndex)
{
    juzzlin::L(TAG).trace() << "Reading Column started";
    const auto index = readUIntAttribute(reader, Constants::xmlKeyIndex());
    const auto lineCount = readUIntAttribute(reader, Constants::xmlKeyLineCount());
    const auto column = std::make_shared<Column>(index, lineCount);
    while (!(reader.isEndElement() && !reader.name().compare(Constants::xmlKeyColumn()))) {
        juzzlin::L(TAG).trace() << "Current element: " << reader.name().toString().toStdString();
        if (reader.isStartElement() && !reader.name().compare(Constants::xmlKeyLines())) {
            deserializeLines(reader, trackIndex, column);
        }
        reader.readNext();
    }
    juzzlin::L(TAG).trace() << "Reading Column ended";
    return column;
}

void Song::deserializeLines(QXmlStreamReader & reader, uint32_t trackIndex, ColumnS column)
{
    juzzlin::L(TAG).trace() << "Reading Lines started";
    while (!(reader.isEndElement() && !reader.name().compare(Constants::xmlKeyLines()))) {
        juzzlin::L(TAG).trace() << "Deserializing Line: " << reader.name().toString().toStdString();
        if (reader.isStartElement() && !reader.name().compare(Constants::xmlKeyLine())) {
            const auto line = deserializeLine(reader, trackIndex, column->index());
            column->addOrReplaceLine(line);
        }
        reader.readNext();
    }
    juzzlin::L(TAG).trace() << "Reading Lines ended";
}

Song::LineS Song::deserializeLine(QXmlStreamReader & reader, uint32_t trackIndex, uint32_t columnIndex)
{
    juzzlin::L(TAG).trace() << "Reading Line started";
    const auto index = readUIntAttribute(reader, Constants::xmlKeyIndex());
    const auto line = std::make_shared<Line>(index);
    while (!(reader.isEndElement() && !reader.name().compare(Constants::xmlKeyLine()))) {
        juzzlin::L(TAG).trace() << "Current element: " << reader.name().toString().toStdString();
        if (reader.isStartElement() && !reader.name().compare(Constants::xmlKeyNoteData())) {
            const auto noteData = deserializeNoteData(reader, trackIndex, columnIndex);
            line->setNoteData(*noteData);
        }
        reader.readNext();
    }
    juzzlin::L(TAG).trace() << "Reading Line ended";
    return line;
}

Song::NoteDataS Song::deserializeNoteData(QXmlStreamReader & reader, uint32_t trackIndex, uint32_t columnIndex)
{
    juzzlin::L(TAG).trace() << "Reading NoteData";
    const auto typeString = readStringAttribute(reader, Constants::xmlKeyType());
    const auto type = typeString == Constants::xmlKeyNoteOn() ? NoteData::Type::NoteOn : NoteData::Type::NoteOff;
    const auto noteData = std::make_shared<NoteData>(trackIndex, columnIndex);
    if (type == NoteData::Type::NoteOn) {
        const auto note = readUIntAttribute(reader, Constants::xmlKeyNote());
        const auto velocity = readUIntAttribute(reader, Constants::xmlKeyVelocity());
        noteData->setAsNoteOn(static_cast<uint8_t>(note), static_cast<uint8_t>(velocity));
    } else {
        noteData->setAsNoteOff();
    }
    return noteData;
}

Song::~Song() = default;

} // namespace noteahead
