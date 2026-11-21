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

#ifndef DUMMY_MIDI_SERVICE_HPP
#define DUMMY_MIDI_SERVICE_HPP

#include "midi_service.hpp"

namespace noteahead {

//! A MidiService without the workers that open MIDI ports, for --smoke-test: a container or a CI
//! machine has no MIDI sequencer, and its absence says nothing about whether the package starts.
class DummyMidiService : public MidiService
{
public:
    explicit DummyMidiService(DeviceServiceS deviceService);
};

} // namespace noteahead

#endif // DUMMY_MIDI_SERVICE_HPP
