#include "midi_service.hpp"

#include <chrono>
#include <iostream>
#include <thread>

namespace cacophony {

MidiService::MidiService()
  : midiOut { std::make_unique<RtMidiOut>() }
{
}

void MidiService::listDevices() const
{
    unsigned int nPorts = midiOut->getPortCount();
    if (nPorts == 0) {
        std::cout << "No MIDI devices found.\n";
        return;
    }

    std::cout << "Available MIDI devices:\n";
    for (unsigned int i = 0; i < nPorts; ++i) {
        std::string portName = midiOut->getPortName(i);
        std::cout << i << ": " << portName << "\n";
    }
}

bool MidiService::openDevice(unsigned int index)
{
    if (index >= midiOut->getPortCount()) {
        std::cerr << "Invalid device index.\n";
        return false;
    }
    midiOut->openPort(index);
    return true;
}

void MidiService::playMiddleC() const
{
    if (!midiOut->isPortOpen()) {
        std::cerr << "No MIDI device is open.\n";
        return;
    }

    std::vector<unsigned char> message = { 0x90, 60, 100 }; // Note On, note 60, velocity 100
    midiOut->sendMessage(&message);

    // Wait a bit to hear the note
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    message = { 0x80, 60, 100 }; // Note Off, note 60, velocity 100
    midiOut->sendMessage(&message);
}

} // namespace cacophony
