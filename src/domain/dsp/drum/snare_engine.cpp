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

#include "snare_engine.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace noteahead {

SnareEngine::SnareEngine()
{
    m_rng.seed(std::random_device{}());
    m_noiseFilter.setMode(CascadedSvf::Mode::BandPass);
}

void SnareEngine::trigger(float velocity)
{
    m_velocity = velocity;
    m_ampEnv = 1.0f;
    m_tonalEnv = 1.0f;
    m_pitchEnv = 1.0f;
    m_tonalPhase1 = 0.0;
    m_tonalPhase2 = 0.0;
    m_active = true;
    m_noiseFilter.reset();
}

float SnareEngine::nextSample()
{
    if (!m_active) {
        return 0.0f;
    }

    // Pitch envelope
    const float pitchEnvDecay { 1.0f - (1.0f / (0.01f * static_cast<float>(sampleRate()))) };
    m_pitchEnv *= pitchEnvDecay;
    const double pitchMod { 1.0 + m_pitchEnv * 1.5 };

    // Tonal part (Body bump 150-600 Hz)
    const double tonalFreq1 { (150.0 + (m_tune * 250.0)) * pitchMod };
    const double tonalPhaseStep1 { tonalFreq1 / sampleRate() };
    m_tonalPhase1 += tonalPhaseStep1;
    if (m_tonalPhase1 >= 1.0) {
        m_tonalPhase1 -= 1.0;
    }
    const float tonal1 { static_cast<float>(std::sin(m_tonalPhase1 * 2.0 * std::numbers::pi)) };

    const double tonalFreq2 { tonalFreq1 * 1.63 };
    const double tonalPhaseStep2 { tonalFreq2 / sampleRate() };
    m_tonalPhase2 += tonalPhaseStep2;
    if (m_tonalPhase2 >= 1.0) {
        m_tonalPhase2 -= 1.0;
    }
    const float tonal2 { static_cast<float>(std::sin(m_tonalPhase2 * 2.0 * std::numbers::pi)) };

    // Mix and saturate the tonal part slightly to make it less "sine-like"
    float tonal { (tonal1 + tonal2 * 0.4f) / 1.4f };
    tonal = std::tanh(tonal * 1.5f); 

    // Noise part (Snappy bump 2000-12000 Hz)
    const float noise { m_dist(m_rng) };
    m_noiseFilter.setSampleRate(sampleRate());
    m_noiseFilter.setCutoff(0.65f + m_tone * 0.3f);
    m_noiseFilter.setResonance(0.3f);
    const float filteredNoise { m_noiseFilter.process(noise) };

    const float out { (tonal * m_tonalEnv * (1.0f - m_snappy) * 0.8f + filteredNoise * m_snappy * 2.5f) * m_ampEnv * m_velocity };

    // Separate decay for tonal part (much faster than noise)
    const float tonalDecayRate { 1.0f - (1.0f / (0.04f * static_cast<float>(sampleRate()))) };
    m_tonalEnv *= tonalDecayRate;

    const float decayRate { 1.0f - (1.0f / (std::max(0.001f, m_decay) * 0.3f * static_cast<float>(sampleRate()))) };
    m_ampEnv *= decayRate;

    if (m_ampEnv < AmplitudeThreshold) {
        m_active = false;
        m_ampEnv = 0.0f;
        m_tonalEnv = 0.0f;
    }

    return out;
}

bool SnareEngine::isActive() const
{
    return m_active;
}

void SnareEngine::reset()
{
    m_active = false;
    m_ampEnv = 0.0f;
}

void SnareEngine::setTune(float tune)
{
    m_tune = tune;
}

void SnareEngine::setDecay(float decay)
{
    m_decay = decay;
}

void SnareEngine::setSnappy(float snappy)
{
    m_snappy = snappy;
}

void SnareEngine::setTone(float tone)
{
    m_tone = tone;
}

} // namespace noteahead
