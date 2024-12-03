// This file is part of Cacophony.
// Copyright (C) 2024 Jussi Lind <jussi.lind@iki.fi>
//
// Cacophony is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// Cacophony is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Cacophony. If not, see <http://www.gnu.org/licenses/>.

#ifndef MIDI_DEVICE_HPP
#define MIDI_DEVICE_HPP

#include <cstdint>
#include <memory>
#include <string>

namespace cacophony {

class MidiDevice
{
public:
    MidiDevice(uint32_t portIndex, std::string portName);

    uint32_t portIndex() const;

    std::string portName() const;

    std::string toString() const;

private:
    uint32_t mPortIndex;

    std::string mPortName;
};

using MidiDeviceS = std::shared_ptr<MidiDevice>;

} // namespace cacophony

#endif // MIDI_DEVICE_HPP
