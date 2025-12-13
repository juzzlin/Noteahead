// This file is part of Noteahead.
// Copyright (C) 2025 Jussi Lind <jussi.lind@iki.fi>
//
// Noteahead is free software: you can redistribute it and/or modify
// it under a modified version of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// Noteahead is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Noteahead. If not, see <http://www.gnu.org/licenses/>.

#include "midi_in_alsa.hpp"

#include "../../../common/utils.hpp"
#include "../../../contrib/SimpleLogger/src/simple_logger.hpp"
#include "../../../infra/midi/midi_port.hpp"

#include <alsa/seq.h>
#include <alsa/seq_event.h>

#include <chrono>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <optional>
#include <vector>

namespace noteahead {

static const auto TAG = "MidiInAlsa";

MidiInAlsa::MidiInAlsa()
  : m_seqHandle { nullptr }
  , m_running { true }
{
    if (snd_seq_open(&m_seqHandle, "default", SND_SEQ_OPEN_INPUT, 0) < 0) {
        throw std::runtime_error { "Error opening ALSA sequencer for input." };
    }

    snd_seq_set_client_name(m_seqHandle, "Noteahead");

    // Create a port
    snd_seq_create_simple_port(m_seqHandle, "Input",
        SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE,
        SND_SEQ_PORT_TYPE_MIDI_GENERIC);

    m_thread = std::thread(&MidiInAlsa::eventLoop, this);
}

MidiInAlsa::~MidiInAlsa()
{
    m_running = false;
    if (m_thread.joinable()) {
        m_thread.join();
    }
    if (m_seqHandle) {
        snd_seq_close(m_seqHandle);
    }
    std::lock_guard<std::mutex> lock { m_mutex };
    m_openPorts.clear();
}

void MidiInAlsa::updatePorts()
{
    snd_seq_client_info_t * clientInfo = nullptr;
    snd_seq_client_info_alloca(&clientInfo);
    snd_seq_port_info_t * portInfo = nullptr;
    snd_seq_port_info_alloca(&portInfo);

    int currentPortIndex = 0; // Assign a unique index for MidiPort

    PortList portsList;
    snd_seq_client_info_set_client(clientInfo, -1);
    while (snd_seq_query_next_client(m_seqHandle, clientInfo) >= 0) {
        int clientId = snd_seq_client_info_get_client(clientInfo);
        snd_seq_port_info_set_client(portInfo, clientId);
        snd_seq_port_info_set_port(portInfo, -1);
        while (snd_seq_query_next_port(m_seqHandle, portInfo) >= 0) {
            const unsigned int caps = snd_seq_port_info_get_capability(portInfo);
            if ((caps & SND_SEQ_PORT_CAP_READ) && (caps & SND_SEQ_PORT_CAP_SUBS_READ)) {
                const std::string displayName = std::string { snd_seq_client_info_get_name(clientInfo) } + std::string { ": " } + std::string { snd_seq_port_info_get_name(portInfo) };
                const std::string portId = std::to_string(snd_seq_port_info_get_client(portInfo)) + ":" + std::to_string(snd_seq_port_info_get_port(portInfo));

                portsList.push_back(std::make_shared<MidiPort>(currentPortIndex++, displayName, portId));
            }
        }
    }
    setPorts(portsList); // Inherited from MidiBackend
}

void MidiInAlsa::openPort(MidiPortCR port)
{
    if (isPortOpen(port)) {
        juzzlin::L(TAG).debug() << "Port " << port.id() << " is already open.";
        return;
    }

    if (const auto parsedId = parsePortId(port.id()); parsedId.has_value()) {
        snd_seq_port_subscribe_t * subscription = nullptr;
        snd_seq_port_subscribe_alloca(&subscription);
        std::memset(subscription, 0, snd_seq_port_subscribe_sizeof());

        snd_seq_addr_t senderAddr {};
        senderAddr.client = parsedId->first;
        senderAddr.port = parsedId->second;
        snd_seq_port_subscribe_set_sender(subscription, &senderAddr);

        snd_seq_addr_t destAddr {};
        destAddr.client = snd_seq_client_id(m_seqHandle);
        destAddr.port = 0; // Our client's input port is 0
        snd_seq_port_subscribe_set_dest(subscription, &destAddr);
        if (snd_seq_subscribe_port(m_seqHandle, subscription) < 0) {
            throw std::runtime_error { "Error subscribing to ALSA input port." };
        }

        std::lock_guard<std::mutex> lock { m_mutex };
        m_openPorts.insert(port.id());
    } else {
        juzzlin::L(TAG).error() << "Invalid port id format: " << std::quoted(port.id());
    }
}

void MidiInAlsa::setCallbackForPort(MidiPortCR port, MidiBackendIn::InputCallback callback)
{
    std::lock_guard<std::mutex> lock { m_mutex };
    m_callbacks[port.id()] = std::move(callback);
}

void MidiInAlsa::clearCallbacks()
{
    std::lock_guard<std::mutex> lock { m_mutex };
    m_callbacks.clear();
}

bool MidiInAlsa::isPortOpen(MidiPortCR port) const
{
    std::lock_guard<std::mutex> lock { m_mutex };
    return m_openPorts.contains(port.id());
}

std::optional<std::pair<int, int>> MidiInAlsa::parsePortId(const std::string & portId) const
{
    if (const std::size_t colonPos = portId.find(':'); colonPos == std::string::npos) {
        return std::nullopt;
    } else {
        try {
            const int client = std::stoi(portId.substr(0, colonPos));
            const int port = std::stoi(portId.substr(colonPos + 1));
            return std::make_pair(client, port);
        } catch (const std::invalid_argument & e) {
            return std::nullopt;
        } catch (const std::out_of_range & e) {
            return std::nullopt;
        }
    }
}

void MidiInAlsa::closePort(MidiPortCR port)
{
    if (const auto parsedId = parsePortId(port.id()); parsedId.has_value()) {
        snd_seq_port_subscribe_t * subscription = nullptr;
        snd_seq_port_subscribe_alloca(&subscription);
        std::memset(subscription, 0, snd_seq_port_subscribe_sizeof());

        snd_seq_addr_t senderAddr {};
        senderAddr.client = parsedId->first;
        senderAddr.port = parsedId->second;
        snd_seq_port_subscribe_set_sender(subscription, &senderAddr);

        snd_seq_addr_t destAddr {};
        destAddr.client = snd_seq_client_id(m_seqHandle);
        destAddr.port = 0; // Our client's input port is 0
        snd_seq_port_subscribe_set_dest(subscription, &destAddr);

        snd_seq_unsubscribe_port(m_seqHandle, subscription);

        std::lock_guard<std::mutex> lock { m_mutex };
        m_openPorts.erase(port.id());
    } else {
        juzzlin::L(TAG).error() << "Invalid port ID format: " << std::quoted(port.id());
    }
}

std::string MidiInAlsa::portName(MidiPortCR port) const
{
    if (const auto parsedId = parsePortId(port.id()); parsedId.has_value()) {
        // Get client info
        snd_seq_client_info_t * clientInfo = nullptr;
        snd_seq_client_info_alloca(&clientInfo);
        snd_seq_client_info_set_client(clientInfo, parsedId->first);
        if (snd_seq_get_client_info(m_seqHandle, clientInfo) < 0) {
            return "";
        }
        // Get port info
        snd_seq_port_info_t * pinfo = nullptr;
        snd_seq_port_info_alloca(&pinfo);
        snd_seq_port_info_set_client(pinfo, parsedId->first);
        snd_seq_port_info_set_port(pinfo, parsedId->second);
        if (snd_seq_get_any_port_info(m_seqHandle, parsedId->first, parsedId->second, pinfo) < 0) { // Using get_any_port_info
            return "";
        }
        return std::string { snd_seq_client_info_get_name(clientInfo = nullptr) } + std::string { ": " } + std::string { snd_seq_port_info_get_name(pinfo) };
    } else {
        juzzlin::L(TAG).error() << "Invalid port id format: " << std::quoted(port.id());
        return "";
    }
}

void MidiInAlsa::eventLoop()
{
    snd_seq_event_t * event = nullptr;
    while (m_running) {
        if (snd_seq_event_input(m_seqHandle, &event) < 0 || !event) {
            continue;
        }
        if (event->type == SND_SEQ_EVENT_NOTEON || event->type == SND_SEQ_EVENT_NOTEOFF || event->type == SND_SEQ_EVENT_CONTROLLER || event->type == SND_SEQ_EVENT_PITCHBEND || event->type == SND_SEQ_EVENT_PGMCHANGE || event->type == SND_SEQ_EVENT_SYSEX) {
            std::vector<unsigned char> message{};
            const auto now = std::chrono::high_resolution_clock::now();
            const auto ns = std::chrono::time_point_cast<std::chrono::nanoseconds>(now);
            const auto epoch = ns.time_since_epoch();
            const double timestamp = static_cast<double>(epoch.count()) / 1'000'000'000.0;

            const std::string portId = std::to_string(event->source.client) + ":" + std::to_string(event->source.port);

            std::lock_guard<std::mutex> lock { m_mutex };
            if (m_callbacks.count(portId)) {
                // Construct MIDI message from ALSA event
                if (event->type == SND_SEQ_EVENT_NOTEON || event->type == SND_SEQ_EVENT_NOTEOFF) {
                    message.push_back(static_cast<unsigned char>(event->type == SND_SEQ_EVENT_NOTEON ? 0x90 | event->data.note.channel : 0x80 | event->data.note.channel));
                    message.push_back(event->data.note.note);
                    message.push_back(event->data.note.velocity);
                } else if (event->type == SND_SEQ_EVENT_CONTROLLER) {
                    message.push_back(static_cast<unsigned char>(0xB0 | event->data.control.channel));
                    message.push_back(event->data.control.param);
                    message.push_back(event->data.control.value);
                } else if (event->type == SND_SEQ_EVENT_PITCHBEND) {
                    message.push_back(static_cast<unsigned char>(0xE0 | event->data.control.channel));
                    message.push_back(static_cast<unsigned char>(event->data.control.value & 0x7F));
                    message.push_back(static_cast<unsigned char>((event->data.control.value >> 7) & 0x7F));
                } else if (event->type == SND_SEQ_EVENT_PGMCHANGE) {
                    message.push_back(static_cast<unsigned char>(0xC0 | event->data.control.channel));
                    message.push_back(event->data.control.value);
                } else if (event->type == SND_SEQ_EVENT_SYSEX) {
                    for (unsigned int i = 0; i < event->data.ext.len; ++i) {
                        message.push_back(static_cast<unsigned char>(reinterpret_cast<uint8_t *>(event->data.ext.ptr)[i]));
                    }
                }
                m_callbacks[portId](timestamp, message);
            }
        }

        if (event) {
            snd_seq_free_event(event);
            event = nullptr;
        }
    }
}

} // namespace noteahead
