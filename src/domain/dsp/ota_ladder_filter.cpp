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

#include "ota_ladder_filter.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace noteahead {

void OtaLadderFilter::setCutoff(double frequency)
{
    m_cutoff = frequency;
}

void OtaLadderFilter::setResonance(double resonance)
{
    // Most of the control is the climb towards the edge, as on the Juno, rather than a straight line
    // that only reaches it in the last few percent: oscillation starts about seven eighths of the way.
    const double distance = 1.0 - std::clamp(resonance, 0.0, 1.0);
    m_feedback = (1.0 - distance * std::sqrt(distance)) * MaxFeedback;
}

double OtaLadderFilter::process(double input)
{
    if (m_cutoff != m_lastCutoff || m_sampleRate != m_lastSampleRate) {
        const double sampleRate = m_sampleRate > 0.0 ? m_sampleRate : 48000.0;
        const double cutoff = std::clamp(m_cutoff, 10.0, sampleRate * 0.45);
        m_g = std::tan(std::numbers::pi * cutoff / sampleRate);
        m_stageGain = m_g / (1.0 + m_g);
        m_lastCutoff = m_cutoff;
        m_lastSampleRate = m_sampleRate;
    }

    // Each stage is y = G x + s / (1 + g), so the last one is G^4 times the ladder's input plus
    // what the four states contribute. Solving the loop for its input takes no delay at all.
    const double G = m_stageGain;
    const double stateShare = 1.0 / (1.0 + m_g);
    const double fromStates = (G * G * G * m_state[0] + G * G * m_state[1] + G * m_state[2] + m_state[3]) * stateShare;
    const double compensated = input * (1.0 + BassCompensation * m_feedback);
    const double ladderInput = std::tanh((compensated - m_feedback * fromStates) / (1.0 + m_feedback * G * G * G * G));

    double value = ladderInput;
    for (auto && state : m_state) {
        const double v = (value - state) * G;
        value = v + state;
        state = value + v;
    }
    return value;
}

void OtaLadderFilter::reset()
{
    m_state.fill(0.0);
}

} // namespace noteahead
