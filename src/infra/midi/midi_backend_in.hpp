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

#ifndef MIDI_BACKEND_IN_HPP
#define MIDI_BACKEND_IN_HPP

#include "midi_backend.hpp"

#include <functional>

namespace noteahead {

//! Base class for MIDI input backend implementations.
class MidiBackendIn : public MidiBackend
{
public:
    MidiBackendIn();
    virtual ~MidiBackendIn() override;

    using InputCallback = std::function<void(double, const std::vector<unsigned char> &)>;
    virtual void setCallbackForPort(const MidiPort & port, InputCallback callback);
    virtual void clearCallbacks();
};

} // namespace noteahead

#endif // MIDI_BACKEND_IN_HPP
