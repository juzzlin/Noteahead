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

#include "interpolator.hpp"

namespace noteahead {

Interpolator::Interpolator(size_t startLine, size_t endLine, double startValue, double endValue)
  : m_startLine { startLine }
  , m_endLine { endLine }
  , m_startValue { startValue }
  , m_endValue { endValue }
{
}

double Interpolator::getValue(size_t line) const
{
    if (line <= m_startLine) {
        return m_startValue;
    }

    if (line >= m_endLine) {
        return m_endValue;
    }

    if (m_startLine == m_endLine) {
        return m_startValue;
    }

    const double t = static_cast<double>(line - m_startLine) / static_cast<double>(m_endLine - m_startLine);
    const double d = m_endValue - m_startValue;
    return m_startValue + t * d;
}

} // namespace noteahead
