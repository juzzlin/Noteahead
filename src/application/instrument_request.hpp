// This file is part of Cacophony.
// Copyright (C) 2025 Jussi Lind <jussi.lind@iki.fi>
//
// Cacophony is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// Cacophony is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Cacophony. If not, see <http://www.gnu.org/licenses/>.

#ifndef INSTRUMENT_REQUEST_HPP
#define INSTRUMENT_REQUEST_HPP

#include <memory>

namespace cacophony {

class Instrument;

class InstrumentRequest
{
public:
    enum class Type
    {
        None,
        Apply,
        Test
    };

    InstrumentRequest() = default;

    using InstrumentS = std::shared_ptr<Instrument>;
    InstrumentRequest(Type type, InstrumentS instrument);

    Type type() const;

    InstrumentS instrument() const;

private:
    Type m_type = Type::None;

    InstrumentS m_instrument;
};

} // namespace cacophony

#endif // INSTRUMENT_REQUEST_HPP
