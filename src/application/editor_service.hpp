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

#ifndef EDITOR_SERVICE_HPP
#define EDITOR_SERVICE_HPP

#include "position.hpp"
#include <QObject>

#include <optional>
#include <utility>

class QXmlStreamReader;

namespace noteahead {

class CopyManager;
class Song;
class Instrument;
class InstrumentRequest;

class EditorService : public QObject
{
    Q_OBJECT

    Q_PROPERTY(size_t beatsPerMinute READ beatsPerMinute NOTIFY beatsPerMinuteChanged)

    Q_PROPERTY(size_t linesPerBeat READ linesPerBeat NOTIFY linesPerBeatChanged)

    Q_PROPERTY(bool isModified READ isModified NOTIFY isModifiedChanged)

    Q_PROPERTY(bool canBeSaved READ canBeSaved NOTIFY canBeSavedChanged)

    Q_PROPERTY(QString currentFileName READ currentFileName NOTIFY currentFileNameChanged)

    Q_PROPERTY(size_t currentPattern READ currentPattern NOTIFY currentPatternChanged)

    Q_PROPERTY(QString currentPatternName READ currentPatternName NOTIFY currentPatternChanged)

    Q_PROPERTY(size_t currentLineCount READ currentLineCount NOTIFY currentLineCountChanged)

    Q_PROPERTY(QString currentTime READ currentTime NOTIFY currentTimeChanged)

    Q_PROPERTY(QString duration READ duration NOTIFY durationChanged)

    Q_PROPERTY(bool hasColumnToPaste READ hasColumnToPaste NOTIFY copyManagerStateChanged)

    Q_PROPERTY(bool hasTrackToPaste READ hasTrackToPaste NOTIFY copyManagerStateChanged)

    Q_PROPERTY(bool hasPatternToPaste READ hasPatternToPaste NOTIFY copyManagerStateChanged)

    Q_PROPERTY(size_t songPosition READ songPosition NOTIFY songPositionChanged)

    Q_PROPERTY(size_t patternAtCurrentSongPosition READ patternAtCurrentSongPosition NOTIFY patternAtCurrentSongPositionChanged)

    Q_PROPERTY(double scrollBarSize READ scrollBarSize NOTIFY scrollBarSizeChanged)

    Q_PROPERTY(double scrollBarStepSize READ scrollBarStepSize NOTIFY scrollBarStepSizeChanged)

    Q_PROPERTY(size_t songLength READ songLength WRITE setSongLength NOTIFY songLengthChanged)

public:
    EditorService();

    ~EditorService() override;

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

    Q_INVOKABLE size_t columnCount(size_t trackIndex) const;

    Q_INVOKABLE size_t lineCount(size_t patternId) const;

    Q_INVOKABLE QString currentFileName() const;

    Q_INVOKABLE size_t currentLineCount() const;

    Q_INVOKABLE void setCurrentLineCount(size_t lineCount);

    Q_INVOKABLE QString currentPatternName() const;

    Q_INVOKABLE void setCurrentPatternName(QString patternName);

    Q_INVOKABLE QString currentTime() const;

    Q_INVOKABLE QString duration() const;

    Q_INVOKABLE size_t minLineCount() const;

    Q_INVOKABLE size_t maxLineCount() const;

    Q_INVOKABLE size_t minPatternIndex() const;

    Q_INVOKABLE size_t maxPatternIndex() const;

    Q_INVOKABLE size_t minSongPosition() const;

    Q_INVOKABLE size_t maxSongPosition() const;

    Q_INVOKABLE int lineNumberAtViewLine(size_t line) const;

    Q_INVOKABLE size_t linesVisible() const;

    QString displayNoteAtPosition(const Position & position) const;

    Q_INVOKABLE QString displayNoteAtPosition(size_t patternId, size_t trackIndex, size_t columnId, size_t line) const;

    QString displayVelocityAtPosition(const Position & position) const;

    Q_INVOKABLE QString displayVelocityAtPosition(size_t pattern, size_t track, size_t column, size_t line) const;

    Q_INVOKABLE QStringList displayNoteAndVelocityAtPosition(size_t patternId, size_t trackIndex, size_t columnId, size_t line) const;

    Q_INVOKABLE double effectiveVolumeAtPosition(size_t pattern, size_t track, size_t column, size_t line) const;

    Q_INVOKABLE QString noDataString() const;

    Q_INVOKABLE size_t patternCount() const;

    Q_INVOKABLE size_t trackCount() const;

    Q_INVOKABLE QString patternName(size_t patternIndex) const;

    Q_INVOKABLE void setPatternName(size_t patternIndex, QString name);

    Q_INVOKABLE QString trackName(size_t trackIndex) const;

    Q_INVOKABLE void setTrackName(size_t trackIndex, QString name);

    Q_INVOKABLE size_t currentPattern() const;

    Q_INVOKABLE void setCurrentPattern(size_t currentPattern);

    Q_INVOKABLE bool hasData(size_t pattern, size_t track, size_t column) const;

    Q_INVOKABLE bool isAtNoteColumn() const;

    Q_INVOKABLE bool isAtVelocityColumn() const;

    Q_INVOKABLE bool isColumnVisible(size_t track, size_t column) const;

    Q_INVOKABLE bool isModified() const;

    Q_INVOKABLE Position position() const;

    Q_INVOKABLE size_t positionBarLine() const;

    Q_INVOKABLE void requestCursorLeft();

    Q_INVOKABLE void requestCursorRight();

    Q_INVOKABLE void requestTrackRight();

    Q_INVOKABLE void requestColumnRight();

    Q_INVOKABLE bool requestDigitSetAtCurrentPosition(uint8_t digit);

    Q_INVOKABLE void requestNewColumn(size_t track);

    Q_INVOKABLE void requestColumnDeletion(size_t track);

    Q_INVOKABLE void requestNoteInsertionAtCurrentPosition();

    Q_INVOKABLE void requestNoteDeletionAtCurrentPosition();

    Q_INVOKABLE bool requestNoteOnAtCurrentPosition(uint8_t note, uint8_t octave, uint8_t velocity);

    Q_INVOKABLE bool requestNoteOffAtCurrentPosition();

    Q_INVOKABLE void requestColumnCut();

    Q_INVOKABLE void requestColumnCopy();

    Q_INVOKABLE void requestColumnPaste();

    Q_INVOKABLE void requestColumnTranspose(int semitones);

    Q_INVOKABLE bool hasColumnToPaste() const;

    Q_INVOKABLE void requestTrackCut();

    Q_INVOKABLE void requestTrackCopy();

    Q_INVOKABLE void requestTrackPaste();

    Q_INVOKABLE bool hasTrackToPaste() const;

    Q_INVOKABLE void requestTrackTranspose(int semitones);

    Q_INVOKABLE void requestPatternCut();

    Q_INVOKABLE void requestPatternCopy();

    Q_INVOKABLE void requestPatternPaste();

    Q_INVOKABLE void requestPatternTranspose(int semitones);

    Q_INVOKABLE bool hasPatternToPaste() const;

    bool requestPosition(const Position & position);

    Q_INVOKABLE bool requestPosition(size_t pattern, size_t track, size_t column, size_t line, size_t lineColumn);

    Q_INVOKABLE void requestScroll(int steps);

    Q_INVOKABLE void requestTrackFocus(size_t track, size_t column);

    Q_INVOKABLE size_t beatsPerMinute() const;

    Q_INVOKABLE void setBeatsPerMinute(size_t beatsPerMinute);

    Q_INVOKABLE size_t linesPerBeat() const;

    Q_INVOKABLE void setLinesPerBeat(size_t linesPerBeat);

    Q_INVOKABLE size_t visibleUnitCount() const;

    Q_INVOKABLE size_t totalUnitCount() const;

    Q_INVOKABLE size_t horizontalScrollPosition() const;

    Q_INVOKABLE size_t trackWidthInUnits(size_t trackIndex) const;

    Q_INVOKABLE int trackPositionInUnits(size_t trackIndex) const;

    Q_INVOKABLE void requestHorizontalScrollPositionChange(double position);

    Q_INVOKABLE double scrollBarStepSize() const;

    Q_INVOKABLE double scrollBarSize() const;

    Q_INVOKABLE size_t songPosition() const;

    Q_INVOKABLE void setSongPosition(size_t songPosition);

    Q_INVOKABLE size_t patternAtCurrentSongPosition() const;

    Q_INVOKABLE void insertPatternToPlayOrder();

    Q_INVOKABLE void removePatternFromPlayOrder();

    Q_INVOKABLE size_t patternAtSongPosition(size_t songPosition) const;

    Q_INVOKABLE void setPatternAtSongPosition(size_t songPosition, size_t pattern);

    Q_INVOKABLE size_t songLength() const;

    Q_INVOKABLE void setSongLength(size_t songLength);

    Q_INVOKABLE size_t maxSongLength() const;

    using InstrumentS = std::shared_ptr<Instrument>;
    InstrumentS instrument(size_t trackIndex) const;

    void setInstrument(size_t trackIndex, InstrumentS instrument);

    void setIsModified(bool isModified);

    using MidiNoteNameAndCode = std::pair<std::string, uint8_t>;
    using MidiNoteNameAndCodeOpt = std::optional<MidiNoteNameAndCode>;
    static MidiNoteNameAndCodeOpt editorNoteToMidiNote(size_t note, size_t octave);

signals:
    void beatsPerMinuteChanged();

    void canBeSavedChanged();

    void columnAdded(size_t track);

    void columnDeleted(size_t track);

    void copyManagerStateChanged();

    void currentFileNameChanged();

    void currentLineCountChanged(); // For the pattern length widget

    void currentLineCountModified(size_t oldLineCount, size_t newLineCount);

    void currentPatternChanged(); // For the pattern index widget

    void currentTimeChanged();

    void durationChanged();

    void horizontalScrollChanged();

    void instrumentRequested(const InstrumentRequest & instrumentRequest);

    void isModifiedChanged();

    void linesPerBeatChanged();

    void noteDataAtPositionChanged(const Position & position);

    void patternAtCurrentSongPositionChanged(); // For the play order widget

    void songPositionChanged(size_t position); // For the play order widget

    void patternCreated(size_t patternIndex);

    void positionChanged(const Position & newPosition, const Position & oldPosition);

    void scrollBarSizeChanged();

    void scrollBarStepSizeChanged();

    void songChanged();

    void songLengthChanged();

    void statusTextRequested(QString text);

    void trackConfigurationChanged();

    void trackNameChanged();

public slots:
    void requestPositionByTick(size_t tick);

private:
    void clampCursorLine(size_t oldLineCount, size_t newLineCount);

    void createPatternIfDoesNotExist(size_t patternIndex);

    size_t currentTrack() const;

    size_t currentColumn() const;

    void deleteNoteDataAtPosition(const Position & position);

    void insertNoteAtPosition(const Position & position);

    SongS deserializeProject(QXmlStreamReader & reader);

    void logPosition() const;

    void notifyPositionChange(const Position & oldPosition);

    QString padVelocityToThreeDigits(QString velocity) const;

    void requestInstruments();

    void removeDuplicateNoteOffs();

    void resetCursorPosition();

    bool setVelocityAtCurrentPosition(uint8_t digit);

    void setCurrentTime(std::chrono::milliseconds currentTime);

    void setDuration(std::chrono::milliseconds duration);

    void updateDuration();

    void updateScrollBar();

    SongS m_song;

    Position m_cursorPosition;

    QString m_currentTime;

    QString m_duration;

    size_t m_horizontalScrollPosition = 0;

    size_t m_songPosition = 0;

    std::unique_ptr<CopyManager> m_copyManager;

    bool m_isModified = false;
};

} // namespace noteahead

#endif // EDITOR_SERVICE_HPP
