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

#include "transistor_ladder_filter.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace noteahead {

void TransistorLadderFilter::setMode(Mode mode)
{
    m_mode = mode;
}

TransistorLadderFilter::Mode TransistorLadderFilter::mode() const
{
    return m_mode;
}

void TransistorLadderFilter::setCutoff(double frequency)
{
    m_cutoff = frequency;
}

void TransistorLadderFilter::setResonance(double resonance)
{
    m_feedback = std::clamp(resonance, 0.0, 1.0) * MaxFeedback;
}

double TransistorLadderFilter::process(double input)
{
    if (m_cutoff != m_lastCutoff || m_sampleRate != m_lastSampleRate) {
        const double sampleRate = m_sampleRate > 0.0 ? m_sampleRate : 48000.0;
        const double cutoff = std::clamp(m_cutoff, 5.0, sampleRate * 0.45);
        m_g = std::tan(std::numbers::pi * cutoff / sampleRate);
        m_stageGain = m_g / (1.0 + m_g);
        m_lastCutoff = m_cutoff;
        m_lastSampleRate = m_sampleRate;
    }

    const double G = m_stageGain;
    const double stateShare = 1.0 / (1.0 + m_g);

    // Each stage is y = G x + s / (1 + g), so the last one is G^4 times the ladder's input plus what
    // the four states contribute. Solving the loop for its input takes no delay at all.
    const double fromStates = (G * G * G * m_state[0] + G * G * m_state[1] + G * m_state[2] + m_state[3]) * stateShare;
    const double ladderInput = std::tanh((input - m_feedback * fromStates) / (1.0 + m_feedback * G * G * G * G));

    // Every stage's pair rounds off what it is given. Small signals pass through it untouched, so the
    // loop solved above still holds for them.
    std::array<double, 4> outputs {};
    double value = ladderInput;
    for (size_t i = 0; i < m_state.size(); i++) {
        const double v = (std::tanh(value) - m_state[i]) * G;
        value = v + m_state[i];
        m_state[i] = value + v;
        outputs[i] = value;
    }

    if (m_mode == Mode::HighPass) {
        // (1 - H)^4: four one-pole high-passes out of the four low-passes and what went into them
        return ladderInput - 4.0 * outputs[0] + 6.0 * outputs[1] - 4.0 * outputs[2] + outputs[3];
    }
    return outputs[3];
}

void TransistorLadderFilter::reset()
{
    m_state.fill(0.0);
}

} // namespace noteahead
