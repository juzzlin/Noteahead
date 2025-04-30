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

#ifndef MIDI_CC_DATA_HPP
#define MIDI_CC_DATA_HPP

#include "event_data.hpp"

#include <cstdint>

namespace noteahead {

class MidiCcData : public EventData
{
public:
    MidiCcData(size_t track, size_t column, uint8_t controller, uint8_t value);

    uint8_t controller() const;
    uint8_t value() const;

    double normalizedValue() const;

private:
    uint8_t m_controller = 0;
    uint8_t m_value = 0;
};

} // namespace noteahead

#endif // MIDI_CC_DATA_HPP
