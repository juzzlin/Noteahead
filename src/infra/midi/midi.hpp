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

#ifndef MIDI_HPP
#define MIDI_HPP

#include "midi_device.hpp"

#include <unordered_map>
#include <vector>

namespace noteahead {

//! Base class for MIDI backend implementations.
class Midi
{
public:
    Midi();

    virtual ~Midi();

    using DeviceList = std::vector<MidiDeviceS>;
    virtual DeviceList devices() const;
    virtual void updateDevices();

    using PortNameList = std::vector<std::string>;
    virtual PortNameList portNames() const;
    virtual PortNameList availablePortNames() const;

    virtual MidiDeviceS deviceByPortIndex(size_t index) const;
    virtual MidiDeviceS deviceByPortName(const std::string & name) const;

    //! \returns e.g. "ALSA"
    virtual std::string midiApiName() const;

    using MidiDeviceCR = const MidiDevice &;

    virtual void openDevice(MidiDeviceCR device);
    virtual void closeDevice(MidiDeviceCR device);

protected:
    void setDevices(DeviceList devices);

    void invalidatePortNameCache();

private:
    DeviceList m_devices;

    using PortNameToDevice = std::unordered_map<std::string, MidiDeviceS>;
    mutable PortNameToDevice m_portNameToDeviceCache;
};

} // namespace noteahead

#endif // MIDI_HPP
