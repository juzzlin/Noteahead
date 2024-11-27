#ifndef MIDI_SERVICE_HPP
#define MIDI_SERVICE_HPP

#include "midi_device.hpp"

#include <cstdint>
#include <vector>

namespace cacophony {

//! Base class for MIDI service implementations.
class MidiService
{
public:
    MidiService();

    virtual ~MidiService();

    using MidiDeviceList = std::vector<MidiDevice>;

    virtual MidiDeviceList listDevices() const;

    virtual bool openDevice(uint32_t index);

    virtual void sendNoteOn(uint32_t channel, uint32_t note, uint32_t velocity) const;

    virtual void sendNoteOff(uint32_t channel, uint32_t note, uint32_t velocity) const;
};

} // namespace cacophony

#endif // MIDI_SERVICE_HPP
