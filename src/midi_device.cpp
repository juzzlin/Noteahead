#include "midi_device.hpp"

namespace cacophony {

MidiDevice::MidiDevice(uint32_t index, std::string portName)
  : mIndex { index }
  , mPortName { portName }
{
}

uint32_t MidiDevice::index() const
{
    return mIndex;
}

std::string MidiDevice::portName() const
{
    return mPortName;
}

std::string MidiDevice::toString() const
{
    return std::to_string(mIndex) + ": " + mPortName;
}

} // namespace cacophony
