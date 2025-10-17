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

#include "../position.hpp"
#include "copy_manager.hpp"

#include <QObject>

#include <optional>
#include <utility>

class QXmlStreamReader;
class QXmlStreamWriter;

namespace noteahead {

class Song;
class Instrument;
class InstrumentRequest;
class InstrumentSettings;
class Line;
class SelectionService;

class EditorService : public QObject
{
    Q_OBJECT

    Q_PROPERTY(quint64 beatsPerMinute READ beatsPerMinute NOTIFY beatsPerMinuteChanged)
    Q_PROPERTY(quint64 linesPerBeat READ linesPerBeat NOTIFY linesPerBeatChanged)

    Q_PROPERTY(bool isModified READ isModified NOTIFY isModifiedChanged)
    Q_PROPERTY(bool canBeSaved READ canBeSaved NOTIFY canBeSavedChanged)
    Q_PROPERTY(QString currentFileName READ currentFileName NOTIFY currentFileNameChanged)

    Q_PROPERTY(quint64 currentPattern READ currentPattern NOTIFY currentPatternChanged)
    Q_PROPERTY(QString currentPatternName READ currentPatternName NOTIFY currentPatternChanged)
    Q_PROPERTY(quint64 currentLineCount READ currentLineCount NOTIFY currentLineCountChanged)
    Q_PROPERTY(QString currentTime READ currentTime NOTIFY currentTimeChanged)
    Q_PROPERTY(QString duration READ duration NOTIFY durationChanged)

    Q_PROPERTY(bool hasColumnToPaste READ hasColumnToPaste NOTIFY copyManagerStateChanged)
    Q_PROPERTY(bool hasTrackToPaste READ hasTrackToPaste NOTIFY copyManagerStateChanged)
    Q_PROPERTY(bool hasPatternToPaste READ hasPatternToPaste NOTIFY copyManagerStateChanged)

    Q_PROPERTY(Position position READ position NOTIFY positionChanged)
    Q_PROPERTY(quint64 songPosition READ songPosition NOTIFY songPositionChanged)
    Q_PROPERTY(quint64 patternAtCurrentSongPosition READ patternAtCurrentSongPosition NOTIFY patternAtCurrentSongPositionChanged)
    Q_PROPERTY(quint64 songLength READ songLength WRITE setSongLength NOTIFY songLengthChanged)

    Q_PROPERTY(double scrollBarHandleSize READ scrollBarHandleSize NOTIFY scrollBarHandleSizeChanged)
    Q_PROPERTY(double scrollBarStepSize READ scrollBarStepSize NOTIFY scrollBarStepSizeChanged)

public:
    EditorService();
    using SelectionServiceS = std::shared_ptr<SelectionService>;
    EditorService(SelectionServiceS selectionService);
    ~EditorService() override;

    void initialize();

    using SongS = std::shared_ptr<Song>;
    SongS song() const;
    void setSong(SongS song);

    void load(QString fileName);
    void save();
    void saveAs(QString fileName);
    void saveAsTemplate(QString fileName);

    void fromXml(QString xml);
    QString toXml();
    QString toXmlAsTemplate();

    Q_INVOKABLE bool canBeSaved() const;

    Q_INVOKABLE quint64 columnCount(quint64 trackIndex) const;
    Q_INVOKABLE quint64 lineCount(quint64 patternId) const;

    Q_INVOKABLE QString currentFileName() const;

    Q_INVOKABLE quint64 currentLineCount() const;
    Q_INVOKABLE void setCurrentLineCount(quint64 lineCount);

    Q_INVOKABLE QString currentTime() const;
    Q_INVOKABLE QString duration() const;

    Q_INVOKABLE quint64 minLineCount() const;
    Q_INVOKABLE quint64 maxLineCount() const;

    Q_INVOKABLE quint64 minPatternIndex() const;
    Q_INVOKABLE quint64 maxPatternIndex() const;

    Q_INVOKABLE quint64 minSongPosition() const;
    Q_INVOKABLE quint64 maxSongPosition() const;

    Q_INVOKABLE int lineNumberAtViewLine(quint64 line) const;

    QString displayNoteAtPosition(const Position & position) const;
    Q_INVOKABLE QString displayNoteAtPosition(quint64 patternId, quint64 trackIndex, quint64 columnId, quint64 line) const;
    using MidiNoteList = std::vector<uint8_t>;
    Q_INVOKABLE MidiNoteList midiNotesAtPosition(quint64 pattern, quint64 track, quint64 line) const;
    QString displayVelocityAtPosition(const Position & position) const;
    Q_INVOKABLE QString displayVelocityAtPosition(quint64 pattern, quint64 track, quint64 column, quint64 line) const;
    Q_INVOKABLE QStringList displayNoteAndVelocityAtPosition(quint64 patternId, quint64 trackIndex, quint64 columnId, quint64 line) const;
    Q_INVOKABLE double velocityAtPosition(quint64 pattern, quint64 track, quint64 column, quint64 line) const;

    Q_INVOKABLE QString noDataString() const;

    Q_INVOKABLE quint64 trackCount() const;
    using TrackIndexList = std::vector<quint64>;
    Q_INVOKABLE TrackIndexList trackIndices() const;
    Q_INVOKABLE quint64 trackPositionByIndex(quint64 trackIndex) const;
    quint64 trackIndexByPosition(quint64 track) const;

    Q_INVOKABLE QString trackName(quint64 trackIndex) const;
    Q_INVOKABLE void setTrackName(quint64 trackIndex, QString name);
    Q_INVOKABLE QString columnName(quint64 trackIndex, quint64 columnIndex) const;
    Q_INVOKABLE void setColumnName(quint64 trackIndex, quint64 columnIndex, QString name);

    Q_INVOKABLE quint64 patternCount() const;
    using PatternIndexList = std::vector<quint64>;
    Q_INVOKABLE PatternIndexList patternIndices() const;
    Q_INVOKABLE quint64 currentPattern() const;
    Q_INVOKABLE void setCurrentPattern(quint64 currentPattern);
    Q_INVOKABLE QString patternName(quint64 patternIndex) const;
    Q_INVOKABLE void setPatternName(quint64 patternIndex, QString name);
    Q_INVOKABLE QString currentPatternName() const;
    Q_INVOKABLE void setCurrentPatternName(QString patternName);

    Q_INVOKABLE bool hasData(quint64 patternIndex, quint64 trackIndex, quint64 columnIndex) const;

    Q_INVOKABLE bool isAtNoteColumn() const;
    Q_INVOKABLE bool isAtVelocityColumn() const;
    Q_INVOKABLE bool isColumnVisible(quint64 track, quint64 column) const;
    Q_INVOKABLE bool isModified() const;

    Q_INVOKABLE Position position() const;
    Q_INVOKABLE quint64 positionBarLine() const;

    Q_INVOKABLE void requestEventRemoval();

    Q_INVOKABLE bool requestDigitSetAtCurrentPosition(quint8 digit);

    Q_INVOKABLE void requestNewColumn(quint64 trackIndex);
    Q_INVOKABLE void requestColumnDeletion(quint64 trackIndex);
    Q_INVOKABLE void requestNewTrackToRight();
    Q_INVOKABLE void requestTrackDeletion();

    Q_INVOKABLE void requestNoteInsertionAtCurrentPosition();
    Q_INVOKABLE void requestNoteDeletionAtCurrentPosition(bool shiftNotes);
    Q_INVOKABLE bool requestNoteOnAtCurrentPosition(quint8 key, quint8 octave, quint8 velocity);
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

    Q_INVOKABLE void requestSelectionCut();
    Q_INVOKABLE void requestSelectionCopy();
    Q_INVOKABLE void requestSelectionPaste();
    Q_INVOKABLE bool hasSelectionToPaste() const;
    Q_INVOKABLE void requestSelectionTranspose(int semitones);

    //! Performs linear interpolation on velocity on current column over given lines.
    Q_INVOKABLE void requestLinearVelocityInterpolationOnColumn(quint64 startLine, quint64 endLine, quint8 startValue, quint8 endValue);
    //! Performs linear interpolation on velocity on current column over given lines.
    Q_INVOKABLE void requestLinearVelocityInterpolationOnTrack(quint64 startLine, quint64 endLine, quint8 startValue, quint8 endValue);

    Q_INVOKABLE void setDelayOnCurrentLine(quint8 ticks);
    Q_INVOKABLE quint8 delayAtCurrentPosition() const;

    bool requestPosition(const Position & position);
    Q_INVOKABLE bool requestPosition(quint64 pattern, quint64 track, quint64 column, qint64 line, quint64 lineColumn);
    Q_INVOKABLE void requestScroll(int steps);

    Q_INVOKABLE quint64 ticksPerLine() const;
    Q_INVOKABLE quint64 beatsPerMinute() const;
    Q_INVOKABLE void setBeatsPerMinute(quint64 beatsPerMinute);
    Q_INVOKABLE quint64 linesPerBeat() const;
    Q_INVOKABLE void setLinesPerBeat(quint64 linesPerBeat);

    Q_INVOKABLE quint64 horizontalScrollPosition() const;
    Q_INVOKABLE quint64 trackWidthInUnits(quint64 trackIndex) const;
    Q_INVOKABLE quint64 visibleUnitCount() const;
    Q_INVOKABLE quint64 totalUnitCount() const;
    quint64 columnPositionInUnits(quint64 trackIndex, quint64 columnIndex) const;
    quint64 trackPositionInUnits(quint64 trackIndex) const;
    Q_INVOKABLE int onScreenColumnPositionInUnits(quint64 trackIndex, quint64 columnIndex) const;
    Q_INVOKABLE int onScreenTrackPositionInUnits(quint64 trackIndex) const;
    Q_INVOKABLE double scrollBarStepSize() const;
    Q_INVOKABLE double scrollBarHandleSize() const;
    Q_INVOKABLE double scrollBarPosition() const;
    Q_INVOKABLE void requestHorizontalScrollBarPositionChange(double scrollBarPosition);
    Q_INVOKABLE void requestCursorLeft();
    Q_INVOKABLE void requestCursorRight();
    Q_INVOKABLE void requestTrackRight();
    Q_INVOKABLE void requestColumnLeft();
    Q_INVOKABLE void requestColumnRight(bool isSelecting = false);

    Q_INVOKABLE quint64 songPosition() const;
    Q_INVOKABLE void setSongPosition(quint64 songPosition);
    Q_INVOKABLE void resetSongPosition();
    Q_INVOKABLE quint64 patternAtCurrentSongPosition() const;
    Q_INVOKABLE void insertPatternToPlayOrder();
    Q_INVOKABLE void removePatternFromPlayOrder();
    Q_INVOKABLE quint64 patternAtSongPosition(quint64 songPosition) const;
    Q_INVOKABLE void setPatternAtSongPosition(quint64 songPosition, quint64 pattern);
    Q_INVOKABLE quint64 songLength() const;
    Q_INVOKABLE void setSongLength(quint64 songLength);
    Q_INVOKABLE quint64 maxSongLength() const;

    // API V2
    using ColumnAddress = std::tuple<quint64, quint64, quint64>;
    using LineS = std::shared_ptr<Line>;
    using LineList = std::vector<LineS>;
    LineList columnData(ColumnAddress columnAddress) const;

    using InstrumentS = std::shared_ptr<Instrument>;
    InstrumentS instrument(quint64 trackIndex) const;
    void setInstrument(quint64 trackIndex, InstrumentS instrument);
    using InstrumentList = std::vector<std::pair<quint64, InstrumentS>>;
    InstrumentList instruments() const;

    using InstrumentSettingsS = std::shared_ptr<InstrumentSettings>;
    InstrumentSettingsS instrumentSettingsAtCurrentPosition() const;
    void setInstrumentSettingsAtCurrentPosition(InstrumentSettingsS instrument);
    void removeInstrumentSettingsAtCurrentPosition();
    Q_INVOKABLE bool hasInstrumentSettings(quint64 pattern, quint64 track, quint64 column, quint64 line) const;

    void setIsModified(bool isModified);

    using LineListCR = const LineList &;

signals:
    void beatsPerMinuteChanged();

    void canBeSavedChanged();

    void columnAdded(quint64 track);
    void columnDeleted(quint64 track);
    void columnNameChanged();

    void copyManagerStateChanged();

    void currentFileNameChanged();
    void currentLineCountChanged(); // For the pattern length widget
    void currentLineCountModified(quint64 oldLineCount, quint64 newLineCount);
    void currentPatternChanged(); // For the pattern index widget

    void currentTimeChanged();
    void durationChanged();

    void horizontalScrollChanged();

    void aboutToInitialize();
    void initialized();

    void instrumentRequested(const InstrumentRequest & instrumentRequest);

    void isModifiedChanged();

    void lineDataChanged(const Position & position);
    void linesPerBeatChanged();

    void automationSerializationRequested(QXmlStreamWriter & xmlStreamWriter);
    void automationDeserializationRequested(QXmlStreamReader & xmlStreamReader);
    void mixerSerializationRequested(QXmlStreamWriter & xmlStreamWriter);
    void mixerDeserializationRequested(QXmlStreamReader & xmlStreamReader);

    void noteDataAtPositionChanged(const Position & position);
    void patternAtCurrentSongPositionChanged(); // For the play order widget
    void songPositionChanged(quint64 position); // For the play order widget
    void patternCreated(quint64 patternIndex);
    void positionChanged(const Position & newPosition, const Position & oldPosition);

    void scrollBarHandleSizeChanged();
    void scrollBarStepSizeChanged();
    void scrollBarPositionChanged();

    void aboutToChangeSong();
    void songChanged();
    void songLengthChanged();

    void errorTextRequested(QString text);
    void statusTextRequested(QString text);

    void trackConfigurationChanged();
    void trackDeleted(quint64 trackIndex);
    void trackNameChanged();

public slots:
    void requestPositionByTick(quint64 tick);

private:
    void clampCursorLine(quint64 oldLineCount, quint64 newLineCount);

    void createPatternIfDoesNotExist(quint64 patternIndex);

    quint64 currentTrack() const;
    quint64 currentColumn() const;

    void deleteNoteDataAtPosition(const Position & position, bool shiftNotes);
    void insertNoteAtPosition(const Position & position);

    SongS deserializeProject(QXmlStreamReader & reader);
    void doVersionCheck(QString fileFormatVersion);

    void logPosition() const;

    void notifyPositionChange(const Position & oldPosition);

    QString padVelocityToThreeDigits(QString velocity) const;

    void requestInstruments();

    void removeDuplicateNoteOffs();

    void resetCursorPosition();
    void moveCursorToNextTrack();
    void moveCursorToPrevTrack();
    void setHorizontalScrollPosition(double position);
    void ensureFocusedColumnIsVisible();
    void ensureFocusedTrackIsVisible();

    bool setVelocityAtCurrentPosition(uint8_t digit);

    void setCurrentTime(std::chrono::milliseconds currentTime);
    void setDuration(std::chrono::milliseconds duration);
    void updateDuration();

    void updateScrollBar();

    SongS m_song;

    SelectionServiceS m_selectionService;

    struct State
    {
        Position cursorPosition;

        QString createdDate;
        QString currentTime;
        QString duration;

        quint64 horizontalScrollPosition = 0;
        quint64 songPosition = 0;

        CopyManager copyManager;

        bool isModified = false;
    };

    State m_state;
};

} // namespace noteahead

#endif // EDITOR_SERVICE_HPP
