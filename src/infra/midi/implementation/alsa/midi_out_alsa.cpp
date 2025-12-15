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

#include "midi_out_alsa.hpp"

#include "../../../contrib/SimpleLogger/src/simple_logger.hpp"
#include "../../../infra/midi/midi_port.hpp"
#include "../../midi_cc_mapping.hpp"

#include <alsa/seq.h>
#include <alsa/seq_event.h>

#include <cstring>
#include <iomanip>
#include <iostream>

namespace noteahead {

static const auto TAG = "MidiOutAlsa";

MidiOutAlsa::MidiOutAlsa()
    : MidiBackendOut { { "Noteahead Virtual MIDI Out" } }
    , m_seqHandle { nullptr }
{
    if (snd_seq_open(&m_seqHandle, "default", SND_SEQ_OPEN_OUTPUT, SND_SEQ_NONBLOCK) < 0) {
        throw std::runtime_error { "Error opening ALSA sequencer for output." };
    }
    snd_seq_set_client_name(m_seqHandle, "Noteahead");

    for (const auto & virtualPort : virtualPorts()) {
        openVirtualPort(virtualPort);
    }

    // Create a separate port for physical output instead of the default 0
    // so as not to mess with virtual ports.
    openPhysicalPort();
}

std::string MidiOutAlsa::midiApiName() const
{
    return "ALSA";
}

void MidiOutAlsa::openPhysicalPort()
{
    m_physicalOutPortId = snd_seq_create_simple_port(m_seqHandle, "Noteahead Physical Output Router",
                                                     SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ | SND_SEQ_PORT_CAP_NO_EXPORT,
                                                     SND_SEQ_PORT_TYPE_MIDI_GENERIC | SND_SEQ_PORT_TYPE_APPLICATION);
    if (m_physicalOutPortId < 0) {
        throw std::runtime_error { "Error opening output port for physical connections." };
    }
}

void MidiOutAlsa::openVirtualPort(const std::string & portName)
{
    std::lock_guard<std::mutex> lock { m_mutex };
    if (m_virtualPorts.contains(portName)) {
        juzzlin::L(TAG).debug() << "Virtual port " << portName << " is already open.";
        return;
    }

    const int portId = snd_seq_create_simple_port(m_seqHandle, portName.c_str(),
                                                  SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ,
                                                  SND_SEQ_PORT_TYPE_MIDI_GENERIC | SND_SEQ_PORT_TYPE_APPLICATION);

    if (portId < 0) {
        throw std::runtime_error { "Error creating ALSA virtual output port." };
    }

    m_virtualPorts[portName] = portId;
}

std::optional<std::pair<int, int>> MidiOutAlsa::parsePortId(const std::string & portId) const
{
    if (const std::size_t colonPos = portId.find(':'); colonPos == std::string::npos) {
        return std::nullopt;
    } else {
        try {
            const int client = std::stoi(portId.substr(0, colonPos));
            const int port = std::stoi(portId.substr(colonPos + 1));
            return std::make_pair(client, port);
        } catch (const std::invalid_argument &) {
            return std::nullopt;
        } catch (const std::out_of_range &) {
            return std::nullopt;
        }
    }
}

void MidiOutAlsa::updatePorts()
{
    snd_seq_client_info_t * clientInfo = nullptr;
    snd_seq_client_info_alloca(&clientInfo);
    snd_seq_port_info_t * portInfo = nullptr;
    snd_seq_port_info_alloca(&portInfo);

    int currentPortIndex = 0; // Assign a unique index for MidiPort

    PortList portsList;
    snd_seq_client_info_set_client(clientInfo, -1);
    while (snd_seq_query_next_client(m_seqHandle, clientInfo) >= 0) {
        const int clientId = snd_seq_client_info_get_client(clientInfo);
        snd_seq_port_info_set_client(portInfo, clientId);
        snd_seq_port_info_set_port(portInfo, -1);
        while (snd_seq_query_next_port(m_seqHandle, portInfo) >= 0) {
            const unsigned int caps = snd_seq_port_info_get_capability(portInfo);
            const unsigned int type = snd_seq_port_info_get_type(portInfo);
            const int ourClientId = snd_seq_client_id(m_seqHandle);
            if (clientId != ourClientId) {
                if ((caps & SND_SEQ_PORT_CAP_WRITE) && (caps & SND_SEQ_PORT_CAP_SUBS_WRITE) && (type & SND_SEQ_PORT_TYPE_MIDI_GENERIC)) {
                    const auto displayName = std::string { snd_seq_client_info_get_name(clientInfo) } + std::string { ": " } + std::string { snd_seq_port_info_get_name(portInfo) };
                    const auto portId = std::to_string(snd_seq_port_info_get_client(portInfo)) + ":" + std::to_string(snd_seq_port_info_get_port(portInfo));
                    portsList.push_back(std::make_shared<MidiPort>(currentPortIndex++, displayName, portId));
                }
            }
        }
    }
    for (const auto & virtualPort : virtualPorts()) {
        portsList.push_back(std::make_shared<MidiPort>(currentPortIndex++, virtualPort, virtualPort));
    }
    setPorts(portsList); // Inherited from MidiBackend
}

void MidiOutAlsa::openPort(MidiPortCR port)
{
    if (isVirtualPort(port.name())) {
        // Virtual ports are opened at construction
        return;
    }

    if (isPortOpen(port)) {
        juzzlin::L(TAG).trace() << "Port " << port.id() << " is already open.";
        return;
    }

    if (const auto parsedId = parsePortId(port.id()); parsedId.has_value()) {
        juzzlin::L(TAG).debug() << "Opening output port " << parsedId->first << ":" << parsedId->second;
        snd_seq_port_subscribe_t * subscription = nullptr;
        snd_seq_port_subscribe_alloca(&subscription);
        std::memset(subscription, 0, snd_seq_port_subscribe_sizeof());

        // Source is our client's default port 0
        snd_seq_addr_t senderAddr {};
        senderAddr.client = static_cast<uint8_t>(snd_seq_client_id(m_seqHandle));
        senderAddr.port = static_cast<uint8_t>(m_physicalOutPortId);
        snd_seq_port_subscribe_set_sender(subscription, &senderAddr);

        // Destination is the target port
        snd_seq_addr_t destAddr {};
        destAddr.client = static_cast<uint8_t>(parsedId->first);
        destAddr.port = static_cast<uint8_t>(parsedId->second);
        snd_seq_port_subscribe_set_dest(subscription, &destAddr);
        if (snd_seq_subscribe_port(m_seqHandle, subscription) < 0) {
            throw std::runtime_error { "Error subscribing to ALSA output port." };
        }

        std::lock_guard<std::mutex> lock { m_mutex };
        m_openPorts.insert(port.id());
    } else {
        juzzlin::L(TAG).error() << "Invalid port id format: " << std::quoted(port.id());
    }
}

bool MidiOutAlsa::isPortOpen(MidiPortCR port) const
{
    std::lock_guard<std::mutex> lock { m_mutex };
    return m_openPorts.contains(port.id());
}

void MidiOutAlsa::closePort(MidiPortCR port)
{
    if (isVirtualPort(port.name())) {
        // Virtual ports are closed by the destructor
        return;
    }

    if (const auto parsedId = parsePortId(port.id()); parsedId.has_value()) {
        snd_seq_port_subscribe_t * subscription = nullptr;
        snd_seq_port_subscribe_alloca(&subscription);
        std::memset(subscription, 0, snd_seq_port_subscribe_sizeof());

        // Source is our client's output port
        snd_seq_addr_t senderAddr {};
        senderAddr.client = static_cast<uint8_t>(snd_seq_client_id(m_seqHandle));
        senderAddr.port = static_cast<uint8_t>(m_physicalOutPortId);
        snd_seq_port_subscribe_set_sender(subscription, &senderAddr);

        // Destination is the target port
        snd_seq_addr_t destAddr {};
        destAddr.client = static_cast<uint8_t>(parsedId->first);
        destAddr.port = static_cast<uint8_t>(parsedId->second);
        snd_seq_port_subscribe_set_dest(subscription, &destAddr);

        snd_seq_unsubscribe_port(m_seqHandle, subscription);
        std::lock_guard<std::mutex> lock { m_mutex };
        m_openPorts.erase(port.id());
    } else {
        juzzlin::L(TAG).error() << "Invalid port id format: " << std::quoted(port.id());
    }
}

std::string MidiOutAlsa::portName(MidiPortCR port) const
{
    return port.name();
}

void MidiOutAlsa::sendNoteOn(MidiPortCR port, uint8_t channel, uint8_t note, uint8_t velocity) const
{
    snd_seq_event_t event {};
    snd_seq_ev_clear(&event);
    snd_seq_ev_set_noteon(&event, channel, note, velocity);
    sendEvent(port, event);

    MidiBackendOut::sendNoteOn(port, channel, note, velocity);
}

void MidiOutAlsa::sendNoteOff(MidiPortCR port, uint8_t channel, uint8_t note) const
{
    snd_seq_event_t event {};
    snd_seq_ev_clear(&event);
    snd_seq_ev_set_noteoff(&event, channel, note, 0); // Velocity can be 0 for Note Off
    sendEvent(port, event);

    MidiBackendOut::sendNoteOff(port, channel, note);
}

void MidiOutAlsa::sendCcData(MidiPortCR port, uint8_t channel, uint8_t controller, uint8_t value) const
{
    snd_seq_event_t event {};
    snd_seq_ev_clear(&event);
    snd_seq_ev_set_controller(&event, channel, controller, value);
    sendEvent(port, event);
}

void MidiOutAlsa::sendPatchChange(MidiPortCR port, uint8_t channel, uint8_t patch) const
{
    snd_seq_event_t event {};
    snd_seq_ev_clear(&event);
    snd_seq_ev_set_pgmchange(&event, channel, patch);
    sendEvent(port, event);
}

void MidiOutAlsa::sendMessage(MidiPortCR port, const Message& message) const
{
    snd_seq_event_t event {};
    snd_seq_ev_clear(&event);
    snd_seq_ev_set_sysex(&event, message.size(), const_cast<unsigned char *>(message.data()));
    sendEvent(port, event);
}

void MidiOutAlsa::stopAllNotes(MidiPortCR port, uint8_t channel) const
{
    // Send "All Notes Off" CC message
    sendCcData(port, channel, static_cast<uint8_t>(MidiCcMapping::Controller::AllNotesOff), 0);

    // All ports won't obey CC #123: Manually stop all notes. Stop only the notes that are actually playing
    // as there are some ports that go crazy if non-playing notes are stopped.
    for (auto && note : notesOn(port, channel)) {
        juzzlin::L(TAG).info() << "Stopping note " << static_cast<int>(note) << " on channel " << static_cast<int>(channel) << " of port " << port.name();
        sendNoteOff(port, channel, note);
    }
}

void MidiOutAlsa::sendBankChange(MidiPortCR port, uint8_t channel, uint8_t msb, uint8_t lsb) const
{
    // Bank change is typically sent as two CC messages
    sendCcData(port, channel, static_cast<uint8_t>(MidiCcMapping::Controller::BankSelectMSB), msb); // Bank Select MSB
    sendCcData(port, channel, static_cast<uint8_t>(MidiCcMapping::Controller::BankSelectLSB), lsb); // Bank Select LSB
}

void MidiOutAlsa::sendPitchBendData(MidiPortCR port, uint8_t channel, uint8_t msb, uint8_t lsb) const
{
    snd_seq_event_t event {};
    snd_seq_ev_clear(&event);
    const int value = (static_cast<int>(msb) << 7) | lsb;
    snd_seq_ev_set_pitchbend(&event, channel, value);
    sendEvent(port, event);
}

void MidiOutAlsa::sendClockPulse(MidiPortCR port) const
{
    snd_seq_event_t event {};
    snd_seq_ev_clear(&event);
    event.type = SND_SEQ_EVENT_CLOCK;
    sendEvent(port, event);
}

void MidiOutAlsa::sendStart(MidiPortCR port) const
{
    snd_seq_event_t event {};
    snd_seq_ev_clear(&event);
    event.type = SND_SEQ_EVENT_START;
    sendEvent(port, event);
}

void MidiOutAlsa::sendStop(MidiPortCR port) const
{
    snd_seq_event_t event {};
    snd_seq_ev_clear(&event);
    event.type = SND_SEQ_EVENT_STOP;
    sendEvent(port, event);
}

void MidiOutAlsa::sendEvent(MidiPortCR port, snd_seq_event_t & event) const
{
    if (isVirtualPort(port.name())) {
        sendEventToVirtualPort(port, event);
    } else {
        sendEventToPhysicalPort(port, event);
    }
}

void MidiOutAlsa::sendEventToPhysicalPort(MidiPortCR port, snd_seq_event_t & event) const
{
    if (const auto parsedId = parsePortId(port.id()); parsedId.has_value()) {
        event.source.client = static_cast<uint8_t>(snd_seq_client_id(m_seqHandle));
        event.source.port = static_cast<uint8_t>(m_physicalOutPortId);
        event.dest.client = static_cast<uint8_t>(parsedId->first);
        event.dest.port = static_cast<uint8_t>(parsedId->second);
        snd_seq_ev_set_direct(&event);
        snd_seq_event_output_direct(m_seqHandle, &event);
    } else {
        juzzlin::L(TAG).error() << "Invalid port ID format for sendEvent: " << std::quoted(port.id());
    }
}

void MidiOutAlsa::sendEventToVirtualPort(MidiPortCR port, snd_seq_event_t & event) const
{
    if (m_virtualPorts.contains(port.name())) {
        event.source.client = static_cast<uint8_t>(snd_seq_client_id(m_seqHandle));
        event.source.port = static_cast<uint8_t>(m_virtualPorts.at(port.name()));
        snd_seq_ev_set_subs(&event);
        snd_seq_ev_set_direct(&event);
        snd_seq_event_output(m_seqHandle, &event);
        snd_seq_drain_output(m_seqHandle);
    } else {
        juzzlin::L(TAG).error() << "Virtual port not found: " << std::quoted(port.name());
    }
}

MidiOutAlsa::~MidiOutAlsa()
{
    if (m_seqHandle) {
        snd_seq_close(m_seqHandle);
    }
    std::lock_guard<std::mutex> lock { m_mutex };
    m_openPorts.clear();
    m_virtualPorts.clear();
}

} // namespace noteahead
