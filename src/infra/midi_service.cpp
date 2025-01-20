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

#include "midi_service.hpp"

#include <algorithm>

namespace cacophony {

MidiService::MidiService() = default;

MidiService::MidiDeviceList MidiService::listDevices() const
{
    return m_devices;
}

MidiDeviceS MidiService::deviceByPortIndex(uint32_t index) const
{
    if (auto device = std::find_if(m_devices.begin(), m_devices.end(), [&index](auto & device) { return device->portIndex() == index; }); device != m_devices.end()) {
        return *device;
    } else {
        return {};
    }
}

MidiDeviceS MidiService::deviceByPortName(const std::string & name) const
{
    if (auto device = std::find_if(m_devices.begin(), m_devices.end(), [&name](auto & device) { return device->portName() == name; }); device != m_devices.end()) {
        return *device;
    } else {
        return {};
    }
}

void MidiService::updateAvailableDevices()
{
}

void MidiService::setDevices(MidiDeviceList devices)
{
    m_devices = devices;
}

void MidiService::openDevice(MidiDeviceS)
{
}

void MidiService::closeDevice(MidiDeviceS)
{
}

void MidiService::sendNoteOn(MidiDeviceS, uint8_t, uint8_t, uint8_t) const
{
}

void MidiService::sendNoteOff(MidiDeviceS, uint8_t, uint8_t, uint8_t) const
{
}

void MidiService::sendPatchChange(MidiDeviceS, uint8_t, uint8_t) const
{
}

MidiService::~MidiService() = default;

} // namespace cacophony
