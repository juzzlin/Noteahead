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

#include "midi_out_rt_midi.hpp"

#include "../../../contrib/SimpleLogger/src/simple_logger.hpp"
#include "../../midi_cc_mapping.hpp"

#include <iomanip>
#include <memory>
#include <stdexcept>

namespace noteahead {

static const auto TAG = "MidiBackendRtMidi";

MidiOutRtMidi::MidiOutRtMidi()
  : MidiOut { { "Noteahead Virtual MIDI Out" } }
{
    for (auto && virtualPort : virtualPorts()) {
        openVirtualPort(virtualPort);
    }
}

void MidiOutRtMidi::updatePorts()
{
    RtMidiOut tempMidiOut; // Temporary instance to list ports
    PortList ports = {};
    const size_t portCount = tempMidiOut.getPortCount();
    for (uint8_t i = 0; i < portCount; ++i) {
        ports.push_back(std::make_shared<MidiPort>(i, tempMidiOut.getPortName(i)));
    }
    auto virtualIndex = portCount;
    for (auto && virtualPort : virtualPorts()) {
        ports.push_back(std::make_shared<MidiPort>(virtualIndex++, virtualPort));
    }
    setPorts(ports);
    invalidatePortNameCache();
    m_ports.clear();
}

Midi::PortNameList MidiOutRtMidi::availablePortNames() const
{
    PortNameList portNameList;
    RtMidiOut tempMidiOut; // Temporary instance to list ports
    const size_t portCount = tempMidiOut.getPortCount();
    for (uint8_t i = 0; i < portCount; i++) {
        portNameList.push_back(tempMidiOut.getPortName(i));
    }
    for (auto && virtualPort : virtualPorts()) {
        portNameList.push_back(virtualPort);
    }
    std::ranges::sort(portNameList);
    return portNameList;
}

void MidiOutRtMidi::openPort(MidiPortCR port)
{
    if (isVirtualPort(port.name())) {
        openVirtualPort(port.name());
    } else if (!m_ports.contains(port.index())) {
        if (auto && midiOut = std::make_unique<RtMidiOut>(); port.index() >= midiOut->getPortCount()) {
            throw std::runtime_error { "Invalid MIDI port index: " + std::to_string(port.index()) };
        } else {
            midiOut->openPort(static_cast<uint8_t>(port.index()));
            m_ports[port.index()] = std::move(midiOut);
        }
    }
}

void MidiOutRtMidi::openVirtualPort(const std::string & name)
{
    if (!m_virtualPorts.contains(name)) {
        juzzlin::L(TAG).info() << "Opening virtual port " << std::quoted(name);
        auto && midiOut = std::make_unique<RtMidiOut>();
        midiOut->openVirtualPort(name);
        m_virtualPorts[name] = std::move(midiOut);
    }
}

void MidiOutRtMidi::closePort(MidiPortCR port)
{
    if (auto && it = m_ports.find(port.index()); it != m_ports.end()) {
        m_ports.erase(it);
    }
}

std::string MidiOutRtMidi::midiApiName() const
{
    return RtMidi::getApiDisplayName(RtMidiOut {}.getCurrentApi());
}

void MidiOutRtMidi::sendMessage(MidiPortCR port, const Message & message) const
{
    if (isVirtualPort(port.name())) {
        if (auto && it = m_virtualPorts.find(port.name()); it == m_virtualPorts.end()) {
            throw std::runtime_error { "virtual port not opened." };
        } else {
            it->second->sendMessage(&message);
        }
    } else {
        if (auto && it = m_ports.find(port.index()); it == m_ports.end()) {
            throw std::runtime_error { "physical port not opened." };
        } else {
            it->second->sendMessage(&message);
        }
    }
}

void MidiOutRtMidi::sendCcData(MidiPortCR port, uint8_t channel, uint8_t controller, uint8_t value) const
{
    const Message message = { static_cast<unsigned char>(0xB0 | (channel & 0x0F)),
                              static_cast<unsigned char>(controller),
                              static_cast<unsigned char>(value) };
    sendMessage(port, message);
}

void MidiOutRtMidi::sendNoteOn(MidiPortCR port, uint8_t channel, uint8_t note, uint8_t velocity) const
{
#ifdef ENABLE_MIDI_DEBUG
    juzzlin::L(TAG).debug() << "Playing note " << static_cast<int>(note) << " with velocity " << static_cast<int>(velocity) << " on channel " << static_cast<int>(channel) << " of port " << port.portName();
#endif
    const Message message = { static_cast<unsigned char>(0x90 | (channel & 0x0F)),
                              static_cast<unsigned char>(note),
                              static_cast<unsigned char>(velocity) };

    sendMessage(port, message);

    MidiOut::sendNoteOn(port, channel, note, velocity);
}

void MidiOutRtMidi::sendNoteOff(MidiPortCR port, uint8_t channel, uint8_t note) const
{
#ifdef ENABLE_MIDI_DEBUG
    juzzlin::L(TAG).debug() << "Stopping note " << static_cast<int>(note) << " on channel " << static_cast<int>(channel) << " of port " << port.portName();
#endif
    const Message message = { static_cast<unsigned char>(0x80 | (channel & 0x0F)),
                              static_cast<unsigned char>(note),
                              static_cast<unsigned char>(0) };

    sendMessage(port, message);

    MidiOut::sendNoteOff(port, channel, note);
}

void MidiOutRtMidi::sendPatchChange(MidiPortCR port, uint8_t channel, uint8_t patch) const
{
    const Message message = { static_cast<unsigned char>(0xC0 | (channel & 0x0F)),
                              static_cast<unsigned char>(patch) };

    sendMessage(port, message);
}

void MidiOutRtMidi::sendBankChange(MidiPortCR port, uint8_t channel, uint8_t msb, uint8_t lsb) const
{
    sendCcData(port, channel, static_cast<uint8_t>(MidiCcMapping::Controller::BankSelectMSB), msb);
    sendCcData(port, channel, static_cast<uint8_t>(MidiCcMapping::Controller::BankSelectLSB), lsb);
}

void MidiOutRtMidi::sendPitchBendData(MidiPortCR port, uint8_t channel, uint8_t msb, uint8_t lsb) const
{
    const Message message = {
        static_cast<unsigned char>(0xE0 | (channel & 0x0F)),
        static_cast<unsigned char>(lsb & 0x7F),
        static_cast<unsigned char>(msb & 0x7F)
    };

    sendMessage(port, message);
}

void MidiOutRtMidi::stopAllNotes(MidiPortCR port, uint8_t channel) const
{
    sendCcData(port, channel, static_cast<uint8_t>(MidiCcMapping::Controller::AllNotesOff), 0);

    // All ports won't obey CC #123: Manually stop all notes. Stop only the notes that are actually playing
    // as there are some ports that go crazy if non-playing notes are stopped.
    for (auto && note : notesOn(port, channel)) {
        juzzlin::L(TAG).info() << "Stopping note " << static_cast<int>(note) << " on channel " << static_cast<int>(channel) << " of port " << port.name();
        sendNoteOff(port, channel, note);
    }
}

void MidiOutRtMidi::sendClockPulse(MidiPortCR port) const
{
    sendMessage(port, { 0xF8 });
}

void MidiOutRtMidi::sendStart(MidiPortCR port) const
{
    sendMessage(port, { 0xFA });
}

void MidiOutRtMidi::sendStop(MidiPortCR port) const
{
    sendMessage(port, { 0xFC });
}

MidiOutRtMidi::~MidiOutRtMidi()
{
    for (auto && port : m_ports) {
        port.second->closePort();
    }
    for (auto && port : m_virtualPorts) {
        port.second->closePort();
    }
}

} // namespace noteahead
