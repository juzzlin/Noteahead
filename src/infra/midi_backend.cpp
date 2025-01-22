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

#include "midi_backend.hpp"

#include <algorithm>

namespace cacophony {

MidiBackend::MidiBackend() = default;

MidiBackend::MidiDeviceList MidiBackend::listDevices() const
{
    return m_devices;
}

MidiDeviceS MidiBackend::deviceByPortIndex(uint32_t index) const
{
    if (auto device = std::find_if(m_devices.begin(), m_devices.end(), [&index](auto & device) { return device->portIndex() == index; }); device != m_devices.end()) {
        return *device;
    } else {
        return {};
    }
}

MidiDeviceS MidiBackend::deviceByPortName(const std::string & name) const
{
    if (auto device = std::find_if(m_devices.begin(), m_devices.end(), [&name](auto & device) { return device->portName() == name; }); device != m_devices.end()) {
        return *device;
    } else {
        return {};
    }
}

void MidiBackend::updateAvailableDevices()
{
}

void MidiBackend::setDevices(MidiDeviceList devices)
{
    m_devices = devices;
}

void MidiBackend::openDevice(MidiDeviceS)
{
}

void MidiBackend::closeDevice(MidiDeviceS)
{
}

void MidiBackend::sendNoteOn(MidiDeviceS, uint8_t, uint8_t, uint8_t) const
{
}

void MidiBackend::sendNoteOff(MidiDeviceS, uint8_t, uint8_t, uint8_t) const
{
}

void MidiBackend::sendPatchChange(MidiDeviceS, uint8_t, uint8_t) const
{
}

MidiBackend::~MidiBackend() = default;

} // namespace cacophony
