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

#include "../contrib/SimpleLogger/src/simple_logger.hpp"
#include "../domain/note_data.hpp"
#include "../domain/song.hpp"
#include "note_converter.hpp"

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

void EditorService::setSong(SongS song)
{
    m_song = song;

    m_position = {};

    emit songChanged();
    emit positionChanged(m_position);
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
    const int lineNumber = (static_cast<int>(line) + m_position.line - static_cast<int>(positionBarLine()));
    const int lineCount = static_cast<int>(this->lineCount(currentPatternId()));
    if (lineNumber < 0) {
        return -(lineCount + lineNumber);
    } else {
        return lineNumber;
    }
}

QString EditorService::noteAtPosition(uint32_t patternId, uint32_t trackId, uint32_t columnId, uint32_t line) const
{
    if (const auto noteData = m_song->noteDataAtPosition(patternId, trackId, columnId, line); noteData) {
        return noteData->noteOff ? "OFF" : NoteConverter::midiToString(noteData->noteOn).c_str();
    } else {
        return "---";
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

Position EditorService::position() const
{
    return m_position;
}

uint32_t EditorService::positionBarLine() const
{
    return 8;
}

void EditorService::requestScroll(int steps)
{
    m_position.line += steps;
    m_position.line %= m_song->lineCount(m_position.pattern);
    emit positionChanged(m_position);
}

void EditorService::requestTrackFocus(uint32_t trackId)
{
    juzzlin::L(TAG).info() << "Focus for track " << trackId << " requested";
    if (trackId < trackCount()) {
        m_position.track = trackId;
        emit positionChanged(m_position);
    }
}

} // namespace cacophony
