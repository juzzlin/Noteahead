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

#ifndef MIDI_OUT_ALSA_HPP
#define MIDI_OUT_ALSA_HPP

#include "infra/midi/midi_backend_out.hpp"

#include <alsa/asoundlib.h>
#include <map>
#include <mutex>
#include <optional>
#include <set>

namespace noteahead {

class MidiOutAlsa : public MidiBackendOut {
public:
    MidiOutAlsa();
    ~MidiOutAlsa() override;

    // Implementation of MidiBackendOut virtual methods
    void updatePorts() override;
    void openPort(MidiPortCR port) override;
    bool isPortOpen(MidiPortCR port) const;
    void closePort(MidiPortCR port) override;
    std::string portName(MidiPortCR port) const;
    std::string midiApiName() const override
    {
        return "ALSA";
    }

    void stopAllNotes(MidiPortCR port, uint8_t channel) const override;
    void sendCcData(MidiPortCR port, uint8_t channel, uint8_t controller, uint8_t value) const override;
    void sendNoteOn(MidiPortCR port, uint8_t channel, uint8_t note, uint8_t velocity) const override;
    void sendNoteOff(MidiPortCR port, uint8_t channel, uint8_t note) const override;
    void sendPatchChange(MidiPortCR port, uint8_t channel, uint8_t patch) const override;
    void sendBankChange(MidiPortCR port, uint8_t channel, uint8_t msb, uint8_t lsb) const override;
    void sendPitchBendData(MidiPortCR port, uint8_t channel, uint8_t msb, uint8_t lsb) const override;
    void sendClockPulse(MidiPortCR port) const override;
    void sendStart(MidiPortCR port) const override;
    void sendStop(MidiPortCR port) const override;

private:
    using Message = std::vector<unsigned char>;
    void openPhysicalPort();
    void openVirtualPort(const std::string & name);
    void sendMessage(MidiPortCR port, const Message & message) const;
    void sendEvent(MidiPortCR port, snd_seq_event_t& event) const;
    void sendEventToPhysicalPort(MidiPortCR port, snd_seq_event_t & event) const;
    void sendEventToVirtualPort(MidiPortCR port, snd_seq_event_t & event) const;
    std::optional<std::pair<int, int>> parsePortId(const std::string & portId) const;

    snd_seq_t * m_seqHandle = nullptr;
    int m_physicalOutPortId = 0;
    std::set<std::string> m_openPorts; // Using port_id strings
    std::map<std::string, int> m_virtualPorts; // port_name -> port_id
    mutable std::mutex m_mutex;
};

} // namespace noteahead

#endif // MIDI_OUT_ALSA_HPP
