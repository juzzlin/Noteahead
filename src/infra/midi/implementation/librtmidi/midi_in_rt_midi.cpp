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

#include "../../../../contrib/SimpleLogger/src/simple_logger.hpp"

#include <algorithm>
#include <memory>
#include <stdexcept>

namespace noteahead {

static const auto TAG = "MidiInRtMidi";

void MidiInRtMidi::updatePorts()
{
    PortList ports = {};
    PortNameList portNameList;
    RtMidiIn tempMidiIn; // Temporary instance to list ports
    const size_t portCount = tempMidiIn.getPortCount();
    for (uint8_t i = 0; i < portCount; i++) {
        ports.push_back(std::make_shared<MidiPort>(i, tempMidiIn.getPortName(i)));
    }
    setPorts(ports);
    invalidatePortNameCache();
}

MidiBackend::PortNameList MidiInRtMidi::availablePortNames() const
{
    PortNameList portNameList;
    RtMidiIn tempMidiIn; // Temporary instance to list ports
    const size_t portCount = tempMidiIn.getPortCount();
    for (uint8_t i = 0; i < portCount; i++) {
        portNameList.push_back(tempMidiIn.getPortName(i));
    }
    std::ranges::sort(portNameList);
    return portNameList;
}

void MidiInRtMidi::openPort(MidiPortCR port)
{
    if (!m_openedPorts.contains(port.name())) {
        if (auto && midiIn = std::make_unique<RtMidiIn>(); port.index() >= midiIn->getPortCount()) {
            throw std::runtime_error { "Invalid MIDI port index: " + std::to_string(port.index()) };
        } else {
            midiIn->openPort(static_cast<uint8_t>(port.index()));
            m_openedPorts[port.name()] = std::move(midiIn);
        }
    }
}

void MidiInRtMidi::closePort(MidiPortCR port)
{
    if (auto && it = m_openedPorts.find(port.name()); it != m_openedPorts.end()) {
        m_callbacks.erase(port.name());
        m_callbackInfos.erase(port.name());
        m_openedPorts.erase(it);
    }
}

bool MidiInRtMidi::isPortOpen(MidiPortCR port) const
{
    return m_openedPorts.contains(port.name());
}

std::string MidiInRtMidi::midiApiName() const
{
    return RtMidi::getApiDisplayName(RtMidiIn {}.getCurrentApi());
}

void MidiInRtMidi::setCallbackForPort(const MidiPort & port, InputCallback callback)
{
    if (const auto name = port.name(); m_openedPorts.contains(name)) {
        m_openedPorts[name]->cancelCallback();
        m_callbacks[name] = std::move(callback);
        m_callbackInfos[name] = std::make_unique<CallbackInfo>(this, name);
        m_openedPorts[name]->setCallback(&MidiInRtMidi::staticCallback, m_callbackInfos[name].get());
        m_openedPorts[name]->ignoreTypes(false, false, true);
    } else {
        throw std::runtime_error("Port must be opened before setting callback!");
    }
}

void MidiInRtMidi::clearCallbacks()
{
    juzzlin::L(TAG).debug() << "Clearing callbacks";
    for (auto & [name, midiIn] : m_openedPorts) {
        midiIn->cancelCallback();
    }
    m_callbacks.clear();
    m_callbackInfos.clear();
}

void MidiInRtMidi::staticCallback(double deltaTime, MessageP message, void * userData)
{
    if (userData && message) {
        const auto info = static_cast<CallbackInfo *>(userData);
        const auto self = info->backend;
        const auto & portName = info->portName;
        if (self->m_callbacks.contains(portName) && self->m_callbacks.at(portName)) {
            self->m_callbacks.at(portName)(deltaTime, *message);
        }
    }
}

} // namespace noteahead
