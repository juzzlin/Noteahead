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

#include "midi_cc.hpp"

#include <memory>
#include <stdexcept>

namespace noteahead {

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
}

void MidiBackendRtMidi::openDevice(MidiDeviceW device)
{
    if (const auto lockedDevice = device.lock(); lockedDevice) {
        if (!m_midiPorts.contains(lockedDevice->portIndex())) {
            if (auto && midiOut = std::make_unique<RtMidiOut>(); lockedDevice->portIndex() >= midiOut->getPortCount()) {
                throw std::runtime_error { "Invalid MIDI port index: " + std::to_string(lockedDevice->portIndex()) };
            } else {
                midiOut->openPort(static_cast<uint8_t>(lockedDevice->portIndex()));
                m_midiPorts[lockedDevice->portIndex()] = std::move(midiOut);
            }
        }
    }
}

void MidiBackendRtMidi::closeDevice(MidiDeviceW device)
{
    if (auto && it = m_midiPorts.find(device.lock()->portIndex()); it != m_midiPorts.end()) {
        m_midiPorts.erase(it);
    }
}

std::string MidiBackendRtMidi::midiApiName() const
{
    return RtMidi::getApiDisplayName(RtMidiOut {}.getCurrentApi());
}

void MidiBackendRtMidi::sendMessage(const MidiDevice & device, const Message & message) const
{
    if (auto && it = m_midiPorts.find(device.portIndex()); it == m_midiPorts.end()) {
        throw std::runtime_error { "Device not opened." };
    } else {
        it->second->sendMessage(&message);
    }
}

void MidiBackendRtMidi::sendCC(MidiDeviceW device, uint8_t channel, uint8_t controller, uint8_t value) const
{
    const Message message = { static_cast<unsigned char>(0xB0 | (channel & 0x0F)),
                              static_cast<unsigned char>(controller),
                              static_cast<unsigned char>(value) };
    sendMessage(*device.lock(), message);
}

void MidiBackendRtMidi::sendNoteOn(MidiDeviceW device, uint8_t channel, uint8_t note, uint8_t velocity) const
{
    const Message message = { static_cast<unsigned char>(0x90 | (channel & 0x0F)),
                              static_cast<unsigned char>(note),
                              static_cast<unsigned char>(velocity) };

    sendMessage(*device.lock(), message);
}

void MidiBackendRtMidi::sendNoteOff(MidiDeviceW device, uint8_t channel, uint8_t note) const
{
    const Message message = { static_cast<unsigned char>(0x80 | (channel & 0x0F)),
                              static_cast<unsigned char>(note),
                              static_cast<unsigned char>(0) };

    sendMessage(*device.lock(), message);
}

void MidiBackendRtMidi::sendPatchChange(MidiDeviceW device, uint8_t channel, uint8_t patch) const
{
    const Message message = { static_cast<unsigned char>(0xC0 | (channel & 0x0F)),
                              static_cast<unsigned char>(patch) };

    sendMessage(*device.lock(), message);
}

void MidiBackendRtMidi::sendBankChange(MidiDeviceW device, uint8_t channel, uint8_t msb, uint8_t lsb) const
{
    sendCC(device, channel, static_cast<uint8_t>(MidiCc::Controller::BankSelectMSB), msb);
    sendCC(device, channel, static_cast<uint8_t>(MidiCc::Controller::BankSelectLSB), lsb);
}

void MidiBackendRtMidi::stopAllNotes(MidiDeviceW device, uint8_t channel) const
{
    sendCC(device, channel, static_cast<uint8_t>(MidiCc::Controller::AllNotesOff), 0);

    // All devices won't obey CC #123: Manually stop all notes
    for (uint8_t note = 0; note < 128; note++) {
        sendNoteOff(device, channel, note);
    }
}

void MidiBackendRtMidi::sendClock(MidiDeviceW device) const
{
    const Message message = { 0xF8 };
    sendMessage(*device.lock(), message);
}

} // namespace noteahead
