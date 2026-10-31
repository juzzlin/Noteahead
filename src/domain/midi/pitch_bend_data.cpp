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

#include <algorithm>
#include <cmath>

namespace noteahead {

namespace {
constexpr double maxValue14 = 16383.0;
}

PitchBendData::PitchBendData(size_t track, size_t column, uint8_t msb, uint8_t lsb)
  : PitchBendData { track, column, static_cast<uint16_t>((msb & 0x7F) << 7 | (lsb & 0x7F)) }
{
}

PitchBendData::PitchBendData(size_t track, size_t column, uint16_t value)
  : EventData { track, column }
  , m_bend { std::clamp(static_cast<double>(value) * 2.0 / maxValue14 - 1.0, -1.0, 1.0) }
{
}

PitchBendData::PitchBendData(size_t track, size_t column, double percentage)
  : EventData { track, column }
  , m_bend { std::clamp(percentage / 100.0, -1.0, 1.0) }
{
}

double PitchBendData::bend() const
{
    return m_bend;
}

double PitchBendData::normalizedValue() const
{
    return static_cast<double>(value14()) / maxValue14;
}

uint16_t PitchBendData::value14() const
{
    return static_cast<uint16_t>(std::round((m_bend + 1.0) * maxValue14 / 2.0));
}

uint8_t PitchBendData::msb() const
{
    return static_cast<uint8_t>((value14() >> 7) & 0x7F);
}

uint8_t PitchBendData::lsb() const
{
    return static_cast<uint8_t>(value14() & 0x7F);
}

} // namespace noteahead
