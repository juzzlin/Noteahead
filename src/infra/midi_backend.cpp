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

#include "midi_backend.hpp"

#include <algorithm>

namespace noteahead {

MidiBackend::MidiBackend() = default;

MidiBackend::MidiDeviceList MidiBackend::listDevices() const
{
    return m_devices;
}

MidiDeviceS MidiBackend::deviceByPortIndex(size_t index) const
{
    if (const auto device = std::ranges::find_if(m_devices, [&index](auto & device) { return device->portIndex() == index; }); device != m_devices.end()) {
        return *device;
    } else {
        return {};
    }
}

static double portNameMatchScore(const std::string & s1, const std::string & s2)
{
    if (s1.empty() || s2.empty()) {
        return 0;
    }

    if (s1 == s2) {
        return 1;
    }

    size_t count = 0;
    while (count < s1.size() && count < s2.size() && s1.at(count) == s2.at(count)) {
        count++;
    }

    return static_cast<double>(count) / static_cast<double>(std::max(s1.size(), s2.size()));
}

MidiDeviceS MidiBackend::deviceByPortName(const std::string & name) const
{
    if (const auto device = m_portNameToDeviceCache.find(name); device != m_portNameToDeviceCache.end()) {
        return device->second;
    }

    const auto bestMatch = std::ranges::max_element(m_devices, {}, [&](const auto & device) { return portNameMatchScore(name, device->portName()); });
    if (bestMatch != m_devices.end() && portNameMatchScore(name, (*bestMatch)->portName()) > 0.75) {
        m_portNameToDeviceCache[name] = *bestMatch;
        return *bestMatch;
    }

    return {};
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

void MidiBackend::sendCC(MidiDeviceS, uint8_t, MidiCC, uint8_t) const
{
}

void MidiBackend::sendNoteOn(MidiDeviceS, uint8_t, uint8_t, uint8_t) const
{
}

void MidiBackend::sendNoteOff(MidiDeviceS, uint8_t, uint8_t) const
{
}

void MidiBackend::sendPatchChange(MidiDeviceS, uint8_t, uint8_t) const
{
}

void MidiBackend::sendBankChange(MidiDeviceS, uint8_t, uint8_t, uint8_t) const
{
}

void MidiBackend::stopAllNotes(MidiDeviceS, uint8_t) const
{
}

MidiBackend::~MidiBackend() = default;

} // namespace noteahead
