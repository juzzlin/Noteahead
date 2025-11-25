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

#include "midi_out.hpp"

#include <algorithm>

namespace noteahead {

MidiOut::MidiOut(PortNameList virtualPorts)
  : m_virtualPorts { virtualPorts }
{
}

void MidiOut::sendCcData(MidiPortCR, uint8_t, uint8_t, uint8_t) const
{
}

void MidiOut::sendNoteOn(MidiPortCR port, uint8_t channel, uint8_t note, uint8_t) const
{
    m_notesOn[port.index()].insert({ channel, note });
}

MidiOut::NotesOnList MidiOut::notesOn(MidiPortCR port, uint8_t channel) const
{
    MidiOut::NotesOnList notesOnList;
    for (auto && channelAndNote : m_notesOn[port.index()]) {
        if (channelAndNote.first == channel) {
            notesOnList.push_back(channelAndNote.second);
        }
    }
    return notesOnList;
}

void MidiOut::sendNoteOff(MidiPortCR port, uint8_t channel, uint8_t note) const
{
    m_notesOn[port.index()].erase({ channel, note });
}

void MidiOut::sendPatchChange(MidiPortCR, uint8_t, uint8_t) const
{
}

void MidiOut::sendBankChange(MidiPortCR, uint8_t, uint8_t, uint8_t) const
{
}

void MidiOut::sendPitchBendData(MidiPortCR, uint8_t, uint8_t, uint8_t) const
{
}

void MidiOut::stopAllNotes(MidiPortCR, uint8_t) const
{
}

void MidiOut::sendClockPulse(MidiPortCR) const
{
}

void MidiOut::sendStart(MidiPortCR) const
{
}

void MidiOut::sendStop(MidiPortCR) const
{
}

MidiOut::PortNameList MidiOut::virtualPorts() const
{
    return m_virtualPorts;
}

bool MidiOut::isVirtualPort(const std::string & name) const
{
    return std::ranges::find(m_virtualPorts, name) != m_virtualPorts.end();
}

MidiOut::~MidiOut() = default;

} // namespace noteahead
