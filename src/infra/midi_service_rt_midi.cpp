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

#include "midi_service_rt_midi.hpp"

#include <memory>
#include <stdexcept>

namespace cacophony {

void MidiServiceRtMidi::updateAvailableDevices()
{
    RtMidiOut tempMidiOut; // Temporary instance to list devices
    MidiDeviceList devices = {};
    const uint32_t portCount = tempMidiOut.getPortCount();
    for (uint32_t i = 0; i < portCount; ++i) {
        devices.push_back(std::make_shared<MidiDevice>(i, tempMidiOut.getPortName(i)));
    }
    setDevices(devices);
}

bool MidiServiceRtMidi::openDevice(MidiDeviceS device)
{
    if (m_midiPorts.find(device->portIndex()) != m_midiPorts.end()) {
        throw std::runtime_error { "Device already opened." };
    }

    if (auto midiOut = std::make_unique<RtMidiOut>(); device->portIndex() >= midiOut->getPortCount()) {
        throw std::runtime_error { "Invalid MIDI port index: " + std::to_string(device->portIndex()) };
    } else {
        midiOut->openPort(device->portIndex());
        m_midiPorts[device->portIndex()] = std::move(midiOut);
        return true;
    }
}

void MidiServiceRtMidi::closeDevice(MidiDeviceS device)
{
    if (auto it = m_midiPorts.find(device->portIndex()); it != m_midiPorts.end()) {
        m_midiPorts.erase(it);
    } else {
        throw std::runtime_error { "Device not opened." };
    }
}

void MidiServiceRtMidi::sendMessage(MidiDeviceS device, const Message & message) const
{
    if (auto it = m_midiPorts.find(device->portIndex()); it == m_midiPorts.end()) {
        throw std::runtime_error { "Device not opened." };
    } else {
        it->second->sendMessage(&message);
    }
}

void MidiServiceRtMidi::sendNoteOn(MidiDeviceS device, uint32_t channel, uint32_t note, uint32_t velocity) const
{
    Message message = { static_cast<unsigned char>(0x90 | (channel & 0x0F)),
                        static_cast<unsigned char>(note),
                        static_cast<unsigned char>(velocity) };

    sendMessage(device, message);
}

void MidiServiceRtMidi::sendNoteOff(MidiDeviceS device, uint32_t channel, uint32_t note, uint32_t velocity) const
{
    Message message = { static_cast<unsigned char>(0x80 | (channel & 0x0F)),
                        static_cast<unsigned char>(note),
                        static_cast<unsigned char>(velocity) };

    sendMessage(device, message);
}

} // namespace cacophony
