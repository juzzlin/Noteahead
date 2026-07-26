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

#ifndef UPSAMPLER_HPP
#define UPSAMPLER_HPP

#include <array>
#include <cstddef>
#include <cstdint>

namespace noteahead {

//! Length of the windowed-sinc half-band FIR shared by the effect interpolator and decimator. Long
//! enough (steep transition, deep stopband) to suppress the broadband harmonics a nonlinear effect
//! generates when oversampled, unlike the short half-band the synth voices use on already-band-limited
//! sources. Must be of the form 4n+3 so the filter is a true half-band centred on a single tap.
inline constexpr int HalfBandLength = 43;

//! 2x polyphase half-band interpolator for oversampling nonlinear effects. Produces two high-rate
//! samples from one base-rate sample; pairs with Decimator2x for a near-transparent round trip.
class Upsampler2x
{
public:
    void process(float sample, float & out0, float & out1);
    void reset();

private:
    static constexpr size_t HistLength = (HalfBandLength + 1) / 2;
    std::array<float, HistLength> m_buffer {};
    size_t m_writeIndex { 0 };
};

//! 2x half-band decimator for oversampling nonlinear effects: filters two high-rate samples and
//! returns one base-rate sample. Uses the same half-band as Upsampler2x.
class Decimator2x
{
public:
    float process(float s0, float s1);
    void reset();

private:
    std::array<float, HalfBandLength> m_buffer {};
    size_t m_writeIndex { 0 };
};

//! Interpolates one base-rate sample to a block of high-rate samples for factors 1, 2 and 4. Factor 1
//! is a passthrough, factor 2 uses one stage and factor 4 cascades two stages. Pairs with Decimator.
class Upsampler
{
public:
    //! Fill @p out with @p factor high-rate samples interpolated from one base-rate @p sample.
    //! @p factor must be 1, 2 or 4; any other value is treated as a passthrough.
    void process(float sample, float * out, uint8_t factor);
    void reset();

private:
    Upsampler2x m_stage1;
    Upsampler2x m_stage2;
};

//! Decimates a block of high-rate samples back to one base-rate sample for factors 1, 2 and 4. Factor
//! 1 is a passthrough, factor 2 uses one stage and factor 4 cascades two stages. Pairs with Upsampler.
class Decimator
{
public:
    //! Decimate @p factor consecutive high-rate samples (pointed to by @p highRate) into one
    //! base-rate sample. @p factor must be 1, 2 or 4; any other value is treated as a passthrough.
    float process(const float * highRate, uint8_t factor);
    void reset();

private:
    Decimator2x m_stage1;
    Decimator2x m_stage2;
};

} // namespace noteahead

#endif // UPSAMPLER_HPP
