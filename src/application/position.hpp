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

#ifndef POSITION_HPP
#define POSITION_HPP

#include <QObject>
#include <cstdint>
#include <sstream>

namespace noteahead {

//! Position (or coordinates) of the editor focus.
struct Position
{
    Q_GADGET
    Q_PROPERTY(size_t pattern MEMBER pattern)
    Q_PROPERTY(size_t track MEMBER track)
    Q_PROPERTY(size_t column MEMBER column)
    Q_PROPERTY(size_t line MEMBER line)
    Q_PROPERTY(size_t lineColumn MEMBER lineColumn)

public:
    //! Index of the pattern.
    size_t pattern = 0;

    //! Index of the active track of the pattern.
    size_t track = 0;

    //! Index of the active note column of the track.
    size_t column = 0;

    //! Index of the active line (common for all tracks).
    size_t line = 0;

    //! Index of the active column of the line consisting of note and velocity values.
    //! 0: Note, e.g. "C-3"
    //! 1: Velocity digit 2, e.g. [1]27
    //! 2: Velocity digit 1, e.g. 1[2]7
    //! 3: Velocity digit 0, e.g. 12[7]
    size_t lineColumn = 0;

    bool operator==(const Position & other) const
    {
        return pattern == other.pattern && track == other.track && column == other.column && line == other.line && lineColumn == other.lineColumn;
    }

    bool operator!=(const Position & other) const
    {
        return !(*this == other);
    }

    std::string toString() const
    {
        std::stringstream ss;
        ss << "[ "
           << "Pattern: " << pattern << " Track: " << track << " Column: " << column << " Line: " << line << " Line Column: " << lineColumn << " ]";
        return ss.str();
    }
};

} // namespace noteahead

#endif // POSITION_HPP
