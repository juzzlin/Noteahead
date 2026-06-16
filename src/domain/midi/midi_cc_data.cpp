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

#include "midi_cc_data.hpp"

namespace noteahead {

MidiCcData::MidiCcData(size_t track, size_t column, uint8_t controller, uint8_t value)
  : EventData { track, column }
  , m_controller { controller }
  , m_value { value }
{
}

uint8_t MidiCcData::controller() const
{
    return m_controller;
}

uint8_t MidiCcData::value() const
{
    return m_value;
}

double MidiCcData::normalizedValue() const
{
    return static_cast<double>(m_value) / 127;
}

} // namespace noteahead
