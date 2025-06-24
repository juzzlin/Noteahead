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

#include "midi_in_rt_midi.hpp"

#include "../../../contrib/SimpleLogger/src/simple_logger.hpp"

#include <algorithm>
#include <memory>
#include <stdexcept>

namespace noteahead {

static const auto TAG = "MidiInRtMidi";

void MidiInRtMidi::updateDevices()
{
    DeviceList devices = {};
    PortNameList portNameList;
    RtMidiIn tempMidiIn; // Temporary instance to list devices
    const size_t portCount = tempMidiIn.getPortCount();
    for (uint8_t i = 0; i < portCount; i++) {
        devices.push_back(std::make_shared<MidiDevice>(i, tempMidiIn.getPortName(i)));
    }
    setDevices(devices);
    invalidatePortNameCache();
    m_openedPorts.clear();
}

Midi::PortNameList MidiInRtMidi::availablePortNames() const
{
    PortNameList portNameList;
    RtMidiIn tempMidiIn; // Temporary instance to list devices
    const size_t portCount = tempMidiIn.getPortCount();
    for (uint8_t i = 0; i < portCount; i++) {
        portNameList.push_back(tempMidiIn.getPortName(i));
    }
    std::ranges::sort(portNameList);
    return portNameList;
}

void MidiInRtMidi::openDevice(MidiDeviceCR device)
{
    if (!m_openedPorts.contains(device.portIndex())) {
        if (auto && midiIn = std::make_unique<RtMidiIn>(); device.portIndex() >= midiIn->getPortCount()) {
            throw std::runtime_error { "Invalid MIDI port index: " + std::to_string(device.portIndex()) };
        } else {
            midiIn->openPort(static_cast<uint8_t>(device.portIndex()));
            m_openedPorts[device.portIndex()] = std::move(midiIn);
        }
    }
}

void MidiInRtMidi::closeDevice(MidiDeviceCR device)
{
    if (auto && it = m_openedPorts.find(device.portIndex()); it != m_openedPorts.end()) {
        m_openedPorts.erase(it);
    }
}

std::string MidiInRtMidi::midiApiName() const
{
    return RtMidi::getApiDisplayName(RtMidiIn {}.getCurrentApi());
}

void MidiInRtMidi::setCallbackForPort(const MidiDevice & device, InputCallback callback)
{
    if (const auto index = device.portIndex(); m_openedPorts.contains(index)) {
        m_callbacks[index] = std::move(callback);
        m_openedPorts[index]->setCallback(&MidiInRtMidi::staticCallback, this);
        m_openedPorts[index]->ignoreTypes(false, false, false); // Enable all types
    } else {
        throw std::runtime_error("Port must be opened before setting callback!");
    }
}

void MidiInRtMidi::clearCallbacks()
{
    juzzlin::L(TAG).debug() << "Clearing callbacks";
    m_callbacks.clear();
}

void MidiInRtMidi::staticCallback(double deltaTime, MessageP message, void * userData)
{
    juzzlin::L(TAG).debug() << "staticCallback called";
    if (userData && message) {
        const auto self = static_cast<MidiInRtMidi *>(userData);
        // RtMidi does NOT pass the device index, so this is a workaround:
        for (auto & [index, midiIn] : self->m_openedPorts) {
            if (midiIn && midiIn->isPortOpen()) {
                if (self->m_callbacks.contains(index) && self->m_callbacks.at(index)) {
                    self->m_callbacks.at(index)(deltaTime, *message);
                }
            }
        }
    }
}

} // namespace noteahead
