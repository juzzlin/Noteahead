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

#include "midi.hpp"

#include "../../common/utils.hpp"

#include <algorithm>

namespace noteahead {

Midi::Midi() = default;

Midi::MidiDeviceList Midi::listDevices() const
{
    return m_devices;
}

MidiDeviceS Midi::deviceByPortIndex(size_t index) const
{
    if (const auto device = std::ranges::find_if(m_devices, [&index](auto & device) { return device->portIndex() == index; }); device != m_devices.end()) {
        return *device;
    } else {
        return {};
    }
}

void Midi::invalidatePortNameCache()
{
    m_portNameToDeviceCache.clear();
}

MidiDeviceS Midi::deviceByPortName(const std::string & name) const
{
    if (const auto device = m_portNameToDeviceCache.find(name); device != m_portNameToDeviceCache.end()) {
        return device->second;
    }

    const auto bestMatch = std::ranges::max_element(m_devices, {}, [&](const auto & device) { return Utils::portNameMatchScore(name, device->portName()); });
    if (bestMatch != m_devices.end() && Utils::portNameMatchScore(name, (*bestMatch)->portName()) > 0.75) {
        m_portNameToDeviceCache[name] = *bestMatch;
        return *bestMatch;
    }

    return {};
}

std::string Midi::midiApiName() const
{
    return "";
}

void Midi::updateAvailableDevices()
{
}

void Midi::setDevices(MidiDeviceList devices)
{
    m_devices = devices;
}

void Midi::openDevice(MidiDeviceCR)
{
}

void Midi::closeDevice(MidiDeviceCR)
{
}

Midi::~Midi() = default;

} // namespace noteahead
