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

#include "song_overview_controller.hpp"

#include "../../application/note_converter.hpp"
#include "../../application/position.hpp"
#include "../../application/service/automation_service.hpp"
#include "../../application/service/editor_service.hpp"
#include "../../application/service/mixer_service.hpp"
#include "../../application/service/player_service.hpp"
#include "../../domain/tracker/song.hpp"

#include <QTimer>

#include <algorithm>
#include <cmath>

namespace noteahead {

SongOverviewController::SongOverviewController(EditorServiceS editorService, PlayerServiceS playerService, MixerServiceS mixerService, AutomationServiceS automationService, QObject * parent)
  : QObject { parent }
  , m_editorService { std::move(editorService) }
  , m_playerService { std::move(playerService) }
  , m_mixerService { std::move(mixerService) }
  , m_automationService { std::move(automationService) }
{
    if (m_editorService) {
        // Everything that changes what is drawn. Each only asks for a rebuild, which an inactive
        // overview ignores.
        const auto editor = m_editorService.get();
        const auto refresh = [this] { scheduleRefresh(); };
        connect(editor, &EditorService::songChanged, this, refresh);
        connect(editor, &EditorService::noteDataAtPositionChanged, this, refresh);
        connect(editor, &EditorService::lineDataChanged, this, refresh);
        connect(editor, &EditorService::songLengthChanged, this, refresh);
        connect(editor, &EditorService::skippedChanged, this, refresh);
        connect(editor, &EditorService::patternAtCurrentSongPositionChanged, this, refresh);
        connect(editor, &EditorService::currentLineCountChanged, this, refresh);
        connect(editor, &EditorService::patternCreated, this, refresh);
        connect(editor, &EditorService::patternsDeleted, this, refresh);
        connect(editor, &EditorService::linesPerBeatChanged, this, refresh);
        connect(editor, &EditorService::trackAdded, this, refresh);
        connect(editor, &EditorService::trackDeleted, this, refresh);
        connect(editor, &EditorService::trackMoved, this, refresh);
        connect(editor, &EditorService::trackNameChanged, this, refresh);
        connect(editor, &EditorService::columnAdded, this, refresh);
        connect(editor, &EditorService::columnDeleted, this, refresh);
        connect(editor, &EditorService::columnMoved, this, refresh);

        connect(editor, &EditorService::songPositionChanged, this, &SongOverviewController::updatePlayheadFromEditor);
        connect(editor, &EditorService::positionChanged, this, &SongOverviewController::updatePlayheadFromEditor);
    }
    if (m_automationService) {
        // An automation added, edited, enabled or deleted
        connect(m_automationService.get(), &AutomationService::lineDataChanged, this, [this] { scheduleRefresh(); });
    }
    if (m_playerService) {
        connect(m_playerService.get(), &PlayerService::tickUpdated, this, &SongOverviewController::updatePlayheadFromTick);
        connect(m_playerService.get(), &PlayerService::isPlayingChanged, this, [this] {
            m_lastTickedLine.reset();
            updatePlayheadFromEditor();
        });
    }
    if (m_mixerService) {
        const auto mixer = m_mixerService.get();
        connect(mixer, &MixerService::trackMuted, this, &SongOverviewController::mixChanged);
        connect(mixer, &MixerService::trackSoloed, this, &SongOverviewController::mixChanged);
        connect(mixer, &MixerService::columnMuted, this, &SongOverviewController::mixChanged);
        connect(mixer, &MixerService::columnSoloed, this, &SongOverviewController::mixChanged);
        connect(mixer, &MixerService::cleared, this, &SongOverviewController::mixChanged);
        connect(mixer, &MixerService::configurationChanged, this, &SongOverviewController::mixChanged);
    }
}

bool SongOverviewController::active() const
{
    return m_active;
}

void SongOverviewController::setActive(bool active)
{
    if (m_active == active) {
        return;
    }
    m_active = active;
    if (m_active) {
        refresh();
        updatePlayheadFromEditor();
    } else {
        // Nothing kept for a closed view
        m_overview = {};
        emit overviewChanged();
    }
    emit activeChanged();
}

int SongOverviewController::rowCount() const
{
    return static_cast<int>(m_overview.rows.size());
}

int SongOverviewController::trackCount() const
{
    return static_cast<int>(m_overview.tracks.size());
}

int SongOverviewController::playheadRow() const
{
    return m_playheadRow;
}

double SongOverviewController::playheadLine() const
{
    return m_playheadLine;
}

double SongOverviewController::playheadSongLine() const
{
    return rowStartLine(m_playheadRow) + m_playheadLine;
}

double SongOverviewController::rowStartLine(int row) const
{
    return row >= 0 && static_cast<size_t>(row) < m_overview.rows.size() ? static_cast<double>(m_overview.rows.at(static_cast<size_t>(row)).startLine) : 0.0;
}

double SongOverviewController::rowLineCount(int row) const
{
    return row >= 0 && static_cast<size_t>(row) < m_overview.rows.size() ? static_cast<double>(m_overview.rows.at(static_cast<size_t>(row)).lineCount) : 0.0;
}

const SongOverview & SongOverviewController::overview() const
{
    return m_overview;
}

bool SongOverviewController::isColumnAudible(size_t track, size_t column) const
{
    return !m_mixerService || m_mixerService->shouldColumnPlay(track, column);
}

QVariantList SongOverviewController::rowStartLinesOfPattern(int pattern) const
{
    QVariantList startLines;
    for (auto && row : m_overview.rows) {
        if (static_cast<int>(row.pattern) == pattern) {
            startLines.append(static_cast<double>(row.startLine));
        }
    }
    return startLines;
}

QVariantMap SongOverviewController::targetAt(int row, int lane, double line, double pitch, double pitchTolerance) const
{
    if (row < 0 || static_cast<size_t>(row) >= m_overview.rows.size() || lane >= static_cast<int>(m_overview.tracks.size())) {
        return { { "valid", false } };
    }
    const auto & overviewRow = m_overview.rows.at(static_cast<size_t>(row));
    if (!m_overview.patterns.contains(overviewRow.pattern) || line < 0.0 || line >= static_cast<double>(overviewRow.lineCount)) {
        return { { "valid", false } };
    }

    QVariantMap target {
        { "valid", true },
        { "position", static_cast<quint64>(overviewRow.position) },
        { "pattern", static_cast<quint64>(overviewRow.pattern) },
        { "line", static_cast<quint64>(std::floor(line)) },
        { "isNote", false }
    };
    const auto cursor = m_editorService ? m_editorService->position() : Position {};
    if (lane < 0) {
        // The play order strip: the cursor stays on its track
        target["track"] = cursor.track;
        target["column"] = cursor.column;
        return target;
    }

    const auto & track = m_overview.tracks.at(static_cast<size_t>(lane));
    if (const auto note = m_overview.noteAt(static_cast<size_t>(row), track.index, line, pitch, pitchTolerance); note.has_value()) {
        target["track"] = static_cast<quint64>(note->track);
        target["column"] = static_cast<quint64>(note->column);
        // The note's own line: a click on the middle of a long bar goes to where it starts
        target["line"] = static_cast<quint64>(std::floor(note->startLine));
        target["isNote"] = true;
        target["trackName"] = QString::fromStdString(track.name);
        target["noteName"] = QString::fromStdString(NoteConverter::midiToString(note->pitch));
        target["pitch"] = static_cast<int>(note->pitch);
        target["noteStartLine"] = note->startLine;
        target["noteLengthLines"] = note->lengthLines;
        target["noteLimitLine"] = static_cast<double>(note->limitLine);
        return target;
    }

    // Empty space: the lane's track, on the cursor's column if the cursor is on it already
    target["track"] = static_cast<quint64>(track.index);
    quint64 column = 0;
    if (cursor.track == track.index) {
        column = cursor.column;
    } else if (m_editorService && m_editorService->song()) {
        if (const auto columns = m_editorService->song()->columnIndices(track.index); !columns.empty()) {
            column = columns.front();
        }
    }
    target["column"] = column;
    return target;
}

QVariantList SongOverviewController::notesStartingIn(int row, double fromLine, double toLine) const
{
    QVariantList notes;
    if (row < 0 || static_cast<size_t>(row) >= m_overview.rows.size()) {
        return notes;
    }
    const auto & overviewRow = m_overview.rows.at(static_cast<size_t>(row));
    const auto pattern = m_overview.patterns.find(overviewRow.pattern);
    if (overviewRow.skipped || pattern == m_overview.patterns.end()) {
        return notes;
    }
    for (auto && note : pattern->second.notes) {
        if (note.startLine < fromLine || note.startLine >= toLine || !isColumnAudible(note.track, note.column)) {
            continue;
        }
        if (const auto lane = m_overview.trackPosition(note.track); lane.has_value()) {
            notes.append(QVariantMap {
              { "lane", static_cast<int>(*lane) },
              { "pitch", static_cast<int>(note.pitch) },
              { "velocity", static_cast<int>(note.velocity) },
              { "songLine", static_cast<double>(overviewRow.startLine) + note.startLine } });
        }
    }
    return notes;
}

void SongOverviewController::refresh()
{
    m_refreshScheduled = false;
    if (!m_active || !m_editorService || !m_editorService->song()) {
        return;
    }
    m_overview = m_automationService ? SongOverview::build(*m_editorService->song(), *m_automationService) : SongOverview::build(*m_editorService->song());
    emit overviewChanged();
    // The playhead's place on the timeline moves with the rows before it
    emit playheadChanged();
}

void SongOverviewController::scheduleRefresh()
{
    if (!m_active || m_refreshScheduled) {
        return;
    }
    m_refreshScheduled = true;
    QTimer::singleShot(0, this, &SongOverviewController::refresh);
}

void SongOverviewController::updatePlayheadFromEditor()
{
    // During playback the ticks say where the song is; the cursor may have been left behind
    if (!m_active || !m_editorService || (m_playerService && m_playerService->isPlaying())) {
        return;
    }
    setPlayhead(static_cast<int>(m_editorService->songPosition()), static_cast<double>(m_editorService->position().line));
}

void SongOverviewController::updatePlayheadFromTick(quint64 tick)
{
    if (!m_active || !m_editorService || !m_editorService->song()) {
        return;
    }
    const auto song = m_editorService->song();
    const auto ticksPerLine = std::max<size_t>(song->ticksPerLine(), 1);
    if (const auto songPosition = song->songPositionByTick((tick / ticksPerLine) * ticksPerLine); songPosition.has_value()) {
        const int row = static_cast<int>(songPosition->position);
        const double line = static_cast<double>(songPosition->line);
        // From the line after the last one reached, so a line the ticks skipped still lights up;
        // after a jump or a loop, only this line
        double fromLine = line;
        if (m_lastTickedLine && m_lastTickedLine->first == row && m_lastTickedLine->second < line) {
            fromLine = m_lastTickedLine->second + 1.0;
        }
        m_lastTickedLine = { row, line };
        setPlayhead(row, line);
        if (auto notes = notesStartingIn(row, fromLine, line + 1.0); !notes.isEmpty()) {
            emit notesStarted(notes);
        }
    }
}

void SongOverviewController::setPlayhead(int row, double line)
{
    if (m_playheadRow == row && m_playheadLine == line) {
        return;
    }
    m_playheadRow = row;
    m_playheadLine = line;
    emit playheadChanged();
}

} // namespace noteahead
