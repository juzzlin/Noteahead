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

#ifndef AUTO_NOTE_OFF_OFFSET_HPP
#define AUTO_NOTE_OFF_OFFSET_HPP

#include <chrono>
#include <cstddef>
#include <vector>

#include <QString>

namespace noteahead {

class ProjectReader;
class ProjectWriter;

//! How long before the next note-on the note playing on the same column is cut. Cutting exactly at
//! the next note-on would be correct on paper, but real MIDI hardware needs the gap to retrigger.
//!
//! Two modes, because both readings of "the right gap" are legitimate: a hardware synth wants the
//! same number of milliseconds no matter the tempo, while a part written against the groove wants a
//! gap that scales with it.
//!
//! Sync mode is stored as an integer denominator (1/16 is 16), never as a fraction of a whole note
//! in floating point: the project format contains no floating-point values anywhere, because
//! std::to_string, std::stod and printf("%f") all follow the C locale and a locale that writes
//! "0,0625" stops the file round-tripping.
class AutoNoteOffOffset
{
public:
    enum class Mode
    {
        Milliseconds, //!< Tempo-independent.
        Sync //!< A fraction of a whole note at the song's tempo.
    };

    AutoNoteOffOffset() = default;
    explicit AutoNoteOffOffset(std::chrono::milliseconds milliseconds);
    explicit AutoNoteOffOffset(int syncDenominator);

    Mode mode() const;
    void setMode(Mode mode);

    bool syncEnabled() const;
    void setSyncEnabled(bool enabled);

    std::chrono::milliseconds milliseconds() const;
    void setMilliseconds(std::chrono::milliseconds milliseconds);

    //! The 1/N of sync mode, e.g. 16 for a sixteenth note.
    int syncDenominator() const;
    void setSyncDenominator(int denominator);

    //! The denominators offered by the UI, in ascending order, which is descending note length.
    //! Includes the sixteenth and eighth triplets (1/24 and 1/48); dotted values are absent, as a
    //! gap that only needs to clear the hardware has no use for them.
    static const std::vector<int> & syncDenominators();

    //! The offset in ticks at the given timing. Both modes land here, so callers never need to know
    //! which one is active.
    size_t ticks(size_t beatsPerMinute, size_t linesPerBeat, size_t ticksPerLine) const;

    //! Writes into the element the caller has open, so that both Song's settings and a channel's
    //! override can carry one of these without a wrapper element of its own.
    void serializeToXmlAttributes(ProjectWriter & writer) const;
    static AutoNoteOffOffset deserializeFromXmlAttributes(ProjectReader & reader);

    bool operator==(const AutoNoteOffOffset & other) const;
    bool operator!=(const AutoNoteOffOffset & other) const;

    QString toString() const;

private:
    Mode m_mode = Mode::Milliseconds;

    std::chrono::milliseconds m_milliseconds { 125 };

    int m_syncDenominator = 32;
};

} // namespace noteahead

#endif // AUTO_NOTE_OFF_OFFSET_HPP
