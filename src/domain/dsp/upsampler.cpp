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

namespace noteahead {

namespace {

//! Windowed-sinc half-band FIR (cutoff at a quarter of the sample rate) plus its two polyphase
//! branches, computed once. The decimator uses the full kernel (unity DC gain); the interpolator uses
//! the branches scaled by 2 so each produced sample keeps unity level after the zero-stuffing.
struct HalfBand
{
    std::array<float, HalfBandLength> kernel {};
    static constexpr size_t Phase0Length = (HalfBandLength + 1) / 2; // even-indexed taps
    static constexpr size_t Phase1Length = HalfBandLength / 2; // odd-indexed taps
    std::array<float, Phase0Length> phase0 {};
    std::array<float, Phase1Length> phase1 {};
};

const HalfBand & halfBand()
{
    static const HalfBand hb = [] {
        HalfBand result;
        constexpr int length = HalfBandLength;
        constexpr int center = (length - 1) / 2;
        const double twoPi = 2.0 * std::numbers::pi;

        double sum = 0.0;
        for (int n = 0; n < length; n++) {
            const double x = 0.5 * static_cast<double>(n - center); // sinc argument for a 0.25 cutoff
            const double sinc = (n == center) ? 1.0 : std::sin(std::numbers::pi * x) / (std::numbers::pi * x);
            // Blackman-Harris window for a deep stopband.
            const double t = static_cast<double>(n) / static_cast<double>(length - 1);
            const double window = 0.35875 - 0.48829 * std::cos(twoPi * t) + 0.14128 * std::cos(2.0 * twoPi * t) - 0.01168 * std::cos(3.0 * twoPi * t);
            const double h = 0.5 * sinc * window;
            result.kernel[static_cast<size_t>(n)] = static_cast<float>(h);
            sum += h;
        }

        // Normalise for unity DC gain.
        for (auto & c : result.kernel) {
            c = static_cast<float>(c / sum);
        }

        // Split into polyphase branches (scaled by 2 for the interpolator).
        for (size_t k = 0; k < HalfBand::Phase0Length; k++) {
            result.phase0[k] = 2.0f * result.kernel[2 * k];
        }
        for (size_t k = 0; k < HalfBand::Phase1Length; k++) {
            result.phase1[k] = 2.0f * result.kernel[2 * k + 1];
        }
        return result;
    }();
    return hb;
}

} // namespace

void Upsampler2x::process(float sample, float & out0, float & out1)
{
    m_buffer[m_writeIndex] = sample;
    m_writeIndex = (m_writeIndex + 1) % HistLength;

    const auto & hb = halfBand();
    float phase0 { 0.0f };
    float phase1 { 0.0f };
    size_t readIndex { m_writeIndex };
    for (size_t k = 0; k < HistLength; k++) {
        if (readIndex == 0) {
            readIndex = HistLength - 1;
        } else {
            readIndex--;
        }
        const float s = m_buffer[readIndex];
        phase0 += s * hb.phase0[k];
        if (k < HalfBand::Phase1Length) {
            phase1 += s * hb.phase1[k];
        }
    }

    out0 = phase0;
    out1 = phase1;
}

void Upsampler2x::reset()
{
    std::fill(m_buffer.begin(), m_buffer.end(), 0.0f);
    m_writeIndex = 0;
}

float Decimator2x::process(float s0, float s1)
{
    m_buffer[m_writeIndex] = s0;
    m_writeIndex = (m_writeIndex + 1) % HalfBandLength;
    m_buffer[m_writeIndex] = s1;
    m_writeIndex = (m_writeIndex + 1) % HalfBandLength;

    const auto & hb = halfBand();
    float output { 0.0f };
    size_t readIndex { m_writeIndex };
    for (size_t k = 0; k < static_cast<size_t>(HalfBandLength); k++) {
        if (readIndex == 0) {
            readIndex = HalfBandLength - 1;
        } else {
            readIndex--;
        }
        output += m_buffer[readIndex] * hb.kernel[k];
    }
    return output;
}

void Decimator2x::reset()
{
    std::fill(m_buffer.begin(), m_buffer.end(), 0.0f);
    m_writeIndex = 0;
}

void Upsampler::process(float sample, float * out, uint8_t factor)
{
    switch (factor) {
    case 2:
        m_stage1.process(sample, out[0], out[1]);
        break;
    case 4: {
        float a = 0.0f;
        float b = 0.0f;
        m_stage1.process(sample, a, b);
        m_stage2.process(a, out[0], out[1]);
        m_stage2.process(b, out[2], out[3]);
        break;
    }
    case 1:
    default:
        out[0] = sample;
        break;
    }
}

void Upsampler::reset()
{
    m_stage1.reset();
    m_stage2.reset();
}

float Decimator::process(const float * highRate, uint8_t factor)
{
    switch (factor) {
    case 2:
        return m_stage1.process(highRate[0], highRate[1]);
    case 4: {
        const float a = m_stage1.process(highRate[0], highRate[1]);
        const float b = m_stage1.process(highRate[2], highRate[3]);
        return m_stage2.process(a, b);
    }
    case 1:
    default:
        return highRate[0];
    }
}

void Decimator::reset()
{
    m_stage1.reset();
    m_stage2.reset();
}

} // namespace noteahead
