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

#include "midi_backend.hpp"

#include "../../common/utils.hpp"

#include <algorithm>

namespace noteahead {

MidiBackend::MidiBackend() = default;

MidiBackend::PortList MidiBackend::ports() const
{
    return m_ports;
}

MidiPortS MidiBackend::portByIndex(size_t index) const
{
    if (const auto device = std::ranges::find_if(m_ports, [&index](auto & device) { return device->index() == index; }); device != m_ports.end()) {
        return *device;
    } else {
        return {};
    }
}

void MidiBackend::invalidatePortNameCache()
{
    m_nameToPortCache.clear();
}

MidiPortS MidiBackend::portByName(const std::string & name) const
{
    if (const auto port = m_nameToPortCache.find(name); port != m_nameToPortCache.end()) {
        return port->second;
    }

    const auto bestMatch = std::ranges::max_element(m_ports, {}, [&](const auto & port) { return Utils::Midi::portNameMatchScore(name, port->name()); });
    if (bestMatch != m_ports.end() && Utils::Midi::portNameMatchScore(name, (*bestMatch)->name()) > 0.75) {
        m_nameToPortCache[name] = *bestMatch;
        return *bestMatch;
    }

    return {};
}

std::string MidiBackend::midiApiName() const
{
    return "";
}

void MidiBackend::updatePorts()
{
    // Call setPorts() from the inherited backend.
}

void MidiBackend::setPorts(PortList ports)
{
    m_ports = ports;
}

void MidiBackend::openPort(MidiPortCR)
{
}

void MidiBackend::closePort(MidiPortCR)
{
}

MidiBackend::PortNameList MidiBackend::portNames() const
{
    MidiBackend::PortNameList portNameList;
    std::ranges::transform(m_ports, std::back_inserter(portNameList),
                           [](const auto & port) { return port->name(); });
    std::ranges::sort(portNameList);
    return portNameList;
}

MidiBackend::PortNameList MidiBackend::availablePortNames() const
{
    return portNames();
}

MidiBackend::~MidiBackend() = default;

} // namespace noteahead
