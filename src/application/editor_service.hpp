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

class QXmlStreamReader;

namespace cacophony {

class Song;

class EditorService : public QObject
{
    Q_OBJECT
    Q_PROPERTY(uint32_t beatsPerMinute READ beatsPerMinute NOTIFY beatsPerMinuteChanged)
    Q_PROPERTY(uint32_t linesPerBeat READ linesPerBeat NOTIFY linesPerBeatChanged)
    Q_PROPERTY(bool isModified READ isModified NOTIFY isModifiedChanged)
    Q_PROPERTY(bool canBeSaved READ canBeSaved NOTIFY canBeSavedChanged)
    Q_PROPERTY(QString currentFileName READ currentFileName NOTIFY currentFileNameChanged)
    Q_PROPERTY(uint32_t currentLineCount READ currentLineCount NOTIFY currentLineCountChanged)
    Q_PROPERTY(uint32_t currentPattern READ currentPattern NOTIFY currentPatternChanged)
    Q_PROPERTY(double scrollBarSize READ scrollBarSize NOTIFY scrollBarSizeChanged)
    Q_PROPERTY(double scrollBarStepSize READ scrollBarStepSize NOTIFY scrollBarStepSizeChanged)

public:
    EditorService();

    void initialize();

    using SongS = std::shared_ptr<Song>;

    SongS song() const;

    void setSong(SongS song);

    void load(QString fileName);

    void save();

    void saveAs(QString fileName);

    void fromXml(QString xml);

    QString toXml() const;

    Q_INVOKABLE bool canBeSaved() const;

    Q_INVOKABLE uint32_t columnCount(uint32_t trackIndex) const;

    Q_INVOKABLE uint32_t lineCount(uint32_t patternId) const;

    Q_INVOKABLE QString currentFileName() const;

    Q_INVOKABLE uint32_t currentLineCount() const;

    Q_INVOKABLE void setCurrentLineCount(uint32_t lineCount);

    Q_INVOKABLE uint32_t minLineCount() const;

    Q_INVOKABLE uint32_t maxLineCount() const;

    Q_INVOKABLE uint32_t minPatternIndex() const;

    Q_INVOKABLE uint32_t maxPatternIndex() const;

    Q_INVOKABLE int lineNumberAtViewLine(uint32_t line) const;

    Q_INVOKABLE uint32_t linesVisible() const;

    Q_INVOKABLE QString displayNoteAtPosition(uint32_t patternId, uint32_t trackIndex, uint32_t columnId, uint32_t line) const;

    Q_INVOKABLE QString displayVelocityAtPosition(uint32_t pattern, uint32_t track, uint32_t column, uint32_t line) const;

    Q_INVOKABLE double effectiveVolumeAtPosition(uint32_t pattern, uint32_t track, uint32_t column, uint32_t line) const;

    Q_INVOKABLE QString noDataString() const;

    Q_INVOKABLE uint32_t patternCount() const;

    Q_INVOKABLE uint32_t trackCount() const;

    Q_INVOKABLE QString trackName(uint32_t trackIndex) const;

    Q_INVOKABLE void setTrackName(uint32_t trackIndex, QString name);

    Q_INVOKABLE uint32_t currentPattern() const;

    Q_INVOKABLE void setCurrentPattern(uint32_t currentPattern);

    Q_INVOKABLE bool hasData(uint32_t pattern, uint32_t track, uint32_t column) const;

    Q_INVOKABLE bool isAtNoteColumn() const;

    Q_INVOKABLE bool isAtVelocityColumn() const;

    Q_INVOKABLE bool isModified() const;

    Q_INVOKABLE Position position() const;

    Q_INVOKABLE uint32_t positionBarLine() const;

    Q_INVOKABLE void requestCursorLeft();

    Q_INVOKABLE void requestCursorRight();

    Q_INVOKABLE void requestTrackRight();

    Q_INVOKABLE void requestColumnRight();

    Q_INVOKABLE bool requestDigitSetAtCurrentPosition(uint8_t digit);

    Q_INVOKABLE void requestNewColumn(uint32_t track);

    Q_INVOKABLE void requestNoteDeletionAtCurrentPosition();

    Q_INVOKABLE bool requestNoteOnAtCurrentPosition(uint8_t note, uint8_t octave, uint8_t velocity);

    Q_INVOKABLE bool requestNoteOffAtCurrentPosition();

    Q_INVOKABLE bool requestPosition(uint32_t pattern, uint32_t track, uint32_t column, uint32_t line, uint32_t lineColumn);

    Q_INVOKABLE void requestScroll(int steps);

    Q_INVOKABLE void requestTrackFocus(uint32_t track, uint32_t column);

    Q_INVOKABLE uint32_t beatsPerMinute() const;

    Q_INVOKABLE void setBeatsPerMinute(uint32_t beatsPerMinute);

    Q_INVOKABLE uint32_t linesPerBeat() const;

    Q_INVOKABLE void setLinesPerBeat(uint32_t linesPerBeat);

    Q_INVOKABLE uint32_t visibleUnitCount() const;

    Q_INVOKABLE uint32_t totalUnitCount() const;

    Q_INVOKABLE uint32_t horizontalScrollPosition() const;

    Q_INVOKABLE uint32_t trackWidthInUnits(uint32_t trackIndex) const;

    Q_INVOKABLE int trackPositionInUnits(uint32_t trackIndex) const;

    Q_INVOKABLE void requestHorizontalScrollPositionChange(double position);

    Q_INVOKABLE double scrollBarStepSize() const;

    Q_INVOKABLE double scrollBarSize() const;

    void setIsModified(bool isModified);

signals:
    void beatsPerMinuteChanged();

    void canBeSavedChanged();

    void currentFileNameChanged();

    void currentLineCountChanged(); // For the pattern length widget

    void currentLineCountModified(uint32_t oldLineCount, uint32_t newLineCount);

    void currentPatternChanged(); // For the pattern index widget

    void linesPerBeatChanged();

    void horizontalScrollChanged();

    void isModifiedChanged();

    void noteDataAtPositionChanged(const Position & position);

    void patternCreated(uint32_t patternIndex);

    void positionChanged(const Position & newPosition, const Position & oldPosition);

    void songChanged();

    void statusTextRequested(QString text);

    void trackConfigurationChanged();

    void scrollBarStepSizeChanged();

    void scrollBarSizeChanged();

public slots:
    void requestPositionByTick(uint32_t tick);

private:
    void clampCursorLine(size_t oldLineCount, size_t newLineCount);

    void createPatternIfDoesNotExist(uint32_t patternIndex);

    using MidiNoteNameAndCode = std::pair<std::string, uint8_t>;
    using MidiNoteNameAndCodeOpt = std::optional<MidiNoteNameAndCode>;
    MidiNoteNameAndCodeOpt editorNoteToMidiNote(uint32_t note, uint32_t octave) const;

    void deleteNoteDataAtPosition(const Position & position);

    SongS deserializeProject(QXmlStreamReader & reader);

    void logPosition() const;

    void notifyPositionChange(const Position & oldPosition);

    QString padVelocityToThreeDigits(QString velocity) const;

    void removeDuplicateNoteOffs();

    void resetCursorPosition();

    bool setVelocityAtCurrentPosition(uint8_t digit);

    void updateScrollBar();

    SongS m_song;

    Position m_cursorPosition;

    uint32_t m_horizontalScrollPosition = 0;

    bool m_isModified = false;
};

} // namespace cacophony

#endif // EDITOR_SERVICE_HPP
