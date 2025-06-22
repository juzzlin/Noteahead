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
#include <set>
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
    virtual void stopAllNotes(MidiDeviceCR device, uint8_t channel) const;

    virtual void sendCcData(MidiDeviceCR device, uint8_t channel, uint8_t controller, uint8_t value) const;

    virtual void sendNoteOn(MidiDeviceCR device, uint8_t channel, uint8_t note, uint8_t velocity) const;
    virtual void sendNoteOff(MidiDeviceCR device, uint8_t channel, uint8_t note) const;

    virtual void sendPatchChange(MidiDeviceCR device, uint8_t channel, uint8_t patch) const;
    virtual void sendBankChange(MidiDeviceCR device, uint8_t channel, uint8_t msb, uint8_t lsb) const;

    virtual void sendPitchBendData(MidiDeviceCR device, uint8_t channel, uint8_t msb, uint8_t lsb) const;

    virtual void sendClockPulse(MidiDeviceCR device) const;
    virtual void sendStart(MidiDeviceCR device) const;
    virtual void sendStop(MidiDeviceCR device) const;

protected:
    void setDevices(MidiDeviceList devices);

    void invalidatePortNameCache();

    using NotesOnList = std::vector<uint8_t>;
    NotesOnList notesOn(MidiDeviceCR device, uint8_t channel) const;

private:
    MidiDeviceList m_devices;

    using ChannelAndNote = std::pair<uint8_t, uint8_t>;
    using DeviceToChannelAndNote = std::unordered_map<size_t, std::set<ChannelAndNote>>;
    mutable DeviceToChannelAndNote m_notesOn;

    using PortNameToDevice = std::unordered_map<std::string, MidiDeviceS>;
    mutable PortNameToDevice m_portNameToDeviceCache;
};

} // namespace noteahead

#endif // MIDI_BACKEND_HPP
