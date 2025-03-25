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

#ifndef MIDI_DEVICE_HPP
#define MIDI_DEVICE_HPP

#include <cstddef>
#include <memory>
#include <string>

namespace noteahead {

class MidiDevice
{
public:
    MidiDevice(size_t portIndex, std::string portName);

    size_t portIndex() const;

    std::string portName() const;

    std::string toString() const;

private:
    size_t mPortIndex;

    std::string mPortName;
};

using MidiDeviceS = std::shared_ptr<MidiDevice>;
using MidiDeviceW = std::weak_ptr<MidiDevice>;

} // namespace noteahead

#endif // MIDI_DEVICE_HPP
