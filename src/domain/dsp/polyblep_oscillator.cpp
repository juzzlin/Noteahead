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

double PolyBLEPOscillator::nextSample()
{
    double value = 0.0;
    const double t = m_phase;

    if (m_waveform == Waveform::Saw) {
        value = (2.0 * t) - 1.0;
        value -= polyBlep(t);
    } else if (m_waveform == Waveform::Pulse) {
        value = (t < m_pulseWidth) ? 1.0 : -1.0;
        value += polyBlep(t);
        value -= polyBlep(std::fmod(t + (1.0 - m_pulseWidth), 1.0));
    } else if (m_waveform == Waveform::Triangle) {
        // Triangle is integrated square wave.
        // For simplicity, let's use a non-bandlimited one first, or a better one.
        // Standard PolyBLEP triangle is harder. 
        // Let's use the standard formula and see.
        value = (t < 0.5) ? (4.0 * t - 1.0) : (3.0 - 4.0 * t);
        // Note: Triangle usually doesn't need PolyBLEP as much, but we could add it.
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
