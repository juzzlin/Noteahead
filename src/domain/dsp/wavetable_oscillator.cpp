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

#include "wavetable_oscillator.hpp"

#include <algorithm>
#include <cmath>

namespace noteahead {

void WavetableOscillator::setSampleRate(double sampleRate)
{
    DspComponent::setSampleRate(sampleRate);
    updatePhaseStep();
    updatePositionCoeff();
}

void WavetableOscillator::setFrequency(double frequency)
{
    m_frequency = frequency;
    updatePhaseStep();
}

void WavetableOscillator::setPosition(double position)
{
    m_targetPosition = std::clamp(position, 0.0, 1.0);

    if (m_snapPosition) {
        m_position = m_targetPosition;
        m_snapPosition = false;
    }
}

void WavetableOscillator::snapPosition()
{
    m_snapPosition = true;
}

void WavetableOscillator::setWavetable(Wavetable::WavetableCS wavetable)
{
    m_wavetable = std::move(wavetable);
}

double WavetableOscillator::nextSample()
{
    if (!m_wavetable) {
        return 0.0;
    }

    // A step in the morph position steps the output sample with it: the same phase reads a
    // different waveform, and that jump is an audible click. Gliding the position keeps the
    // waveform continuous no matter how abruptly the modulation source moves.
    m_position += (m_targetPosition - m_position) * m_positionCoeff;

    const double sample = m_wavetable->getSample(m_phase, m_position, m_frequency, m_sampleRate);

    m_phase += m_phaseStep;
    if (m_phase >= 1.0) {
        m_phase -= 1.0;
    }

    return sample;
}

void WavetableOscillator::sync(double phase)
{
    m_phase = std::fmod(phase, 1.0);
}

double WavetableOscillator::frequency() const
{
    return m_frequency;
}

double WavetableOscillator::position() const
{
    return m_position;
}

double WavetableOscillator::targetPosition() const
{
    return m_targetPosition;
}

double WavetableOscillator::phase() const
{
    return m_phase;
}

void WavetableOscillator::updatePhaseStep()
{
    if (m_sampleRate > 0.0) {
        m_phaseStep = m_frequency / m_sampleRate;
    } else {
        m_phaseStep = 0.0;
    }
}

void WavetableOscillator::updatePositionCoeff()
{
    if (m_sampleRate > 0.0) {
        m_positionCoeff = 1.0 - std::exp(-1.0 / (PositionSmoothingSeconds * m_sampleRate));
    } else {
        m_positionCoeff = 1.0;
    }
}

} // namespace noteahead
