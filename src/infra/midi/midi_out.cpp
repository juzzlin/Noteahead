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

namespace noteahead {

MidiOut::MidiOut() = default;

void MidiOut::sendCcData(MidiDeviceCR, uint8_t, uint8_t, uint8_t) const
{
}

void MidiOut::sendNoteOn(MidiDeviceCR device, uint8_t channel, uint8_t note, uint8_t) const
{
    m_notesOn[device.portIndex()].insert({ channel, note });
}

MidiOut::NotesOnList MidiOut::notesOn(MidiDeviceCR device, uint8_t channel) const
{
    MidiOut::NotesOnList notesOnList;
    for (auto && channelAndNote : m_notesOn[device.portIndex()]) {
        if (channelAndNote.first == channel) {
            notesOnList.push_back(channelAndNote.second);
        }
    }
    return notesOnList;
}

void MidiOut::sendNoteOff(MidiDeviceCR device, uint8_t channel, uint8_t note) const
{
    m_notesOn[device.portIndex()].erase({ channel, note });
}

void MidiOut::sendPatchChange(MidiDeviceCR, uint8_t, uint8_t) const
{
}

void MidiOut::sendBankChange(MidiDeviceCR, uint8_t, uint8_t, uint8_t) const
{
}

void MidiOut::sendPitchBendData(MidiDeviceCR, uint8_t, uint8_t, uint8_t) const
{
}

void MidiOut::stopAllNotes(MidiDeviceCR, uint8_t) const
{
}

void MidiOut::sendClockPulse(MidiDeviceCR) const
{
}

void MidiOut::sendStart(MidiDeviceCR) const
{
}

void MidiOut::sendStop(MidiDeviceCR) const
{
}

MidiOut::~MidiOut() = default;

} // namespace noteahead
