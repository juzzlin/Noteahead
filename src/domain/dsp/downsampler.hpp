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

#ifndef DOWNSAMPLER_HPP
#define DOWNSAMPLER_HPP

#include "oversampler.hpp"

#include <cstdint>

namespace noteahead {

//! Clamps an arbitrary oversampling factor to a supported value (1, 2 or 4), defaulting to 2 for
//! anything unexpected. Devices call this before rendering so a bad setting can never misbehave.
uint8_t clampOversampleFactor(uint8_t factor);

//! Decimates a block of high-rate samples back to the base rate for oversampling factors 1, 2 and 4.
//! Factor 1 is a passthrough, factor 2 uses a single half-band stage and factor 4 cascades two
//! half-band stages. This lets a device pick its oversampling ratio at runtime (e.g. lower for
//! realtime playback, higher for offline export) without changing its rendering structure.
class Downsampler
{
public:
    //! Decimate @p factor consecutive high-rate samples (pointed to by @p highRate) into one
    //! base-rate sample. @p factor must be 1, 2 or 4; any other value is treated as a passthrough.
    float process(const float * highRate, uint8_t factor);

    void reset();

private:
    Oversampler2x m_stage1;
    Oversampler2x m_stage2;
};

} // namespace noteahead

#endif // DOWNSAMPLER_HPP
