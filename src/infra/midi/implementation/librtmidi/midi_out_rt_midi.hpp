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

#ifndef MIDI_OUT_RT_MIDI_HPP
#define MIDI_OUT_RT_MIDI_HPP

#include "../../midi_backend_out.hpp"

#include <memory>
#include <unordered_map>

#include <rtmidi/RtMidi.h>

namespace noteahead {

//! Implementation of the MIDI output backend using the RtMidi library.
class MidiOutRtMidi : public MidiBackendOut
{
public:
    MidiOutRtMidi();
    ~MidiOutRtMidi() override;

    void updatePorts() override;

    void openPort(MidiPortCR port) override;
    void closePort(MidiPortCR port) override;

    std::string midiApiName() const override;

    PortNameList availablePortNames() const override;

    void sendCcData(MidiPortCR port, uint8_t channel, uint8_t controller, uint8_t value) const override;

    void sendNoteOn(MidiPortCR port, uint8_t channel, uint8_t note, uint8_t velocity) const override;
    void sendNoteOff(MidiPortCR port, uint8_t channel, uint8_t note) const override;
    void stopAllNotes(MidiPortCR port, uint8_t channel) const override;

    void sendPatchChange(MidiPortCR port, uint8_t channel, uint8_t patch) const override;
    void sendBankChange(MidiPortCR port, uint8_t channel, uint8_t msb, uint8_t lsb) const override;

    void sendPitchBendData(MidiPortCR port, uint8_t channel, uint8_t msb, uint8_t lsb) const override;

    void sendClockPulse(MidiPortCR port) const override;
    void sendStart(MidiPortCR port) const override;
    void sendStop(MidiPortCR port) const override;

private:
    using Message = std::vector<unsigned char>;

    void openVirtualPort(const std::string & name);

    void sendMessage(MidiPortCR port, const Message & message) const;

    std::unordered_map<size_t, std::unique_ptr<RtMidiOut>> m_ports;
    std::unordered_map<std::string, std::unique_ptr<RtMidiOut>> m_virtualPorts;
};

} // namespace noteahead

#endif // MIDI_OUT_RT_MIDI_HPP
