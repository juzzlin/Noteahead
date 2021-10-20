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

#ifndef BASE_RATE_SOURCE_HPP
#define BASE_RATE_SOURCE_HPP

#include "upsampler.hpp"

#include <array>
#include <cstdint>

namespace noteahead {

//! Runs a signal generator at the base rate and interpolates it to the oversampled rate, so what it
//! produces does not depend on the oversampling factor.
//!
//! Why this and not band-limited (polyBLEP) oscillators: the problem being solved is *consistency*,
//! not aliasing. A naive oscillator's spectrum depends on the rate it runs at, because everything
//! above Nyquist folds back — but so does a polyBLEP one's, because polyBLEP is a first-order
//! correction whose accuracy scales with frequency/sampleRate. Measured on the hi-hat's bank, going
//! polyBLEP moved the 12-20 kHz band from -1.5 dB at 4x to +3.0 dB at 4x: still rate-dependent,
//! just in the other direction, and on top of that it thinned the sound by ~4.8 dB in that band by
//! removing the aliasing these voices are voiced around.
//!
//! Generating at a fixed rate sidesteps both: every factor sees the same waveform, sample for
//! sample, and the character these drum voices get from their aliasing is preserved exactly as it
//! sounds at 1x. It is also cheaper, since the bank is evaluated once per base-rate sample instead
//! of once per oversampled one.
//!
//! The interpolator's group delay (about five base-rate samples, and none at 1x) shifts the
//! interpolated part slightly against anything the caller still generates at the oversampled rate.
//! At a tenth of a millisecond that is inaudible, and it is constant.
class BaseRateSource
{
public:
    //! Idempotent: safe to call every sample, and only re-initialises when the factor changes.
    void setOversampleFactor(uint8_t factor);

    //! True when the next nextSample() call needs a fresh base-rate sample supplied first.
    //!
    //! This and the two below are defined here rather than in the .cpp because every drum voice
    //! calls them once per sample; left out of line they cost more than the work they do.
    bool needsBaseSample() const
    {
        return m_index == 0;
    }

    //! Feed the base-rate generator's output. Only meaningful right after needsBaseSample().
    void setBaseSample(float sample)
    {
        if (m_factor == 1) {
            m_interpolated[0] = sample; // Nothing to interpolate, and this is the realtime default
            return;
        }
        m_upsampler.process(sample, m_interpolated.data(), m_factor);
    }

    //! The next sample at the oversampled rate.
    float nextSample()
    {
        const float sample = m_interpolated[m_index];
        if (++m_index >= m_factor) {
            m_index = 0;
        }
        return sample;
    }

    void reset();

private:
    Upsampler m_upsampler;
    std::array<float, 4> m_interpolated {};
    uint8_t m_factor { 1 };
    uint8_t m_index { 0 };
};

} // namespace noteahead

#endif // BASE_RATE_SOURCE_HPP
