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

#include "song_overview_builder.hpp"

#include "../../domain/tracker/line.hpp"
#include "../../domain/tracker/note_data.hpp"
#include "../../domain/tracker/pattern.hpp"
#include "../../domain/tracker/song.hpp"
#include "../position.hpp"
#include "automation_service.hpp"

#include <algorithm>
#include <cmath>

namespace noteahead {

namespace {

//! The bars of one column: each note runs to the next note or note-off, or to the pattern's end.
void addColumnNotes(const Song & song, size_t patternIndex, size_t trackIndex, size_t columnIndex, std::vector<SongOverview::Note> & notes)
{
    const auto lines = song.lines({ .pattern = patternIndex, .track = trackIndex, .column = columnIndex });
    const double ticksPerLine = static_cast<double>(std::max<size_t>(song.ticksPerLine(), 1));
    const double lineCount = static_cast<double>(lines.size());

    std::optional<SongOverview::Note> open;
    //! The note a note-off has ended, whose limit is the event after that
    std::optional<size_t> endedByNoteOff;
    const auto close = [&](double endLine, size_t eventLine, bool isNoteOff) {
        if (open) {
            open->lengthLines = std::max(endLine - open->startLine, 0.0);
            open->limitLine = eventLine;
            if (open->lengthLines > 0.0) {
                notes.push_back(*open);
                if (isNoteOff) {
                    endedByNoteOff = notes.size() - 1;
                }
            }
            open.reset();
        }
    };

    for (size_t lineIndex = 0; lineIndex < lines.size(); lineIndex++) {
        const auto noteData = lines.at(lineIndex) ? lines.at(lineIndex)->noteData() : nullptr;
        if (!noteData || noteData->type() == NoteData::Type::None) {
            continue;
        }
        // The delay is in ticks, and moves the note into its line
        const double line = static_cast<double>(lineIndex) + std::min(static_cast<double>(noteData->delay()) / ticksPerLine, 1.0);
        if (endedByNoteOff) {
            notes.at(*endedByNoteOff).limitLine = lineIndex;
            endedByNoteOff.reset();
        }
        close(line, lineIndex, noteData->type() == NoteData::Type::NoteOff);
        if (noteData->type() == NoteData::Type::NoteOn && noteData->note().has_value()) {
            open = SongOverview::Note {
                .startLine = line,
                .lengthLines = 0.0,
                .pitch = *noteData->note(),
                .velocity = noteData->velocity(),
                .track = trackIndex,
                .column = columnIndex
            };
        }
    }
    if (endedByNoteOff) {
        notes.at(*endedByNoteOff).limitLine = lines.size();
    }
    close(lineCount, lines.size(), false);
}

//! The automations of one column, sampled on every line of the pattern
void addColumnCurves(const AutomationService & automationService, size_t patternIndex, size_t lineCount, size_t trackIndex, size_t columnIndex, std::vector<SongOverview::Curve> & curves)
{
    if (lineCount == 0) {
        return;
    }
    for (auto && curve : automationService.automationCurves(patternIndex, trackIndex, columnIndex, 0, lineCount - 1)) {
        std::string comment;
        std::optional<uint8_t> controller;
        std::string parameterKey;
        const auto automation = automationService.automationById(curve.kind, curve.id);
        for (auto && midiCc : automation.midiCc) {
            comment = midiCc.comment().toStdString();
            controller = midiCc.controller();
        }
        for (auto && pitchBend : automation.pitchBend) {
            comment = pitchBend.comment().toStdString();
        }
        for (auto && parameter : automation.parameter) {
            comment = parameter.comment().toStdString();
            parameterKey = parameter.target().parameter;
        }
        curves.push_back({ .track = trackIndex,
                           .column = columnIndex,
                           .colorIndex = curve.indexInPattern,
                           .kind = static_cast<int>(curve.kind),
                           .automationId = curve.id,
                           .comment = std::move(comment),
                           .controller = controller,
                           .parameter = std::move(parameterKey),
                           .values = std::move(curve.values) });
    }
}

//! The automation service is optional: without one there are no curves
SongOverview buildOverview(const Song & song, const AutomationService * automationService)
{
    using Pattern = SongOverview::Pattern;
    SongOverview overview;
    overview.linesPerBeat = std::max<size_t>(song.linesPerBeat(), 1);

    for (size_t position = 0; position < song.trackCount(); position++) {
        if (const auto trackIndex = song.trackIndexByPosition(position); trackIndex.has_value()) {
            overview.tracks.push_back({ .index = *trackIndex, .name = song.trackName(*trackIndex) });
        }
    }

    for (size_t position = 0; position < song.length(); position++) {
        const size_t patternIndex = song.patternAtSongPosition(position);
        const size_t lineCount = song.hasPattern(patternIndex) ? song.lineCount(patternIndex) : 0;
        overview.rows.push_back({ .position = position, .pattern = patternIndex, .skipped = song.isSkipped(position), .startLine = overview.totalLines, .lineCount = lineCount });
        overview.totalLines += lineCount;
        if (overview.patterns.contains(patternIndex) || !song.hasPattern(patternIndex)) {
            continue;
        }
        Pattern pattern;
        pattern.lineCount = lineCount;
        pattern.name = song.patternName(patternIndex);
        const auto songPattern = song.pattern(patternIndex);
        for (auto && track : overview.tracks) {
            for (const size_t columnIndex : songPattern->columnIndices(track.index)) {
                addColumnNotes(song, patternIndex, track.index, columnIndex, pattern.notes);
                if (automationService) {
                    addColumnCurves(*automationService, patternIndex, lineCount, track.index, columnIndex, pattern.curves);
                }
            }
        }
        overview.patterns.emplace(patternIndex, std::move(pattern));
    }

    // Each lane's own range, at least an octave
    for (auto && track : overview.tracks) {
        std::optional<int> lowest;
        std::optional<int> highest;
        for (auto && [index, pattern] : overview.patterns) {
            for (auto && note : pattern.notes) {
                if (note.track == track.index) {
                    lowest = std::min(lowest.value_or(note.pitch), static_cast<int>(note.pitch));
                    highest = std::max(highest.value_or(note.pitch), static_cast<int>(note.pitch));
                }
            }
        }
        if (lowest && highest) {
            constexpr int minSpan = 12;
            if (const int missing = minSpan - (*highest - *lowest + 1); missing > 0) {
                *lowest -= missing / 2;
                *highest += missing - missing / 2;
            }
            const int shift = std::max(0 - *lowest, 0) - std::max(*highest - 127, 0);
            track.lowestPitch = static_cast<uint8_t>(std::clamp(*lowest + shift, 0, 127));
            track.highestPitch = static_cast<uint8_t>(std::clamp(*highest + shift, 0, 127));
        }
    }
    return overview;
}

} // namespace

SongOverview SongOverview::build(const Song & song)
{
    return buildOverview(song, nullptr);
}

SongOverview SongOverview::build(const Song & song, const AutomationService & automationService)
{
    return buildOverview(song, &automationService);
}

std::optional<SongOverview::Note> SongOverview::noteAt(size_t row, size_t track, double line, double pitch, double pitchTolerance) const
{
    if (row >= rows.size()) {
        return {};
    }
    const auto pattern = patterns.find(rows.at(row).pattern);
    if (pattern == patterns.end()) {
        return {};
    }
    const auto & notes = pattern->second.notes;
    for (auto note = notes.rbegin(); note != notes.rend(); note++) {
        if (note->track == track && line >= note->startLine && line < note->startLine + note->lengthLines && std::abs(pitch - static_cast<double>(note->pitch)) <= pitchTolerance) {
            return *note;
        }
    }
    return {};
}

std::optional<size_t> SongOverview::rowAt(double songLine) const
{
    if (songLine < 0.0) {
        return {};
    }
    // The first row ending past the line; rows are in timeline order
    const auto row = std::ranges::upper_bound(rows, songLine, std::less {}, [](const Row & r) { return static_cast<double>(r.startLine + r.lineCount); });
    return row != rows.end() ? std::optional { static_cast<size_t>(std::distance(rows.begin(), row)) } : std::nullopt;
}

std::optional<size_t> SongOverview::trackPosition(size_t trackIndex) const
{
    const auto track = std::ranges::find(tracks, trackIndex, &Track::index);
    return track != tracks.end() ? std::optional { static_cast<size_t>(std::distance(tracks.begin(), track)) } : std::nullopt;
}

} // namespace noteahead
