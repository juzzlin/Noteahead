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

#include "midi_backend_rt_midi.hpp"

#include "midi_cc_mapping.hpp"

#include "../../contrib/SimpleLogger/src/simple_logger.hpp"

#include <memory>
#include <stdexcept>

namespace noteahead {

static const auto TAG = "MidiBackendRtMidi";

void MidiBackendRtMidi::updateAvailableDevices()
{
    RtMidiOut tempMidiOut; // Temporary instance to list devices
    MidiDeviceList devices = {};
    const size_t portCount = tempMidiOut.getPortCount();
    for (uint8_t i = 0; i < portCount; ++i) {
        devices.push_back(std::make_shared<MidiDevice>(i, tempMidiOut.getPortName(i)));
    }
    setDevices(devices);
    invalidatePortNameCache();
    m_midiPorts.clear();
}

void MidiBackendRtMidi::openDevice(MidiDeviceCR device)
{
    if (!m_midiPorts.contains(device.portIndex())) {
        if (auto && midiOut = std::make_unique<RtMidiOut>(); device.portIndex() >= midiOut->getPortCount()) {
            throw std::runtime_error { "Invalid MIDI port index: " + std::to_string(device.portIndex()) };
        } else {
            midiOut->openPort(static_cast<uint8_t>(device.portIndex()));
            m_midiPorts[device.portIndex()] = std::move(midiOut);
        }
    }
}

void MidiBackendRtMidi::closeDevice(MidiDeviceCR device)
{
    if (auto && it = m_midiPorts.find(device.portIndex()); it != m_midiPorts.end()) {
        m_midiPorts.erase(it);
    }
}

std::string MidiBackendRtMidi::midiApiName() const
{
    return RtMidi::getApiDisplayName(RtMidiOut {}.getCurrentApi());
}

void MidiBackendRtMidi::sendMessage(MidiDeviceCR device, const Message & message) const
{
    if (auto && it = m_midiPorts.find(device.portIndex()); it == m_midiPorts.end()) {
        throw std::runtime_error { "Device not opened." };
    } else {
        it->second->sendMessage(&message);
    }
}

void MidiBackendRtMidi::sendCcData(MidiDeviceCR device, uint8_t channel, uint8_t controller, uint8_t value) const
{
    const Message message = { static_cast<unsigned char>(0xB0 | (channel & 0x0F)),
                              static_cast<unsigned char>(controller),
                              static_cast<unsigned char>(value) };
    sendMessage(device, message);
}

void MidiBackendRtMidi::sendNoteOn(MidiDeviceCR device, uint8_t channel, uint8_t note, uint8_t velocity) const
{
    const Message message = { static_cast<unsigned char>(0x90 | (channel & 0x0F)),
                              static_cast<unsigned char>(note),
                              static_cast<unsigned char>(velocity) };

    sendMessage(device, message);

    MidiBackend::sendNoteOn(device, channel, note, velocity);
}

void MidiBackendRtMidi::sendNoteOff(MidiDeviceCR device, uint8_t channel, uint8_t note) const
{
    const Message message = { static_cast<unsigned char>(0x80 | (channel & 0x0F)),
                              static_cast<unsigned char>(note),
                              static_cast<unsigned char>(0) };

    sendMessage(device, message);

    MidiBackend::sendNoteOff(device, channel, note);
}

void MidiBackendRtMidi::sendPatchChange(MidiDeviceCR device, uint8_t channel, uint8_t patch) const
{
    const Message message = { static_cast<unsigned char>(0xC0 | (channel & 0x0F)),
                              static_cast<unsigned char>(patch) };

    sendMessage(device, message);
}

void MidiBackendRtMidi::sendBankChange(MidiDeviceCR device, uint8_t channel, uint8_t msb, uint8_t lsb) const
{
    sendCcData(device, channel, static_cast<uint8_t>(MidiCcMapping::Controller::BankSelectMSB), msb);
    sendCcData(device, channel, static_cast<uint8_t>(MidiCcMapping::Controller::BankSelectLSB), lsb);
}

void MidiBackendRtMidi::sendPitchBendData(MidiDeviceCR device, uint8_t channel, uint8_t msb, uint8_t lsb) const
{
    const Message message = {
        static_cast<unsigned char>(0xE0 | (channel & 0x0F)),
        static_cast<unsigned char>(lsb & 0x7F),
        static_cast<unsigned char>(msb & 0x7F)
    };

    sendMessage(device, message);
}

void MidiBackendRtMidi::stopAllNotes(MidiDeviceCR device, uint8_t channel) const
{
    sendCcData(device, channel, static_cast<uint8_t>(MidiCcMapping::Controller::AllNotesOff), 0);

    // All devices won't obey CC #123: Manually stop all notes. Stop only the notes that are actually playing
    // as there are some devices that go crazy if non-playing notes are stopped.
    for (auto && note : notesOn(device, channel)) {
        juzzlin::L(TAG).info() << "Stopping note " << static_cast<int>(note) << " on channel " << static_cast<int>(channel) << " of device " << device.portName();
        sendNoteOff(device, channel, note);
    }
}

void MidiBackendRtMidi::sendClockPulse(MidiDeviceCR device) const
{
    const Message message = { 0xF8 };
    sendMessage(device, message);
}

} // namespace noteahead
