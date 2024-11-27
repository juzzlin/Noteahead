#include "midi_service.hpp"

namespace cacophony {

MidiService::MidiService() = default;

void MidiService::listDevices() const
{
}

bool MidiService::openDevice(uint32_t)
{
    return false;
}

void MidiService::sendNoteOn(uint32_t, uint32_t, uint32_t) const
{
}

void MidiService::sendNoteOff(uint32_t, uint32_t, uint32_t) const
{
}

MidiService::~MidiService() = default;

} // namespace cacophony
