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
#include <QXmlStreamWriter>

namespace cacophony {

static const auto TAG = "EditorService";

EditorService::EditorService()
{
    initialize();
}

void EditorService::initialize()
{
    setSong(std::make_unique<Song>());
}

EditorService::SongS EditorService::song() const
{
    return m_song;
}

void EditorService::setSong(SongS song)
{
    m_song = song;

    m_position = {};

    emit songChanged();
    emit positionChanged(m_position, m_position);

    setIsModified(false);
}

void EditorService::saveAs(QString fileName)
{
    if (!fileName.endsWith(Constants::fileFormatExtension().c_str())) {
        fileName += Constants::fileFormatExtension().c_str();
    }

    juzzlin::L(TAG).info() << "Saving to " << fileName.toStdString();

    QFile file { fileName };
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        throw std::runtime_error("Failed to open file for writing: " + fileName.toStdString());
    }

    QXmlStreamWriter writer(&file);
    writer.setAutoFormatting(true);
    writer.setAutoFormattingIndent(1);

    // Start the XML document
    writer.writeStartDocument();

    // Root element <Project>
    writer.writeStartElement("Project");
    writer.writeAttribute("fileFormatversion", Constants::fileFormatVersion());
    writer.writeAttribute("applicationName", Constants::applicationName());
    writer.writeAttribute("applicationVersion", Constants::applicationVersion());
    writer.writeAttribute("createdDate", QDateTime::currentDateTime().toString(Qt::DateFormat::ISODateWithMs));

    m_song->serializeToXml(writer);

    writer.writeEndElement();

    writer.writeEndDocument();
}

bool EditorService::canBeSaved() const
{
    return m_song && !m_song->fileName().empty() && QFile::exists(m_song->fileName().c_str());
}

uint32_t EditorService::columnCount(uint32_t trackId) const
{
    return m_song->columnCount(trackId);
}

uint32_t EditorService::lineCount(uint32_t patternId) const
{
    return m_song->lineCount(patternId);
}

uint32_t EditorService::currentLineCount() const
{
    return m_song->lineCount(m_currentPatternId);
}

uint32_t EditorService::linesVisible() const
{
    return 32;
}

int EditorService::lineNumberAtViewLine(uint32_t line) const
{
    // Encode underflow and overflow as negative numbers. The view will show "-64" as "64" but in a different color.
    const int lineNumber = (static_cast<int>(line) + static_cast<int>(m_position.line) - static_cast<int>(positionBarLine()));
    const int lineCount = static_cast<int>(this->lineCount(currentPatternId()));
    if (lineNumber < 0) {
        return -(lineCount + lineNumber);
    } else {
        return lineNumber;
    }
}

QString EditorService::displayNoteAtPosition(uint32_t patternId, uint32_t trackId, uint32_t columnId, uint32_t line) const
{
    if (const auto noteData = m_song->noteDataAtPosition({ patternId, trackId, columnId, line }); noteData->type() != NoteData::Type::None) {
        return noteData->type() == NoteData::Type::NoteOff ? "OFF" : NoteConverter::midiToString(noteData->note()).c_str();
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

QString EditorService::trackName(uint32_t trackId) const
{
    return m_song->trackName(trackId).c_str();
}

void EditorService::setTrackName(uint32_t trackId, QString name)
{
    m_song->setTrackName(trackId, name.toStdString());

    setIsModified(true);
}

uint32_t EditorService::currentPatternId() const
{
    return m_currentPatternId;
}

void EditorService::setCurrentPatternId(uint32_t currentPatternId)
{
    if (m_currentPatternId != currentPatternId) {
        m_currentPatternId = currentPatternId;
        emit currentPatternChanged();
    }
}

bool EditorService::isAtNoteColumn() const
{
    return !m_position.lineColumn;
}

bool EditorService::isAtVelocityColumn() const
{
    return m_position.lineColumn >= 1 && m_position.lineColumn <= 3;
}

bool EditorService::isModified() const
{
    return m_isModified;
}

Position EditorService::position() const
{
    return m_position;
}

uint32_t EditorService::positionBarLine() const
{
    return 8;
}

void EditorService::requestCursorLeft()
{
    juzzlin::L(TAG).debug() << "Cursor left requested";
    const auto oldPosition = m_position;
    if (m_position.lineColumn) {
        m_position.lineColumn--;
    } else {
        m_position.lineColumn = 3;
        m_position.track--;
        m_position.track %= trackCount();
    }

    notifyPositionChange(oldPosition);
}

void EditorService::requestCursorRight()
{
    juzzlin::L(TAG).debug() << "Cursor right requested";
    const auto oldPosition = m_position;
    if (m_position.lineColumn < 3) {
        m_position.lineColumn++;
    } else {
        m_position.lineColumn = 0;
        m_position.track++;
        m_position.track %= trackCount();
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
    juzzlin::L(TAG).debug() << "Set velocity digit at position " << m_position.toString() << ": " << static_cast<int>(digit);

    const auto noteData = m_song->noteDataAtPosition(m_position);
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

    if (m_position.lineColumn == 1) {
        if (digit == 0 || digit == 1) {
            currentVelocity = (digit * 100) + (currentVelocity % 100);
            if (currentVelocity > 127) {
                currentVelocity = 127;
            }
        } else {
            return false; // Invalid digit for hundreds place
        }
    } else if (m_position.lineColumn == 2) {
        currentVelocity = (currentVelocity / 100) * 100 + (digit * 10) + (currentVelocity % 10);
        if (currentVelocity > 127) {
            currentVelocity -= 100;
        }
    } else if (m_position.lineColumn == 3) {
        currentVelocity = (currentVelocity / 10) * 10 + digit;
        if (currentVelocity > 127) {
            currentVelocity -= 10;
        }
    } else {
        return false;
    }

    if (currentVelocity <= 127) {
        noteData->setVelocity(currentVelocity);
        emit noteDataAtPositionChanged(m_position);
        setIsModified(true);
        return true;
    }

    return false;
}

void EditorService::setIsModified(bool isModified)
{
    if (m_isModified != isModified) {
        m_isModified = isModified;
        emit isModifiedChanged();
    }
}

bool EditorService::requestDigitSetAtCurrentPosition(uint8_t digit)
{
    juzzlin::L(TAG).debug() << "Digit set requested at position " << m_position.toString() << ": " << static_cast<int>(digit);

    if (isAtVelocityColumn()) {
        return setVelocityAtCurrentPosition(digit);
    }

    return false;
}

void EditorService::requestNoteDeletionAtCurrentPosition()
{
    juzzlin::L(TAG).debug() << "Note deletion requested at position " << m_position.toString();
    const NoteData noteData {};
    m_song->setNoteDataAtPosition(noteData, m_position);
    emit noteDataAtPositionChanged(m_position);
    setIsModified(true);
}

bool EditorService::requestNoteOnAtCurrentPosition(uint8_t note, uint8_t octave, uint8_t velocity)
{
    if (m_position.lineColumn) {
        juzzlin::L(TAG).debug() << "Not on note column";
        return false;
    }

    if (const auto midiNote = editorNoteToMidiNote(note, octave); midiNote.has_value()) {
        juzzlin::L(TAG).debug() << "Note ON requested at position " << m_position.toString() << ": " << midiNote->first
                                << ", MIDI Note = " << static_cast<int>(midiNote->second) << ", Velocity = " << static_cast<int>(velocity);
        if (const auto noteDataAtPosition = m_song->noteDataAtPosition(m_position); noteDataAtPosition->type() == NoteData::Type::NoteOn) {
            NoteData noteData { m_position.track, m_position.column };
            noteData.setAsNoteOn(midiNote->second, noteDataAtPosition->velocity());
            m_song->setNoteDataAtPosition(noteData, m_position);
        } else {
            NoteData noteData { m_position.track, m_position.column };
            noteData.setAsNoteOn(midiNote->second, velocity);
            m_song->setNoteDataAtPosition(noteData, m_position);
        }
        emit noteDataAtPositionChanged(m_position);
        setIsModified(true);
        return true;
    }

    return false;
}

void EditorService::logPosition() const
{
    juzzlin::L(TAG).debug() << "Position: " << m_position.toString();
}

void EditorService::notifyPositionChange(const Position & oldPosition)
{
    logPosition();

    emit positionChanged(m_position, oldPosition);
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

    const auto oldPosition = m_position;
    m_position.pattern = pattern;
    m_position.track = track;
    m_position.column = column;
    m_position.line = line;
    m_position.lineColumn = lineColumn;

    notifyPositionChange(oldPosition);

    return true;
}

void EditorService::requestPositionByTick(uint32_t tick)
{
    const auto oldPosition = m_position;
    if (auto && patternAndLine = m_song->patternAndLineByTick(tick); patternAndLine.has_value()) {
        m_position.pattern = patternAndLine->first;
        m_position.line = patternAndLine->second;
        notifyPositionChange(oldPosition);
    }
}

void EditorService::requestScroll(int steps)
{
    const auto oldPosition = m_position;
    m_position.line += static_cast<uint32_t>(steps);
    m_position.line %= m_song->lineCount(m_position.pattern);

    notifyPositionChange(oldPosition);
}

void EditorService::requestTrackFocus(uint32_t trackId)
{
    juzzlin::L(TAG).info() << "Focus for track " << trackId << " requested";
    if (trackId < trackCount()) {
        const auto oldPosition = m_position;
        m_position.track = trackId;

        notifyPositionChange(oldPosition);
    }
}

uint32_t EditorService::beatsPerMinute() const
{
    return m_song->beatsPerMinute();
}

void EditorService::setBeatsPerMinute(uint32_t bpm)
{
    m_song->setBeatsPerMinute(bpm);
    setIsModified(true);
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

} // namespace cacophony
