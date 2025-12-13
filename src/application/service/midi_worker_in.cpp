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

#include "midi_worker_in.hpp"

#include "../../contrib/SimpleLogger/src/simple_logger.hpp"
#include "../../domain/midi_address.hpp"
#include "../../domain/midi_note_data.hpp"

#ifdef __linux__
#include "../../infra/midi/implementation/alsa/midi_in_alsa.hpp"
#else
#include "../../infra/midi/implementation/librtmidi/midi_in_rt_midi.hpp"
#endif

#include <iomanip>

using namespace std::chrono_literals;

namespace noteahead {

static const auto TAG = "MidiInWorker";

MidiWorkerIn::MidiWorkerIn(QObject * parent)
#ifdef __linux__
  : MidiWorker { std::make_unique<MidiInAlsa>(), "IN", parent }
#else
  : MidiWorker { std::make_unique<MidiInRtMidi>(), "IN", parent }
#endif
  , m_midiWorkerIn { std::dynamic_pointer_cast<MidiBackendIn>(midiBackend()) }
{
    juzzlin::L(TAG).info() << "Midi API name: " << m_midiWorkerIn->midiApiName();
}

void MidiWorkerIn::handlePortsChanged()
{
    setControllerPort(m_controllerPort);
}

MidiAddress MidiWorkerIn::currentAddress(uint8_t channel) const
{
    return { m_controllerPort, channel };
}

void MidiWorkerIn::setControllerPort(QString portName)
{
    if (!portName.isEmpty()) {
        m_controllerPort = portName;
        try {
            if (const auto port = midiBackend()->portByName(portName.toStdString()); port) {
                juzzlin::L(TAG).info() << "Opening controller input port: " << portName.toStdString();
                m_midiWorkerIn->openPort(*port);
                m_midiWorkerIn->clearCallbacks();
                m_midiWorkerIn->setCallbackForPort(*port,
                                                   [this](double deltaTime, MessageCR message) {
                                                       handleIncomingMessage(deltaTime, message);
                                                   });
            } else {
                juzzlin::L(TAG).warning() << "No port found for port name: " << portName.toStdString();
            }
        } catch (std::runtime_error & e) {
            juzzlin::L(TAG).error() << e.what();
        }
    }
}

void MidiWorkerIn::handleIncomingMessage(double deltaTime, MessageCR message)
{
    if (!message.empty()) {
        logMidiMessage(deltaTime, message);

        const quint8 statusByte = message.at(0);

        // Handle real-time messages directly
        switch (statusByte) {
        case 0xFA: // Start
            emit startReceived();
            return;
        case 0xFC: // Stop
            emit stopReceived();
            return;
        case 0xFB: // Continue (optional)
            emit continueReceived();
            return;
        default:
            break;
        }

        const quint8 status = statusByte & 0xF0;
        const quint8 channel = statusByte & 0x0F;
        switch (status) {
        case 0x80:
            handleNoteOff(channel, message);
            break;
        case 0x90:
            handleNoteOn(channel, message);
            break;
        case 0xA0:
            handlePolyAftertouch(channel, message);
            break;
        case 0xB0:
            handleControlChange(channel, message);
            break;
        case 0xC0:
            handleProgramChange(channel, message);
            break;
        case 0xD0:
            handleChannelAftertouch(channel, message);
            break;
        case 0xE0:
            handlePitchBend(channel, message);
            break;
        case 0xF0:
            handleSysEx(message);
            break;
        default:
            break;
        }
    }
}

void MidiWorkerIn::logMidiMessage(double deltaTime, MessageCR message)
{
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (auto byte : message) {
        oss << "0x" << std::setw(2) << static_cast<int>(byte) << " ";
    }
    juzzlin::L(TAG).debug() << "Received MIDI message (" << deltaTime << "s): " << oss.str();
    emit dataReceived(QString::fromStdString(oss.str()));
}

void MidiWorkerIn::handleNoteOff(quint8 channel, MessageCR message)
{
    if (message.size() >= 3) {
        emit noteOffReceived(currentAddress(channel), { message.at(1), 0 });
    }
}

void MidiWorkerIn::handleNoteOn(quint8 channel, MessageCR message)
{
    if (message.size() >= 3) {
        const quint8 note = message.at(1);
        if (const quint8 velocity = message.at(2); velocity > 0) {
            emit noteOnReceived(currentAddress(channel), { note, velocity });
        } else {
            emit noteOffReceived(currentAddress(channel), { note, 0 });
        }
    }
}

void MidiWorkerIn::handlePolyAftertouch(quint8 channel, MessageCR message)
{
    if (message.size() >= 3) {
        emit polyAftertouchReceived(currentAddress(channel), message.at(1), message.at(2));
    }
}

void MidiWorkerIn::handleControlChange(quint8 channel, MessageCR message)
{
    if (message.size() < 3) {
        return;
    }

    const auto controller = message.at(1);
    const auto value = message.at(2);
    emit controlChangeReceived(currentAddress(channel), controller, value);

    // Check if this CC is mapped to a transport action
    if (value > 0) { // optional: ignore value==0
        if (const auto it = m_transportMappings.find(controller); it != m_transportMappings.end() && it->second) {
            it->second(); // Call the mapped action
        }
    }

    switch (controller) {
    case 6:
    case 38:
        if (const auto rpnState = m_rpnState[channel]; rpnState) {
            emit rpnReceived(currentAddress(channel), rpnState->first, rpnState->second, controller == 6 ? static_cast<quint8>(value << 7) : value);
        } else if (const auto nrpnState = m_nrpnState[channel]; nrpnState) {
            emit nrpnReceived(currentAddress(channel), nrpnState->first, nrpnState->second, controller == 6 ? static_cast<quint8>(value << 7) : value);
        }
        break;
    case 98:
    case 99:
        if (!m_nrpnState[channel]) {
            m_nrpnState[channel] = std::pair<quint8, quint8> { 0, 0 };
        }
        if (controller == 98) {
            m_nrpnState[channel]->second = value;
        } else {
            m_nrpnState[channel]->first = value;
        }
        m_rpnState[channel].reset();
        break;
    case 100:
    case 101:
        if (!m_rpnState[channel]) {
            m_rpnState[channel] = std::pair<quint8, quint8> { 0, 0 };
        }
        if (controller == 100) {
            m_rpnState[channel]->second = value;
        } else {
            m_rpnState[channel]->first = value;
        }
        m_nrpnState[channel].reset();
        break;
    case 120:
    case 121:
    case 122:
    case 123:
        m_rpnState[channel].reset();
        m_nrpnState[channel].reset();
        break;
    default:
        break;
    }
}

void MidiWorkerIn::handleProgramChange(quint8 channel, MessageCR message)
{
    if (message.size() >= 2) {
        emit programChangeReceived(currentAddress(channel), message.at(1));
    }
}

void MidiWorkerIn::handleChannelAftertouch(quint8 channel, MessageCR message)
{
    if (message.size() >= 2) {
        emit aftertouchReceived(currentAddress(channel), message.at(1));
    }
}

void MidiWorkerIn::handlePitchBend(quint8 channel, MessageCR message)
{
    if (message.size() >= 3) {
        emit pitchBendReceived(currentAddress(channel), static_cast<quint16>(message.at(2) << 7) | message.at(1));
    }
}

void MidiWorkerIn::handleSysEx(MessageCR message)
{
    if (message.at(0) == 0xF0 && message.back() == 0xF7 && message.size() >= 2) {
        emit sysExReceived(QByteArray { reinterpret_cast<const char *>(message.data()), static_cast<int>(message.size()) });
    }
}

} // namespace noteahead
