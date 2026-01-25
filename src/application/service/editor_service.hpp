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
#include "../command/undo_stack.hpp"

class QXmlStreamReader;
class QXmlStreamWriter;

namespace noteahead {

class Song;
class Instrument;
class InstrumentRequest;
class InstrumentSettings;
class Line;
class SelectionService;
class SettingsService;
class ColumnSettings;

class EditorService : public QObject
{
    Q_OBJECT

    Q_PROPERTY(quint64 beatsPerMinute READ beatsPerMinute NOTIFY beatsPerMinuteChanged)
    Q_PROPERTY(quint64 linesPerBeat READ linesPerBeat NOTIFY linesPerBeatChanged)

    Q_PROPERTY(bool isModified READ isModified NOTIFY isModifiedChanged)
    Q_PROPERTY(bool canBeSaved READ canBeSaved NOTIFY canBeSavedChanged)
    Q_PROPERTY(bool canUndo READ canUndo NOTIFY canUndoChanged)
    Q_PROPERTY(bool canRedo READ canRedo NOTIFY canRedoChanged)
    Q_PROPERTY(QString currentFileName READ currentFileName NOTIFY currentFileNameChanged)

    Q_PROPERTY(quint64 currentPattern READ currentPattern NOTIFY currentPatternChanged)
    Q_PROPERTY(QString currentPatternName READ currentPatternName NOTIFY currentPatternChanged)
    Q_PROPERTY(quint64 currentLineCount READ currentLineCount NOTIFY currentLineCountChanged)
    Q_PROPERTY(QString currentPatternTime READ currentPatternTime NOTIFY currentPatternTimeChanged)
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
    using SettingsServiceS = std::shared_ptr<SettingsService>;
    EditorService(SelectionServiceS selectionService, SettingsServiceS settingsService);
    ~EditorService() override;

    void initialize();

    using SongS = std::shared_ptr<Song>;
    SongS song() const;
    void setSong(SongS song);

    SelectionServiceS selectionService() const { return m_selectionService; }
    SettingsServiceS settingsService() const { return m_settingsService; }

    void load(QString fileName);
    void save();
    void saveAs(QString fileName);
    void saveAsTemplate(QString fileName);

    void fromXml(QString xml);
    QString toXml();
    QString toXmlAsTemplate();

    Q_INVOKABLE bool canBeSaved() const;

    virtual Q_INVOKABLE quint64 columnCount(quint64 trackIndex) const;
    virtual Q_INVOKABLE quint64 lineCount(quint64 patternId) const;

    virtual Q_INVOKABLE QString currentFileName() const;

    virtual Q_INVOKABLE quint64 currentLineCount() const;
    virtual Q_INVOKABLE void setCurrentLineCount(quint64 lineCount);

    virtual Q_INVOKABLE QString currentPatternTime() const;
    virtual Q_INVOKABLE QString currentTime() const;
    virtual Q_INVOKABLE QString duration() const;

    virtual Q_INVOKABLE quint64 minLineCount() const;
    virtual Q_INVOKABLE quint64 maxLineCount() const;

    virtual Q_INVOKABLE quint64 minPatternIndex() const;
    virtual Q_INVOKABLE quint64 maxPatternIndex() const;

    virtual Q_INVOKABLE quint64 minSongPosition() const;
    virtual Q_INVOKABLE quint64 maxSongPosition() const;

    virtual Q_INVOKABLE int lineNumberAtViewLine(quint64 line) const;

    virtual QString displayNoteAtPosition(const Position & position) const;
    virtual Q_INVOKABLE QString displayNoteAtPosition(quint64 patternId, quint64 trackIndex, quint64 columnId, quint64 line) const;
    using MidiNoteList = std::vector<uint8_t>;
    virtual Q_INVOKABLE MidiNoteList midiNotesAtPosition(quint64 pattern, quint64 track, quint64 line) const;
    virtual QString displayVelocityAtPosition(const Position & position) const;
    virtual Q_INVOKABLE QString displayVelocityAtPosition(quint64 pattern, quint64 track, quint64 column, quint64 line) const;
    virtual Q_INVOKABLE QStringList displayNoteAndVelocityAtPosition(quint64 patternId, quint64 trackIndex, quint64 columnId, quint64 line) const;
    virtual Q_INVOKABLE double velocityAtPosition(quint64 pattern, quint64 track, quint64 column, quint64 line) const;

    virtual Q_INVOKABLE QString noDataString() const;

    virtual Q_INVOKABLE quint64 trackCount() const;
    using TrackIndexList = std::vector<quint64>;
    virtual Q_INVOKABLE TrackIndexList trackIndices() const;
    virtual Q_INVOKABLE quint64 trackPositionByIndex(quint64 trackIndex) const;
    virtual quint64 trackIndexByPosition(quint64 track) const;

    virtual Q_INVOKABLE QString trackName(quint64 trackIndex) const;
    virtual Q_INVOKABLE void setTrackName(quint64 trackIndex, QString name);
    virtual Q_INVOKABLE QString columnName(quint64 trackIndex, quint64 columnIndex) const;
    virtual Q_INVOKABLE void setColumnName(quint64 trackIndex, quint64 columnIndex, QString name);

    virtual Q_INVOKABLE quint64 patternCount() const;
    using PatternIndexList = std::vector<quint64>;
    virtual Q_INVOKABLE PatternIndexList patternIndices() const;
    virtual Q_INVOKABLE quint64 currentPattern() const;
    virtual Q_INVOKABLE void setCurrentPattern(quint64 currentPattern);
    virtual Q_INVOKABLE QString patternName(quint64 patternIndex) const;
    virtual Q_INVOKABLE void setPatternName(quint64 patternIndex, QString name);
    virtual Q_INVOKABLE QString currentPatternName() const;
    virtual Q_INVOKABLE void setCurrentPatternName(QString patternName);

    virtual Q_INVOKABLE bool hasData(quint64 patternIndex, quint64 trackIndex, quint64 columnIndex) const;

    virtual Q_INVOKABLE bool isAtNoteColumn() const;
    virtual Q_INVOKABLE bool isAtVelocityColumn() const;
    virtual Q_INVOKABLE bool isAtDelayColumn() const;
    virtual Q_INVOKABLE bool isColumnVisible(quint64 track, quint64 column) const;
    virtual Q_INVOKABLE bool isTrackVisible(quint64 track) const;
    virtual Q_INVOKABLE bool isModified() const;
    virtual Q_INVOKABLE void resetModified();

    virtual Q_INVOKABLE Position position() const;
    virtual Q_INVOKABLE quint64 positionBarLine() const;

    virtual Q_INVOKABLE void requestEventRemoval();

    virtual Q_INVOKABLE bool requestDigitSetAtCurrentPosition(quint8 digit);

    virtual Q_INVOKABLE void requestNewColumn(quint64 trackIndex);
    virtual Q_INVOKABLE void requestColumnDeletion(quint64 trackIndex);
    virtual Q_INVOKABLE void requestNewTrackToRight();
    virtual Q_INVOKABLE void requestNewTrackToLeft();
    virtual Q_INVOKABLE void requestTrackDeletion();

    virtual Q_INVOKABLE void requestNoteInsertionAtCurrentPosition();
    virtual Q_INVOKABLE void requestNoteDeletionAtCurrentPosition(bool shiftNotes);
    virtual Q_INVOKABLE bool requestNoteOnAtCurrentPosition(quint8 key, quint8 octave, quint8 velocity);
    virtual Q_INVOKABLE bool requestNoteOffAtCurrentPosition();

    virtual Q_INVOKABLE void requestColumnCut();
    virtual Q_INVOKABLE void requestColumnCopy();
    virtual Q_INVOKABLE void requestColumnPaste();
    virtual Q_INVOKABLE void requestColumnTranspose(int semitones);
    virtual Q_INVOKABLE bool hasColumnToPaste() const;

    virtual Q_INVOKABLE void requestTrackCut();
    virtual Q_INVOKABLE void requestTrackCopy();
    virtual Q_INVOKABLE void requestTrackPaste();
    virtual Q_INVOKABLE bool hasTrackToPaste() const;
    virtual Q_INVOKABLE void requestTrackTranspose(int semitones);

    virtual Q_INVOKABLE void requestPatternCut();
    virtual Q_INVOKABLE void requestPatternCopy();
    virtual Q_INVOKABLE void requestPatternPaste();
    virtual Q_INVOKABLE void requestPatternTranspose(int semitones);
    virtual Q_INVOKABLE bool hasPatternToPaste() const;

    virtual Q_INVOKABLE void requestSelectionCut();
    virtual Q_INVOKABLE void requestSelectionCopy();
    virtual Q_INVOKABLE void requestSelectionPaste();
    virtual Q_INVOKABLE bool hasSelectionToPaste() const;
    virtual Q_INVOKABLE void requestSelectionTranspose(int semitones);

    //! Performs linear interpolation on velocity on current column over given lines.
    virtual Q_INVOKABLE void requestLinearVelocityInterpolationOnColumn(quint64 startLine, quint64 endLine, quint8 startValue, quint8 endValue, bool usePercentages);
    //! Performs linear interpolation on velocity on current column over given lines.
    virtual Q_INVOKABLE void requestLinearVelocityInterpolationOnTrack(quint64 startLine, quint64 endLine, quint8 startValue, quint8 endValue, bool usePercentages);
    //! Performs linear interpolation on velocity on currently selected columns.
    virtual Q_INVOKABLE void requestLinearVelocityInterpolationOnSelection(quint64 startLine, quint64 endLine, quint8 startValue, quint8 endValue, bool usePercentages);

    virtual Q_INVOKABLE void setDelayOnCurrentLine(quint8 ticks);
    virtual Q_INVOKABLE quint8 delayAtCurrentPosition() const;

    virtual bool requestPosition(const Position & position);
    virtual Q_INVOKABLE bool requestPosition(quint64 pattern, quint64 track, quint64 column, qint64 line, quint64 lineColumn);
    virtual Q_INVOKABLE void requestScroll(int steps);

    virtual Q_INVOKABLE quint64 ticksPerLine() const;
    virtual Q_INVOKABLE quint64 beatsPerMinute() const;
    virtual Q_INVOKABLE void setBeatsPerMinute(quint64 beatsPerMinute);
    virtual Q_INVOKABLE quint64 linesPerBeat() const;
    virtual Q_INVOKABLE void setLinesPerBeat(quint64 linesPerBeat);

    virtual Q_INVOKABLE quint64 horizontalScrollPosition() const;
    virtual Q_INVOKABLE quint64 trackWidthInUnits(quint64 trackIndex) const;
    virtual Q_INVOKABLE quint64 visibleUnitCount() const;
    virtual Q_INVOKABLE quint64 totalUnitCount() const;
    quint64 columnPositionInUnits(quint64 trackIndex, quint64 columnIndex) const;
    quint64 trackPositionInUnits(quint64 trackIndex) const;
    virtual Q_INVOKABLE int onScreenColumnPositionInUnits(quint64 trackIndex, quint64 columnIndex) const;
    virtual Q_INVOKABLE int onScreenTrackPositionInUnits(quint64 trackIndex) const;
    virtual Q_INVOKABLE double scrollBarStepSize() const;
    virtual Q_INVOKABLE double scrollBarHandleSize() const;
    virtual Q_INVOKABLE double scrollBarPosition() const;
    virtual Q_INVOKABLE void requestHorizontalScrollBarPositionChange(double scrollBarPosition);
    virtual Q_INVOKABLE void requestCursorLeft();
    virtual Q_INVOKABLE void requestCursorRight();
    virtual Q_INVOKABLE void requestTrackRight();
    virtual Q_INVOKABLE void requestColumnLeft();
    virtual Q_INVOKABLE void requestColumnRight(bool isSelecting = false);

    virtual Q_INVOKABLE quint64 songPosition() const;
    virtual Q_INVOKABLE void setSongPosition(quint64 songPosition);
    virtual Q_INVOKABLE void resetSongPosition();
    virtual Q_INVOKABLE quint64 patternAtCurrentSongPosition() const;
    virtual Q_INVOKABLE void insertPatternToPlayOrder();
    virtual Q_INVOKABLE void removePatternFromPlayOrder();
    virtual Q_INVOKABLE quint64 patternAtSongPosition(quint64 songPosition) const;
    virtual Q_INVOKABLE void setPatternAtSongPosition(quint64 songPosition, quint64 pattern);
    virtual Q_INVOKABLE quint64 songLength() const;
    virtual Q_INVOKABLE void setSongLength(quint64 songLength);
    virtual Q_INVOKABLE quint64 maxSongLength() const;

    // API V2
    using ColumnAddress = std::tuple<quint64, quint64, quint64>;
    using LineS = std::shared_ptr<Line>;
    using LineList = std::vector<LineS>;
    virtual LineList columnData(ColumnAddress columnAddress) const;

    using InstrumentS = std::shared_ptr<Instrument>;
    virtual InstrumentS instrument(quint64 trackIndex) const;
    virtual void setInstrument(quint64 trackIndex, InstrumentS instrument);
    using InstrumentList = std::vector<std::pair<quint64, InstrumentS>>;
    virtual InstrumentList instruments() const;

    using InstrumentSettingsS = std::shared_ptr<InstrumentSettings>;
    virtual InstrumentSettingsS instrumentSettingsAtCurrentPosition() const;
    virtual void setInstrumentSettingsAtCurrentPosition(InstrumentSettingsS instrument);
    virtual void removeInstrumentSettingsAtCurrentPosition();
    virtual Q_INVOKABLE bool hasInstrumentSettings(quint64 pattern, quint64 track, quint64 column, quint64 line) const;

    using ColumnSettingsS = std::shared_ptr<ColumnSettings>;
    virtual ColumnSettingsS columnSettings(quint64 trackIndex, quint64 columnIndex) const;
    virtual void setColumnSettings(quint64 trackIndex, quint64 columnIndex, ColumnSettingsS settings);

    virtual void setIsModified(bool isModified);

    virtual Q_INVOKABLE bool canUndo() const;
    virtual Q_INVOKABLE bool canRedo() const;
    virtual Q_INVOKABLE void undo();
    virtual Q_INVOKABLE void redo();

    using LineListCR = const LineList &;

signals:
    void beatsPerMinuteChanged();

    void canBeSavedChanged();
    void canUndoChanged();
    void canRedoChanged();

    void columnAdded(quint64 track);
    void columnDeleted(quint64 track);
    void columnNameChanged();

    void copyManagerStateChanged();

    void currentFileNameChanged();
    void currentLineCountChanged(); // For the pattern length widget
    void currentLineCountModified(quint64 oldLineCount, quint64 newLineCount);
    void currentPatternChanged(); // For the pattern index widget

    void currentPatternTimeChanged();
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
    void sideChainSerializationRequested(QXmlStreamWriter & xmlStreamWriter);
    void sideChainDeserializationRequested(QXmlStreamReader & xmlStreamReader);

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

    void trackAdded(quint64 trackIndex);
    void trackDeleted(quint64 trackIndex);
    void trackNameChanged();

public slots:
    void requestPositionByTick(quint64 tick);

private:
    void clampCursorLine(quint64 oldLineCount, quint64 newLineCount);

    void setSongPositionInternal(quint64 songPosition, bool updateTime);

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
    bool setDelayAtCurrentPosition(uint8_t digit);

    void updateTimes(std::chrono::milliseconds songTime, std::chrono::milliseconds patternTime);
    void updateTimesFromCurrentPosition();
    void setDuration(std::chrono::milliseconds duration);
    void updateDuration();

    void updateScrollBar();

    SongS m_song;
    std::unique_ptr<UndoStack> m_undoStack;

    SelectionServiceS m_selectionService;
    SettingsServiceS m_settingsService;

    struct State
    {
        Position cursorPosition;

        QString createdDate;
        QString currentPatternTime;
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
