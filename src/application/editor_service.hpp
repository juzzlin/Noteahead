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

#ifndef EDITOR_SERVICE_HPP
#define EDITOR_SERVICE_HPP

#include "position.hpp"
#include <QObject>

#include <optional>
#include <utility>

namespace cacophony {

class Song;

class EditorService : public QObject
{
    Q_OBJECT
    Q_PROPERTY(uint32_t linesPerBeat READ linesPerBeat NOTIFY linesPerBeatChanged)
    Q_PROPERTY(bool isModified READ isModified NOTIFY isModifiedChanged)

public:
    EditorService();

    void initialize();

    using SongS = std::shared_ptr<Song>;

    SongS song() const;

    void setSong(SongS song);

    void saveAs(QString fileName);

    Q_INVOKABLE bool canBeSaved() const;

    Q_INVOKABLE uint32_t columnCount(uint32_t trackId) const;

    Q_INVOKABLE uint32_t lineCount(uint32_t patternId) const;

    Q_INVOKABLE uint32_t currentLineCount() const;

    Q_INVOKABLE int lineNumberAtViewLine(uint32_t line) const;

    Q_INVOKABLE uint32_t linesVisible() const;

    Q_INVOKABLE QString displayNoteAtPosition(uint32_t patternId, uint32_t trackId, uint32_t columnId, uint32_t line) const;

    Q_INVOKABLE QString displayVelocityAtPosition(uint32_t pattern, uint32_t track, uint32_t column, uint32_t line) const;

    Q_INVOKABLE double effectiveVolumeAtPosition(uint32_t pattern, uint32_t track, uint32_t column, uint32_t line) const;

    Q_INVOKABLE QString noDataString() const;

    Q_INVOKABLE uint32_t patternCount() const;

    Q_INVOKABLE uint32_t trackCount() const;

    Q_INVOKABLE QString trackName(uint32_t trackId) const;

    Q_INVOKABLE void setTrackName(uint32_t trackId, QString name);

    Q_INVOKABLE uint32_t currentPatternId() const;

    Q_INVOKABLE void setCurrentPatternId(uint32_t currentPatternId);

    Q_INVOKABLE bool isAtNoteColumn() const;

    Q_INVOKABLE bool isAtVelocityColumn() const;

    Q_INVOKABLE bool isModified() const;

    Q_INVOKABLE Position position() const;

    Q_INVOKABLE uint32_t positionBarLine() const;

    Q_INVOKABLE void requestCursorLeft();

    Q_INVOKABLE void requestCursorRight();

    Q_INVOKABLE bool requestDigitSetAtCurrentPosition(uint8_t digit);

    Q_INVOKABLE void requestNoteDeletionAtCurrentPosition();

    Q_INVOKABLE bool requestNoteOnAtCurrentPosition(uint8_t note, uint8_t octave, uint8_t velocity);

    Q_INVOKABLE bool requestPosition(uint32_t pattern, uint32_t track, uint32_t column, uint32_t line, uint32_t lineColumn);

    Q_INVOKABLE void requestScroll(int steps);

    Q_INVOKABLE void requestTrackFocus(uint32_t trackId);

    Q_INVOKABLE uint32_t beatsPerMinute() const;

    Q_INVOKABLE void setBeatsPerMinute(uint32_t bpm);

    Q_INVOKABLE uint32_t linesPerBeat() const;

    Q_INVOKABLE void setLinesPerBeat(uint32_t linesPerBeat);

signals:
    void currentPatternChanged();

    void linesPerBeatChanged();

    void isModifiedChanged();

    void noteDataAtPositionChanged(const Position & position);

    void positionChanged(const Position & newPosition, const Position & oldPosition);

    void songChanged();

public slots:
    void requestPositionByTick(uint32_t tick);

private:
    using MidiNoteNameAndCode = std::pair<std::string, uint8_t>;
    using MidiNoteNameAndCodeOpt = std::optional<MidiNoteNameAndCode>;
    MidiNoteNameAndCodeOpt editorNoteToMidiNote(uint32_t note, uint32_t octave) const;

    void logPosition() const;

    void notifyPositionChange(const Position & oldPosition);

    QString padVelocityToThreeDigits(QString velocity) const;

    bool setVelocityAtCurrentPosition(uint8_t digit);

    void setIsModified(bool isModified);

    SongS m_song;

    uint32_t m_currentPatternId = 0;

    Position m_position;

    bool m_isModified = false;
};

} // namespace cacophony

#endif // EDITOR_SERVICE_HPP
