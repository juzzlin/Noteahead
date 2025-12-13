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

#ifndef MIDI_IN_ALSA_HPP
#define MIDI_IN_ALSA_HPP

#include "infra/midi/midi_backend_in.hpp"

#include <alsa/asoundlib.h>

#include <map>
#include <mutex>
#include <optional>
#include <set>
#include <thread>

namespace noteahead {

class MidiInAlsa : public MidiBackendIn
{
public:
    MidiInAlsa();
    ~MidiInAlsa() override;

    // Implementation of MidiBackendIn virtual methods
    void updatePorts() override;
    void openPort(MidiPortCR port) override;
    void setCallbackForPort(MidiPortCR port, MidiBackendIn::InputCallback callback) override;
    void clearCallbacks() override;
    bool isPortOpen(MidiPortCR port) const;
    void closePort(MidiPortCR port) override;
    std::string portName(MidiPortCR port) const;
    std::string midiApiName() const override { return "ALSA"; }

private:
    void eventLoop();
    std::optional<std::pair<int, int>> parsePortId(const std::string & portId) const;

    snd_seq_t * m_seqHandle = nullptr;
    std::set<std::string> m_openPorts; // Using port_id strings

    std::map<std::string, MidiBackendIn::InputCallback> m_callbacks; // Store callbacks per port_id
    std::thread m_thread;
    mutable std::mutex m_mutex;
    bool m_running;
};

} // namespace noteahead

#endif // MIDI_IN_ALSA_HPP
