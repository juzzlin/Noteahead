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

#ifndef MIDI_BACKEND_HPP
#define MIDI_BACKEND_HPP

#include "midi_device.hpp"

#include <cstdint>
#include <unordered_map>
#include <vector>

namespace noteahead {

//! Base class for MIDI backend implementations.
class MidiBackend
{
public:
    MidiBackend();

    virtual ~MidiBackend();

    using MidiDeviceList = std::vector<MidiDeviceS>;

    virtual MidiDeviceList listDevices() const;

    virtual MidiDeviceS deviceByPortIndex(size_t index) const;

    virtual MidiDeviceS deviceByPortName(const std::string & name) const;

    //! \returns e.g. "ALSA"
    virtual std::string midiApiName() const;

    virtual void updateAvailableDevices();

    using MidiDeviceCR = const MidiDevice &;

    virtual void openDevice(MidiDeviceCR device);

    virtual void closeDevice(MidiDeviceCR device);

    virtual void sendCC(MidiDeviceCR device, uint8_t channel, uint8_t controller, uint8_t value) const;

    virtual void sendNoteOn(MidiDeviceCR device, uint8_t channel, uint8_t note, uint8_t velocity) const;

    virtual void sendNoteOff(MidiDeviceCR device, uint8_t channel, uint8_t note) const;

    virtual void sendPatchChange(MidiDeviceCR device, uint8_t channel, uint8_t patch) const;

    virtual void sendBankChange(MidiDeviceCR device, uint8_t channel, uint8_t msb, uint8_t lsb) const;

    virtual void stopAllNotes(MidiDeviceCR device, uint8_t channel) const;

    virtual void sendClock(MidiDeviceCR device) const;

protected:
    void setDevices(MidiDeviceList devices);

    void invalidatePortNameCache();

private:
    MidiDeviceList m_devices;

    mutable std::unordered_map<std::string, MidiDeviceS> m_portNameToDeviceCache;
};

} // namespace noteahead

#endif // MIDI_BACKEND_HPP
