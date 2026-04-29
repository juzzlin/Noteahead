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

#include "polyblep_oscillator.hpp"

#include <cmath>
#include <algorithm>

namespace noteahead {

void PolyBLEPOscillator::setSampleRate(double sampleRate)
{
    m_sampleRate = sampleRate;
    updatePhaseStep();
}

void PolyBLEPOscillator::setFrequency(double frequency)
{
    m_frequency = frequency;
    updatePhaseStep();
}

void PolyBLEPOscillator::setWaveform(Waveform waveform)
{
    m_waveform = waveform;
}

void PolyBLEPOscillator::setPulseWidth(double pw)
{
    m_pulseWidth = std::clamp(pw, 0.01, 0.99);
}

void PolyBLEPOscillator::setShape(double shape)
{
    m_shape = std::clamp(shape, 0.0, 1.0);
}

double PolyBLEPOscillator::nextSample()
{
    double value = 0.0;
    const double t = m_phase;

    if (m_waveform == Waveform::Saw) {
        value = (2.0 * t) - 1.0;
        if (m_shape > 0.0) {
            // Basic saw shaping: mix in a bit of folding
            value = (1.0 - m_shape) * value + m_shape * std::sin(std::numbers::pi * value);
        }
        value -= polyBlep(t);
    } else if (m_waveform == Waveform::Pulse) {
        // Map shape 0..1 to pulse width 0.5..0.01
        double pw = 0.5 * (1.0 - m_shape * 0.98); 
        value = (t < pw) ? 1.0 : -1.0;
        value += polyBlep(t);
        value -= polyBlep(std::fmod(t + (1.0 - pw), 1.0));
    } else if (m_waveform == Waveform::Triangle) {
        value = (t < 0.5) ? (4.0 * t - 1.0) : (3.0 - 4.0 * t);
        if (m_shape > 0.0) {
            // Basic triangle shaping: variable center point (simplified)
            double s = 0.5 + m_shape * 0.45;
            value = (t < s) ? (2.0 * t / s - 1.0) : (1.0 - 2.0 * (t - s) / (1.0 - s));
        }
    }

    m_phase += m_phaseStep;
    if (m_phase >= 1.0) {
        m_phase -= 1.0;
    }

    return value;
}

void PolyBLEPOscillator::sync(double phase)
{
    m_phase = phase;
}

double PolyBLEPOscillator::phase() const
{
    return m_phase;
}

double PolyBLEPOscillator::polyBlep(double t) const
{
    const double dt = m_phaseStep;
    // 0 <= t < 1
    if (t < dt) {
        const double normalizedT = t / dt;
        return normalizedT + normalizedT - normalizedT * normalizedT - 1.0;
    }
    // -1 < t < 0
    else if (t > 1.0 - dt) {
        const double normalizedT = (t - 1.0) / dt;
        return normalizedT * normalizedT + normalizedT + normalizedT + 1.0;
    } else {
        return 0.0;
    }
}

void PolyBLEPOscillator::updatePhaseStep()
{
    m_phaseStep = m_frequency / m_sampleRate;
}

} // namespace noteahead
