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

#include "editor_service.hpp"

#include "../common/constants.hpp"
#include "../contrib/SimpleLogger/src/simple_logger.hpp"
#include "../domain/note_data.hpp"
#include "../domain/song.hpp"
#include "note_converter.hpp"

#include <QDateTime>
#include <QFile>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

namespace cacophony {

static const auto TAG = "EditorService";

EditorService::EditorService()
  : m_song { std::make_unique<Song>() } // Initial dummy song to guarantee valid requests
{
}

void EditorService::initialize()
{
    juzzlin::L(TAG).info() << "Initializing an empty song";

    setSong(std::make_unique<Song>());

    emit statusTextRequested(tr("An empty song initialized"));
}

EditorService::SongS EditorService::song() const
{
    return m_song;
}

void EditorService::setSong(SongS song)
{
    m_song = song;

    m_cursorPosition = {};

    emit songChanged();
    emit positionChanged(m_cursorPosition, m_cursorPosition);
    emit beatsPerMinuteChanged();
    emit linesPerBeatChanged();
    emit currentLineCountChanged();
    emit currentPatternChanged();

    updateScrollBar();

    setIsModified(false);
}

EditorService::SongS EditorService::deserializeProject(QXmlStreamReader & reader)
{
    juzzlin::L(TAG).trace() << "Reading project started";
    SongS song;
    const auto applicationName = reader.attributes().value("applicationName").toString();
    const auto applicationVersion = reader.attributes().value("applicationVersion").toString();
    const auto createdDate = reader.attributes().value("createdDate").toString();
    const auto fileFormatVersion = reader.attributes().value("fileFormatVersion").toString();
    while (!(reader.isEndElement() && !reader.name().compare(Constants::xmlKeyProject()))) {
        if (reader.isStartElement() && !reader.name().compare(Constants::xmlKeySong())) {
            song = std::make_unique<Song>();
            song->deserializeFromXml(reader);
        }
        reader.readNext();
    }
    juzzlin::L(TAG).trace() << "Reading project ended";
    return song;
}

void EditorService::fromXml(QString xml)
{
    juzzlin::L(TAG).info() << "Reading Project from XML";
    juzzlin::L(TAG).debug() << xml.toStdString();
    QXmlStreamReader reader { xml };
    while (!(reader.atEnd())) {
        juzzlin::L(TAG).trace() << "Current element: " << reader.name().toString().toStdString();
        if (reader.isStartElement() && !reader.name().compare(Constants::xmlKeyProject())) {
            if (const auto song = deserializeProject(reader); song) {
                setSong(song);
            }
        }
        reader.readNext();
    }
}

void EditorService::resetCursorPosition()
{
    const auto oldPosition = m_cursorPosition;
    m_cursorPosition = {};
    notifyPositionChange(oldPosition);
}

void EditorService::load(QString fileName)
{
    if (QFile file { fileName }; file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        fromXml(file.readAll());
        m_song->setFileName(fileName.toStdString());
        const auto message = QString { "Project successfully loaded from: %1 " }.arg(fileName);
        juzzlin::L(TAG).info() << message.toStdString();
        emit statusTextRequested(message);
        emit canBeSavedChanged();
        emit currentFileNameChanged();
    } else {
        throw std::runtime_error("Failed to open file for reading: " + fileName.toStdString());
    }
}

QString EditorService::toXml() const
{
    QString xml;
    QXmlStreamWriter writer { &xml };

    writer.setAutoFormatting(true);
    writer.setAutoFormattingIndent(1);

    writer.writeStartDocument();

    writer.writeStartElement(Constants::xmlKeyProject());
    writer.writeAttribute("fileFormatversion", Constants::fileFormatVersion());
    writer.writeAttribute("applicationName", Constants::applicationName());
    writer.writeAttribute("applicationVersion", Constants::applicationVersion());
    writer.writeAttribute("createdDate", QDateTime::currentDateTime().toString(Qt::DateFormat::ISODateWithMs));

    m_song->serializeToXml(writer);

    writer.writeEndElement();

    writer.writeEndDocument();

    return xml;
}

void EditorService::save()
{
    if (canBeSaved()) {
        saveAs(QString::fromStdString(m_song->fileName()));
    } else {
        throw std::runtime_error("Song cannot be saved!");
    }
}

void EditorService::saveAs(QString fileName)
{
    if (QFile file { fileName }; file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        juzzlin::L(TAG).info() << "Saving to " << fileName.toStdString();
        file.write(toXml().toUtf8());
        const auto message = QString { "Project successfully saved to: %1 " }.arg(fileName);
        juzzlin::L(TAG).info() << message.toStdString();
        emit statusTextRequested(message);
        m_song->setFileName(fileName.toStdString());
        emit currentFileNameChanged();
        setIsModified(false);
    } else {
        throw std::runtime_error("Failed to open file for writing: " + fileName.toStdString());
    }
}

bool EditorService::canBeSaved() const
{
    return isModified() && m_song && !m_song->fileName().empty() && QFile::exists(QString::fromStdString(m_song->fileName()));
}

uint32_t EditorService::columnCount(uint32_t trackIndex) const
{
    return m_song->columnCount(trackIndex);
}

uint32_t EditorService::lineCount(uint32_t patternId) const
{
    return m_song->lineCount(patternId);
}

QString EditorService::currentFileName() const
{
    return QString::fromStdString(m_song->fileName());
}

uint32_t EditorService::currentLineCount() const
{
    return m_song->lineCount(currentPattern());
}

void EditorService::clampCursorLine(size_t oldLineCount, size_t newLineCount)
{
    // Remove cursor focus from non-existent row before updating UI
    if (newLineCount < oldLineCount) {
        if (const auto oldPosition = m_cursorPosition; m_cursorPosition.line >= newLineCount) {
            m_cursorPosition.line = newLineCount - 1;
            notifyPositionChange(oldPosition);
        }
    }
}

void EditorService::setCurrentLineCount(uint32_t lineCount)
{
    if (const auto oldLineCount = currentLineCount(); lineCount != oldLineCount) {
        m_song->setLineCount(currentPattern(), std::min(std::max(lineCount, minLineCount()), maxLineCount()));
        clampCursorLine(oldLineCount, currentLineCount());
        emit currentLineCountChanged();
        emit currentLineCountModified(oldLineCount, lineCount);
        notifyPositionChange(m_cursorPosition); // Force focus after tracks are rebuilt
        setIsModified(true);
    }
}

uint32_t EditorService::minLineCount() const
{
    return 2;
}

uint32_t EditorService::maxLineCount() const
{
    return 999;
}

uint32_t EditorService::minPatternIndex() const
{
    return 0;
}

uint32_t EditorService::maxPatternIndex() const
{
    return 999;
}

uint32_t EditorService::linesVisible() const
{
    return 32;
}

int EditorService::lineNumberAtViewLine(uint32_t line) const
{
    // Encode underflow and overflow as negative numbers. The view will show "-64" as "64" but in a different color.
    const int lineNumber = (static_cast<int>(line) + static_cast<int>(m_cursorPosition.line) - static_cast<int>(positionBarLine()));
    const int lineCount = static_cast<int>(this->lineCount(currentPattern()));
    if (lineNumber < 0) {
        return -(lineCount + lineNumber);
    } else {
        return lineNumber;
    }
}

QString EditorService::displayNoteAtPosition(uint32_t patternId, uint32_t trackIndex, uint32_t columnId, uint32_t line) const
{
    if (const auto noteData = m_song->noteDataAtPosition({ patternId, trackIndex, columnId, line }); noteData->type() != NoteData::Type::None) {
        return noteData->type() == NoteData::Type::NoteOff ? "OFF" : QString::fromStdString(NoteConverter::midiToString(*noteData->note()));
    } else {
        return noDataString();
    }
}

QString EditorService::noDataString() const
{
    return "---";
}

QString EditorService::padVelocityToThreeDigits(QString velocity) const
{
    return velocity.rightJustified(3, '0', true);
}

QString EditorService::displayVelocityAtPosition(uint32_t pattern, uint32_t track, uint32_t column, uint32_t line) const
{
    if (const auto noteData = m_song->noteDataAtPosition({ pattern, track, column, line }); noteData->type() != NoteData::Type::None) {
        return noteData->type() == NoteData::Type::NoteOff ? noDataString() : padVelocityToThreeDigits(QString::number(noteData->velocity()));
    } else {
        return noDataString();
    }
}

double EditorService::effectiveVolumeAtPosition(uint32_t pattern, uint32_t track, uint32_t column, uint32_t line) const
{
    if (const auto noteData = m_song->noteDataAtPosition({ pattern, track, column, line }); noteData->type() != NoteData::Type::None) {
        return noteData->type() == NoteData::Type::NoteOff ? 0 : static_cast<double>(noteData->velocity()) / 127;
    } else {
        return 0;
    }
}

uint32_t EditorService::patternCount() const
{
    return m_song->patternCount();
}

uint32_t EditorService::trackCount() const
{
    return m_song->trackCount();
}

QString EditorService::trackName(uint32_t trackIndex) const
{
    return QString::fromStdString(m_song->trackName(trackIndex));
}

void EditorService::setTrackName(uint32_t trackIndex, QString name)
{
    m_song->setTrackName(trackIndex, name.toStdString());

    setIsModified(true);
}

uint32_t EditorService::currentPattern() const
{
    return m_cursorPosition.pattern;
}

void EditorService::createPatternIfDoesNotExist(uint32_t patternIndex)
{
    if (!m_song->hasPattern(patternIndex)) {
        m_song->createPattern(patternIndex);
        emit patternCreated(patternIndex);
        emit statusTextRequested(tr("A new pattern created!"));
        setIsModified(true);
    }
}

void EditorService::setCurrentPattern(uint32_t patternIndex)
{
    if (currentPattern() == patternIndex) {
        return;
    }

    const auto oldPosition = m_cursorPosition;
    m_cursorPosition.pattern = patternIndex;

    const auto oldLineCount = m_song->lineCount(oldPosition.pattern);

    createPatternIfDoesNotExist(patternIndex);

    if (const auto newLineCount = m_song->lineCount(m_cursorPosition.pattern); newLineCount != oldLineCount) {
        clampCursorLine(oldLineCount, newLineCount);
        emit currentLineCountChanged();
    }

    notifyPositionChange(oldPosition);
}

bool EditorService::hasData(uint32_t pattern, uint32_t track, uint32_t column) const
{
    return m_song->hasData(pattern, track, column);
}

bool EditorService::isAtNoteColumn() const
{
    return !m_cursorPosition.lineColumn;
}

bool EditorService::isAtVelocityColumn() const
{
    return m_cursorPosition.lineColumn >= 1 && m_cursorPosition.lineColumn <= 3;
}

bool EditorService::isModified() const
{
    return m_isModified;
}

void EditorService::setIsModified(bool isModified)
{
    if (m_isModified != isModified) {
        m_isModified = isModified;
        emit isModifiedChanged();
        emit canBeSavedChanged();
        if (isModified) {
            juzzlin::L(TAG).info() << "Project set as modified";
        }
    }
}

Position EditorService::position() const
{
    return m_cursorPosition;
}

uint32_t EditorService::positionBarLine() const
{
    return 8;
}

void EditorService::requestCursorLeft()
{
    juzzlin::L(TAG).debug() << "Cursor left requested";
    const auto oldPosition = m_cursorPosition;
    // Switch line column => switch column => switch track
    if (m_cursorPosition.lineColumn) {
        m_cursorPosition.lineColumn--;
    } else {
        m_cursorPosition.lineColumn = 3;
        if (m_cursorPosition.column) {
            m_cursorPosition.column--;
        } else {
            m_cursorPosition.track--;
            m_cursorPosition.track %= trackCount();
            m_cursorPosition.column = m_song->columnCount(m_cursorPosition.track) - 1;
        }
    }

    notifyPositionChange(oldPosition);
}

void EditorService::requestCursorRight()
{
    juzzlin::L(TAG).debug() << "Cursor right requested";
    const auto oldPosition = m_cursorPosition;
    // Switch line column => switch column => switch track
    if (m_cursorPosition.lineColumn < 3) {
        m_cursorPosition.lineColumn++;
    } else {
        m_cursorPosition.lineColumn = 0;
        if (m_cursorPosition.column + 1 < m_song->columnCount(m_cursorPosition.track)) {
            m_cursorPosition.column++;
        } else {
            m_cursorPosition.column = 0;
            m_cursorPosition.track++;
            m_cursorPosition.track %= trackCount();
        }
    }

    notifyPositionChange(oldPosition);
}

void EditorService::requestTrackRight()
{
    juzzlin::L(TAG).debug() << "Track right requested";
    const auto oldPosition = m_cursorPosition;
    m_cursorPosition.column = 0;
    m_cursorPosition.lineColumn = 0;
    ++m_cursorPosition.track %= trackCount();

    notifyPositionChange(oldPosition);
}

void EditorService::requestColumnRight()
{
    juzzlin::L(TAG).debug() << "Column right requested";
    const auto oldPosition = m_cursorPosition;
    if (oldPosition.column + 1 < m_song->columnCount(oldPosition.track)) {
        m_cursorPosition.column++;
    } else {
        m_cursorPosition.column = 0;
        m_cursorPosition.lineColumn = 0;
        ++m_cursorPosition.track %= trackCount();
    }

    notifyPositionChange(oldPosition);
}

EditorService::MidiNoteNameAndCodeOpt EditorService::editorNoteToMidiNote(uint32_t note, uint32_t octave) const
{
    if (note < 1 || note > 12) {
        juzzlin::L(TAG).error() << "Invalid note value: " << note << ". Valid range is 1..12.";
        return {};
    }

    static const std::array<std::string, 12> noteNames = {
        "C-", "C#", "D-", "D#", "E-", "F-", "F#", "G-", "G#", "A-", "A#", "B-"
    };

    const auto midiNote = static_cast<uint8_t>(12 * octave + (note - 1));
    const auto noteName = noteNames.at(note - 1) + std::to_string(octave);

    return { { noteName, midiNote } };
}

bool EditorService::setVelocityAtCurrentPosition(uint8_t digit)
{
    juzzlin::L(TAG).debug() << "Set velocity digit at position " << m_cursorPosition.toString() << ": " << static_cast<int>(digit);

    const auto noteData = m_song->noteDataAtPosition(m_cursorPosition);
    if (!noteData) {
        return false;
    }

    if (noteData->type() != NoteData::Type::NoteOn) {
        return false;
    }

    auto currentVelocity = noteData->velocity();

    if (digit > 9) {
        juzzlin::L(TAG).error() << "Invalid digit: " << static_cast<int>(digit);
        return false;
    }

    if (m_cursorPosition.lineColumn == 1) {
        if (digit == 0 || digit == 1) {
            currentVelocity = (digit * 100) + (currentVelocity % 100);
            if (currentVelocity > 127) {
                currentVelocity = 127;
            }
        } else {
            return false; // Invalid digit for hundreds place
        }
    } else if (m_cursorPosition.lineColumn == 2) {
        currentVelocity = (currentVelocity / 100) * 100 + (digit * 10) + (currentVelocity % 10);
        if (currentVelocity > 127) {
            currentVelocity -= 100;
        }
    } else if (m_cursorPosition.lineColumn == 3) {
        currentVelocity = (currentVelocity / 10) * 10 + digit;
        if (currentVelocity > 127) {
            currentVelocity -= 10;
        }
    } else {
        return false;
    }

    if (currentVelocity <= 127) {
        noteData->setVelocity(currentVelocity);
        emit noteDataAtPositionChanged(m_cursorPosition);
        setIsModified(true);
        return true;
    }

    return false;
}

bool EditorService::requestDigitSetAtCurrentPosition(uint8_t digit)
{
    juzzlin::L(TAG).debug() << "Digit set requested at position " << m_cursorPosition.toString() << ": " << static_cast<int>(digit);

    if (isAtVelocityColumn()) {
        return setVelocityAtCurrentPosition(digit);
    }

    return false;
}

void EditorService::requestNewColumn(uint32_t track)
{
    juzzlin::L(TAG).debug() << "New column requested on track " << track;

    m_song->addColumn(track);

    emit trackConfigurationChanged();
    updateScrollBar();
    notifyPositionChange(m_cursorPosition); // Re-focuses the previous track
    setIsModified(true);
}

void EditorService::requestNoteDeletionAtCurrentPosition()
{
    deleteNoteDataAtPosition(m_cursorPosition);
}

void EditorService::deleteNoteDataAtPosition(const Position & position)
{
    juzzlin::L(TAG).debug() << "Note deletion requested at position " << position.toString();
    const NoteData noteData {};
    m_song->setNoteDataAtPosition(noteData, position);
    emit noteDataAtPositionChanged(position);
    setIsModified(true);
}

bool EditorService::requestNoteOnAtCurrentPosition(uint8_t note, uint8_t octave, uint8_t velocity)
{
    if (m_cursorPosition.lineColumn) {
        juzzlin::L(TAG).debug() << "Not on note column";
        return false;
    }

    if (const auto midiNote = editorNoteToMidiNote(note, octave); midiNote.has_value()) {
        juzzlin::L(TAG).debug() << "Note ON requested at position " << m_cursorPosition.toString() << ": " << midiNote->first
                                << ", MIDI Note = " << static_cast<int>(midiNote->second) << ", Velocity = " << static_cast<int>(velocity);
        if (const auto noteDataAtPosition = m_song->noteDataAtPosition(m_cursorPosition); noteDataAtPosition->type() == NoteData::Type::NoteOn) {
            NoteData noteData { m_cursorPosition.track, m_cursorPosition.column };
            noteData.setAsNoteOn(midiNote->second, noteDataAtPosition->velocity());
            m_song->setNoteDataAtPosition(noteData, m_cursorPosition);
        } else {
            NoteData noteData { m_cursorPosition.track, m_cursorPosition.column };
            noteData.setAsNoteOn(midiNote->second, velocity);
            m_song->setNoteDataAtPosition(noteData, m_cursorPosition);
        }
        emit noteDataAtPositionChanged(m_cursorPosition);
        setIsModified(true);
        return true;
    }

    return false;
}

void EditorService::removeDuplicateNoteOffs()
{
    juzzlin::L(TAG).debug() << "Removing duplicate note offs";
    if (const auto prevNoteDataPosition = m_song->prevNoteDataOnSameColumn(m_cursorPosition); prevNoteDataPosition != m_cursorPosition) {
        if (const auto previousNoteData = m_song->noteDataAtPosition(prevNoteDataPosition); previousNoteData->type() == NoteData::Type::NoteOff) {
            deleteNoteDataAtPosition(prevNoteDataPosition);
        }
    }
    if (const auto nextNoteDataPosition = m_song->nextNoteDataOnSameColumn(m_cursorPosition); nextNoteDataPosition != m_cursorPosition) {
        if (const auto nextNoteData = m_song->noteDataAtPosition(nextNoteDataPosition); nextNoteData->type() == NoteData::Type::NoteOff) {
            deleteNoteDataAtPosition(nextNoteDataPosition);
        }
    }
}

bool EditorService::requestNoteOffAtCurrentPosition()
{
    if (m_cursorPosition.lineColumn) {
        juzzlin::L(TAG).debug() << "Not on note column";
        return false;
    }

    NoteData noteData { m_cursorPosition.track, m_cursorPosition.column };
    noteData.setAsNoteOff();
    m_song->setNoteDataAtPosition(noteData, m_cursorPosition);
    emit noteDataAtPositionChanged(m_cursorPosition);
    setIsModified(true);

    removeDuplicateNoteOffs();

    return true;
}

void EditorService::logPosition() const
{
    juzzlin::L(TAG).debug() << "Position: " << m_cursorPosition.toString();
}

void EditorService::notifyPositionChange(const Position & oldPosition)
{
    logPosition();

    emit positionChanged(m_cursorPosition, oldPosition);
}

bool EditorService::requestPosition(uint32_t pattern, uint32_t track, uint32_t column, uint32_t line, uint32_t lineColumn)
{
    juzzlin::L(TAG).debug() << "Requesting position: " << pattern << " " << track << " " << column << " " << line << " " << lineColumn;

    if (pattern >= m_song->patternCount()) {
        juzzlin::L(TAG).error() << "Invalid pattern index: " << pattern;
        return false;
    }

    if (line >= m_song->lineCount(pattern)) {
        juzzlin::L(TAG).error() << "Invalid line index: " << line;
        return false;
    }

    if (track >= m_song->trackCount()) {
        juzzlin::L(TAG).error() << "Invalid track index: " << track;
        return false;
    }

    if (column >= m_song->columnCount(track)) {
        juzzlin::L(TAG).error() << "Invalid column index: " << column;
        return false;
    }

    if (lineColumn > 3) {
        juzzlin::L(TAG).error() << "Invalid line column index: " << lineColumn;
        return false;
    }

    const auto oldPosition = m_cursorPosition;
    m_cursorPosition.pattern = pattern;
    m_cursorPosition.track = track;
    m_cursorPosition.column = column;
    m_cursorPosition.line = line;
    m_cursorPosition.lineColumn = lineColumn;

    notifyPositionChange(oldPosition);

    return true;
}

void EditorService::requestPositionByTick(uint32_t tick)
{
    const auto oldPosition = m_cursorPosition;
    if (auto && patternAndLine = m_song->patternAndLineByTick(tick); patternAndLine.has_value()) {
        m_cursorPosition.pattern = patternAndLine->first;
        m_cursorPosition.line = patternAndLine->second;
        notifyPositionChange(oldPosition);
    }
}

void EditorService::requestScroll(int steps)
{
    const auto oldPosition = m_cursorPosition;
    m_cursorPosition.line += static_cast<uint32_t>(steps);
    m_cursorPosition.line %= m_song->lineCount(m_cursorPosition.pattern);

    notifyPositionChange(oldPosition);
}

void EditorService::requestTrackFocus(uint32_t track, uint32_t column)
{
    juzzlin::L(TAG).info() << "Focus for track " << track << " on column " << column << " requested";
    if (track < trackCount() && column < m_song->columnCount(track)) {
        const auto oldPosition = m_cursorPosition;
        m_cursorPosition.track = track;
        m_cursorPosition.column = column;
        notifyPositionChange(oldPosition);
    }
}

uint32_t EditorService::beatsPerMinute() const
{
    return m_song->beatsPerMinute();
}

void EditorService::setBeatsPerMinute(uint32_t beatsPerMinute)
{
    if (m_song->beatsPerMinute() != beatsPerMinute) {
        m_song->setBeatsPerMinute(beatsPerMinute);
        emit beatsPerMinuteChanged();
        setIsModified(true);
    }
}

uint32_t EditorService::linesPerBeat() const
{
    return m_song->linesPerBeat();
}

void EditorService::setLinesPerBeat(uint32_t linesPerBeat)
{
    if (m_song->linesPerBeat() != linesPerBeat) {
        m_song->setLinesPerBeat(linesPerBeat);
        emit linesPerBeatChanged();
        setIsModified(true);
    }
}

uint32_t EditorService::visibleUnitCount() const
{
    return 6;
}

uint32_t EditorService::horizontalScrollPosition() const
{
    return m_horizontalScrollPosition;
}

void EditorService::requestHorizontalScrollPositionChange(double position)
{
    const auto oldPosition = m_horizontalScrollPosition;

    if (visibleUnitCount() < totalUnitCount()) {
        const auto maxPosition = totalUnitCount() - visibleUnitCount();
        m_horizontalScrollPosition = std::round(position / scrollBarSize() * visibleUnitCount());
        m_horizontalScrollPosition = std::min(m_horizontalScrollPosition, maxPosition);
    } else {
        m_horizontalScrollPosition = 0;
    }

    if (m_horizontalScrollPosition != oldPosition) {
        emit horizontalScrollChanged();
        notifyPositionChange(m_cursorPosition); // Forces vertical scroll update
    }
}

uint32_t EditorService::totalUnitCount() const
{
    size_t columnCount = 0;
    for (uint32_t trackIndex = 0; trackIndex < m_song->trackCount(); trackIndex++) {
        columnCount += m_song->columnCount(trackIndex);
    }
    return columnCount;
}

uint32_t EditorService::trackWidthInUnits(uint32_t trackIndex) const
{
    return m_song->columnCount(trackIndex);
}

int EditorService::trackPositionInUnits(uint32_t trackIndex) const
{
    int unitPosition = -m_horizontalScrollPosition;
    for (uint32_t track = 0; track < trackIndex; track++) {
        unitPosition += m_song->columnCount(track);
    }
    return unitPosition;
}

double EditorService::scrollBarStepSize() const
{
    return 1.0 / static_cast<double>(totalUnitCount() - visibleUnitCount());
}

double EditorService::scrollBarSize() const
{
    return static_cast<double>(visibleUnitCount()) / totalUnitCount();
}

void EditorService::updateScrollBar()
{
    emit scrollBarSizeChanged();
    emit scrollBarStepSizeChanged();
}

} // namespace cacophony
