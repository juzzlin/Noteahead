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

#ifndef SONG_OVERVIEW_BUILDER_HPP
#define SONG_OVERVIEW_BUILDER_HPP

#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace noteahead {

class AutomationService;
class Song;

//! The song as the Song Overview draws it: the play order laid end to end as a timeline, and every
//! pattern it plays reduced to note bars per track, the way a piano roll shows them.
//!
//! A tracker note has no length of its own. It lasts until the next note or note-off in its column,
//! or to the end of its pattern, and that is the length a bar gets here.
struct SongOverview
{
    struct Note
    {
        //! In lines from the pattern's start, the note's delay included.
        double startLine { 0.0 };
        double lengthLines { 0.0 };
        uint8_t pitch { 0 };
        uint8_t velocity { 0 };
        size_t track { 0 };
        size_t column { 0 };
    };

    //! An automation of a column, as the editor's Curve display draws it: one value per line of
    //! its pattern, 0 to 1, and none on the lines it does not cover.
    struct Curve
    {
        size_t track { 0 };
        size_t column { 0 };
        //! Its place among the automations of its column, which is what picks its color.
        size_t colorIndex { 0 };
        std::vector<std::optional<double>> values;
    };

    struct Pattern
    {
        size_t lineCount { 0 };
        std::string name;
        //! In drawing order: tracks left to right as the editor shows them, so that where bars
        //! overlap the last one is the one on top.
        std::vector<Note> notes;
        std::vector<Curve> curves;
    };

    //! A song position: where its pattern starts on the song's timeline, in lines.
    struct Row
    {
        size_t position { 0 };
        size_t pattern { 0 };
        bool skipped { false };
        size_t startLine { 0 };
        size_t lineCount { 0 };
    };

    //! A lane. Its pitch range is its own notes', at least an octave, so a lane of a few drums is
    //! as readable as a lane of chords.
    struct Track
    {
        size_t index { 0 };
        std::string name;
        uint8_t lowestPitch { 54 };
        uint8_t highestPitch { 65 };
    };

    std::vector<Row> rows;
    //! Each pattern the play order uses, once, however many times it is played.
    std::map<size_t, Pattern> patterns;
    //! In the order the editor shows them.
    std::vector<Track> tracks;
    //! The whole song's length in lines: where the last position ends.
    size_t totalLines { 0 };
    size_t linesPerBeat { 4 };

    //! Without automations.
    static SongOverview build(const Song & song);
    //! With the enabled automations of @p automationService as curves.
    static SongOverview build(const Song & song, const AutomationService & automationService);

    //! The note of @p track under @p line (in @p row's pattern) and @p pitch, within
    //! @p pitchTolerance semitones: the one drawn last where several are, which is the one seen.
    std::optional<Note> noteAt(size_t row, size_t track, double line, double pitch, double pitchTolerance) const;

    //! The row playing at @p songLine on the timeline, if any.
    std::optional<size_t> rowAt(double songLine) const;

    //! The display position of @p trackIndex, which is what picks its colour.
    std::optional<size_t> trackPosition(size_t trackIndex) const;
};

} // namespace noteahead

#endif // SONG_OVERVIEW_BUILDER_HPP
