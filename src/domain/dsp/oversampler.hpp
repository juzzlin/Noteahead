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

#ifndef OVERSAMPLER_HPP
#define OVERSAMPLER_HPP

#include <array>
#include <cstddef>

namespace noteahead {

/**
 * @brief A lightweight 2x decimation filter for oversampling.
 *
 * Uses a polyphase FIR half-band filter for efficient 2x downsampling.
 */
class Oversampler2x
{
public:
    /**
     * @brief Process two high-rate samples and return one decimated low-rate sample.
     * @param s0 First high-rate sample
     * @param s1 Second high-rate sample
     * @return The downsampled value
     */
    float process(float s0, float s1);

    /**
     * @brief Reset the filter state.
     */
    void reset();

private:
    // 12-tap half-band filter coefficients (only odd taps are non-zero, except center)
    // Designed for ~100dB stopband rejection and flat passband up to 0.45 * Nyquist
    // Normalized for unity gain at DC.
    static constexpr std::array<float, 12> Coefficients = {
        -0.005925f, 0.0f, 0.028282f, 0.0f, -0.093472f, 0.571115f,
        0.571115f, -0.093472f, 0.0f, 0.028282f, 0.0f, -0.005925f
    };

    std::array<float, 12> m_buffer {};
    size_t m_writeIndex { 0 };
};

} // namespace noteahead

#endif // OVERSAMPLER_HPP
