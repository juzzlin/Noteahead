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

#include "drum_synth_constants.hpp"

#include <map>

namespace noteahead::DrumSynth {

QString voiceName(int index)
{
    static const std::map<VoiceIndex, QString> names = {
        { VoiceIndex::Kick, "Kick" },
        { VoiceIndex::Snare, "Snare" },
        { VoiceIndex::ClosedHiHat, "Closed Hi-Hat" },
        { VoiceIndex::Clap, "Clap" },
        { VoiceIndex::OpenHiHat, "Open Hi-Hat" },
        { VoiceIndex::LowTom, "Low Tom" },
        { VoiceIndex::MidTom, "Mid Tom" },
        { VoiceIndex::HighTom, "High Tom" },
        { VoiceIndex::Crash, "Crash" },
        { VoiceIndex::Ride, "Ride" },
        { VoiceIndex::ReverseCrash, "Reverse Crash" }
    };
    const auto it = names.find(static_cast<VoiceIndex>(index));
    return it != names.end() ? it->second : "Undefined";
}

std::string voiceId(int index)
{
    static const std::map<VoiceIndex, std::string> ids = {
        { VoiceIndex::Kick, "Kick" },
        { VoiceIndex::Snare, "Snare" },
        { VoiceIndex::ClosedHiHat, "ClosedHiHat" },
        { VoiceIndex::Clap, "Clap" },
        { VoiceIndex::OpenHiHat, "OpenHiHat" },
        { VoiceIndex::LowTom, "LowTom" },
        { VoiceIndex::MidTom, "MidTom" },
        { VoiceIndex::HighTom, "HighTom" },
        { VoiceIndex::Crash, "Crash" },
        { VoiceIndex::Ride, "Ride" },
        { VoiceIndex::ReverseCrash, "ReverseCrash" }
    };
    const auto it = ids.find(static_cast<VoiceIndex>(index));
    return it != ids.end() ? it->second : "Undefined";
}

} // namespace noteahead::DrumSynth
