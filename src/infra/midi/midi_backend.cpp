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

#include "midi_backend.hpp"

#include "../../common/utils.hpp"

#include <algorithm>

namespace noteahead {

MidiBackend::MidiBackend() = default;

MidiBackend::MidiDeviceList MidiBackend::listDevices() const
{
    return m_devices;
}

MidiDeviceS MidiBackend::deviceByPortIndex(size_t index) const
{
    if (const auto device = std::ranges::find_if(m_devices, [&index](auto & device) { return device->portIndex() == index; }); device != m_devices.end()) {
        return *device;
    } else {
        return {};
    }
}

void MidiBackend::invalidatePortNameCache()
{
    m_portNameToDeviceCache.clear();
}

MidiDeviceS MidiBackend::deviceByPortName(const std::string & name) const
{
    if (const auto device = m_portNameToDeviceCache.find(name); device != m_portNameToDeviceCache.end()) {
        return device->second;
    }

    const auto bestMatch = std::ranges::max_element(m_devices, {}, [&](const auto & device) { return Utils::portNameMatchScore(name, device->portName()); });
    if (bestMatch != m_devices.end() && Utils::portNameMatchScore(name, (*bestMatch)->portName()) > 0.75) {
        m_portNameToDeviceCache[name] = *bestMatch;
        return *bestMatch;
    }

    return {};
}

std::string MidiBackend::midiApiName() const
{
    return "";
}

void MidiBackend::updateAvailableDevices()
{
}

void MidiBackend::setDevices(MidiDeviceList devices)
{
    m_devices = devices;
}

void MidiBackend::openDevice(MidiDeviceCR)
{
}

void MidiBackend::closeDevice(MidiDeviceCR)
{
}

void MidiBackend::sendCcData(MidiDeviceCR, uint8_t, uint8_t, uint8_t) const
{
}

void MidiBackend::sendNoteOn(MidiDeviceCR device, uint8_t channel, uint8_t note, uint8_t) const
{
    m_notesOn[device.portIndex()].insert({ channel, note });
}

MidiBackend::NotesOnList MidiBackend::notesOn(MidiDeviceCR device, uint8_t channel) const
{
    MidiBackend::NotesOnList notesOnList;
    for (auto && channelAndNote : m_notesOn[device.portIndex()]) {
        if (channelAndNote.first == channel) {
            notesOnList.push_back(channelAndNote.second);
        }
    }
    return notesOnList;
}

void MidiBackend::sendNoteOff(MidiDeviceCR device, uint8_t channel, uint8_t note) const
{
    m_notesOn[device.portIndex()].erase({ channel, note });
}

void MidiBackend::sendPatchChange(MidiDeviceCR, uint8_t, uint8_t) const
{
}

void MidiBackend::sendBankChange(MidiDeviceCR, uint8_t, uint8_t, uint8_t) const
{
}

void MidiBackend::sendPitchBendData(MidiDeviceCR, uint8_t, uint8_t, uint8_t) const
{
}

void MidiBackend::stopAllNotes(MidiDeviceCR, uint8_t) const
{
}

void MidiBackend::sendClockPulse(MidiDeviceCR) const
{
}

void MidiBackend::sendStart(MidiDeviceCR) const
{
}

void MidiBackend::sendStop(MidiDeviceCR) const
{
}

MidiBackend::~MidiBackend() = default;

} // namespace noteahead
