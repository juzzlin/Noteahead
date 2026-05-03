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

#include "multi_engine.hpp"

#include <cmath>
#include <algorithm>
#include <numbers>

namespace noteahead {

MultiEngine::MultiEngine()
    : m_rng(std::random_device{}())
    , m_dist(-1.0f, 1.0f)
{
}

void MultiEngine::setSampleRate(double sampleRate)
{
    m_sampleRate = sampleRate;
}

void MultiEngine::setType(Type type)
{
    if (m_type != type) {
        m_type = type;
        reset();
    }
}

void MultiEngine::setShape(float shape)
{
    m_shape = std::clamp(shape, 0.0f, 1.0f);
}

void MultiEngine::setKeyTrack(float keyTrack)
{
    m_keyTrack = std::clamp(keyTrack, 0.0f, 1.0f);
}

void MultiEngine::setNote(uint8_t note)
{
    m_note = note;
}

void MultiEngine::reset()
{
    m_s1 = 0.0;
    m_s2 = 0.0;
    m_phase = 0.0;
    m_lastSample = 0.0f;
}

float MultiEngine::nextSample()
{
    float noise = m_dist(m_rng);

    if (m_type == Type::High) {
        // High-pass [10Hz ... 21kHz]
        float cutoff = std::pow(m_shape, 2.0f); // Non-linear for better control
        return processFilter(noise, cutoff, 0.1f, 1); // 1 = HP
    } else if (m_type == Type::Low) {
        // Low-pass [10Hz ... 21kHz]
        float cutoff = std::pow(m_shape, 2.0f);
        return processFilter(noise, cutoff, 0.1f, 0); // 0 = LP
    } else if (m_type == Type::Peak) {
        // Bandpass [110Hz ... 880Hz]
        // Map 0..1 to 110..880
        float freqHz = 110.0f + m_shape * (880.0f - 110.0f);
        // Map Hz back to 0..1 range for processFilter approx
        float cutoff = std::log10(freqHz / 20.0f) / std::log10(20000.0f / 20.0f);
        return processFilter(noise, cutoff, 0.8f, 2); // 2 = BP
    } else if (m_type == Type::Decim) {
        // Decimator [240Hz ... 48kHz]
        float baseRate = 240.0f + m_shape * (48000.0f - 240.0f);
        float trackedRate = baseRate * std::pow(2.0f, (m_note - 60) / 12.0f * m_keyTrack);
        trackedRate = std::clamp(trackedRate, 240.0f, static_cast<float>(m_sampleRate));

        m_phase += trackedRate / m_sampleRate;
        if (m_phase >= 1.0) {
            m_phase -= 1.0;
            m_lastSample = noise;
        }
        return m_lastSample;
    }

    return noise;
}

float MultiEngine::processFilter(float input, float cutoff, float resonance, int mode)
{
    // Simple SVF
    const double freq = 20.0 * std::pow(std::min(20000.0, m_sampleRate * 0.49) / 20.0, cutoff);
    const double g = std::tan(std::numbers::pi * freq / m_sampleRate);
    const double k = 2.0 * (1.0 - resonance);
    const double damping = 1.0 / (1.0 + g * (g + k));

    const double hp = (input - (g + k) * m_s1 - m_s2) * damping;
    const double v1 = g * hp;
    const double bp = v1 + m_s1;
    m_s1 = v1 + bp;
    const double v2 = g * bp;
    const double lp = v2 + m_s2;
    m_s2 = v2 + lp;

    if (mode == 0) return static_cast<float>(lp);
    if (mode == 1) return static_cast<float>(hp);
    if (mode == 2) return static_cast<float>(bp);
    return input;
}

} // namespace noteahead
