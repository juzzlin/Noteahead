// This file is part of Noteahead.
// Copyright (C) 2025 Jussi Lind <jussi.lind@iki.fi>
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

#ifndef MIDI_SIDE_CHAIN_SERVICE_HPP
#define MIDI_SIDE_CHAIN_SERVICE_HPP

#include "../../domain/song.hpp"

namespace noteahead {

namespace MidiSideChainService {
Song::EventList renderToEvents(const Song & song, const Song::EventList & events, size_t endPosition);
}

} // namespace noteahead

#endif // MIDI_SIDE_CHAIN_SERVICE_HPP
