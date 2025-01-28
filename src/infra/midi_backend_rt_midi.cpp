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

#include "midi_backend_rt_midi.hpp"

#include <memory>
#include <stdexcept>

namespace noteahead {

void MidiBackendRtMidi::updateAvailableDevices()
{
    RtMidiOut tempMidiOut; // Temporary instance to list devices
    MidiDeviceList devices = {};
    const size_t portCount = tempMidiOut.getPortCount();
    for (size_t i = 0; i < portCount; ++i) {
        devices.push_back(std::make_shared<MidiDevice>(i, tempMidiOut.getPortName(i)));
    }
    setDevices(devices);
}

void MidiBackendRtMidi::openDevice(MidiDeviceS device)
{
    if (!m_midiPorts.contains(device->portIndex())) {
        if (auto && midiOut = std::make_unique<RtMidiOut>(); device->portIndex() >= midiOut->getPortCount()) {
            throw std::runtime_error { "Invalid MIDI port index: " + std::to_string(device->portIndex()) };
        } else {
            midiOut->openPort(device->portIndex());
            m_midiPorts[device->portIndex()] = std::move(midiOut);
        }
    }
}

void MidiBackendRtMidi::closeDevice(MidiDeviceS device)
{
    if (auto && it = m_midiPorts.find(device->portIndex()); it != m_midiPorts.end()) {
        m_midiPorts.erase(it);
    }
}

void MidiBackendRtMidi::sendMessage(MidiDeviceS device, const Message & message) const
{
    if (auto && it = m_midiPorts.find(device->portIndex()); it == m_midiPorts.end()) {
        throw std::runtime_error { "Device not opened." };
    } else {
        it->second->sendMessage(&message);
    }
}

void MidiBackendRtMidi::sendNoteOn(MidiDeviceS device, uint8_t channel, uint8_t note, uint8_t velocity) const
{
    const Message message = { static_cast<unsigned char>(0x90 | (channel & 0x0F)),
                              static_cast<unsigned char>(note),
                              static_cast<unsigned char>(velocity) };

    sendMessage(device, message);
}

void MidiBackendRtMidi::sendNoteOff(MidiDeviceS device, uint8_t channel, uint8_t note) const
{
    const Message message = { static_cast<unsigned char>(0x80 | (channel & 0x0F)),
                              static_cast<unsigned char>(note),
                              static_cast<unsigned char>(0) };

    sendMessage(device, message);
}

void MidiBackendRtMidi::sendPatchChange(MidiDeviceS device, uint8_t channel, uint8_t patch) const
{
    const Message message = { static_cast<unsigned char>(0xC0 | (channel & 0x0F)),
                              static_cast<unsigned char>(patch) };

    sendMessage(device, message);
}

void MidiBackendRtMidi::sendBankChange(MidiDeviceS device, uint8_t channel, uint8_t msb, uint8_t lsb) const
{
    // Send MSB (Control Change 0)
    const Message msbMessage = { static_cast<unsigned char>(0xB0 | (channel & 0x0F)),
                                 0x00, // CC #0 (Bank Select MSB)
                                 static_cast<unsigned char>(msb) };
    sendMessage(device, msbMessage);

    // Send LSB (Control Change 32)
    const Message lsbMessage = { static_cast<unsigned char>(0xB0 | (channel & 0x0F)),
                                 0x20, // CC #32 (Bank Select LSB)
                                 static_cast<unsigned char>(lsb) };
    sendMessage(device, lsbMessage);
}

void MidiBackendRtMidi::stopAllNotes(MidiDeviceS device, uint8_t channel) const
{
    const Message message = { static_cast<unsigned char>(0xB0 | (channel & 0x0F)),
                              123, // CC #123 (All Notes Off)
                              0 }; // Value for "All Notes Off"
    sendMessage(device, message);
}

} // namespace noteahead
