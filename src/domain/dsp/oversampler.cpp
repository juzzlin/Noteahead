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

#include "oversampler.hpp"

#include <algorithm>

namespace noteahead {

float Oversampler2x::process(float s0, float s1)
{
    // Write high-rate samples into circular buffer
    m_buffer[m_writeIndex] = s0;
    m_writeIndex = (m_writeIndex + 1) % Coefficients.size();
    m_buffer[m_writeIndex] = s1;
    m_writeIndex = (m_writeIndex + 1) % Coefficients.size();

    // FIR convolution
    float output { 0.0f };
    size_t readIndex { m_writeIndex };
    
    for (const float coeff : Coefficients) {
        if (readIndex == 0) {
            readIndex = Coefficients.size() - 1;
        } else {
            readIndex--;
        }
        output += m_buffer[readIndex] * coeff;
    }

    return output;
}

void Oversampler2x::reset()
{
    std::fill(m_buffer.begin(), m_buffer.end(), 0.0f);
    m_writeIndex = 0;
}

} // namespace noteahead
