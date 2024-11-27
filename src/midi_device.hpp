#ifndef MIDI_DEVICE_HPP
#define MIDI_DEVICE_HPP

#include <cstdint>
#include <string>

namespace cacophony {

class MidiDevice
{
public:
    MidiDevice(uint32_t index, std::string portName);

    uint32_t index() const;

    std::string portName() const;

    std::string toString() const;

private:
    uint32_t mIndex;

    std::string mPortName;
};

} // namespace cacophony

#endif // MIDI_DEVICE_HPP
