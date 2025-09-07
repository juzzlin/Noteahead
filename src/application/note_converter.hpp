#ifndef NOTE_CONVERTER_HPP
#define NOTE_CONVERTER_HPP

#include <cstdint>
#include <optional>
#include <string>
#include <utility>

namespace noteahead::NoteConverter {

//! Converts a MIDI note to a string in "key-octave" format, e.g. "C-3".
std::string midiToString(uint8_t midiNote);

//! Converts a string in "key-octave" format to a MIDI note [0..127].
uint8_t stringToMidi(const std::string & noteString);

//! Converts a MIDI note to key [1..12 ] and octave [0..10] pair.
std::pair<uint8_t, uint8_t> midiToKeyAndOctave(uint8_t midiNote);

using MidiNoteNameAndCode = std::pair<std::string, uint8_t>;
using MidiNoteNameAndCodeOpt = std::optional<MidiNoteNameAndCode>;
//! Converts key [1..12] and octave to a MIDI note [0..127] and "key-octave" code pair optional.
//! \returns a nullopt on invalid key.
MidiNoteNameAndCodeOpt keyAndOctaveToMidiNote(uint8_t key, uint8_t octave, int transpose = 0);

} // namespace noteahead::NoteConverter

#endif // NOTE_CONVERTER_HPP
