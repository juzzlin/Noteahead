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

#ifndef MIDI_BACKEND_HPP
#define MIDI_BACKEND_HPP

#include "midi_port.hpp"

#include <unordered_map>
#include <vector>

namespace noteahead {

class MidiBackend
{
public:
    MidiBackend();
    virtual ~MidiBackend();

    using PortList = std::vector<MidiPortS>;
    virtual PortList ports() const;
    virtual void updatePorts();
    using PortNameList = std::vector<std::string>;
    virtual PortNameList portNames() const;
    virtual PortNameList availablePortNames() const;

    virtual MidiPortS portByIndex(size_t index) const;
    virtual MidiPortS portByName(const std::string & name) const;

    //! \returns e.g. "ALSA"
    virtual std::string midiApiName() const;

    using MidiPortCR = const MidiPort &;
    virtual void openPort(MidiPortCR port);
    virtual void closePort(MidiPortCR port);

protected:
    void setPorts(PortList devices);
    void invalidatePortNameCache();

private:
    void initializeScanTimer();

    PortList m_ports;

    using NameToPort = std::unordered_map<std::string, MidiPortS>;
    mutable NameToPort m_nameToPortCache;
};

} // namespace noteahead

#endif // MIDI_BACKEND_HPP
