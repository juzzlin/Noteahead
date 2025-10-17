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

#include "../../common/constants.hpp"
#include "../../contrib/SimpleLogger/src/simple_logger.hpp"
#include "../../domain/note_data.hpp"
#include "../../domain/note_data_manipulator.hpp"
#include "../../domain/song.hpp"
#include "../instrument_request.hpp"
#include "../note_converter.hpp"
#include "copy_manager.hpp"
#include "selection_service.hpp"

#include <QDateTime>
#include <QFile>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

namespace noteahead {

using namespace std::chrono_literals;

static const auto TAG = "EditorService";

EditorService::EditorService()
  : EditorService { std::make_shared<SelectionService>() }
{
}

EditorService::EditorService(SelectionServiceS selectionService)
  : m_selectionService { selectionService }
{
    initialize();
}

void EditorService::initialize()
{
    emit aboutToInitialize();

    juzzlin::L(TAG).info() << "Initializing an empty song";

    // Initial dummy song to guarantee valid requests and UI
    setSong(std::make_unique<Song>());

    emit initialized();
    emit statusTextRequested(tr("An empty song initialized"));
}

EditorService::SongS EditorService::song() const
{
    return m_song;
}

void EditorService::setSong(SongS song)
{
    m_state = State {};

    m_song = song;
    emit songChanged();
    emit beatsPerMinuteChanged();
    emit linesPerBeatChanged();
    emit currentLineCountChanged();
    emit currentPatternChanged();
    emit songLengthChanged();
    emit currentFileNameChanged();

    updateScrollBar();

    requestInstruments();

    setCurrentTime(0ms);

    resetSongPosition();

    updateDuration();

    notifyPositionChange(m_state.cursorPosition);
    emit songPositionChanged(m_state.songPosition);
    emit patternAtCurrentSongPositionChanged();

    setIsModified(false);
}

void EditorService::requestInstruments()
{
    for (quint64 trackIndex : m_song->trackIndices()) {
        if (const auto instrument = m_song->instrument(trackIndex); instrument) {
            juzzlin::L(TAG).info() << "Requesting instrument for track index=" << trackIndex;
            emit instrumentRequested({ InstrumentRequest::Type::ApplyAll, *instrument });
        }
    }
}

void EditorService::doVersionCheck(QString fileFormatVersion)
{
    juzzlin::L(TAG).info() << "File format version: " << fileFormatVersion.toStdString();

    if (fileFormatVersion.isEmpty()) {
        const auto errorText = tr("Couldn't parse file format version!");
        emit errorTextRequested(errorText);
        juzzlin::L(TAG).error() << errorText.toStdString();
        return;
    }

    const auto parts = fileFormatVersion.split(".");
    if (parts.size() < 2) {
        const auto errorText = tr("Invalid file format version: '%1'").arg(fileFormatVersion);
        emit errorTextRequested(errorText);
        juzzlin::L(TAG).error() << errorText.toStdString();
        return;
    }

    bool majorOk = false, minorOk = false;
    const int majorVersion = parts[0].toInt(&majorOk);
    const int minorVersion = parts[1].toInt(&minorOk);
    if (!majorOk || !minorOk) {
        const auto errorText = tr("File format version contains non-numeric values: '%1'").arg(fileFormatVersion);
        emit errorTextRequested(errorText);
        juzzlin::L(TAG).error() << errorText.toStdString();
        return;
    }

    const auto supportedParts = Constants::fileFormatVersion().split(".");
    if (supportedParts.size() < 2) {
        const auto errorText = tr("Internal error: Supported file format version is invalid.");
        emit errorTextRequested(errorText);
        juzzlin::L(TAG).error() << errorText.toStdString();
        return;
    }

    bool majorSupportedOk = false, minorSupportedOk = false;
    const int majorVersionSupported = supportedParts[0].toInt(&majorSupportedOk);
    const int minorVersionSupported = supportedParts[1].toInt(&minorSupportedOk);

    if (!majorSupportedOk || !minorSupportedOk) {
        const auto errorText = tr("Internal error: Supported file format version contains non-numeric values.");
        emit errorTextRequested(errorText);
        juzzlin::L(TAG).error() << errorText.toStdString();
        return;
    }

    if ((majorVersion == majorVersionSupported && minorVersion > minorVersionSupported) || (majorVersion > majorVersionSupported)) {
        const auto errorText = QString(tr("File format version '%1' is greater than supported version '%2'. Project may not load correctly!"))
                                 .arg(fileFormatVersion, Constants::fileFormatVersion());
        emit errorTextRequested(errorText);
        juzzlin::L(TAG).error() << errorText.toStdString();
    }
}

EditorService::SongS EditorService::deserializeProject(QXmlStreamReader & reader)
{
    try {
        juzzlin::L(TAG).info() << "Reading project started";
        SongS song;
        doVersionCheck(reader.attributes().value(Constants::NahdXml::xmlKeyFileFormatVersion()).toString());
        const auto applicationName = reader.attributes().value(Constants::NahdXml::xmlKeyApplicationName()).toString();
        juzzlin::L(TAG).info() << "Creator application name: " << applicationName.toStdString();
        const auto applicationVersion = reader.attributes().value(Constants::NahdXml::xmlKeyApplicationVersion()).toString();
        juzzlin::L(TAG).info() << "Creator application version: " << applicationVersion.toStdString();
        m_state.createdDate = reader.attributes().value(Constants::NahdXml::xmlKeyCreatedDate()).toString();
        juzzlin::L(TAG).info() << "Created date: " << m_state.createdDate.toStdString();
        const auto mixerDeserializationCallback = [this](QXmlStreamReader & reader) {
            emit mixerDeserializationRequested(reader);
        };
        const auto automationDeserializationCallback = [this](QXmlStreamReader & reader) {
            emit automationDeserializationRequested(reader);
        };
        while (!(reader.isEndElement() && !reader.name().compare(Constants::NahdXml::xmlKeyProject()))) {
            if (reader.isStartElement() && !reader.name().compare(Constants::NahdXml::xmlKeySong())) {
                song = std::make_unique<Song>();
                song->deserializeFromXml(reader, mixerDeserializationCallback, automationDeserializationCallback);
            }
            reader.readNext();
        }
        juzzlin::L(TAG).info() << "Reading project ended";
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
    emit aboutToChangeSong();

    juzzlin::L(TAG).info() << "Reading Project from XML";
    juzzlin::L(TAG).trace() << xml.toStdString();
    QXmlStreamReader reader { xml };
    while (!(reader.atEnd())) {
        juzzlin::L(TAG).trace() << "Current element: " << reader.name().toString().toStdString();
        if (reader.isStartElement() && !reader.name().compare(Constants::NahdXml::xmlKeyProject())) {
            if (const auto song = deserializeProject(reader); song) {
                setSong(song);
            }
        }
        reader.readNext();
    }
}

void EditorService::resetCursorPosition()
{
    const auto oldPosition = m_state.cursorPosition;
    m_state.cursorPosition = {};
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

QString EditorService::toXml()
{
    QString xml;
    QXmlStreamWriter writer { &xml };

    writer.setAutoFormatting(true);
    writer.setAutoFormattingIndent(1);

    writer.writeStartDocument();

    writer.writeStartElement(Constants::NahdXml::xmlKeyProject());
    writer.writeAttribute(Constants::NahdXml::xmlKeyFileFormatVersion(), Constants::fileFormatVersion());
    writer.writeAttribute(Constants::NahdXml::xmlKeyApplicationName(), Constants::applicationName());
    writer.writeAttribute(Constants::NahdXml::xmlKeyApplicationVersion(), Constants::applicationVersion());
    if (m_state.createdDate.isEmpty()) {
        m_state.createdDate = QDateTime::currentDateTime().toString(Qt::DateFormat::ISODateWithMs);
    }
    writer.writeAttribute(Constants::NahdXml::xmlKeyCreatedDate(), m_state.createdDate);
    const auto mixerSerializationCallback = [this](QXmlStreamWriter & writer) {
        emit mixerSerializationRequested(writer);
    };
    const auto automationSerializationCallback = [this](QXmlStreamWriter & writer) {
        emit automationSerializationRequested(writer);
    };
    m_song->serializeToXml(writer, mixerSerializationCallback, automationSerializationCallback);

    writer.writeEndElement();
    writer.writeEndDocument();

    return xml;
}

QString EditorService::toXmlAsTemplate()
{
    QString xml;
    QXmlStreamWriter writer { &xml };

    writer.setAutoFormatting(true);
    writer.setAutoFormattingIndent(1);

    writer.writeStartDocument();

    writer.writeStartElement(Constants::NahdXml::xmlKeyProject());
    writer.writeAttribute(Constants::NahdXml::xmlKeyFileFormatVersion(), Constants::fileFormatVersion());
    writer.writeAttribute(Constants::NahdXml::xmlKeyApplicationName(), Constants::applicationName());
    writer.writeAttribute(Constants::NahdXml::xmlKeyApplicationVersion(), Constants::applicationVersion());
    writer.writeAttribute(Constants::NahdXml::xmlKeyCreatedDate(), QDateTime::currentDateTime().toString(Qt::DateFormat::ISODateWithMs));

    const auto mixerSerializationCallback = [this](QXmlStreamWriter & writer) {
        emit mixerSerializationRequested(writer);
    };
    const auto automationSerializationCallback = [this](QXmlStreamWriter & writer) {
        emit automationSerializationRequested(writer);
    };
    m_song->serializeToXmlAsTemplate(writer, mixerSerializationCallback, automationSerializationCallback);

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

void EditorService::saveAsTemplate(QString fileName)
{
    if (QFile file { fileName }; file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        juzzlin::L(TAG).info() << "Saving as a template to " << fileName.toStdString();
        file.write(toXmlAsTemplate().toUtf8());
        const auto message = QString { "Project successfully saved to: %1 " }.arg(fileName);
        juzzlin::L(TAG).info() << message.toStdString();
        emit statusTextRequested(message);
    } else {
        throw std::runtime_error("Failed to open file for writing: " + fileName.toStdString());
    }
}

bool EditorService::canBeSaved() const
{
    return isModified() && m_song && !m_song->fileName().empty() && QFile::exists(QString::fromStdString(m_song->fileName()));
}

quint64 EditorService::columnCount(quint64 trackIndex) const
{
    return m_song->columnCount(trackIndex);
}

quint64 EditorService::lineCount(quint64 patternId) const
{
    return m_song->lineCount(patternId);
}

QString EditorService::currentFileName() const
{
    return QString::fromStdString(m_song->fileName());
}

quint64 EditorService::currentLineCount() const
{
    return m_song->lineCount(currentPattern());
}

void EditorService::clampCursorLine(quint64 oldLineCount, quint64 newLineCount)
{
    // Remove cursor focus from non-existent row before updating UI
    if (newLineCount < oldLineCount) {
        if (const auto oldPosition = m_state.cursorPosition; m_state.cursorPosition.line >= newLineCount) {
            m_state.cursorPosition.line = newLineCount - 1;
            notifyPositionChange(oldPosition);
        }
    }
}

void EditorService::setCurrentLineCount(quint64 lineCount)
{
    if (const auto oldLineCount = currentLineCount(); lineCount != oldLineCount) {
        m_song->setLineCount(currentPattern(), std::min(std::max(lineCount, minLineCount()), maxLineCount()));
        clampCursorLine(oldLineCount, currentLineCount());
        emit currentLineCountChanged();
        emit currentLineCountModified(oldLineCount, lineCount);
        notifyPositionChange(m_state.cursorPosition); // Force focus after tracks are rebuilt
        setIsModified(true);
        updateDuration();
    }
}

quint64 EditorService::minLineCount() const
{
    return 2;
}

quint64 EditorService::maxLineCount() const
{
    return 999;
}

quint64 EditorService::minPatternIndex() const
{
    return 0;
}

quint64 EditorService::maxPatternIndex() const
{
    return 999;
}

quint64 EditorService::minSongPosition() const
{
    return 0;
}

quint64 EditorService::maxSongPosition() const
{
    return 999;
}

int EditorService::lineNumberAtViewLine(quint64 line) const
{
    // Encode underflow and overflow as negative numbers. The view will show "-64" as "64" but in a different color.
    const int lineNumber = (static_cast<int>(line) + static_cast<int>(m_state.cursorPosition.line) - static_cast<int>(positionBarLine()));
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

QString EditorService::displayNoteAtPosition(quint64 patternId, quint64 trackIndex, quint64 columnId, quint64 line) const
{
    if (const auto noteData = m_song->noteDataAtPosition({ patternId, trackIndex, columnId, line }); noteData->type() != NoteData::Type::None) {
        return noteData->type() == NoteData::Type::NoteOff ? "OFF" : QString::fromStdString(NoteConverter::midiToString(*noteData->note()));
    } else {
        return noDataString();
    }
}

EditorService::MidiNoteList EditorService::midiNotesAtPosition(quint64 patternId, quint64 trackIndex, quint64 line) const
{
    EditorService::MidiNoteList midiNoteList;
    for (quint64 column = 0; column < m_song->columnCount(trackIndex); column++) {
        if (const auto noteData = m_song->noteDataAtPosition({ patternId, trackIndex, column, line }); noteData->type() != NoteData::Type::None) {
            if (noteData->type() == NoteData::Type::NoteOn) {
                midiNoteList.push_back(*noteData->note());
            }
        }
    }
    return midiNoteList;
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

QString EditorService::displayVelocityAtPosition(quint64 pattern, quint64 track, quint64 column, quint64 line) const
{
    if (const auto noteData = m_song->noteDataAtPosition({ pattern, track, column, line }); noteData->type() != NoteData::Type::None) {
        return noteData->type() == NoteData::Type::NoteOff ? noDataString() : padVelocityToThreeDigits(QString::number(noteData->velocity()));
    } else {
        return noDataString();
    }
}

QStringList EditorService::displayNoteAndVelocityAtPosition(quint64 patternId, quint64 trackIndex, quint64 columnId, quint64 line) const
{
    return {
        displayNoteAtPosition(patternId, trackIndex, columnId, line),
        displayVelocityAtPosition(patternId, trackIndex, columnId, line)
    };
}

double EditorService::velocityAtPosition(quint64 pattern, quint64 track, quint64 column, quint64 line) const
{
    if (const auto noteData = m_song->noteDataAtPosition({ pattern, track, column, line }); noteData->type() != NoteData::Type::None) {
        return noteData->type() == NoteData::Type::NoteOff ? 0 : static_cast<double>(noteData->velocity());
    } else {
        return 0;
    }
}

quint64 EditorService::patternCount() const
{
    return m_song->patternCount();
}

EditorService::PatternIndexList EditorService::patternIndices() const
{
    EditorService::PatternIndexList result;
    std::ranges::transform(m_song->patternIndices(), std::back_inserter(result),
                           [](std::size_t index) { return static_cast<quint64>(index); });
    return result;
}

quint64 EditorService::trackCount() const
{
    return m_song->trackCount();
}

EditorService::TrackIndexList EditorService::trackIndices() const
{
    EditorService::TrackIndexList result;
    std::ranges::transform(m_song->trackIndices(), std::back_inserter(result),
                           [](std::size_t index) { return static_cast<quint64>(index); });
    return result;
}

quint64 EditorService::trackPositionByIndex(quint64 trackIndex) const
{
    return m_song->trackPositionByIndex(trackIndex).value_or(0);
}

quint64 EditorService::trackIndexByPosition(quint64 track) const
{
    return m_song->trackIndexByPosition(track).value_or(0);
}

QString EditorService::patternName(quint64 patternIndex) const
{
    return QString::fromStdString(m_song->patternName(patternIndex));
}

void EditorService::setPatternName(quint64 patternIndex, QString name)
{
    m_song->setPatternName(patternIndex, name.toStdString());

    setIsModified(true);
}

QString EditorService::trackName(quint64 trackIndex) const
{
    return QString::fromStdString(m_song->trackName(trackIndex));
}

void EditorService::setTrackName(quint64 trackIndex, QString name)
{
    if (m_song->trackName(trackIndex) != name.toStdString()) {
        m_song->setTrackName(trackIndex, name.toStdString());
        emit trackNameChanged();
        setIsModified(true);
    }
}

QString EditorService::columnName(quint64 trackIndex, quint64 columnIndex) const
{
    return QString::fromStdString(m_song->columnName(trackIndex, columnIndex));
}

void EditorService::setColumnName(quint64 trackIndex, quint64 columnIndex, QString name)
{
    if (m_song->columnName(trackIndex, columnIndex) != name.toStdString()) {
        m_song->setColumnName(trackIndex, columnIndex, name.toStdString());
        emit columnNameChanged();
        setIsModified(true);
    }
}

EditorService::InstrumentS EditorService::instrument(quint64 trackIndex) const
{
    return m_song->instrument(trackIndex);
}

void EditorService::setInstrument(quint64 trackIndex, InstrumentS instrument)
{
    m_song->setInstrument(trackIndex, instrument);

    setIsModified(true);
}

EditorService::InstrumentList EditorService::instruments() const
{
    EditorService::InstrumentList instrumentList;
    for (auto trackIndex : trackIndices()) {
        if (const auto instrument = this->instrument(trackIndex); instrument) {
            instrumentList.push_back({ trackIndex, instrument });
        }
    }
    return instrumentList;
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

bool EditorService::hasInstrumentSettings(quint64 pattern, quint64 track, quint64 column, quint64 line) const
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

quint64 EditorService::currentPattern() const
{
    return m_state.cursorPosition.pattern;
}

quint64 EditorService::currentTrack() const
{
    return m_state.cursorPosition.track;
}

quint64 EditorService::currentColumn() const
{
    return m_state.cursorPosition.column;
}

void EditorService::createPatternIfDoesNotExist(quint64 patternIndex)
{
    if (!m_song->hasPattern(patternIndex)) {
        juzzlin::L(TAG).debug() << "Creating pattern, index=" << patternIndex;
        m_song->createPattern(patternIndex);
        emit patternCreated(patternIndex);
        emit statusTextRequested(tr("A new pattern created!"));
        setIsModified(true);
        updateDuration();
    }
}

void EditorService::setCurrentPattern(quint64 patternIndex)
{
    if (currentPattern() != patternIndex) {

        const auto oldPosition = m_state.cursorPosition;
        m_state.cursorPosition.pattern = patternIndex;

        const auto oldLineCount = m_song->lineCount(oldPosition.pattern);
        createPatternIfDoesNotExist(patternIndex);

        if (const auto newLineCount = m_song->lineCount(m_state.cursorPosition.pattern); newLineCount != oldLineCount) {
            clampCursorLine(oldLineCount, newLineCount);
            emit currentLineCountChanged();
        }

        notifyPositionChange(oldPosition);
    }
}

QString EditorService::currentTime() const
{
    return m_state.currentTime;
}

QString EditorService::duration() const
{
    return m_state.duration;
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
    if (const QString newCurrentTime = getFormattedTime(currentTime); m_state.currentTime != newCurrentTime) {
        m_state.currentTime = newCurrentTime;
        emit currentTimeChanged();
    }
}

void EditorService::updateDuration()
{
    setDuration(m_song->duration());
}

void EditorService::setDuration(std::chrono::milliseconds duration)
{
    if (const QString newDuration = getFormattedTime(duration); m_state.duration != newDuration) {
        m_state.duration = newDuration;
        emit durationChanged();
    }
}

bool EditorService::hasData(quint64 patternIndex, quint64 trackIndex, quint64 columnIndex) const
{
    return m_song->hasData(patternIndex, trackIndex, columnIndex);
}

bool EditorService::isAtNoteColumn() const
{
    return !m_state.cursorPosition.lineColumn;
}

bool EditorService::isAtVelocityColumn() const
{
    return m_state.cursorPosition.lineColumn >= 1 && m_state.cursorPosition.lineColumn <= 3;
}

bool EditorService::isColumnVisible(quint64 track, quint64 column) const
{
    const int columnPosition = onScreenColumnPositionInUnits(track, column);
    return columnPosition >= 0 && columnPosition < static_cast<int>(visibleUnitCount());
}

bool EditorService::isModified() const
{
    return m_state.isModified;
}

void EditorService::setIsModified(bool isModified)
{
    if (m_state.isModified != isModified) {
        m_state.isModified = isModified;
        emit isModifiedChanged();
        emit canBeSavedChanged();
        if (isModified) {
            juzzlin::L(TAG).info() << "Project set as modified";
        }
    }
}

Position EditorService::position() const
{
    return m_state.cursorPosition;
}

quint64 EditorService::positionBarLine() const
{
    return 8;
}

bool EditorService::setVelocityAtCurrentPosition(uint8_t digit)
{
    juzzlin::L(TAG).debug() << "Set velocity digit at position " << m_state.cursorPosition.toString() << ": " << static_cast<int>(digit);

    const auto noteData = m_song->noteDataAtPosition(m_state.cursorPosition);
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

    if (m_state.cursorPosition.lineColumn == 1) {
        if (digit == 0 || digit == 1) {
            currentVelocity = (digit * 100) + (currentVelocity % 100);
            if (currentVelocity > 127) {
                currentVelocity = 127;
            }
        } else {
            return false; // Invalid digit for hundreds place
        }
    } else if (m_state.cursorPosition.lineColumn == 2) {
        currentVelocity = (currentVelocity / 100) * 100 + (digit * 10) + (currentVelocity % 10);
        if (currentVelocity > 127) {
            currentVelocity -= 100;
        }
    } else if (m_state.cursorPosition.lineColumn == 3) {
        currentVelocity = (currentVelocity / 10) * 10 + digit;
        if (currentVelocity > 127) {
            currentVelocity -= 10;
        }
    } else {
        return false;
    }

    if (currentVelocity <= 127) {
        noteData->setVelocity(currentVelocity);
        emit noteDataAtPositionChanged(m_state.cursorPosition);
        setIsModified(true);
        return true;
    }

    return false;
}

bool EditorService::requestDigitSetAtCurrentPosition(uint8_t digit)
{
    juzzlin::L(TAG).debug() << "Digit set requested at position " << m_state.cursorPosition.toString() << ": " << static_cast<int>(digit);

    if (isAtVelocityColumn()) {
        return setVelocityAtCurrentPosition(digit);
    }

    return false;
}

void EditorService::requestNewColumn(quint64 trackIndex)
{
    juzzlin::L(TAG).debug() << "New column requested on track " << trackIndex;

    m_song->addColumn(trackIndex);

    emit columnAdded(trackIndex);
    updateScrollBar();
    notifyPositionChange(m_state.cursorPosition); // Re-focuses the previous track
    setIsModified(true);
}

void EditorService::requestColumnDeletion(quint64 trackIndex)
{
    juzzlin::L(TAG).debug() << "Column deletion requested on track " << trackIndex;

    m_selectionService->clear();

    if (m_song->deleteColumn(trackIndex)) {
        const auto oldPosition = m_state.cursorPosition;
        if (oldPosition.track == trackIndex && m_state.cursorPosition.column >= m_song->columnCount(trackIndex)) {
            m_state.cursorPosition.column--;
        }
        notifyPositionChange(oldPosition);
        emit columnDeleted(trackIndex);
        updateScrollBar();
        notifyPositionChange(m_state.cursorPosition); // Re-focuses the previous track
        setIsModified(true);
    }
}

void EditorService::requestNewTrackToRight()
{
    juzzlin::L(TAG).debug() << "New track requested to the right of track " << position().track;
    m_song->addTrackToRightOf(position().track);
    emit trackConfigurationChanged();
    updateScrollBar();
    notifyPositionChange(m_state.cursorPosition); // Re-focuses the previous track
    setIsModified(true);
}

void EditorService::requestTrackDeletion()
{
    juzzlin::L(TAG).debug() << "Deletion of track requested: " << position().track;

    m_selectionService->clear();

    if (trackCount() > visibleUnitCount()) {
        const auto trackToDelete = position().track;
        if (m_song->isFirstTrack(trackToDelete)) {
            moveCursorToNextTrack();
        } else {
            moveCursorToPrevTrack();
            m_state.cursorPosition.lineColumn = 0;
        }
        notifyPositionChange(m_state.cursorPosition); // Re-focuses the previous track
        if (m_song->deleteTrack(trackToDelete)) {
            emit trackDeleted(trackToDelete);
            updateScrollBar();
            setIsModified(true);
        }
    } else {
        emit statusTextRequested(tr("Cannot have less than ") + QString::number(visibleUnitCount()) + " tracks");
    }
}

void EditorService::requestNoteInsertionAtCurrentPosition()
{
    insertNoteAtPosition(m_state.cursorPosition);
}

void EditorService::requestNoteDeletionAtCurrentPosition(bool shiftNotes)
{
    deleteNoteDataAtPosition(m_state.cursorPosition, shiftNotes);
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

bool EditorService::requestNoteOnAtCurrentPosition(quint8 key, quint8 octave, quint8 velocity)
{
    if (m_state.cursorPosition.lineColumn) {
        juzzlin::L(TAG).debug() << "Not on note column";
        return false;
    }

    if (const auto midiNote = NoteConverter::keyAndOctaveToMidiNote(key, octave); midiNote.has_value()) {
        juzzlin::L(TAG).debug() << "Note ON requested at position " << m_state.cursorPosition.toString() << ": " << midiNote->first
                                << ", MIDI Note = " << static_cast<int>(midiNote->second) << ", Velocity = " << static_cast<int>(velocity);
        NoteData noteData { m_state.cursorPosition.track, m_state.cursorPosition.column };
        noteData.setAsNoteOn(midiNote->second, velocity);
        m_song->setNoteDataAtPosition(noteData, m_state.cursorPosition);
        emit noteDataAtPositionChanged(m_state.cursorPosition);
        setIsModified(true);
        return true;
    } else {
        return false;
    }
}

void EditorService::removeDuplicateNoteOffs()
{
    juzzlin::L(TAG).debug() << "Removing duplicate note offs";
    if (const auto prevNoteDataPosition = m_song->prevNoteDataOnSameColumn(m_state.cursorPosition); prevNoteDataPosition != m_state.cursorPosition) {
        if (const auto previousNoteData = m_song->noteDataAtPosition(prevNoteDataPosition); previousNoteData->type() == NoteData::Type::NoteOff) {
            deleteNoteDataAtPosition(prevNoteDataPosition, false);
        }
    }
    if (const auto nextNoteDataPosition = m_song->nextNoteDataOnSameColumn(m_state.cursorPosition); nextNoteDataPosition != m_state.cursorPosition) {
        if (const auto nextNoteData = m_song->noteDataAtPosition(nextNoteDataPosition); nextNoteData->type() == NoteData::Type::NoteOff) {
            deleteNoteDataAtPosition(nextNoteDataPosition, false);
        }
    }
}

bool EditorService::requestNoteOffAtCurrentPosition()
{
    if (m_state.cursorPosition.lineColumn) {
        juzzlin::L(TAG).debug() << "Not on note column";
        return false;
    }

    NoteData noteData { m_state.cursorPosition.track, m_state.cursorPosition.column };
    noteData.setAsNoteOff();
    m_song->setNoteDataAtPosition(noteData, m_state.cursorPosition);
    emit noteDataAtPositionChanged(m_state.cursorPosition);
    setIsModified(true);

    removeDuplicateNoteOffs();

    return true;
}

void EditorService::logPosition() const
{
#ifdef NOTEAHEAD_DEBUG
    juzzlin::L(TAG).trace() << "Position: " << m_state.m_cursorPosition.toString();
#endif
}

void EditorService::notifyPositionChange(const Position & oldPosition)
{
    logPosition();

    emit positionChanged(m_state.cursorPosition, oldPosition);

    if (m_state.cursorPosition.pattern != oldPosition.pattern) {
        emit currentPatternChanged();
        emit currentLineCountChanged();
    }
}

void EditorService::requestColumnCut()
{
    juzzlin::L(TAG).info() << "Requesting column cut";
    for (auto && changedPosition : m_song->cutColumn(currentPattern(), currentTrack(), currentColumn(), m_state.copyManager)) {
        emit noteDataAtPositionChanged(changedPosition);
    }
    emit copyManagerStateChanged();
    emit statusTextRequested(tr("Column cut"));
    setIsModified(true);
}

void EditorService::requestColumnCopy()
{
    juzzlin::L(TAG).info() << "Requesting column copy";
    m_song->copyColumn(currentPattern(), currentTrack(), currentColumn(), m_state.copyManager);
    emit copyManagerStateChanged();
    emit statusTextRequested(tr("Column copied"));
}

void EditorService::requestColumnPaste()
{
    try {
        juzzlin::L(TAG).info() << "Requesting paste for copied column";
        for (auto && changedPosition : m_song->pasteColumn(currentPattern(), currentTrack(), currentColumn(), m_state.copyManager)) {
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
    for (auto && changedPosition : m_song->transposeColumn(m_state.cursorPosition, semitones)) {
        emit noteDataAtPositionChanged(changedPosition);
    }
    setIsModified(true);
}

EditorService::LineList EditorService::columnData(ColumnAddress columnAddress) const
{
    const auto [patternIndex, trackIndex, columnIndex] = columnAddress;
    juzzlin::L(TAG).trace() << "Requesting line data for patternIndex=" << patternIndex << ", trackIndex=" << trackIndex << ", columnIndex=" << columnIndex;
    return m_song->lines({ patternIndex, trackIndex, columnIndex, 0 });
}

bool EditorService::hasColumnToPaste() const
{
    return m_state.copyManager.mode() == CopyManager::Mode::Column;
}

void EditorService::requestTrackCut()
{
    juzzlin::L(TAG).info() << "Requesting track cut";
    for (auto && changedPosition : m_song->cutTrack(currentPattern(), currentTrack(), m_state.copyManager)) {
        emit noteDataAtPositionChanged(changedPosition);
    }
    emit copyManagerStateChanged();
    emit statusTextRequested(tr("Track cut"));
    setIsModified(true);
}

void EditorService::requestTrackCopy()
{
    juzzlin::L(TAG).info() << "Requesting track copy";
    m_song->copyTrack(currentPattern(), currentTrack(), m_state.copyManager);
    emit copyManagerStateChanged();
    emit statusTextRequested(tr("Track copied"));
}

void EditorService::requestTrackPaste()
{
    try {
        juzzlin::L(TAG).info() << "Requesting paste for copied track";
        for (auto && changedPosition : m_song->pasteTrack(currentPattern(), currentTrack(), m_state.copyManager)) {
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
    return m_state.copyManager.mode() == CopyManager::Mode::Track;
}

void EditorService::requestTrackTranspose(int semitones)
{
    for (auto && changedPosition : m_song->transposeTrack(m_state.cursorPosition, semitones)) {
        emit noteDataAtPositionChanged(changedPosition);
    }
    setIsModified(true);
}

void EditorService::requestPatternCut()
{
    juzzlin::L(TAG).info() << "Requesting pattern cut";
    for (auto && changedPosition : m_song->cutPattern(currentPattern(), m_state.copyManager)) {
        emit noteDataAtPositionChanged(changedPosition);
    }
    emit statusTextRequested(tr("Pattern cut"));
    emit copyManagerStateChanged();
    setIsModified(true);
}

void EditorService::requestPatternCopy()
{
    juzzlin::L(TAG).info() << "Requesting pattern copy";
    m_song->copyPattern(currentPattern(), m_state.copyManager);
    emit copyManagerStateChanged();
    emit statusTextRequested(tr("Pattern copied"));
}

void EditorService::requestPatternPaste()
{
    try {
        juzzlin::L(TAG).info() << "Requesting paste for copied pattern";
        for (auto && changedPosition : m_song->pastePattern(currentPattern(), m_state.copyManager)) {
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
    return m_state.copyManager.mode() == CopyManager::Mode::Pattern;
}

void EditorService::requestPatternTranspose(int semitones)
{
    for (auto && changedPosition : m_song->transposePattern(m_state.cursorPosition, semitones)) {
        emit noteDataAtPositionChanged(changedPosition);
    }
    setIsModified(true);
}

void EditorService::requestSelectionCut()
{
    juzzlin::L(TAG).info() << "Requesting selection cut";
    for (auto && changedPosition : m_song->cutSelection(m_selectionService->selectedPositions(), m_state.copyManager)) {
        emit noteDataAtPositionChanged(changedPosition);
    }
    emit statusTextRequested(tr("Selection cut"));
    emit copyManagerStateChanged();
    setIsModified(true);
}

void EditorService::requestSelectionCopy()
{
    juzzlin::L(TAG).info() << "Requesting selection copy";
    m_song->copySelection(m_selectionService->selectedPositions(), m_state.copyManager);
    emit copyManagerStateChanged();
    emit statusTextRequested(tr("Selection copied"));
}

void EditorService::requestSelectionPaste()
{
    try {
        juzzlin::L(TAG).info() << "Requesting paste for copied selection";
        for (auto && changedPosition : m_song->pasteSelection(position(), m_state.copyManager)) {
            emit noteDataAtPositionChanged(changedPosition);
        }
        emit statusTextRequested(tr("Copied selection pasted"));
        setIsModified(true);
        updateDuration();
    } catch (const std::runtime_error & e) {
        emit statusTextRequested(tr("Failed to paste selection: ") + e.what());
    }
}

bool EditorService::hasSelectionToPaste() const
{
    return m_state.copyManager.mode() == CopyManager::Mode::Selection;
}

void EditorService::requestSelectionTranspose(int semitones)
{
    juzzlin::L(TAG).info() << "Requesting selection transpose by " << semitones << " semitones";
    if (m_selectionService->isValidSelection()) {
        bool modified = false;
        for (auto && position : m_selectionService->selectedPositions()) {
            if (const auto noteData = m_song->noteDataAtPosition(position); noteData && noteData->type() == NoteData::Type::NoteOn) {
                noteData->transpose(semitones);
                emit noteDataAtPositionChanged(position);
                modified = true;
            }
        }
        setIsModified(modified);
    }
}

void EditorService::requestLinearVelocityInterpolationOnColumn(quint64 startLine, quint64 endLine, quint8 startValue, quint8 endValue)
{
    auto start = position();
    start.line = startLine;

    auto end = position();
    end.line = endLine;

    if (const auto changedPositions = NoteDataManipulator::interpolateVelocityOnColumn(m_song, start, end, startValue, endValue); !changedPositions.empty()) {
        for (auto && position : changedPositions) {
            emit noteDataAtPositionChanged(position);
        }
        setIsModified(true);
    }
}

void EditorService::requestLinearVelocityInterpolationOnTrack(quint64 startLine, quint64 endLine, quint8 startValue, quint8 endValue)
{
    auto start = position();
    start.line = startLine;

    auto end = position();
    end.line = endLine;

    if (const auto changedPositions = NoteDataManipulator::interpolateVelocityOnTrack(m_song, start, end, startValue, endValue); !changedPositions.empty()) {
        for (auto && position : changedPositions) {
            emit noteDataAtPositionChanged(position);
        }
        setIsModified(true);
    }
}

void EditorService::setDelayOnCurrentLine(quint8 ticks)
{
    if (const auto noteData = m_song->noteDataAtPosition(position()); noteData) {
        noteData->setDelay(ticks);
    }
}

quint8 EditorService::delayAtCurrentPosition() const
{
    if (const auto noteData = m_song->noteDataAtPosition(position()); noteData) {
        return noteData->delay();
    }

    return 0;
}

bool EditorService::requestPosition(const Position & position)
{
    return requestPosition(position.pattern, position.track, position.column, static_cast<qint32>(position.line), position.lineColumn);
}

bool EditorService::requestPosition(quint64 pattern, quint64 track, quint64 column, qint64 line, quint64 lineColumn)
{
    juzzlin::L(TAG).debug() << "Requesting position: " << pattern << " " << track << " " << column << " " << line << " " << lineColumn;

    if (pattern >= m_song->patternCount()) {
        juzzlin::L(TAG).error() << "Invalid pattern index: " << pattern;
        return false;
    }

    // The requested line index can be outside the valid indices if clicked on the empty area of a column.
    // In that case we'll just clamp to either zero or to the last index which is what the user very likely wants.
    line = std::min(static_cast<int>(line), static_cast<int>(m_song->lineCount(pattern)) - 1);
    line = std::max(static_cast<int>(line), 0);

    if (!m_song->hasTrack(track)) {
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

    const auto oldPosition = m_state.cursorPosition;
    m_state.cursorPosition.pattern = pattern;
    m_state.cursorPosition.track = track;
    m_state.cursorPosition.column = column;
    m_state.cursorPosition.line = static_cast<quint64>(line);
    m_state.cursorPosition.lineColumn = lineColumn;
    notifyPositionChange(oldPosition);

    setCurrentTime(m_song->lineToTime(static_cast<quint32>(m_state.cursorPosition.line)));

    return true;
}

void EditorService::requestPositionByTick(quint64 tick)
{
    // Skip unnecessary updates
    if (tick % m_song->ticksPerLine()) {
        return;
    }

    const auto oldPosition = m_state.cursorPosition;
    if (auto && songPosition = m_song->songPositionByTick(tick); songPosition.has_value()) {
        m_state.cursorPosition.pattern = songPosition->pattern;
        m_state.cursorPosition.line = static_cast<quint64>(songPosition->line);
        notifyPositionChange(oldPosition);
        setSongPosition(songPosition->position);
        setCurrentTime(songPosition->currentTime);
    }
}

void EditorService::requestScroll(int steps)
{
    const auto oldPosition = m_state.cursorPosition;

    // Work in signed domain to handle negative steps correctly
    auto newLine = static_cast<qint64>(m_state.cursorPosition.line) + steps;
    const auto lineCount = static_cast<qint64>(m_song->lineCount(m_state.cursorPosition.pattern));

    // Wrap around correctly
    if (newLine < 0) {
        newLine = (newLine % lineCount + lineCount) % lineCount; // Proper modulo for negatives
    } else {
        newLine %= lineCount;
    }

    m_state.cursorPosition.line = static_cast<quint64>(newLine);

    setCurrentTime(m_song->lineToTime(m_state.cursorPosition.line));
    notifyPositionChange(oldPosition);
}

quint64 EditorService::ticksPerLine() const
{
    return m_song->ticksPerLine();
}

quint64 EditorService::beatsPerMinute() const
{
    return m_song->beatsPerMinute();
}

void EditorService::setBeatsPerMinute(quint64 beatsPerMinute)
{
    if (m_song->beatsPerMinute() != beatsPerMinute) {
        m_song->setBeatsPerMinute(beatsPerMinute);
        emit beatsPerMinuteChanged();
        setIsModified(true);
        updateDuration();
    }
}

quint64 EditorService::linesPerBeat() const
{
    return m_song->linesPerBeat();
}

void EditorService::setLinesPerBeat(quint64 linesPerBeat)
{
    if (m_song->linesPerBeat() != linesPerBeat) {
        m_song->setLinesPerBeat(linesPerBeat);
        emit linesPerBeatChanged();
        setIsModified(true);
        updateDuration();
    }
}

quint64 EditorService::visibleUnitCount() const
{
    return 6;
}

quint64 EditorService::horizontalScrollPosition() const
{
    return m_state.horizontalScrollPosition;
}

void EditorService::setHorizontalScrollPosition(double position)
{
    const auto oldPosition = m_state.horizontalScrollPosition;

    if (visibleUnitCount() < totalUnitCount()) {
        const auto maxPosition = totalUnitCount() - visibleUnitCount();
        m_state.horizontalScrollPosition = static_cast<quint64>(std::ceil(position * static_cast<double>(maxPosition)));
        m_state.horizontalScrollPosition = std::min(m_state.horizontalScrollPosition, maxPosition);
    } else {
        m_state.horizontalScrollPosition = 0;
    }

    if (m_state.horizontalScrollPosition != oldPosition) {
        emit horizontalScrollChanged();
        notifyPositionChange(m_state.cursorPosition); // Forces vertical scroll update
    }
}

void EditorService::moveCursorToPrevTrack()
{
    if (const auto currentTrack = m_song->trackPositionByIndex(m_state.cursorPosition.track); currentTrack.has_value()) {
        const qint64 trackCount = static_cast<qint64>(m_song->trackIndices().size());
        qint64 newTrack = static_cast<qint64>(*currentTrack) - 1;
        // Wrap around correctly for negative index
        newTrack = (newTrack % trackCount + trackCount) % trackCount;
        m_state.cursorPosition.track = m_song->trackIndices().at(static_cast<quint64>(newTrack));
        m_state.cursorPosition.column = m_song->columnCount(m_state.cursorPosition.track) - 1;
        m_state.cursorPosition.lineColumn = 3;
    }
}

void EditorService::moveCursorToNextTrack()
{
    if (const auto currentTrack = m_song->trackPositionByIndex(m_state.cursorPosition.track); currentTrack.has_value()) {
        quint64 newTrack = *currentTrack + 1;
        newTrack %= m_song->trackIndices().size();
        m_state.cursorPosition.track = m_song->trackIndices().at(newTrack);
        m_state.cursorPosition.column = 0;
        m_state.cursorPosition.lineColumn = 0;
    }
}

void EditorService::ensureFocusedColumnIsVisible()
{
    const auto onScreenColumnPosition = onScreenColumnPositionInUnits(m_state.cursorPosition.track, m_state.cursorPosition.column);
    if (onScreenColumnPosition < 0 || //
        onScreenColumnPosition >= static_cast<int>(visibleUnitCount())) {
        const auto columnPositionInUnits = this->columnPositionInUnits(m_state.cursorPosition.track, m_state.cursorPosition.column);
        juzzlin::L(TAG).debug() << "Column position in units: " << columnPositionInUnits;
        juzzlin::L(TAG).debug() << "Total unit count: " << totalUnitCount();
        const auto newScroll = static_cast<double>(columnPositionInUnits) / static_cast<double>(totalUnitCount());
        juzzlin::L(TAG).debug() << "Setting scroll position to: " << newScroll;
        setHorizontalScrollPosition(newScroll);
        updateScrollBar();
    }
}

void EditorService::ensureFocusedTrackIsVisible()
{
    const auto onScreenTrackPosition = onScreenTrackPositionInUnits(m_state.cursorPosition.track);
    if (onScreenTrackPosition < 0 || //
        onScreenTrackPosition >= static_cast<int>(visibleUnitCount())) {
        const auto trackPositionInUnits = this->trackPositionInUnits(m_state.cursorPosition.track);
        juzzlin::L(TAG).debug() << "Track position in units: " << trackPositionInUnits;
        juzzlin::L(TAG).debug() << "Total unit count: " << totalUnitCount();
        const auto newScroll = static_cast<double>(trackPositionInUnits) / static_cast<double>(totalUnitCount());
        juzzlin::L(TAG).debug() << "Setting scroll position to: " << newScroll;
        setHorizontalScrollPosition(newScroll);
        updateScrollBar();
    }
}

void EditorService::requestCursorLeft()
{
    juzzlin::L(TAG).debug() << "Cursor left requested";
    const auto oldPosition = m_state.cursorPosition;
    // Switch line column => switch column => switch track
    if (m_state.cursorPosition.lineColumn) {
        m_state.cursorPosition.lineColumn--;
    } else {
        m_state.cursorPosition.lineColumn = 3;
        if (m_state.cursorPosition.column) {
            m_state.cursorPosition.column--;
        } else {
            moveCursorToPrevTrack();
        }
    }
    ensureFocusedColumnIsVisible();
    notifyPositionChange(oldPosition);
}

void EditorService::requestCursorRight()
{
    juzzlin::L(TAG).debug() << "Cursor right requested";
    const auto oldPosition = m_state.cursorPosition;
    // Switch line column => switch column => switch track
    if (m_state.cursorPosition.lineColumn < 3) {
        m_state.cursorPosition.lineColumn++;
    } else {
        m_state.cursorPosition.lineColumn = 0;
        if (m_state.cursorPosition.column + 1 < m_song->columnCount(m_state.cursorPosition.track)) {
            m_state.cursorPosition.column++;
        } else {
            moveCursorToNextTrack();
        }
    }
    notifyPositionChange(oldPosition);
    ensureFocusedColumnIsVisible();
}

void EditorService::requestTrackRight()
{
    juzzlin::L(TAG).debug() << "Track right requested";
    const auto oldPosition = m_state.cursorPosition;
    moveCursorToNextTrack();
    notifyPositionChange(oldPosition);
    ensureFocusedTrackIsVisible();
}

void EditorService::requestColumnLeft()
{
    juzzlin::L(TAG).debug() << "Column left requested";
    const auto oldPosition = m_state.cursorPosition;
    if (oldPosition.column) {
        m_state.cursorPosition.column--;
        notifyPositionChange(oldPosition);
        ensureFocusedColumnIsVisible();
    }
}

void EditorService::requestColumnRight(bool isSelecting)
{
    juzzlin::L(TAG).debug() << "Column right requested";
    const auto oldPosition = m_state.cursorPosition;
    if (oldPosition.column + 1 < m_song->columnCount(oldPosition.track)) {
        m_state.cursorPosition.column++;
    } else if (!isSelecting) {
        moveCursorToNextTrack();
    }
    notifyPositionChange(oldPosition);
    ensureFocusedColumnIsVisible();
}

quint64 EditorService::totalUnitCount() const
{
    quint64 columnCount = 0;
    for (auto && trackIndex : m_song->trackIndices()) {
        columnCount += m_song->columnCount(trackIndex);
    }
    return columnCount;
}

quint64 EditorService::trackWidthInUnits(quint64 trackIndex) const
{
    return m_song->columnCount(trackIndex);
}

quint64 EditorService::columnPositionInUnits(quint64 trackIndex, quint64 columnIndex) const
{
    return trackPositionInUnits(trackIndex) + columnIndex;
}

quint64 EditorService::trackPositionInUnits(quint64 trackIndex) const
{
    quint64 unitPosition = 0;
    const auto trackPosition = m_song->trackPositionByIndex(trackIndex);
    for (quint64 track = 0; track < trackPosition; track++) {
        unitPosition += m_song->columnCount(m_song->trackIndexByPosition(track).value_or(0));
    }
    return unitPosition;
}

int EditorService::onScreenColumnPositionInUnits(quint64 trackIndex, quint64 columnIndex) const
{
    return onScreenTrackPositionInUnits(trackIndex) + static_cast<int>(columnIndex);
}

int EditorService::onScreenTrackPositionInUnits(quint64 trackIndex) const
{
    int unitPosition = -static_cast<int>(m_state.horizontalScrollPosition);
    const auto trackPosition = m_song->trackPositionByIndex(trackIndex);
    for (quint64 track = 0; track < trackPosition; track++) {
        unitPosition += m_song->columnCount(m_song->trackIndexByPosition(track).value_or(0));
    }
    return unitPosition;
}

void EditorService::requestHorizontalScrollBarPositionChange(double scrollBarPosition)
{
    setHorizontalScrollPosition(scrollBarPosition / (1.0 - scrollBarHandleSize()));
}

double EditorService::scrollBarStepSize() const
{
    return 1.0 / static_cast<double>(totalUnitCount() - visibleUnitCount());
}

double EditorService::scrollBarHandleSize() const
{
    return static_cast<double>(visibleUnitCount()) / static_cast<double>(totalUnitCount());
}

double EditorService::scrollBarPosition() const
{
    return static_cast<double>(m_state.horizontalScrollPosition) * scrollBarStepSize() * (1.0 - scrollBarHandleSize());
}

void EditorService::updateScrollBar()
{
    emit scrollBarHandleSizeChanged();
    emit scrollBarStepSizeChanged();
    emit scrollBarPositionChanged();
}

quint64 EditorService::songPosition() const
{
    return m_state.songPosition;
}

void EditorService::setSongPosition(quint64 songPosition)
{
    if (m_state.songPosition != songPosition) {
        m_state.songPosition = songPosition;
        emit songPositionChanged(songPosition);
        emit patternAtCurrentSongPositionChanged();
        if (songPosition >= songLength()) {
            setSongLength(songPosition + 1);
        }
    }
}

void EditorService::resetSongPosition()
{
    setSongPosition(0);
    const auto oldPosition = m_state.cursorPosition;
    const quint64 firstPattern = m_song->patternAtSongPosition(0);
    quint64 track = oldPosition.track;
    if (!m_song->trackPositionByIndex(track).has_value()) {
        track = m_song->trackIndexByPosition(0).value_or(0);
    }
    m_state.cursorPosition = { firstPattern, track, 0, 0, 0 };
    notifyPositionChange(oldPosition);
}

void EditorService::setPatternAtSongPosition(quint64 songPosition, quint64 pattern)
{
    setCurrentPattern(pattern);

    if (m_song->patternAtSongPosition(songPosition) != pattern) {
        m_song->setPatternAtSongPosition(songPosition, pattern);
        if (m_state.songPosition == songPosition) {
            emit patternAtCurrentSongPositionChanged();
        }
        if (songPosition >= songLength()) {
            setSongLength(songPosition + 1);
        }
        setIsModified(true);
        updateDuration();
    }
}

quint64 EditorService::patternAtCurrentSongPosition() const
{
    return m_song->patternAtSongPosition(m_state.songPosition);
}

void EditorService::insertPatternToPlayOrder()
{
    m_song->insertPatternToPlayOrder(m_state.songPosition);
    emit songPositionChanged(m_state.songPosition);
    emit patternAtCurrentSongPositionChanged();
    setSongLength(songLength() + 1);
    setIsModified(true);
    updateDuration();
}

void EditorService::removePatternFromPlayOrder()
{
    m_song->removePatternFromPlayOrder(m_state.songPosition);
    emit songPositionChanged(m_state.songPosition);
    emit patternAtCurrentSongPositionChanged();
    setSongLength(songLength() - 1);
    setIsModified(true);
    updateDuration();
}

quint64 EditorService::patternAtSongPosition(quint64 songPosition) const
{
    return m_song->patternAtSongPosition(songPosition);
}

quint64 EditorService::songLength() const
{
    return m_song->length();
}

void EditorService::setSongLength(quint64 songLength)
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

quint64 EditorService::maxSongLength() const
{
    return 999;
}

EditorService::~EditorService() = default;

} // namespace noteahead
