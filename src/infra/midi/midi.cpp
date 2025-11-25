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

Midi::PortList Midi::ports() const
{
    return m_ports;
}

MidiPortS Midi::portByIndex(size_t index) const
{
    if (const auto device = std::ranges::find_if(m_ports, [&index](auto & device) { return device->index() == index; }); device != m_ports.end()) {
        return *device;
    } else {
        return {};
    }
}

void Midi::invalidatePortNameCache()
{
    m_nameToPortCache.clear();
}

MidiPortS Midi::portByName(const std::string & name) const
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

std::string Midi::midiApiName() const
{
    return "";
}

void Midi::updatePorts()
{
}

void Midi::setPorts(PortList ports)
{
    m_ports = ports;
}

void Midi::openPort(MidiPortCR)
{
}

void Midi::closePort(MidiPortCR)
{
}

Midi::PortNameList Midi::portNames() const
{
    Midi::PortNameList portNameList;
    std::ranges::transform(m_ports, std::back_inserter(portNameList),
                           [](const auto & port) { return port->name(); });
    std::ranges::sort(portNameList);
    return portNameList;
}

Midi::PortNameList Midi::availablePortNames() const
{
    return {};
}

Midi::~Midi() = default;

} // namespace noteahead
