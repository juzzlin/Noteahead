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

#include "midi_backend_out.hpp"

#include <algorithm>

namespace noteahead {

MidiBackendOut::MidiBackendOut(PortNameList virtualPorts)
  : m_virtualPorts { virtualPorts }
{
}

void MidiBackendOut::sendCcData(MidiPortCR, uint8_t, uint8_t, uint8_t) const
{
}

void MidiBackendOut::sendNoteOn(MidiPortCR port, uint8_t channel, uint8_t note, uint8_t) const
{
    m_notesOn[port.index()].insert({ channel, note });
}

MidiBackendOut::NotesOnList MidiBackendOut::notesOn(MidiPortCR port, uint8_t channel) const
{
    MidiBackendOut::NotesOnList notesOnList;
    for (auto && channelAndNote : m_notesOn[port.index()]) {
        if (channelAndNote.first == channel) {
            notesOnList.push_back(channelAndNote.second);
        }
    }
    return notesOnList;
}

void MidiBackendOut::sendNoteOff(MidiPortCR port, uint8_t channel, uint8_t note) const
{
    m_notesOn[port.index()].erase({ channel, note });
}

void MidiBackendOut::sendPatchChange(MidiPortCR, uint8_t, uint8_t) const
{
}

void MidiBackendOut::sendBankChange(MidiPortCR, uint8_t, uint8_t, uint8_t) const
{
}

void MidiBackendOut::sendPitchBendData(MidiPortCR, uint8_t, uint8_t, uint8_t) const
{
}

void MidiBackendOut::stopAllNotes(MidiPortCR, uint8_t) const
{
}

void MidiBackendOut::sendClockPulse(MidiPortCR) const
{
}

void MidiBackendOut::sendStart(MidiPortCR) const
{
}

void MidiBackendOut::sendStop(MidiPortCR) const
{
}

MidiBackendOut::PortNameList MidiBackendOut::virtualPorts() const
{
    return m_virtualPorts;
}

bool MidiBackendOut::isVirtualPort(const std::string & name) const
{
    return std::ranges::find(m_virtualPorts, name) != m_virtualPorts.end();
}

MidiBackendOut::~MidiBackendOut() = default;

} // namespace noteahead
