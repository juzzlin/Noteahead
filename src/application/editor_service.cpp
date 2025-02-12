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

#include "editor_service.hpp"

#include "../common/constants.hpp"
#include "../contrib/SimpleLogger/src/simple_logger.hpp"
#include "../domain/note_data.hpp"
#include "../domain/song.hpp"
#include "copy_manager.hpp"
#include "instrument_request.hpp"
#include "note_converter.hpp"

#include <QDateTime>
#include <QFile>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

namespace noteahead {

using namespace std::chrono_literals;

static const auto TAG = "EditorService";

EditorService::EditorService()
  : m_copyManager { std::make_unique<CopyManager>() }
{
    setSong(std::make_unique<Song>()); // Initial dummy song to guarantee valid requests and UI
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
    emit beatsPerMinuteChanged();
    emit linesPerBeatChanged();
    emit currentLineCountChanged();
    emit currentPatternChanged();
    emit songLengthChanged();

    updateScrollBar();

    requestInstruments();

    setCurrentTime(0ms);

    updateDuration();

    notifyPositionChange(m_cursorPosition);

    setIsModified(false);
}

void EditorService::requestInstruments()
{
    for (size_t trackIndex = 0; trackIndex < m_song->trackCount(); trackIndex++) {
        if (const auto instrument = m_song->instrument(trackIndex); instrument) {
            juzzlin::L(TAG).info() << "Requesting instrument for track index=" << trackIndex;
            emit instrumentRequested({ InstrumentRequest::Type::ApplyAll, *instrument });
        }
    }
}

EditorService::SongS EditorService::deserializeProject(QXmlStreamReader & reader)
{
    try {
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
    } catch (std::runtime_error & e) {
        juzzlin::L(TAG).error() << e.what();
        emit errorTextRequested(e.what());
        initialize();
        return m_song;
    }
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

size_t EditorService::columnCount(size_t trackIndex) const
{
    return m_song->columnCount(trackIndex);
}

size_t EditorService::lineCount(size_t patternId) const
{
    return m_song->lineCount(patternId);
}

QString EditorService::currentFileName() const
{
    return QString::fromStdString(m_song->fileName());
}

size_t EditorService::currentLineCount() const
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

void EditorService::setCurrentLineCount(size_t lineCount)
{
    if (const auto oldLineCount = currentLineCount(); lineCount != oldLineCount) {
        m_song->setLineCount(currentPattern(), std::min(std::max(lineCount, minLineCount()), maxLineCount()));
        clampCursorLine(oldLineCount, currentLineCount());
        emit currentLineCountChanged();
        emit currentLineCountModified(oldLineCount, lineCount);
        notifyPositionChange(m_cursorPosition); // Force focus after tracks are rebuilt
        setIsModified(true);
        updateDuration();
    }
}

size_t EditorService::minLineCount() const
{
    return 2;
}

size_t EditorService::maxLineCount() const
{
    return 999;
}

size_t EditorService::minPatternIndex() const
{
    return 0;
}

size_t EditorService::maxPatternIndex() const
{
    return 999;
}

size_t EditorService::minSongPosition() const
{
    return 0;
}

size_t EditorService::maxSongPosition() const
{
    return 999;
}

int EditorService::lineNumberAtViewLine(size_t line) const
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

QString EditorService::displayNoteAtPosition(const Position & position) const
{
    return displayNoteAtPosition(position.pattern, position.track, position.column, position.line);
}

QString EditorService::displayNoteAtPosition(size_t patternId, size_t trackIndex, size_t columnId, size_t line) const
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

QString EditorService::displayVelocityAtPosition(const Position & position) const
{
    return displayVelocityAtPosition(position.pattern, position.track, position.column, position.line);
}

QString EditorService::displayVelocityAtPosition(size_t pattern, size_t track, size_t column, size_t line) const
{
    if (const auto noteData = m_song->noteDataAtPosition({ pattern, track, column, line }); noteData->type() != NoteData::Type::None) {
        return noteData->type() == NoteData::Type::NoteOff ? noDataString() : padVelocityToThreeDigits(QString::number(noteData->velocity()));
    } else {
        return noDataString();
    }
}

QStringList EditorService::displayNoteAndVelocityAtPosition(size_t patternId, size_t trackIndex, size_t columnId, size_t line) const
{
    return {
        displayNoteAtPosition(patternId, trackIndex, columnId, line),
        displayVelocityAtPosition(patternId, trackIndex, columnId, line)
    };
}

double EditorService::effectiveVolumeAtPosition(size_t pattern, size_t track, size_t column, size_t line) const
{
    if (const auto noteData = m_song->noteDataAtPosition({ pattern, track, column, line }); noteData->type() != NoteData::Type::None) {
        return noteData->type() == NoteData::Type::NoteOff ? 0 : static_cast<double>(noteData->velocity()) / 127;
    } else {
        return 0;
    }
}

size_t EditorService::patternCount() const
{
    return m_song->patternCount();
}

size_t EditorService::trackCount() const
{
    return m_song->trackCount();
}

EditorService::TrackIndexList EditorService::trackIndices() const
{
    return m_song->trackIndices();
}

QString EditorService::patternName(size_t patternIndex) const
{
    return QString::fromStdString(m_song->patternName(patternIndex));
}

void EditorService::setPatternName(size_t patternIndex, QString name)
{
    m_song->setPatternName(patternIndex, name.toStdString());

    setIsModified(true);
}

QString EditorService::trackName(size_t trackIndex) const
{
    return QString::fromStdString(m_song->trackName(trackIndex));
}

void EditorService::setTrackName(size_t trackIndex, QString name)
{
    if (m_song->trackName(trackIndex) != name.toStdString()) {
        m_song->setTrackName(trackIndex, name.toStdString());
        emit trackNameChanged();
        setIsModified(true);
    }
}

EditorService::InstrumentS EditorService::instrument(size_t trackIndex) const
{
    return m_song->instrument(trackIndex);
}

void EditorService::setInstrument(size_t trackIndex, InstrumentS instrument)
{
    m_song->setInstrument(trackIndex, instrument);

    setIsModified(true);
}

EditorService::InstrumentSettingsS EditorService::instrumentSettingsAtCurrentPosition() const
{
    if (m_song->hasPosition(position())) {
        return m_song->instrumentSettings(position());
    } else {
        return {};
    }
}

void EditorService::removeInstrumentSettingsAtCurrentPosition()
{
    m_song->setInstrumentSettings(position(), {});
    emit lineDataChanged(position());
    setIsModified(true);
}

void EditorService::setInstrumentSettingsAtCurrentPosition(InstrumentSettingsS instrumentSettings)
{
    m_song->setInstrumentSettings(position(), instrumentSettings);
    emit lineDataChanged(position());
    setIsModified(true);
}

bool EditorService::hasInstrumentSettings(size_t pattern, size_t track, size_t column, size_t line) const
{
    const Position testPosition { pattern, track, column, line };
    return m_song->hasPosition(testPosition) && m_song->instrumentSettings(testPosition);
}

void EditorService::requestEventRemoval()
{
    removeInstrumentSettingsAtCurrentPosition();
}

QString EditorService::currentPatternName() const
{
    return patternName(currentPattern());
}

void EditorService::setCurrentPatternName(QString patternName)
{
    setPatternName(currentPattern(), patternName);
}

size_t EditorService::currentPattern() const
{
    return m_cursorPosition.pattern;
}

size_t EditorService::currentTrack() const
{
    return m_cursorPosition.track;
}

size_t EditorService::currentColumn() const
{
    return m_cursorPosition.column;
}

void EditorService::createPatternIfDoesNotExist(size_t patternIndex)
{
    if (!m_song->hasPattern(patternIndex)) {
        m_song->createPattern(patternIndex);
        emit patternCreated(patternIndex);
        emit statusTextRequested(tr("A new pattern created!"));
        setIsModified(true);
        updateDuration();
    }
}

void EditorService::setCurrentPattern(size_t patternIndex)
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

QString EditorService::currentTime() const
{
    return m_currentTime;
}

QString EditorService::duration() const
{
    return m_duration;
}

// Return display time as "hh:mm:ss.sss"
static QString getFormattedTime(std::chrono::milliseconds currentTime)
{
    using namespace std::chrono;

    if (!currentTime.count()) {
        return "00:00:00.000";
    }

    const auto totalSeconds = duration_cast<seconds>(currentTime).count();
    const auto hours = totalSeconds / 3600;
    const auto minutes = (totalSeconds % 3600) / 60;
    const auto seconds = totalSeconds % 60;
    const auto milliseconds = currentTime.count() % 1000;

    return QString("%1:%2:%3.%4")
      .arg(hours, 2, 10, QChar('0'))
      .arg(minutes, 2, 10, QChar('0'))
      .arg(seconds, 2, 10, QChar('0'))
      .arg(milliseconds, 3, 10, QChar('0'));
}

void EditorService::setCurrentTime(std::chrono::milliseconds currentTime)
{
    if (const QString newCurrentTime = getFormattedTime(currentTime); m_currentTime != newCurrentTime) {
        m_currentTime = newCurrentTime;
        emit currentTimeChanged();
    }
}

void EditorService::updateDuration()
{
    setDuration(m_song->duration());
}

void EditorService::setDuration(std::chrono::milliseconds duration)
{
    if (const QString newDuration = getFormattedTime(duration); m_duration != newDuration) {
        m_duration = newDuration;
        emit durationChanged();
    }
}

bool EditorService::hasData(size_t patternIndex, size_t trackIndex, size_t columnIndex) const
{
    return m_song->hasData(patternIndex, trackIndex, columnIndex);
}

bool EditorService::isAtNoteColumn() const
{
    return !m_cursorPosition.lineColumn;
}

bool EditorService::isAtVelocityColumn() const
{
    return m_cursorPosition.lineColumn >= 1 && m_cursorPosition.lineColumn <= 3;
}

bool EditorService::isColumnVisible(size_t track, size_t column) const
{
    const int columnPosition = trackPositionInUnits(track) + static_cast<int>(column);
    return columnPosition >= 0 && columnPosition < static_cast<int>(visibleUnitCount());
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

size_t EditorService::positionBarLine() const
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

EditorService::MidiNoteNameAndCodeOpt EditorService::editorNoteToMidiNote(size_t note, size_t octave)
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

void EditorService::requestNewColumn(size_t track)
{
    juzzlin::L(TAG).debug() << "New column requested on track " << track;

    m_song->addColumn(track);

    emit columnAdded(track);
    updateScrollBar();
    notifyPositionChange(m_cursorPosition); // Re-focuses the previous track
    setIsModified(true);
}

void EditorService::requestColumnDeletion(size_t track)
{
    juzzlin::L(TAG).debug() << "Column deletion requested on track " << track;

    if (m_song->deleteColumn(track)) {
        const auto oldPosition = m_cursorPosition;
        if (oldPosition.track == track && m_cursorPosition.column >= m_song->columnCount(track)) {
            m_cursorPosition.column--;
        }
        notifyPositionChange(oldPosition);
        emit columnDeleted(track);
        updateScrollBar();
        notifyPositionChange(m_cursorPosition); // Re-focuses the previous track
        setIsModified(true);
    }
}

void EditorService::requestNewTrackToRight()
{
}

void EditorService::requestNoteInsertionAtCurrentPosition()
{
    insertNoteAtPosition(m_cursorPosition);
}

void EditorService::requestNoteDeletionAtCurrentPosition(bool shiftNotes)
{
    deleteNoteDataAtPosition(m_cursorPosition, shiftNotes);
}

void EditorService::insertNoteAtPosition(const Position & position)
{
    juzzlin::L(TAG).debug() << "Note insertion requested at position " << position.toString();
    if (const auto changedPositions = m_song->insertNoteDataAtPosition({}, position); !changedPositions.empty()) {
        for (auto && changedPosition : changedPositions) {
            emit noteDataAtPositionChanged(changedPosition);
        }
        setIsModified(true);
        updateDuration();
    }
}

void EditorService::deleteNoteDataAtPosition(const Position & position, bool shiftNotes)
{
    juzzlin::L(TAG).debug() << "Note deletion requested at position " << position.toString();
    if (!shiftNotes) {
        m_song->setNoteDataAtPosition({}, position);
        emit noteDataAtPositionChanged(position);
        setIsModified(true);
        updateDuration();
    } else {
        // For traditional backspace function we actually delete the previous line and shift
        auto positionToBeDeleted = position;
        positionToBeDeleted.line = position.line - 1;
        if (m_song->hasPosition(positionToBeDeleted)) {
            if (const auto changedPositions = m_song->deleteNoteDataAtPosition(positionToBeDeleted); !changedPositions.empty()) {
                for (auto && changedPosition : changedPositions) {
                    emit noteDataAtPositionChanged(changedPosition);
                }
                requestScroll(-1);
                setIsModified(true);
                updateDuration();
            }
        }
    }
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
        NoteData noteData { m_cursorPosition.track, m_cursorPosition.column };
        noteData.setAsNoteOn(midiNote->second, velocity);
        m_song->setNoteDataAtPosition(noteData, m_cursorPosition);
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
            deleteNoteDataAtPosition(prevNoteDataPosition, false);
        }
    }
    if (const auto nextNoteDataPosition = m_song->nextNoteDataOnSameColumn(m_cursorPosition); nextNoteDataPosition != m_cursorPosition) {
        if (const auto nextNoteData = m_song->noteDataAtPosition(nextNoteDataPosition); nextNoteData->type() == NoteData::Type::NoteOff) {
            deleteNoteDataAtPosition(nextNoteDataPosition, false);
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
#ifdef NOTEAHEAD_DEBUG
    juzzlin::L(TAG).trace() << "Position: " << m_cursorPosition.toString();
#endif
}

void EditorService::notifyPositionChange(const Position & oldPosition)
{
    logPosition();

    emit positionChanged(m_cursorPosition, oldPosition);

    if (m_cursorPosition.pattern != oldPosition.pattern) {
        emit currentPatternChanged();
    }
}

void EditorService::requestColumnCut()
{
    juzzlin::L(TAG).info() << "Requesting column cut";
    for (auto && changedPosition : m_song->cutColumn(currentPattern(), currentTrack(), currentColumn(), *m_copyManager)) {
        emit noteDataAtPositionChanged(changedPosition);
    }
    emit copyManagerStateChanged();
    emit statusTextRequested(tr("Column cut"));
    setIsModified(true);
}

void EditorService::requestColumnCopy()
{
    juzzlin::L(TAG).info() << "Requesting column copy";
    m_song->copyColumn(currentPattern(), currentTrack(), currentColumn(), *m_copyManager);
    emit copyManagerStateChanged();
    emit statusTextRequested(tr("Column copied"));
}

void EditorService::requestColumnPaste()
{
    try {
        juzzlin::L(TAG).info() << "Requesting paste for copied column";
        for (auto && changedPosition : m_song->pasteColumn(currentPattern(), currentTrack(), currentColumn(), *m_copyManager)) {
            emit noteDataAtPositionChanged(changedPosition);
        }
        emit statusTextRequested(tr("Copied column pasted"));
        setIsModified(true);
        updateDuration();
    } catch (const std::runtime_error & e) {
        emit statusTextRequested(tr("Failed to paste column: ") + e.what());
    }
}

void EditorService::requestColumnTranspose(int semitones)
{
    for (auto && changedPosition : m_song->transposeColumn(m_cursorPosition, semitones)) {
        emit noteDataAtPositionChanged(changedPosition);
    }
    setIsModified(true);
}

bool EditorService::hasColumnToPaste() const
{
    return m_copyManager->mode() == CopyManager::Mode::Column;
}

void EditorService::requestTrackCut()
{
    juzzlin::L(TAG).info() << "Requesting track cut";
    for (auto && changedPosition : m_song->cutTrack(currentPattern(), currentTrack(), *m_copyManager)) {
        emit noteDataAtPositionChanged(changedPosition);
    }
    emit copyManagerStateChanged();
    emit statusTextRequested(tr("Track cut"));
    setIsModified(true);
}

void EditorService::requestTrackCopy()
{
    juzzlin::L(TAG).info() << "Requesting track copy";
    m_song->copyTrack(currentPattern(), currentTrack(), *m_copyManager);
    emit copyManagerStateChanged();
    emit statusTextRequested(tr("Track copied"));
}

void EditorService::requestTrackPaste()
{
    try {
        juzzlin::L(TAG).info() << "Requesting paste for copied track";
        for (auto && changedPosition : m_song->pasteTrack(currentPattern(), currentTrack(), *m_copyManager)) {
            emit noteDataAtPositionChanged(changedPosition);
        }
        emit statusTextRequested(tr("Copied track pasted"));
        setIsModified(true);
        updateDuration();
    } catch (const std::runtime_error & e) {
        emit statusTextRequested(tr("Failed to paste track: ") + e.what());
    }
}

bool EditorService::hasTrackToPaste() const
{
    return m_copyManager->mode() == CopyManager::Mode::Track;
}

void EditorService::requestTrackTranspose(int semitones)
{
    for (auto && changedPosition : m_song->transposeTrack(m_cursorPosition, semitones)) {
        emit noteDataAtPositionChanged(changedPosition);
    }
    setIsModified(true);
}

void EditorService::requestPatternCut()
{
    juzzlin::L(TAG).info() << "Requesting pattern cut";
    for (auto && changedPosition : m_song->cutPattern(currentPattern(), *m_copyManager)) {
        emit noteDataAtPositionChanged(changedPosition);
    }
    emit statusTextRequested(tr("Pattern cut"));
    emit copyManagerStateChanged();
    setIsModified(true);
}

void EditorService::requestPatternCopy()
{
    juzzlin::L(TAG).info() << "Requesting pattern copy";
    m_song->copyPattern(currentPattern(), *m_copyManager);
    emit copyManagerStateChanged();
    emit statusTextRequested(tr("Pattern copied"));
}

void EditorService::requestPatternPaste()
{
    try {
        juzzlin::L(TAG).info() << "Requesting paste for copied pattern";
        for (auto && changedPosition : m_song->pastePattern(currentPattern(), *m_copyManager)) {
            emit noteDataAtPositionChanged(changedPosition);
        }
        emit statusTextRequested(tr("Copied pattern pasted"));
        setIsModified(true);
        updateDuration();
    } catch (const std::runtime_error & e) {
        emit statusTextRequested(tr("Failed to paste pattern: ") + e.what());
    }
}

bool EditorService::hasPatternToPaste() const
{
    return m_copyManager->mode() == CopyManager::Mode::Pattern;
}

void EditorService::requestPatternTranspose(int semitones)
{
    for (auto && changedPosition : m_song->transposePattern(m_cursorPosition, semitones)) {
        emit noteDataAtPositionChanged(changedPosition);
    }
    setIsModified(true);
}

bool EditorService::requestPosition(const Position & position)
{
    return requestPosition(position.pattern, position.track, position.column, position.line, position.lineColumn);
}

bool EditorService::requestPosition(size_t pattern, size_t track, size_t column, size_t line, size_t lineColumn)
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

    setCurrentTime(m_song->lineToTime(m_cursorPosition.line));

    return true;
}

void EditorService::requestPositionByTick(size_t tick)
{
    // Skip unnecessary updates
    if (tick % m_song->ticksPerLine()) {
        return;
    }

    const auto oldPosition = m_cursorPosition;
    if (auto && songPosition = m_song->songPositionByTick(tick); songPosition.has_value()) {
        m_cursorPosition.pattern = songPosition->pattern;
        m_cursorPosition.line = songPosition->line;
        notifyPositionChange(oldPosition);
        setSongPosition(songPosition->position);
        setCurrentTime(songPosition->currentTime);
    }
}

void EditorService::requestScroll(int steps)
{
    const auto oldPosition = m_cursorPosition;
    m_cursorPosition.line += static_cast<size_t>(steps);
    m_cursorPosition.line %= m_song->lineCount(m_cursorPosition.pattern);
    setCurrentTime(m_song->lineToTime(m_cursorPosition.line));
    notifyPositionChange(oldPosition);
}

void EditorService::requestTrackFocus(size_t track, size_t column, size_t line)
{
    juzzlin::L(TAG).info() << "Focus for track " << track << " on column " << column << " on line " << line << " requested";
    if (track < trackCount() && column < m_song->columnCount(track) && line < currentLineCount()) {
        const auto oldPosition = m_cursorPosition;
        m_cursorPosition.track = track;
        m_cursorPosition.column = column;
        m_cursorPosition.line = line;
        notifyPositionChange(oldPosition);
    }
}

size_t EditorService::beatsPerMinute() const
{
    return m_song->beatsPerMinute();
}

void EditorService::setBeatsPerMinute(size_t beatsPerMinute)
{
    if (m_song->beatsPerMinute() != beatsPerMinute) {
        m_song->setBeatsPerMinute(beatsPerMinute);
        emit beatsPerMinuteChanged();
        setIsModified(true);
        updateDuration();
    }
}

size_t EditorService::linesPerBeat() const
{
    return m_song->linesPerBeat();
}

void EditorService::setLinesPerBeat(size_t linesPerBeat)
{
    if (m_song->linesPerBeat() != linesPerBeat) {
        m_song->setLinesPerBeat(linesPerBeat);
        emit linesPerBeatChanged();
        setIsModified(true);
        updateDuration();
    }
}

size_t EditorService::visibleUnitCount() const
{
    return 6;
}

size_t EditorService::horizontalScrollPosition() const
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

size_t EditorService::totalUnitCount() const
{
    size_t columnCount = 0;
    for (size_t trackIndex = 0; trackIndex < m_song->trackCount(); trackIndex++) {
        columnCount += m_song->columnCount(trackIndex);
    }
    return columnCount;
}

size_t EditorService::trackWidthInUnits(size_t trackIndex) const
{
    return m_song->columnCount(trackIndex);
}

int EditorService::trackPositionInUnits(size_t trackIndex) const
{
    int unitPosition = -static_cast<int>(m_horizontalScrollPosition);
    for (size_t track = 0; track < trackIndex; track++) {
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

size_t EditorService::songPosition() const
{
    return m_songPosition;
}

void EditorService::setSongPosition(size_t songPosition)
{
    if (m_songPosition != songPosition) {
        m_songPosition = songPosition;
        emit songPositionChanged(songPosition);
        emit patternAtCurrentSongPositionChanged();
        if (songPosition >= songLength()) {
            setSongLength(songPosition + 1);
        }
    }
}

void EditorService::setPatternAtSongPosition(size_t songPosition, size_t pattern)
{
    setCurrentPattern(pattern);

    if (m_song->patternAtSongPosition(songPosition) != pattern) {
        m_song->setPatternAtSongPosition(songPosition, pattern);
        if (m_songPosition == songPosition) {
            emit patternAtCurrentSongPositionChanged();
        }
        if (songPosition >= songLength()) {
            setSongLength(songPosition + 1);
        }
        setIsModified(true);
        updateDuration();
    }
}

size_t EditorService::patternAtCurrentSongPosition() const
{
    return m_song->patternAtSongPosition(m_songPosition);
}

void EditorService::insertPatternToPlayOrder()
{
    m_song->insertPatternToPlayOrder(m_songPosition);
    emit songPositionChanged(m_songPosition);
    emit patternAtCurrentSongPositionChanged();
    setSongLength(songLength() + 1);
    setIsModified(true);
    updateDuration();
}

void EditorService::removePatternFromPlayOrder()
{
    m_song->removePatternFromPlayOrder(m_songPosition);
    emit songPositionChanged(m_songPosition);
    emit patternAtCurrentSongPositionChanged();
    setSongLength(songLength() - 1);
    setIsModified(true);
    updateDuration();
}

size_t EditorService::patternAtSongPosition(size_t songPosition) const
{
    return m_song->patternAtSongPosition(songPosition);
}

size_t EditorService::songLength() const
{
    return m_song->length();
}

void EditorService::setSongLength(size_t songLength)
{
    if (m_song->length() != songLength && songLength <= maxSongLength()) {
        m_song->setLength(songLength);
        emit songLengthChanged();
        updateDuration();
        if (songPosition() >= m_song->length()) {
            setSongPosition(songPosition() - 1);
        }
        setIsModified(true);
    }
}

size_t EditorService::maxSongLength() const
{
    return 999;
}

EditorService::~EditorService() = default;

} // namespace noteahead
