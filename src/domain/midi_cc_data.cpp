#include "midi_cc_data.hpp"

namespace noteahead {

MidiCcData::MidiCcData(size_t track, size_t column, uint8_t controller, uint8_t value)
  : EventData { track, column }
  , m_controller { controller }
  , m_value { value }
{
}

uint8_t MidiCcData::controller() const
{
    return m_controller;
}

uint8_t MidiCcData::value() const
{
    return m_value;
}

} // namespace noteahead
