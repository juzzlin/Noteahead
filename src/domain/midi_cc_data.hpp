#ifndef MIDI_CC_DATA_HPP
#define MIDI_CC_DATA_HPP

#include "event_data.hpp"

#include <cstdint>

namespace noteahead {

class MidiCcData : public EventData
{
public:
    MidiCcData(size_t track, size_t column, uint8_t controller, uint8_t value);

    uint8_t controller() const;
    uint8_t value() const;

private:
    uint8_t m_controller = 0;
    uint8_t m_value = 0;
};

} // namespace noteahead

#endif // MIDI_CC_DATA_HPP
