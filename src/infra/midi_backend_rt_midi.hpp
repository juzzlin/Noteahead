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

#ifndef MIDI_BACKEND_RT_MIDI_HPP
#define MIDI_BACKEND_RT_MIDI_HPP

#include "midi_backend.hpp"

#include <memory>
#include <unordered_map>

#include <rtmidi/RtMidi.h>

namespace noteahead {

//! MIDI backend implementation on the RtMidi library.
class MidiBackendRtMidi : public MidiBackend
{
public:
    MidiBackendRtMidi() = default;

    void updateAvailableDevices() override;

    void openDevice(MidiDeviceS device) override;

    void closeDevice(MidiDeviceS device) override;

    void sendCC(MidiDeviceS device, uint8_t channel, MidiCc controller, uint8_t value) const override;

    void sendNoteOn(MidiDeviceS device, uint8_t channel, uint8_t note, uint8_t velocity) const override;

    void sendNoteOff(MidiDeviceS device, uint8_t channel, uint8_t note) const override;

    void sendPatchChange(MidiDeviceS device, uint8_t channel, uint8_t patch) const override;

    void sendBankChange(MidiDeviceS device, uint8_t channel, uint8_t msb, uint8_t lsb) const override;

    void stopAllNotes(MidiDeviceS device, uint8_t channel) const override;

private:
    using Message = std::vector<unsigned char>;

    void sendMessage(MidiDeviceS device, const Message & message) const;

    std::unordered_map<size_t, std::unique_ptr<RtMidiOut>> m_midiPorts;
};

} // namespace noteahead

#endif // MIDI_BACKEND_RT_MIDI_HPP
