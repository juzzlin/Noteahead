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

#ifndef MIDI_IN_RT_MIDI_HPP
#define MIDI_IN_RT_MIDI_HPP

#include "../../midi_in.hpp"

#include <memory>
#include <unordered_map>

#include <rtmidi/RtMidi.h>

namespace noteahead {

class MidiInWorker;

//! MIDI input backend implementation on the RtMidi library.
class MidiInRtMidi : public MidiIn
{
public:
    void updatePorts() override;

    void openPort(MidiPortCR port) override;
    void closePort(MidiPortCR port) override;

    std::string midiApiName() const override;

    void clearCallbacks() override;
    void setCallbackForPort(const MidiPort & port, InputCallback callback) override;

    PortNameList availablePortNames() const override;

private:
    std::unordered_map<size_t, std::unique_ptr<RtMidiIn>> m_openedPorts;
    std::unordered_map<size_t, InputCallback> m_callbacks;

    using Message = std::vector<unsigned char>;
    using MessageP = Message *;
    static void staticCallback(double deltaTime, MessageP message, void * userData);
};

} // namespace noteahead

#endif // MIDI_IN_RT_MIDI_HPP
