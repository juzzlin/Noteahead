#ifndef NOTE_CONVERTER_HPP
#define NOTE_CONVERTER_HPP

#include <cstdint>
#include <string>

namespace noteahead::NoteConverter {

// Converts a MIDI note (uint8_t) to a string in "key-octave" format
std::string midiToString(uint8_t midiNote);

// Converts a string in "key-octave" format to a MIDI note (uint8_t)
uint8_t stringToMidi(const std::string & noteString);

} // namespace noteahead::NoteConverter

#endif // NOTE_CONVERTER_HPP
