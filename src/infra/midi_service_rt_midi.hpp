// This file is part of Cacophony.
// Copyright (C) 2024 Jussi Lind <jussi.lind@iki.fi>
//
// Cacophony is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// Cacophony is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Cacophony. If not, see <http://www.gnu.org/licenses/>.

#ifndef MIDI_SERVICE_RT_MIDI_HPP
#define MIDI_SERVICE_RT_MIDI_HPP

#include "midi_service.hpp"

#include <map>
#include <memory>

#include <rtmidi/RtMidi.h>

namespace cacophony {

//! MIDI-service implementation on the RtMidi library.
class MidiServiceRtMidi : public MidiService
{
public:
    MidiServiceRtMidi() = default;

    void updateAvailableDevices() override;

    bool openDevice(MidiDeviceS device) override;

    void closeDevice(MidiDeviceS device) override;

    void sendNoteOn(MidiDeviceS device, uint32_t channel, uint32_t note, uint32_t velocity) const override;

    void sendNoteOff(MidiDeviceS device, uint32_t channel, uint32_t note, uint32_t velocity) const override;

private:
    using Message = std::vector<unsigned char>;

    void sendMessage(MidiDeviceS device, const Message & message) const;

    std::map<uint32_t, std::unique_ptr<RtMidiOut>> m_midiPorts;
};

} // namespace cacophony

#endif // MIDI_SERVICE_RT_MIDI_HPP
