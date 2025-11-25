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

#ifndef MIDI_OUT_HPP
#define MIDI_OUT_HPP

#include "midi.hpp"

#include <cstdint>
#include <set>
#include <unordered_map>
#include <vector>

namespace noteahead {

//! Base class for MIDI output backend implementations.
class MidiOut : public Midi
{
public:
    using PortNameList = std::vector<std::string>;
    MidiOut(PortNameList virtualPorts = {});
    virtual ~MidiOut() override;

    using MidiPortCR = const MidiPort &;

    virtual void stopAllNotes(MidiPortCR port, uint8_t channel) const;

    virtual void sendCcData(MidiPortCR port, uint8_t channel, uint8_t controller, uint8_t value) const;

    virtual void sendNoteOn(MidiPortCR port, uint8_t channel, uint8_t note, uint8_t velocity) const;
    virtual void sendNoteOff(MidiPortCR port, uint8_t channel, uint8_t note) const;

    virtual void sendPatchChange(MidiPortCR port, uint8_t channel, uint8_t patch) const;
    virtual void sendBankChange(MidiPortCR port, uint8_t channel, uint8_t msb, uint8_t lsb) const;

    virtual void sendPitchBendData(MidiPortCR port, uint8_t channel, uint8_t msb, uint8_t lsb) const;

    virtual void sendClockPulse(MidiPortCR port) const;
    virtual void sendStart(MidiPortCR port) const;
    virtual void sendStop(MidiPortCR port) const;

protected:
    using NotesOnList = std::vector<uint8_t>;
    NotesOnList notesOn(MidiPortCR port, uint8_t channel) const;

    PortNameList virtualPorts() const;
    bool isVirtualPort(const std::string & name) const;

private:
    using ChannelAndNote = std::pair<uint8_t, uint8_t>;
    using PortToChannelAndNote = std::unordered_map<size_t, std::set<ChannelAndNote>>;
    mutable PortToChannelAndNote m_notesOn;

    PortNameList m_virtualPorts;
};

} // namespace noteahead

#endif // MIDI_OUT_HPP
