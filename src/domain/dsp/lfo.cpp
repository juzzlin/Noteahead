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

#include <cmath>
#include <algorithm>

namespace noteahead {

void LFO::setSampleRate(double sampleRate)
{
    m_sampleRate = sampleRate;
    updatePhaseStep();
}

void LFO::setFrequency(double frequency)
{
    m_frequency = frequency;
    updatePhaseStep();
}

void LFO::setWaveform(Waveform waveform)
{
    m_waveform = waveform;
}

void LFO::setMode(Mode mode)
{
    m_mode = mode;
}

void LFO::reset()
{
    m_phase = 0.0;
    m_oneShotActive = true;
}

double LFO::nextSample()
{
    if (m_mode == Mode::OneShot && !m_oneShotActive) {
        return 0.0;
    }

    double value = 0.0;

    switch (m_waveform) {
    case Waveform::Saw:
        value = 2.0 * m_phase - 1.0;
        break;
    case Waveform::Triangle:
        value = (m_phase < 0.5) ? (4.0 * m_phase - 1.0) : (3.0 - 4.0 * m_phase);
        break;
    case Waveform::Square:
        value = (m_phase < 0.5) ? 1.0 : -1.0;
        break;
    }

    m_phase += m_phaseStep;
    if (m_phase >= 1.0) {
        m_phase -= 1.0;
        if (m_mode == Mode::OneShot) {
            m_oneShotActive = false;
            value = 0.0;
        }
    }

    return value;
}

void LFO::updatePhaseStep()
{
    m_phaseStep = m_frequency / m_sampleRate;
}

} // namespace noteahead
