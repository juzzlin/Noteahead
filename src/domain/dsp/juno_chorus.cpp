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

#include "juno_chorus.hpp"

#include <algorithm>
#include <cmath>

namespace noteahead {

namespace {

//! Chorus I and II sweep the same range, I+II a narrow band around its middle.
constexpr double WideMinDelayMs { 1.66 };
constexpr double WideMaxDelayMs { 5.35 };
constexpr double NarrowMinDelayMs { 3.3 };
constexpr double NarrowMaxDelayMs { 3.7 };

constexpr double RateIHz { 0.513 };
constexpr double RateIIHz { 0.863 };
constexpr double RateBothHz { 9.75 };

//! Where the bucket brigades' filters roll the delayed signal off.
constexpr double DelayLowPassHz { 9000.0 };

//! Dry plus a delayed copy is some 3 dB up on dry alone, since the copy is mostly uncorrelated with
//! it. Taken back off so that switching the chorus on changes the sound and not the level.
constexpr double WetPairGain { 0.70710678118654752 };

//! Longest delay any mode reaches, with room for the interpolator's taps.
constexpr double BufferMs { 8.0 };

} // namespace

JunoChorus::JunoChorus()
{
    JunoChorus::setSampleRate(44100.0);
}

void JunoChorus::setSampleRate(double sampleRate)
{
    if (sampleRate == m_sampleRate && !m_buffer.empty()) {
        return;
    }
    DspComponent::setSampleRate(sampleRate);
    m_buffer.assign(static_cast<size_t>(std::ceil(BufferMs * 0.001 * sampleRate)) + 4, 0.0);
    m_writePos = 0;
    for (auto && filter : m_lowPassL) {
        filter.calculate(DelayLowPassHz, sampleRate);
    }
    for (auto && filter : m_lowPassR) {
        filter.calculate(DelayLowPassHz, sampleRate);
    }
}

void JunoChorus::setMode(Mode mode)
{
    m_mode = mode;
}

JunoChorus::Mode JunoChorus::mode() const
{
    return m_mode;
}

double JunoChorus::minDelayMs(Mode mode)
{
    return mode == Mode::Both ? NarrowMinDelayMs : WideMinDelayMs;
}

double JunoChorus::maxDelayMs(Mode mode)
{
    return mode == Mode::Both ? NarrowMaxDelayMs : WideMaxDelayMs;
}

double JunoChorus::rateHz(Mode mode)
{
    switch (mode) {
    case Mode::I:
        return RateIHz;
    case Mode::II:
        return RateIIHz;
    case Mode::Both:
        return RateBothHz;
    case Mode::Off:
        break;
    }
    return 0.0;
}

std::pair<double, double> JunoChorus::currentDelaysMs() const
{
    return m_currentDelaysMs;
}

double JunoChorus::readDelayed(double delaySamples) const
{
    // Cubic Hermite between the four samples around the read point: linear interpolation is itself
    // a low pass that moves with the fractional delay, which would make the sweep audible as a
    // flutter in the treble on top of the pitch movement the chorus is for.
    const auto size = static_cast<double>(m_buffer.size());
    double readPos = static_cast<double>(m_writePos) - delaySamples;
    while (readPos < 0.0) {
        readPos += size;
    }
    const auto index = static_cast<size_t>(readPos);
    const double fraction = readPos - static_cast<double>(index);
    const auto at = [this](size_t i) { return m_buffer[i % m_buffer.size()]; };
    const double xm1 = at(index + m_buffer.size() - 1);
    const double x0 = at(index);
    const double x1 = at(index + 1);
    const double x2 = at(index + 2);
    const double c1 = 0.5 * (x1 - xm1);
    const double c2 = xm1 - 2.5 * x0 + 2.0 * x1 - 0.5 * x2;
    const double c3 = 0.5 * (x2 - xm1) + 1.5 * (x0 - x1);
    return ((c3 * fraction + c2) * fraction + c1) * fraction + x0;
}

void JunoChorus::process(double input, double & left, double & right)
{
    // Written even while off, so that switching on starts from the signal that is playing rather
    // than from whatever was in the line when it was last used.
    m_buffer[m_writePos] = input;

    if (m_mode == Mode::Off) {
        left = input;
        right = input;
        m_currentDelaysMs = { 0.0, 0.0 };
    } else {
        // A triangle, -1 to 1. The right-hand delay runs on its inverse.
        const double triangle = 4.0 * std::abs(m_lfoPhase - 0.5) - 1.0;
        m_lfoPhase += rateHz(m_mode) / m_sampleRate;
        if (m_lfoPhase >= 1.0) {
            m_lfoPhase -= 1.0;
        }

        const double centreMs = 0.5 * (minDelayMs(m_mode) + maxDelayMs(m_mode));
        const double sweepMs = 0.5 * (maxDelayMs(m_mode) - minDelayMs(m_mode));
        const double leftMs = centreMs + sweepMs * triangle;
        const double rightMs = centreMs - sweepMs * triangle;
        m_currentDelaysMs = { leftMs, rightMs };

        double wetL = readDelayed(leftMs * 0.001 * m_sampleRate);
        double wetR = readDelayed(rightMs * 0.001 * m_sampleRate);
        for (auto && filter : m_lowPassL) {
            filter.process(wetL);
            wetL = filter.lowPass();
        }
        for (auto && filter : m_lowPassR) {
            filter.process(wetR);
            wetR = filter.lowPass();
        }

        left = (input + wetL) * WetPairGain;
        right = (input + wetR) * WetPairGain;
    }

    m_writePos = (m_writePos + 1) % m_buffer.size();
}

void JunoChorus::reset()
{
    std::fill(m_buffer.begin(), m_buffer.end(), 0.0);
    m_writePos = 0;
    m_lfoPhase = 0.0;
    m_currentDelaysMs = { 0.0, 0.0 };
    for (auto && filter : m_lowPassL) {
        filter.reset();
    }
    for (auto && filter : m_lowPassR) {
        filter.reset();
    }
}

} // namespace noteahead
