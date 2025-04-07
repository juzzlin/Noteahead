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

#ifndef INSTRUMENT_REQUEST_HPP
#define INSTRUMENT_REQUEST_HPP

#include "../domain/instrument.hpp"

namespace noteahead {

class InstrumentRequest
{
public:
    enum class Type
    {
        None,
        ApplyAll,
        ApplyPatch,
        ApplyMidiCc,
    };

    InstrumentRequest();

    InstrumentRequest(Type type, const Instrument & instrument);

    Type type() const;

    const Instrument & instrument() const;

private:
    Type m_type = Type::None;

    Instrument m_instrument;
};

} // namespace noteahead

#endif // INSTRUMENT_REQUEST_HPP
