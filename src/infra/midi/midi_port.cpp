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

#include "midi_port.hpp"

namespace noteahead {

MidiPort::MidiPort(size_t index, const std::string & name, const std::string & id)
  : mIndex { index }
  , mName { std::move(name) }
  , mId { std::move(id) }
{
}

size_t MidiPort::index() const
{
    return mIndex;
}

const std::string & MidiPort::name() const
{
    return mName;
}

const std::string & MidiPort::id() const
{
    return mId;
}

std::string MidiPort::toString() const
{
    return std::to_string(mIndex) + ": " + mName;
}

} // namespace noteahead
