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

#include "lfo.hpp"

#include <algorithm>
#include <cmath>

namespace noteahead {

std::vector<std::string> Lfo::waveformNames()
{
    return { "Saw", "Triangle", "Square", "Sine", "Random" };
}

void Lfo::setSampleRate(double sampleRate)
{
    if (std::abs(m_sampleRate - sampleRate) < 0.1) {
        return;
    }
    DspComponent::setSampleRate(sampleRate);
    updatePhaseStep();
    updateEnvelopeTimes();
}

void Lfo::setFrequency(double frequency)
{
    m_frequency = frequency;
    updatePhaseStep();
}

void Lfo::setFrequency(double bpm, double syncRate)
{
    setFrequency((bpm / 60.0) * (0.25 / std::max(0.0001, syncRate)));
}

void Lfo::setWaveform(Waveform waveform)
{
    m_waveform = waveform;
}

void Lfo::setMode(Mode mode)
{
    m_mode = mode;
}

void Lfo::setDelayTime(double seconds)
{
    m_delayTime = std::max(0.0, seconds);
    updateEnvelopeTimes();
}

void Lfo::setFadeTime(double seconds)
{
    m_fadeTime = std::max(0.0, seconds);
    updateEnvelopeTimes();
}

void Lfo::setPhase(double phase)
{
    m_phase = phase;
    while (m_phase >= 1.0) {
        m_phase -= 1.0;
    }
    while (m_phase < 0.0) {
        m_phase += 1.0;
    }
}

double Lfo::phase() const
{
    return m_phase;
}

void Lfo::trigger()
{
    m_phase = 0.0;
    m_oneShotActive = true;
    m_oneShotHold = 0.0;
    m_elapsedSamples = 0;
}

void Lfo::reset()
{
    trigger();
    m_rng.seed(0);
    m_randomValue = m_dist(m_rng);
}

double Lfo::waveformValue(double phase) const
{
    switch (m_waveform) {
    case Waveform::Sine:
        return std::sin(2.0 * M_PI * phase);
    case Waveform::Saw:
        return 2.0 * phase - 1.0;
    case Waveform::Triangle:
        return (phase < 0.5) ? (4.0 * phase - 1.0) : (3.0 - 4.0 * phase);
    case Waveform::Square:
        return (phase < 0.5) ? 1.0 : -1.0;
    case Waveform::Random:
        return m_randomValue;
    }
    return 0.0;
}

double Lfo::nextSample()
{
    const auto elapsed = static_cast<double>(m_elapsedSamples);
    m_elapsedSamples++;

    if (elapsed < m_delaySamples) {
        // The phase is held rather than advanced, so a one-shot sweep happens after the delay
        // instead of being spent during it and every note starts its first cycle from the same
        // place. Zero is the value that modulates nothing, whatever the destination.
        return 0.0;
    }

    const auto fade = fadeLevel(elapsed - m_delaySamples);

    if (m_mode == Mode::OneShot && !m_oneShotActive) {
        return m_oneShotHold * fade;
    }

    double value = waveformValue(m_phase);

    m_phase += m_phaseStep;
    if (m_phase >= 1.0) {
        m_phase -= 1.0;
        if (m_mode == Mode::OneShot) {
            m_oneShotActive = false;
            // Park on the value the shape ends on rather than snapping to zero. Zero made the two
            // things a one-shot is for impossible: leaving the target somewhere other than where it
            // started, and doing so without a step — a saw jumping from +1 to 0 is an audible click
            // at the end of every note.
            m_oneShotHold = waveformValue(1.0);
            value = m_oneShotHold;
        } else if (m_waveform == Waveform::Random) {
            m_randomValue = m_dist(m_rng);
        }
    }

    return value * fade;
}

void Lfo::updatePhaseStep()
{
    m_phaseStep = m_frequency / m_sampleRate;
}

void Lfo::updateEnvelopeTimes()
{
    // The elapsed count is deliberately left alone: the devices push these times every block, so
    // re-arming here would keep restarting the delay and it would never expire.
    m_delaySamples = m_delayTime * m_sampleRate;
    m_fadeSamples = m_fadeTime * m_sampleRate;
}

double Lfo::fadeLevel(double samplesSinceDelay) const
{
    if (m_fadeSamples <= 0.0) {
        return 1.0;
    }
    return std::clamp(samplesSinceDelay / m_fadeSamples, 0.0, 1.0);
}

} // namespace noteahead
