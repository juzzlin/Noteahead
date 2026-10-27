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

#include "sweep_envelope.hpp"

#include <algorithm>
#include <cmath>

namespace noteahead {

namespace {

//! Exponent at full curve: the midpoint of a riser is then 1 / (e^3 + 1) ≈ 5 % of the way.
constexpr double MaxSteepness = 6.0;

//! Absorbs the rounding of a step summed over millions of samples, so a sweep lands on its sample.
constexpr double EndTolerance = 1e-9;

} // namespace

void SweepEnvelope::setSampleRate(double sampleRate)
{
    DspComponent::setSampleRate(sampleRate);
    updateStep();
}

void SweepEnvelope::setStartValue(double value)
{
    m_startValue = value;
}

double SweepEnvelope::startValue() const
{
    return m_startValue;
}

void SweepEnvelope::setEndValue(double value)
{
    m_endValue = value;
}

double SweepEnvelope::endValue() const
{
    return m_endValue;
}

void SweepEnvelope::setCurve(double curve)
{
    m_curve = std::clamp(curve, -1.0, 1.0);
}

double SweepEnvelope::curve() const
{
    return m_curve;
}

void SweepEnvelope::setMode(Mode mode)
{
    m_mode = mode;
}

SweepEnvelope::Mode SweepEnvelope::mode() const
{
    return m_mode;
}

void SweepEnvelope::setLengthSeconds(double seconds)
{
    m_synced = false;
    m_lengthSeconds = std::max(0.0, seconds);
    updateStep();
}

void SweepEnvelope::setLengthBeats(double beats)
{
    m_synced = true;
    m_lengthBeats = std::max(0.0, beats);
    updateStep();
}

void SweepEnvelope::setBpm(double bpm)
{
    m_bpm = std::max(1.0, bpm);
    updateStep();
}

void SweepEnvelope::updateStep()
{
    const double seconds = m_synced ? m_lengthBeats * 60.0 / m_bpm : m_lengthSeconds;
    const double samples = seconds * m_sampleRate;
    // A zero length gets there on the next sample.
    m_step = samples > 0.0 ? 1.0 / samples : 1.0;
}

void SweepEnvelope::trigger()
{
    m_progress = 0.0;
    m_direction = 1.0;
    m_running = true;
}

double SweepEnvelope::shaped(double progress) const
{
    const double steepness = m_curve * MaxSteepness;
    if (std::abs(steepness) < 1e-6) {
        return progress;
    }
    return std::expm1(steepness * progress) / std::expm1(steepness);
}

double SweepEnvelope::nextSample()
{
    const double value = m_startValue + (m_endValue - m_startValue) * shaped(m_progress);
    if (!m_running) {
        return value;
    }

    m_progress += m_direction * m_step;
    if (m_progress >= 1.0 - EndTolerance) {
        switch (m_mode) {
        case Mode::Hold:
            m_progress = 1.0;
            m_running = false;
            break;
        case Mode::Loop:
            m_progress = std::max(0.0, m_progress - 1.0);
            break;
        case Mode::PingPong:
            m_progress = std::clamp(2.0 - m_progress, 0.0, 1.0);
            m_direction = -1.0;
            break;
        }
    } else if (m_progress <= EndTolerance && m_direction < 0.0) {
        m_progress = std::clamp(-m_progress, 0.0, 1.0);
        m_direction = 1.0;
    }
    return value;
}

bool SweepEnvelope::isRunning() const
{
    return m_running;
}

double SweepEnvelope::progress() const
{
    return m_progress;
}

} // namespace noteahead
