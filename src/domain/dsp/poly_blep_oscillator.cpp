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

#include "poly_blep_oscillator.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace {

//! Narrowest duty the Shape control reaches. Below roughly this the pulse stops being a tone with a
//! character and starts being a click: at 0.5 %, where this control used to end up, a 220 Hz pulse
//! is about one sample wide at 48 kHz.
constexpr double MinPulseWidth = 0.05;

//! Corner of the coupling that removes the pulse's offset. Low enough to leave the fundamental
//! alone, high enough that the sag is visible along the top of the wave the way it is on a scope
//! looking at an analog synth.
constexpr double PulseCouplingHz = 5.0;

//! Bandwidth of the stage the pulse comes out of, as a corner. This is what rounds the corners.
constexpr double PulseEdgeHz = 8000.0;

} // namespace

namespace noteahead {

std::vector<std::string> PolyBlepOscillator::waveformNames()
{
    return { "Triangle", "Saw", "Square", "Sine" };
}

void PolyBlepOscillator::setSampleRate(double sampleRate)
{
    if (std::abs(m_sampleRate - sampleRate) < 0.1) {
        return;
    }
    DspComponent::setSampleRate(sampleRate);
    updatePhaseStep();
}

void PolyBlepOscillator::setFrequency(double frequency)
{
    m_frequency = frequency;
    updatePhaseStep();
}

void PolyBlepOscillator::setWaveform(Waveform waveform)
{
    m_waveform = waveform;
}

void PolyBlepOscillator::setPulseWidth(double pw)
{
    m_pulseWidth = std::clamp(pw, 0.01, 0.99);
}

void PolyBlepOscillator::setShape(double shape)
{
    m_shape = std::clamp(shape, 0.0, 1.0);
}

double PolyBlepOscillator::nextSample()
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
    } else if (m_waveform == Waveform::Square) {
        double pw = m_pulseWidth;
        if (m_shape > 0.0) {
            pw = MinPulseWidth + (0.5 - MinPulseWidth) * (1.0 - m_shape);
        }
        value = (t < pw) ? 1.0 : -1.0;
        // The offset a duty other than half carries is taken off here, exactly, rather than left
        // for the coupling below to remove. The coupling would get there in the end, but only over
        // its own time constant, and a note would start with that decaying offset under it. The sag
        // the coupling gives the wave does not depend on the offset being there.
        value -= (2.0 * pw - 1.0);
        value += polyBlep(t);
        value -= polyBlep(std::fmod(t + (1.0 - pw), 1.0));
        value = analogPulse(value);

        // A pulse swings between the same two rails whatever its duty, so once the offset is gone
        // the shorter side of the wave is left standing further from zero than the longer one --
        // at a tenth of a cycle, nearly twice as far. Scaling by the taller side keeps Shape a
        // timbre control rather than a volume one, and stops a thin pulse arriving at the filter
        // some 5 dB hotter than the saw next to it.
        value *= 0.5 / std::max(pw, 1.0 - pw);
    } else if (m_waveform == Waveform::Triangle) {
        value = (t < 0.5) ? (4.0 * t - 1.0) : (3.0 - 4.0 * t);
        if (m_shape > 0.0) {
            // Triangle shaping: fold
            value *= (1.0 + m_shape * 4.0);
            while (value > 1.0 || value < -1.0) {
                if (value > 1.0) {
                    value = 2.0 - value;
                } else if (value < -1.0) {
                    value = -2.0 - value;
                }
            }
        }
    } else if (m_waveform == Waveform::Sine) {
        value = std::sin(std::numbers::pi * 2.0 * t);
        if (m_shape > 0.0) {
            // Sine shaping: fold
            value *= (1.0 + m_shape * 4.0);
            while (value > 1.0 || value < -1.0) {
                if (value > 1.0) {
                    value = 2.0 - value;
                } else if (value < -1.0) {
                    value = -2.0 - value;
                }
            }
        }
    }

    m_phase += m_phaseStep;
    if (m_phase >= 1.0) {
        m_phase -= 1.0;
    }

    return value;
}

double PolyBlepOscillator::analogPulse(double value)
{
    const double sampleRate = m_sampleRate > 0.0 ? m_sampleRate : 48000.0;

    m_pulseCoupling.calculate(PulseCouplingHz, sampleRate);
    m_pulseCoupling.process(value);
    const double coupled = m_pulseCoupling.highPass();

    m_pulseEdge.calculate(PulseEdgeHz, sampleRate);
    m_pulseEdge.process(coupled);
    return m_pulseEdge.lowPass();
}

void PolyBlepOscillator::sync(double phase)
{
    m_phase = phase;
}

void PolyBlepOscillator::reset()
{
    m_phase = 0.0;
    m_pulseCoupling.reset();
    m_pulseEdge.reset();
}

double PolyBlepOscillator::frequency() const
{
    return m_frequency;
}

PolyBlepOscillator::Waveform PolyBlepOscillator::waveform() const
{
    return m_waveform;
}

double PolyBlepOscillator::pulseWidth() const
{
    return m_pulseWidth;
}

double PolyBlepOscillator::shape() const
{
    return m_shape;
}

double PolyBlepOscillator::phase() const
{
    return m_phase;
}

double PolyBlepOscillator::polyBlep(double t) const
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

void PolyBlepOscillator::updatePhaseStep()
{
    m_phaseStep = m_frequency / m_sampleRate;
}

} // namespace noteahead
