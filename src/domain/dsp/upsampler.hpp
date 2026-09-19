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

//! Clamps an arbitrary oversampling factor to a supported value (1, 2 or 4), defaulting to 2 for
//! anything unexpected. Devices call this before rendering so a bad setting can never misbehave.
uint8_t clampOversampleFactor(uint8_t factor);

//! Amplitude compensation for white noise drawn one sample per clock at an oversampled rate.
//!
//! Such a generator has a fixed total power spread flat up to Nyquist, so raising the render rate
//! spreads it over a wider band and the part that survives decimation — everything below the base
//! Nyquist — drops by 10*log10(factor): 3 dB at 2x, 6 dB at 4x. Without this a noisy patch gets
//! quieter the more you oversample, which is the opposite of oversampling being transparent.
float noiseGainForOversampling(uint8_t factor);

//! Lengths of the two windowed-sinc half-band FIRs. Both must be of the form 4n+3 so the filter is a
//! true half-band centred on a single tap, with every other tap zero.
//!
//! The outer one sits at the base rate's edge, where the audio band ends only a few kilohertz below
//! Nyquist: at 44.1 kHz it has to pass 20 kHz and reject 24.1 kHz, which takes a long Kaiser window.
//! A 43-tap filter there took 3.6 dB off 20 kHz per round trip and rejected only 14.5 dB at 24.1 kHz.
//!
//! The inner one runs between 2x and 4x, where everything it must pass is below a quarter of its
//! Nyquist, so a short filter is already transparent.
inline constexpr int OuterHalfBandLength = 119;
inline constexpr int InnerHalfBandLength = 43;

//! Which of the two half-bands a 2x stage uses. See OuterHalfBandLength.
enum class HalfBandStage
{
    Outer,
    Inner
};

//! 2x polyphase half-band interpolator for oversampling nonlinear effects. Produces two high-rate
//! samples from one base-rate sample; pairs with Decimator2x for a near-transparent round trip.
class Upsampler2x
{
public:
    explicit Upsampler2x(HalfBandStage stage = HalfBandStage::Outer);

    void process(float sample, float & out0, float & out1);
    void reset();

private:
    static constexpr size_t MaxHistLength = (OuterHalfBandLength + 1) / 2;
    // Twice the history, each sample written to both halves, so that the newest-first window is
    // always one contiguous run and the inner loop needs no wrap-around.
    std::array<float, 2 * MaxHistLength> m_buffer {};
    size_t m_histLength;
    size_t m_position { 0 };
    HalfBandStage m_stage;
};

//! 2x half-band decimator for oversampling nonlinear effects: filters two high-rate samples and
//! returns one base-rate sample. Uses the same half-band as Upsampler2x.
class Decimator2x
{
public:
    explicit Decimator2x(HalfBandStage stage = HalfBandStage::Outer);

    float process(float s0, float s1);
    void reset();

private:
    static constexpr size_t MaxOddLength = (OuterHalfBandLength + 1) / 2;
    static constexpr size_t MaxEvenDelay = (OuterHalfBandLength + 1) / 4;
    // The second sample of each pair meets every nonzero tap but the centre one, and the first sample
    // meets only the centre one, so each gets its own line. Doubled as in Upsampler2x.
    std::array<float, 2 * MaxOddLength> m_odd {};
    std::array<float, 2 * MaxEvenDelay> m_even {};
    size_t m_oddLength;
    size_t m_evenDelay;
    size_t m_oddPosition { 0 };
    size_t m_evenPosition { 0 };
    HalfBandStage m_stage;
};

//! Interpolates one base-rate sample to a block of high-rate samples for factors 1, 2 and 4. Factor 1
//! is a passthrough, factor 2 uses one stage and factor 4 cascades two stages. Pairs with Decimator.
class Upsampler
{
public:
    //! @p outerStage picks the filter for the base-rate stage. Only a source whose images a decimator
    //! removes afterwards anyway should pass Inner: it trades the flat top octave for a third of the
    //! latency. See BaseRateSource.
    explicit Upsampler(HalfBandStage outerStage = HalfBandStage::Outer);

    //! Fill @p out with @p factor high-rate samples interpolated from one base-rate @p sample.
    //! @p factor must be 1, 2 or 4; any other value is treated as a passthrough.
    void process(float sample, float * out, uint8_t factor);
    void reset();

private:
    Upsampler2x m_outer;
    Upsampler2x m_inner { HalfBandStage::Inner };
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
    Decimator2x m_inner { HalfBandStage::Inner };
    Decimator2x m_outer { HalfBandStage::Outer };
};

} // namespace noteahead

#endif // UPSAMPLER_HPP
