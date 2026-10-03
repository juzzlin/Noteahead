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

#include "parameter_data.hpp"

namespace noteahead {

ParameterData::ParameterData(size_t track, size_t column, TargetW target, std::string parameter, ParameterAutomation::Mode mode, double value)
  : EventData { track, column }
  , m_target { std::move(target) }
  , m_parameter { std::move(parameter) }
  , m_mode { mode }
  , m_value { value }
{
}

const ParameterData::TargetW & ParameterData::target() const
{
    return m_target;
}

const std::string & ParameterData::parameter() const
{
    return m_parameter;
}

ParameterAutomation::Mode ParameterData::mode() const
{
    return m_mode;
}

double ParameterData::value() const
{
    return m_value;
}

} // namespace noteahead
