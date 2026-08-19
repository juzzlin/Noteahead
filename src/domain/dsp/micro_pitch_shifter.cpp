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

#include "micro_pitch_shifter.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace noteahead {

namespace {

//! How far the read pointer travels before it has to jump back. Short, because the shifted signal
//! is delayed by between one and two windows and that delay is heard: at fifteen milliseconds it
//! reads as width, at fifty it reads as an echo.
constexpr double windowMs = 15.0;

//! How long the handover at each jump takes. Fixed in time rather than as a share of the window,
//! because it is the only moment at which both taps are summed, and what matters is how many
//! milliseconds that lasts rather than what fraction of a cycle it happens to be.
constexpr double crossfadeMs = 8.0;

//! Room for the interpolator to read either side of the tap.
constexpr double guardSamples = 4.0;

constexpr double centsPerOctave = 1200.0;

} // namespace

void MicroPitchShifter::setSampleRate(double sampleRate)
{
    DspComponent::setSampleRate(sampleRate);
    m_dirty = true;
}

void MicroPitchShifter::setCents(double cents)
{
    if (m_cents != cents) {
        m_cents = cents;
        m_dirty = true;
    }
}

double MicroPitchShifter::cents() const
{
    return m_cents;
}

void MicroPitchShifter::update()
{
    const double sampleRate = m_sampleRate > 0 ? m_sampleRate : 48000.0;
    if (!m_dirty && std::abs(sampleRate - m_lastSampleRate) < 0.1) {
        return;
    }

    if (std::abs(sampleRate - m_lastSampleRate) >= 0.1) {
        m_windowSamples = windowMs * 0.001 * sampleRate;
        m_crossfadeSamples = crossfadeMs * 0.001 * sampleRate;
        // The partner tap sits a whole window away, so the line has to reach past two of them.
        m_delayLine.setSampleRate(sampleRate);
        m_delayLine.setMaxDelay(static_cast<size_t>(m_windowSamples * 2.0 + guardSamples) + 1);
        m_delayLine.reset();
        m_delay = m_windowSamples * 1.5;
        m_lastSampleRate = sampleRate;
    }

    // Reading the line faster than it is written raises the pitch, which means the delay has to
    // shrink: a ratio of r is a delay that loses r - 1 samples every sample.
    const double ratio = std::pow(2.0, m_cents / centsPerOctave);
    m_rate = 1.0 - ratio;

    m_dirty = false;
}

double MicroPitchShifter::process(double input)
{
    update();

    if (m_windowSamples <= 1.0) {
        return input;
    }

    if (m_rate == 0.0) {
        // Nothing to shift. The line still runs, so that turning the control up starts from a
        // window that is already full rather than from silence.
        m_delayLine.setFractionalDelay(m_delay);
        const double dry = m_delayLine.read();
        m_delayLine.write(input);
        return dry;
    }

    // The partner tap sits a whole window away, on the side the pointer is travelling towards, so
    // that when the jump comes the new lead lands exactly where the partner already was. Summing
    // two taps of the same signal combs, and at some frequencies cancels outright, so it is done
    // only across the handover -- a few milliseconds however small the shift is, rather than the
    // whole time as an always-on crossfade would.
    const bool rising = m_rate > 0.0;
    const double distanceToJump = rising ? m_windowSamples * 2.0 - m_delay : m_delay - m_windowSamples;
    const double partner = rising ? m_delay - m_windowSamples : m_delay + m_windowSamples;

    const double fadeSpan = m_crossfadeSamples * std::abs(m_rate);
    const double blend = fadeSpan > 0.0 ? std::clamp(distanceToJump / fadeSpan, 0.0, 1.0) : 1.0;

    m_delayLine.setFractionalDelay(m_delay);
    const double lead = m_delayLine.read();

    double output = lead;
    if (blend < 1.0) {
        m_delayLine.setFractionalDelay(std::clamp(partner, 0.0, m_windowSamples * 2.0));
        const double trail = m_delayLine.read();
        // Equal power across the handover, so the level neither dips nor bumps through it.
        output = lead * std::sin(blend * std::numbers::pi / 2.0) + trail * std::cos(blend * std::numbers::pi / 2.0);
    }

    m_delayLine.write(input);

    m_delay += m_rate;
    if (rising) {
        if (m_delay >= m_windowSamples * 2.0) {
            m_delay -= m_windowSamples;
        }
    } else if (m_delay < m_windowSamples) {
        m_delay += m_windowSamples;
    }

    return output;
}

void MicroPitchShifter::reset()
{
    m_delayLine.reset();
    m_delay = m_windowSamples * 1.5;
}

} // namespace noteahead
