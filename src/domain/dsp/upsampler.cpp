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

#include "upsampler.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <vector>

namespace noteahead {

uint8_t clampOversampleFactor(uint8_t factor)
{
    return (factor == 1 || factor == 2 || factor == 4) ? factor : 2;
}

float noiseGainForOversampling(uint8_t factor)
{
    return std::sqrt(static_cast<float>(clampOversampleFactor(factor)));
}

namespace {

//! One half-band FIR h[0..L-1], stored by what is left of it once the zeros are dropped. With the
//! centre c = (L-1)/2 odd, h[n] is zero wherever n - c is even except at the centre itself, which
//! leaves exactly the even-indexed taps and the centre. Skipping the zeros is what makes the long
//! outer filter cost less per sample than the old 43-tap one did.
struct HalfBand
{
    static constexpr size_t MaxEvenTaps = (OuterHalfBandLength + 1) / 2;
    std::array<float, MaxEvenTaps> evenTaps {}; // h[0], h[2], h[4], ... h[L-1]
    std::array<float, MaxEvenTaps> evenTaps2x {}; // 2 * evenTaps for zero-stuffed upsampling
    size_t evenTapCount { 0 };
    size_t halfTapCount { 0 };
    float centre { 0.0f }; // h[c]
    float centre2x { 0.0f }; // 2 * h[c]
    size_t centreDelay { 0 }; // (c-1)/2: how many base-rate samples back the centre tap reaches
};

double blackmanHarris(int n, int length)
{
    const double twoPi = 2.0 * std::numbers::pi;
    const double t = static_cast<double>(n) / static_cast<double>(length - 1);
    return 0.35875 - 0.48829 * std::cos(twoPi * t) + 0.14128 * std::cos(2.0 * twoPi * t) - 0.01168 * std::cos(3.0 * twoPi * t);
}

double kaiser(int n, int length, double beta)
{
    const double x = 2.0 * static_cast<double>(n) / static_cast<double>(length - 1) - 1.0;
    return std::cyl_bessel_i(0.0, beta * std::sqrt(std::max(0.0, 1.0 - x * x))) / std::cyl_bessel_i(0.0, beta);
}

//! Windowed sinc with its cutoff at a quarter of the sample rate, normalised for unity DC gain.
template<typename Window>
HalfBand makeHalfBand(int length, Window window)
{
    const int centre = (length - 1) / 2;
    std::vector<double> h(static_cast<size_t>(length), 0.0);
    double sum = 0.0;
    for (int n = 0; n < length; n++) {
        const int offset = n - centre;
        if (offset != 0 && offset % 2 == 0) {
            continue; // Zero by construction: sin(pi * offset / 2) vanishes
        }
        const double x = 0.5 * static_cast<double>(offset);
        const double sinc = offset == 0 ? 1.0 : std::sin(std::numbers::pi * x) / (std::numbers::pi * x);
        h[static_cast<size_t>(n)] = 0.5 * sinc * window(n, length);
        sum += h[static_cast<size_t>(n)];
    }

    HalfBand result;
    for (int n = 0; n < length; n += 2) {
        const float tap = static_cast<float>(h[static_cast<size_t>(n)] / sum);
        result.evenTaps[result.evenTapCount] = tap;
        result.evenTaps2x[result.evenTapCount] = 2.0f * tap;
        result.evenTapCount++;
    }
    result.halfTapCount = result.evenTapCount / 2;
    result.centre = static_cast<float>(h[static_cast<size_t>(centre)] / sum);
    result.centre2x = 2.0f * result.centre;
    result.centreDelay = static_cast<size_t>((centre - 1) / 2);
    return result;
}

const HalfBand & halfBand(HalfBandStage stage)
{
    // Kaiser at beta 9: flat within 0.01 dB to 20 kHz at a 44.1 kHz base rate and more than 70 dB
    // down from 24.1 kHz.
    static const HalfBand outer = makeHalfBand(OuterHalfBandLength, [](int n, int length) { return kaiser(n, length, 9.0); });
    static const HalfBand inner = makeHalfBand(InnerHalfBandLength, blackmanHarris);
    return stage == HalfBandStage::Outer ? outer : inner;
}

size_t lengthOf(HalfBandStage stage)
{
    return static_cast<size_t>(stage == HalfBandStage::Outer ? OuterHalfBandLength : InnerHalfBandLength);
}

//! Writes @p sample as the newest entry of a doubled, newest-first delay line of @p length and
//! returns where the line now starts.
template<size_t Size>
size_t pushNewestFirst(std::array<float, Size> & buffer, size_t position, size_t length, float sample)
{
    position = position == 0 ? length - 1 : position - 1;
    buffer[position] = sample;
    buffer[position + length] = sample;
    return position;
}

} // namespace

Upsampler2x::Upsampler2x(HalfBandStage stage)
{
    const auto & hb = halfBand(stage);
    m_taps = hb.evenTaps2x.data();
    m_histLength = hb.evenTapCount;
    m_halfTapCount = hb.halfTapCount;
    m_centre = hb.centre2x;
    m_centreDelay = hb.centreDelay;
}

void Upsampler2x::process(float sample, float & out0, float & out1)
{
    m_position = pushNewestFirst(m_buffer, m_position, m_histLength, sample);
    const float * const history = m_buffer.data() + m_position;

    // Zero-stuffing doubles the rate and halves the level, so both branches use 2x pre-scaled taps.
    // Symmetric even taps are folded so each pair of equidistant samples shares one multiply.
    const size_t last = m_histLength - 1;
    float even = 0.0f;
    for (size_t k = 0; k < m_halfTapCount; k++) {
        even += (history[k] + history[last - k]) * m_taps[k];
    }
    out0 = even;
    out1 = m_centre * history[m_centreDelay];
}

void Upsampler2x::reset()
{
    std::fill(m_buffer.begin(), m_buffer.end(), 0.0f);
    m_position = 0;
}

Decimator2x::Decimator2x(HalfBandStage stage)
{
    const auto & hb = halfBand(stage);
    m_taps = hb.evenTaps.data();
    m_oddLength = hb.evenTapCount;
    m_evenDelay = (lengthOf(stage) + 1) / 4;
    m_halfTapCount = hb.halfTapCount;
    m_centre = hb.centre;
    m_centreDelay = hb.centreDelay;
}

float Decimator2x::process(float s0, float s1)
{
    m_evenPosition = pushNewestFirst(m_even, m_evenPosition, m_evenDelay, s0);
    m_oddPosition = pushNewestFirst(m_odd, m_oddPosition, m_oddLength, s1);
    const float * const odd = m_odd.data() + m_oddPosition;

    const size_t last = m_oddLength - 1;
    float output = m_centre * m_even[m_evenPosition + m_centreDelay];
    for (size_t k = 0; k < m_halfTapCount; k++) {
        output += (odd[k] + odd[last - k]) * m_taps[k];
    }
    return output;
}

void Decimator2x::reset()
{
    std::fill(m_odd.begin(), m_odd.end(), 0.0f);
    std::fill(m_even.begin(), m_even.end(), 0.0f);
    m_oddPosition = 0;
    m_evenPosition = 0;
}

Upsampler::Upsampler(HalfBandStage outerStage)
  : m_outer { outerStage }
{
}

void Upsampler::process(float sample, float * out, uint8_t factor)
{
    switch (factor) {
    case 2:
        m_outer.process(sample, out[0], out[1]);
        break;
    case 4: {
        float a = 0.0f;
        float b = 0.0f;
        m_outer.process(sample, a, b);
        m_inner.process(a, out[0], out[1]);
        m_inner.process(b, out[2], out[3]);
        break;
    }
    case 1:
    default:
        out[0] = sample;
        break;
    }
}

void Upsampler::processBlock(const float * in, float * out, size_t frames, uint8_t factor)
{
    switch (factor) {
    case 2:
        for (size_t i = 0; i < frames; i++) {
            m_outer.process(in[i], out[i * 2], out[i * 2 + 1]);
        }
        break;
    case 4:
        for (size_t i = 0; i < frames; i++) {
            float a = 0.0f;
            float b = 0.0f;
            m_outer.process(in[i], a, b);
            float * const dst = out + i * 4;
            m_inner.process(a, dst[0], dst[1]);
            m_inner.process(b, dst[2], dst[3]);
        }
        break;
    case 1:
    default:
        std::copy_n(in, frames, out);
        break;
    }
}

void Upsampler::reset()
{
    m_outer.reset();
    m_inner.reset();
}

float Decimator::process(const float * highRate, uint8_t factor)
{
    switch (factor) {
    case 2:
        return m_outer.process(highRate[0], highRate[1]);
    case 4: {
        const float a = m_inner.process(highRate[0], highRate[1]);
        const float b = m_inner.process(highRate[2], highRate[3]);
        return m_outer.process(a, b);
    }
    case 1:
    default:
        return highRate[0];
    }
}

void Decimator::processBlock(const float * highRate, float * out, size_t frames, uint8_t factor)
{
    switch (factor) {
    case 2:
        for (size_t i = 0; i < frames; i++) {
            out[i] = m_outer.process(highRate[i * 2], highRate[i * 2 + 1]);
        }
        break;
    case 4:
        for (size_t i = 0; i < frames; i++) {
            const float * const src = highRate + i * 4;
            const float a = m_inner.process(src[0], src[1]);
            const float b = m_inner.process(src[2], src[3]);
            out[i] = m_outer.process(a, b);
        }
        break;
    case 1:
        std::copy_n(highRate, frames, out);
        break;
    default:
        for (size_t i = 0; i < frames; i++) {
            out[i] = highRate[i * factor];
        }
        break;
    }
}

void Decimator::reset()
{
    m_outer.reset();
    m_inner.reset();
}

} // namespace noteahead
