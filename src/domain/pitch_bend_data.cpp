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

#include "pitch_bend_data.hpp"

namespace noteahead {

PitchBendData::PitchBendData(size_t track, size_t column, uint8_t msb, uint8_t lsb)
  : EventData { track, column }
  , m_msb { msb }
  , m_lsb { lsb }
{
}

PitchBendData::PitchBendData(size_t track, size_t column, uint16_t value)
  : PitchBendData { track, column, static_cast<uint8_t>((value >> 7) & 0x7F), static_cast<uint8_t>(value & 0x7F) }
{
}

PitchBendData::PitchBendData(size_t track, size_t column, double percentage)
  : PitchBendData { track, column, static_cast<uint16_t>((percentage + 100.0) * (16383.0 / 200.0)) }
{
}

double PitchBendData::normalizedValue() const
{
    return static_cast<double>((m_msb << 7) | m_lsb) / 16383.0;
}

uint8_t PitchBendData::msb() const
{
    return m_msb;
}

uint8_t PitchBendData::lsb() const
{
    return m_lsb;
}

} // namespace noteahead
