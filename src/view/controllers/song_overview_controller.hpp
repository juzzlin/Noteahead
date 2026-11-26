// This file is part of Noteahead.
// Copyright (C) 2026 Jussi Lind <jussi.lind@iki.fi>
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

#ifndef SONG_OVERVIEW_CONTROLLER_HPP
#define SONG_OVERVIEW_CONTROLLER_HPP

#include "../../application/service/song_overview_builder.hpp"

#include <QObject>
#include <QVariantMap>

#include <memory>
#include <optional>
#include <utility>

namespace noteahead {

class AutomationService;
class EditorService;
class MixerService;
class PlayerService;

//! Hands the Song Overview its song and its playhead.
//!
//! Builds only while the view is active: a closed overview costs nothing, and an open one is rebuilt
//! once per pass of the event loop however many notes an edit changes. The playhead follows the
//! player's ticks rather than the editor's cursor, which stays put during playback when the editor
//! is set not to follow it.
class SongOverviewController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool active READ active WRITE setActive NOTIFY activeChanged)
    Q_PROPERTY(int rowCount READ rowCount NOTIFY overviewChanged)
    Q_PROPERTY(int trackCount READ trackCount NOTIFY overviewChanged)
    Q_PROPERTY(int playheadRow READ playheadRow NOTIFY playheadChanged)
    Q_PROPERTY(double playheadLine READ playheadLine NOTIFY playheadChanged)
    //! The playhead on the song's timeline: the row's start plus the line in it.
    Q_PROPERTY(double playheadSongLine READ playheadSongLine NOTIFY playheadChanged)

public:
    using AutomationServiceS = std::shared_ptr<AutomationService>;
    using EditorServiceS = std::shared_ptr<EditorService>;
    using MixerServiceS = std::shared_ptr<MixerService>;
    using PlayerServiceS = std::shared_ptr<PlayerService>;

    SongOverviewController(EditorServiceS editorService, PlayerServiceS playerService, MixerServiceS mixerService, AutomationServiceS automationService, QObject * parent = nullptr);

    bool active() const;
    void setActive(bool active);

    int rowCount() const;
    int trackCount() const;

    int playheadRow() const;
    double playheadLine() const;
    double playheadSongLine() const;

    //! Where @p row starts on the song's timeline and how long it is, in lines.
    Q_INVOKABLE double rowStartLine(int row) const;
    Q_INVOKABLE double rowLineCount(int row) const;

    const SongOverview & overview() const;

    //! Whether the mixer lets the column be heard, so that what is muted can be drawn dimmed.
    bool isColumnAudible(size_t track, size_t column) const;

    //! Where each position that plays @p pattern starts on the song's timeline, in lines: where an
    //! edit of the pattern shows.
    Q_INVOKABLE QVariantList rowStartLinesOfPattern(int pattern) const;

    //! Where a click at @p line and @p pitch in @p row and lane @p lane takes the editor: the note
    //! there if there is one, otherwise that line on the lane's track. A lane of -1 (the play order
    //! strip) keeps the cursor's track. Keys: valid, position, pattern, track, column, line, isNote,
    //! and for a note trackName, noteName, pitch,
    //! noteStartLine, noteLengthLines and
    //! noteLimitLine.
    Q_INVOKABLE QVariantMap targetAt(int row, int lane, double line, double pitch, double pitchTolerance) const;

    //! The audible notes of @p row that start in [@p fromLine, @p toLine), as maps of lane, pitch,
    //! velocity and songLine: what the playhead has just reached.
    QVariantList notesStartingIn(int row, double fromLine, double toLine) const;

    //! Rebuilds from the song now.
    Q_INVOKABLE void refresh();

signals:
    void activeChanged();
    void overviewChanged();
    void playheadChanged();
    //! During playback, the notes the playhead has just reached (see notesStartingIn()), so the view
    //! can light them up.
    void notesStarted(const QVariantList & notes);
    //! A mute or solo changed: nothing to rebuild, only to repaint.
    void mixChanged();

private:
    //! Asks for a rebuild on the next pass of the event loop, so a paste of a thousand notes
    //! rebuilds once.
    void scheduleRefresh();
    void updatePlayheadFromEditor();
    void updatePlayheadFromTick(quint64 tick);
    void setPlayhead(int row, double line);

    EditorServiceS m_editorService;
    PlayerServiceS m_playerService;
    MixerServiceS m_mixerService;
    AutomationServiceS m_automationService;

    SongOverview m_overview;
    bool m_active { false };
    bool m_refreshScheduled { false };
    int m_playheadRow { 0 };
    double m_playheadLine { 0.0 };
    //! The last line a tick reached, so that the notes of every line are announced once, those of
    //! lines the ticks skipped included. Cleared when playback starts or stops.
    std::optional<std::pair<int, double>> m_lastTickedLine;
};

} // namespace noteahead

#endif // SONG_OVERVIEW_CONTROLLER_HPP
