#include "midi_service.hpp"

#include <iostream>
#include <stdexcept>

namespace cacophony {

MidiService::MidiService()
  : midiOut { std::make_unique<RtMidiOut>() }
{
}

void MidiService::listDevices() const
{
    if (uint32_t portCount = midiOut->getPortCount(); !portCount) {
        throw std::runtime_error { "No MIDI devices found." };
    } else {
        std::cout << "Available MIDI devices:\n";
        for (uint32_t i = 0; i < portCount; ++i) {
            const std::string portName = midiOut->getPortName(i);
            std::cout << i << ": " << portName << "\n";
        }
    }
}

bool MidiService::openDevice(uint32_t index)
{
    if (index >= midiOut->getPortCount()) {
        throw std::runtime_error { "Invalid device index." };
    }
    midiOut->openPort(index);
    return true;
}

void MidiService::sendNoteOn(uint32_t channel, uint32_t note, uint32_t velocity) const
{
    if (!midiOut->isPortOpen()) {
        throw std::runtime_error { "No MIDI device is open." };
    }
    std::vector<unsigned char> message = { static_cast<unsigned char>(0x90 | (channel & 0x0F)),
                                           static_cast<unsigned char>(note),
                                           static_cast<unsigned char>(velocity) };
    midiOut->sendMessage(&message);
}

void MidiService::sendNoteOff(uint32_t channel, uint32_t note, uint32_t velocity) const
{
    if (!midiOut->isPortOpen()) {
        throw std::runtime_error { "No MIDI device is open." };
    }
    std::vector<unsigned char> message = { static_cast<unsigned char>(0x80 | (channel & 0x0F)),
                                           static_cast<unsigned char>(note),
                                           static_cast<unsigned char>(velocity) };
    midiOut->sendMessage(&message);
}

} // namespace cacophony
