#include "note_converter.hpp"

#include "../contrib/SimpleLogger/src/simple_logger.hpp"

#include <array>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace noteahead::NoteConverter {

static const auto TAG = "NoteConverter";

std::string midiToString(uint8_t midiNote)
{
    // clang-format off
    static const std::array<std::string, 128> midiToStringTable = { {
        "C-0", "C#0", "D-0", "D#0", "E-0", "F-0", "F#0", "G-0", "G#0", "A-0", "A#0", "B-0",
        "C-1", "C#1", "D-1", "D#1", "E-1", "F-1", "F#1", "G-1", "G#1", "A-1", "A#1", "B-1",
        "C-2", "C#2", "D-2", "D#2", "E-2", "F-2", "F#2", "G-2", "G#2", "A-2", "A#2", "B-2",
        "C-3", "C#3", "D-3", "D#3", "E-3", "F-3", "F#3", "G-3", "G#3", "A-3", "A#3", "B-3",
        "C-4", "C#4", "D-4", "D#4", "E-4", "F-4", "F#4", "G-4", "G#4", "A-4", "A#4", "B-4",
        "C-5", "C#5", "D-5", "D#5", "E-5", "F-5", "F#5", "G-5", "G#5", "A-5", "A#5", "B-5",
        "C-6", "C#6", "D-6", "D#6", "E-6", "F-6", "F#6", "G-6", "G#6", "A-6", "A#6", "B-6",
        "C-7", "C#7", "D-7", "D#7", "E-7", "F-7", "F#7", "G-7", "G#7", "A-7", "A#7", "B-7",
        "C-8", "C#8", "D-8", "D#8", "E-8", "F-8", "F#8", "G-8", "G#8", "A-8", "A#8", "B-8",
        "C-9", "C#9", "D-9", "D#9", "E-9", "F-9", "F#9", "G-9", "G#9", "A-9", "A#9", "B-9",
        "C-A", "C#A", "D-A", "D#A", "E-A", "F-A", "F#A", "G-A" } };
    // clang-format on

    if (midiNote < midiToStringTable.size()) {
        return midiToStringTable[midiNote];
    } else {
        throw std::out_of_range("MIDI note out of valid range: " + std::to_string(midiNote));
    }
}

uint8_t stringToMidi(const std::string & noteString)
{
    // clang-format off
    static const std::unordered_map<std::string, uint8_t> stringToMidiTable = {
        { "C-0", 0 }, { "C#0", 1 }, { "D-0", 2 }, { "D#0", 3 }, { "E-0", 4 }, { "F-0", 5 }, { "F#0", 6 }, { "G-0", 7 }, { "G#0", 8 }, { "A-0", 9 }, { "A#0", 10 }, { "B-0", 11 },
        { "C-1", 12 }, { "C#1", 13 }, { "D-1", 14 }, { "D#1", 15 }, { "E-1", 16 }, { "F-1", 17 }, { "F#1", 18 }, { "G-1", 19 }, { "G#1", 20 }, { "A-1", 21 }, { "A#1", 22 }, { "B-1", 23 },
        { "C-2", 24 }, { "C#2", 25 }, { "D-2", 26 }, { "D#2", 27 }, { "E-2", 28 }, { "F-2", 29 }, { "F#2", 30 }, { "G-2", 31 }, { "G#2", 32 }, { "A-2", 33 }, { "A#2", 34 }, { "B-2", 35 },
        { "C-3", 36 }, { "C#3", 37 }, { "D-3", 38 }, { "D#3", 39 }, { "E-3", 40 }, { "F-3", 41 }, { "F#3", 42 }, { "G-3", 43 }, { "G#3", 44 }, { "A-3", 45 }, { "A#3", 46 }, { "B-3", 47 },
        { "C-4", 48 }, { "C#4", 49 }, { "D-4", 50 }, { "D#4", 51 }, { "E-4", 52 }, { "F-4", 53 }, { "F#4", 54 }, { "G-4", 55 }, { "G#4", 56 }, { "A-4", 57 }, { "A#4", 58 }, { "B-4", 59 },
        { "C-5", 60 }, { "C#5", 61 }, { "D-5", 62 }, { "D#5", 63 }, { "E-5", 64 }, { "F-5", 65 }, { "F#5", 66 }, { "G-5", 67 }, { "G#5", 68 }, { "A-5", 69 }, { "A#5", 70 }, { "B-5", 71 },
        { "C-6", 72 }, { "C#6", 73 }, { "D-6", 74 }, { "D#6", 75 }, { "E-6", 76 }, { "F-6", 77 }, { "F#6", 78 }, { "G-6", 79 }, { "G#6", 80 }, { "A-6", 81 }, { "A#6", 82 }, { "B-6", 83 },
        { "C-7", 84 }, { "C#7", 85 }, { "D-7", 86 }, { "D#7", 87 }, { "E-7", 88 }, { "F-7", 89 }, { "F#7", 90 }, { "G-7", 91 }, { "G#7", 92 }, { "A-7", 93 }, { "A#7", 94 }, { "B-7", 95 },
        { "C-8", 96 }, { "C#8", 97 }, { "D-8", 98 }, { "D#8", 99 }, { "E-8", 100 }, { "F-8", 101 }, { "F#8", 102 }, { "G-8", 103 }, { "G#8", 104 }, { "A-8", 105 }, { "A#8", 106 }, { "B-8", 107 },
        { "C-9", 108 }, { "C#9", 109 }, { "D-9", 110 }, { "D#9", 111 }, { "E-9", 112 }, { "F-9", 113 }, { "F#9", 114 }, { "G-9", 115 }, { "G#9", 116 }, { "A-9", 117 }, { "A#9", 118 }, { "B-9", 119 },
        { "C-A", 120 }, { "C#A", 121 }, { "D-A", 122 }, { "D#A", 123 }, { "E-A", 124 }, { "F-A", 125 }, { "F#A", 126 }, { "G-A", 127 }
    };
    // clang-format on

    if (const auto it = stringToMidiTable.find(noteString); it != stringToMidiTable.end()) {
        return it->second;
    } else {
        throw std::invalid_argument("Invalid note string: " + noteString);
    }
}

std::pair<uint8_t, uint8_t> midiToKeyAndOctave(uint8_t midiNote)
{
    return { midiNote % 12 + 1, midiNote / 12 };
}

NoteConverter::MidiNoteNameAndCodeOpt keyAndOctaveToMidiNote(uint8_t key, uint8_t octave, int transpose)
{
    if (key < 1 || key > 12) {
        juzzlin::L(TAG).error() << "Invalid key value: " << static_cast<int>(key) << ". Valid range is 1..12.";
        return {};
    }

    static const std::array<std::string, 12> keyNames = {
        "C-", "C#", "D-", "D#", "E-", "F-", "F#", "G-", "G#", "A-", "A#", "B-"
    };

    // Calculate transposed note
    int rawNote = 12 * octave + (key - 1) + transpose;

    // If out of MIDI range, return nullopt
    if (rawNote < 0 || rawNote > 127) {
        juzzlin::L(TAG).warning() << "Note out of MIDI range: " << rawNote;
        return {};
    }

    const auto noteName = keyNames.at(key - 1) + std::to_string(octave);
    const auto midiNote = static_cast<uint8_t>(rawNote);

    return { { noteName, midiNote } };
}

} // namespace noteahead::NoteConverter
