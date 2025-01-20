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

#ifndef MIDI_SERVICE_HPP
#define MIDI_SERVICE_HPP

#include "midi_device.hpp"

#include <cstdint>
#include <vector>

namespace cacophony {

//! Base class for MIDI service implementations.
class MidiService
{
public:
    MidiService();

    virtual ~MidiService();

    using MidiDeviceList = std::vector<MidiDeviceS>;

    virtual MidiDeviceList listDevices() const;

    virtual MidiDeviceS deviceByPortIndex(uint32_t index) const;

    virtual MidiDeviceS deviceByPortName(const std::string & name) const;

    virtual void updateAvailableDevices();

    virtual void openDevice(MidiDeviceS device);

    virtual void closeDevice(MidiDeviceS device);

    virtual void sendNoteOn(MidiDeviceS device, uint8_t channel, uint8_t note, uint8_t velocity) const;

    virtual void sendNoteOff(MidiDeviceS device, uint8_t channel, uint8_t note, uint8_t velocity) const;

    virtual void sendPatchChange(MidiDeviceS device, uint8_t channel, uint8_t patch) const;

protected:
    void setDevices(MidiDeviceList devices);

private:
    MidiDeviceList m_devices;
};

} // namespace cacophony

#endif // MIDI_SERVICE_HPP
