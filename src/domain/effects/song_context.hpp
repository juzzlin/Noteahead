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

#ifndef SONG_CONTEXT_HPP
#define SONG_CONTEXT_HPP

#include <cstddef>
#include <string>

namespace noteahead {

struct SongContext
{
    bool hasSongData { false };
    double bpm { 128.0 };
    double lowestBassHz { 41.2 };
    bool hasSubOr808 { false };
    bool hasPunchyKick { false };
    bool isFourOnTheFloor { false };
    double kickDensityPerBeat { 0.0 };
    bool hasBrightOrFmSynths { false };
    bool hasSidechainDucking { false };
    double arrangementDensity { 0.5 };
    double arrangementContrast { 0.3 };
    double stereoSpreadHint { 0.5 };
    size_t activeInstrumentCount { 0 };
    std::string summaryText;
};

} // namespace noteahead

#endif // SONG_CONTEXT_HPP
