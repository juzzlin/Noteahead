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
#include "../../common/constants.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace noteahead {

MultiEngine::MultiEngine()
  : m_rng(0)
  , m_dist(-1.0f, 1.0f)
{
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
        float cutoff = m_shape * m_shape; // Use multiplication instead of std::pow
        updateCoefficients(cutoff, 0.1f);
        return processFilter(noise, 1); // 1 = HP
    } else if (m_type == Type::Low) {
        // Low-pass [10Hz ... 21kHz]
        float cutoff = m_shape * m_shape;
        updateCoefficients(cutoff, 0.1f);
        return processFilter(noise, 0); // 0 = LP
    } else if (m_type == Type::Peak) {
        // Bandpass [110Hz ... 880Hz]
        // Map 0..1 to 110..880
        float freqHz = 110.0f + m_shape * (880.0f - 110.0f);
        // Map Hz back to 0..1 range for processFilter approx
        // log10(20000/20) = 3.0
        float cutoff = std::log10(freqHz * 0.05f) * 0.3333333333333333f;
        updateCoefficients(cutoff, 0.8f);
        return processFilter(noise, 2); // 2 = BP
    } else if (m_type == Type::Decim) {
        // Decimator [240Hz ... 48kHz]
        updateDecimRate();
        m_phase += m_decimRate;
        if (m_phase >= 1.0) {
            m_phase -= 1.0;
            m_lastSample = noise;
        }
        return m_lastSample;
    }

    return noise;
}

void MultiEngine::updateCoefficients(float cutoff, float resonance)
{
    if (std::abs(cutoff - m_lastCutoff) < 0.000001f && std::abs(resonance - m_lastResonance) < 0.000001f && std::abs(m_sampleRate - m_lastSampleRate) < 0.1) {
        return;
    }

    const double maxFreq = std::min(20000.0, m_sampleRate * 0.49);
    const double freq = 20.0 * std::exp2(static_cast<double>(cutoff) * std::log2(maxFreq / 20.0));
    m_g = std::tan(std::numbers::pi * freq / m_sampleRate);
    m_k = 2.0 * (1.0 - resonance);
    m_damping = 1.0 / (1.0 + m_g * (m_g + m_k));

    m_lastCutoff = cutoff;
    m_lastResonance = resonance;
    m_lastSampleRate = m_sampleRate;
}

void MultiEngine::updateDecimRate()
{
    if (std::abs(m_shape - m_lastDecimRateParam) < 0.000001f && m_note == m_lastNote && std::abs(m_sampleRate - m_lastSampleRate) < 0.1) {
        return;
    }

    float baseRate = 240.0f + m_shape * (static_cast<float>(Constants::defaultSampleRate()) - 240.0f);
    float trackedRate = baseRate * std::exp2((static_cast<float>(m_note) - 60.0f) / 12.0f * m_keyTrack);
    trackedRate = std::clamp(trackedRate, 240.0f, static_cast<float>(m_sampleRate));

    m_decimRate = static_cast<double>(trackedRate) / m_sampleRate;

    m_lastDecimRateParam = m_shape;
    m_lastNote = m_note;
    m_lastSampleRate = m_sampleRate;
}

float MultiEngine::processFilter(float input, int mode)
{
    const double hp = (input - (m_g + m_k) * m_s1 - m_s2) * m_damping;
    const double v1 = m_g * hp;
    const double bp = v1 + m_s1;
    m_s1 = v1 + bp;
    const double v2 = m_g * bp;
    const double lp = v2 + m_s2;
    m_s2 = v2 + lp;

    if (mode == 0)
        return static_cast<float>(lp);
    if (mode == 1)
        return static_cast<float>(hp);
    if (mode == 2)
        return static_cast<float>(bp);
    return input;
}

} // namespace noteahead
