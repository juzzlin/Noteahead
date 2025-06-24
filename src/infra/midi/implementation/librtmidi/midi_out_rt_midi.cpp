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

#include <memory>
#include <stdexcept>

namespace noteahead {

static const auto TAG = "MidiBackendRtMidi";

void MidiOutRtMidi::updateDevices()
{
    RtMidiOut tempMidiOut; // Temporary instance to list devices
    DeviceList devices = {};
    const size_t portCount = tempMidiOut.getPortCount();
    for (uint8_t i = 0; i < portCount; ++i) {
        devices.push_back(std::make_shared<MidiDevice>(i, tempMidiOut.getPortName(i)));
    }
    setDevices(devices);
    invalidatePortNameCache();
    m_ports.clear();
}

void MidiOutRtMidi::openDevice(MidiDeviceCR device)
{
    if (!m_ports.contains(device.portIndex())) {
        if (auto && midiOut = std::make_unique<RtMidiOut>(); device.portIndex() >= midiOut->getPortCount()) {
            throw std::runtime_error { "Invalid MIDI port index: " + std::to_string(device.portIndex()) };
        } else {
            midiOut->openPort(static_cast<uint8_t>(device.portIndex()));
            m_ports[device.portIndex()] = std::move(midiOut);
        }
    }
}

void MidiOutRtMidi::closeDevice(MidiDeviceCR device)
{
    if (auto && it = m_ports.find(device.portIndex()); it != m_ports.end()) {
        m_ports.erase(it);
    }
}

std::string MidiOutRtMidi::midiApiName() const
{
    return RtMidi::getApiDisplayName(RtMidiOut {}.getCurrentApi());
}

void MidiOutRtMidi::sendMessage(MidiDeviceCR device, const Message & message) const
{
    if (auto && it = m_ports.find(device.portIndex()); it == m_ports.end()) {
        throw std::runtime_error { "Device not opened." };
    } else {
        it->second->sendMessage(&message);
    }
}

void MidiOutRtMidi::sendCcData(MidiDeviceCR device, uint8_t channel, uint8_t controller, uint8_t value) const
{
    const Message message = { static_cast<unsigned char>(0xB0 | (channel & 0x0F)),
                              static_cast<unsigned char>(controller),
                              static_cast<unsigned char>(value) };
    sendMessage(device, message);
}

void MidiOutRtMidi::sendNoteOn(MidiDeviceCR device, uint8_t channel, uint8_t note, uint8_t velocity) const
{
    const Message message = { static_cast<unsigned char>(0x90 | (channel & 0x0F)),
                              static_cast<unsigned char>(note),
                              static_cast<unsigned char>(velocity) };

    sendMessage(device, message);

    MidiOut::sendNoteOn(device, channel, note, velocity);
}

void MidiOutRtMidi::sendNoteOff(MidiDeviceCR device, uint8_t channel, uint8_t note) const
{
    const Message message = { static_cast<unsigned char>(0x80 | (channel & 0x0F)),
                              static_cast<unsigned char>(note),
                              static_cast<unsigned char>(0) };

    sendMessage(device, message);

    MidiOut::sendNoteOff(device, channel, note);
}

void MidiOutRtMidi::sendPatchChange(MidiDeviceCR device, uint8_t channel, uint8_t patch) const
{
    const Message message = { static_cast<unsigned char>(0xC0 | (channel & 0x0F)),
                              static_cast<unsigned char>(patch) };

    sendMessage(device, message);
}

void MidiOutRtMidi::sendBankChange(MidiDeviceCR device, uint8_t channel, uint8_t msb, uint8_t lsb) const
{
    sendCcData(device, channel, static_cast<uint8_t>(MidiCcMapping::Controller::BankSelectMSB), msb);
    sendCcData(device, channel, static_cast<uint8_t>(MidiCcMapping::Controller::BankSelectLSB), lsb);
}

void MidiOutRtMidi::sendPitchBendData(MidiDeviceCR device, uint8_t channel, uint8_t msb, uint8_t lsb) const
{
    const Message message = {
        static_cast<unsigned char>(0xE0 | (channel & 0x0F)),
        static_cast<unsigned char>(lsb & 0x7F),
        static_cast<unsigned char>(msb & 0x7F)
    };

    sendMessage(device, message);
}

void MidiOutRtMidi::stopAllNotes(MidiDeviceCR device, uint8_t channel) const
{
    sendCcData(device, channel, static_cast<uint8_t>(MidiCcMapping::Controller::AllNotesOff), 0);

    // All devices won't obey CC #123: Manually stop all notes. Stop only the notes that are actually playing
    // as there are some devices that go crazy if non-playing notes are stopped.
    for (auto && note : notesOn(device, channel)) {
        juzzlin::L(TAG).info() << "Stopping note " << static_cast<int>(note) << " on channel " << static_cast<int>(channel) << " of device " << device.portName();
        sendNoteOff(device, channel, note);
    }
}

void MidiOutRtMidi::sendClockPulse(MidiDeviceCR device) const
{
    sendMessage(device, { 0xF8 });
}

void MidiOutRtMidi::sendStart(MidiDeviceCR device) const
{
    sendMessage(device, { 0xFA });
}

void MidiOutRtMidi::sendStop(MidiDeviceCR device) const
{
    sendMessage(device, { 0xFC });
}

} // namespace noteahead
