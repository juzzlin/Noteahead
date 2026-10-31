// This file is part of Noteahead.
// Copyright (C) 2025 Jussi Lind <jussi.lind@iki.fi>
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

#ifndef PITCH_BEND_DATA_HPP
#define PITCH_BEND_DATA_HPP

#include "../tracker/event_data.hpp"

#include <cstdint>

namespace noteahead {

//! A pitch bend, held at full precision rather than in any protocol's resolution: MIDI 1.0 has 14
//! bits for it and MIDI 2.0 has 32, and only the path that sends one knows which it speaks.
class PitchBendData : public EventData
{
public:
    PitchBendData(size_t track, size_t column, uint8_t msb, uint8_t lsb);
    //! A MIDI 1.0 value, 0 to 16383 with the centre at 8192.
    PitchBendData(size_t track, size_t column, uint16_t value);
    //! -100 to 100 percent of the bend range.
    PitchBendData(size_t track, size_t column, double percentage);

    //! -1 to 1, unquantized.
    double bend() const;

    //! The MIDI 1.0 value, 0 to 16383, as 0 to 1.
    double normalizedValue() const;

    //! The MIDI 1.0 value, 0 to 16383, and its two data bytes.
    uint16_t value14() const;
    uint8_t msb() const;
    uint8_t lsb() const;

private:
    double m_bend = 0.0;
};
} // namespace noteahead

#endif // PITCH_BEND_DATA_HPP
