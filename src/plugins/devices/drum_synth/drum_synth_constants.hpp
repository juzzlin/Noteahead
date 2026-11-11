// This file is part of Noteahead.
// Copyright (C) 2026 Jussi Lind <jussi.lind@iki.fi>
//
// Noteahead is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// Noteahead is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Noteahead. If not, see <http://www.gnu.org/licenses/>.

#ifndef DRUM_SYNTH_CONSTANTS_HPP
#define DRUM_SYNTH_CONSTANTS_HPP

#include <QString>
#include <cstdint>
#include <string>

namespace noteahead::DrumSynth {

enum class VoiceIndex : int
{
    Kick = 0,
    Snare = 1,
    ClosedHiHat = 2,
    Clap = 3,
    OpenHiHat = 4,
    LowTom = 5,
    MidTom = 6,
    HighTom = 7,
    Crash = 8,
    Ride = 9,
    ReverseCrash = 10
};

static constexpr int NumVoices = 11;

enum class MidiNote : uint8_t
{
    Kick = 36,
    Snare = 38,
    ClosedHiHat = 42,
    Clap = 39,
    PedalHiHat = 44,
    OpenHiHat = 46,
    LowTom = 41,
    MidTom = 43,
    HiTom = 45,
    Crash = 49,
    Ride = 51,
    ReverseCrash = 52
};

static constexpr uint8_t CcStartRange1 = 14;
static constexpr int NumVoicesRange1 = 6;
static constexpr uint8_t CcStartRange2 = 102;
static constexpr int NumVoicesRange2 = 5;

QString voiceName(int index);
std::string voiceId(int index);

} // namespace noteahead::DrumSynth

#endif // DRUM_SYNTH_CONSTANTS_HPP
