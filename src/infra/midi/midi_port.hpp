// This file is part of Noteahead.
// Copyright (C) 2024 Jussi Lind <jussi.lind@iki.fi>
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

#ifndef MIDI_PORT_HPP
#define MIDI_PORT_HPP

#include <cstddef>
#include <memory>
#include <string>

namespace noteahead {

class MidiPort
{
    MidiPort(size_t index, const std::string & name, const std::string & id);

    size_t index() const;
    const std::string & name() const;
    const std::string & id() const; // New accessor for the unique ID

    std::string toString() const;

private:
    size_t mIndex;
    std::string mName;
    std::string mId; // New member to store the unique identifier
};

using MidiPortS = std::shared_ptr<MidiPort>;
using MidiPortW = std::weak_ptr<MidiPort>;

} // namespace noteahead

#endif // MIDI_PORT_HPP
