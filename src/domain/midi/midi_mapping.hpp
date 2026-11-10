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

#ifndef MIDI_MAPPING_HPP
#define MIDI_MAPPING_HPP

#include <cstdint>
#include <optional>
#include <string>

namespace noteahead {

class ProjectReader;
class ProjectWriter;

//! One controller of a device moving one of its parameters: "CC 74 drives the cutoff between these
//! two knob positions". Kept with the device, so a project brings its controller layout along.
//!
//! The controller is a kind and a number, and the value arrives as 0..1, so that the wider
//! controllers of MIDI 2.0 -- RPN/NRPN, per-note, 32-bit values -- need no change to the mapping or
//! to the devices, only a new kind and whatever reads them off the wire.
struct MidiMapping
{
    //! Append only: saved by name, but compared by ordinal everywhere else.
    enum class Kind
    {
        Cc
    };

    Kind kind { Kind::Cc };
    uint16_t controller {};
    //! The parameter's key, as automatableParameters() names it.
    std::string target;
    //! Knob positions, 0..1, the controller's bottom and top land on. Min above Max inverts it.
    double min { 0.0 };
    double max { 1.0 };

    //! Where the knob goes for a controller at @p normalized, 0..1.
    double targetValue(double normalized) const;

    void serializeToXml(ProjectWriter & writer) const;
    //! Reads the <MidiMapping> the reader sits on, and leaves it at its end. Nothing for one this
    //! version cannot use: a kind it does not know, or no target.
    static std::optional<MidiMapping> deserializeFromXml(ProjectReader & reader);

    bool operator==(const MidiMapping & other) const = default;
};

} // namespace noteahead

#endif // MIDI_MAPPING_HPP
