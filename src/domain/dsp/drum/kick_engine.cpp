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

#include "kick_engine.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace noteahead {

KickEngine::KickEngine()
{
    m_rng.seed(0);
    m_noiseFilter.setMode(CascadedSvf::Mode::HighPass);
}

void KickEngine::trigger(float velocity)
{
    updateRates();
    m_retriggerOffset = m_lastOut;
    m_velocity = velocity;
    m_pitchEnv = 1.0f;
    m_phase = 0.0;
    m_clickPhase = 0.0;
    m_active = true;
    m_ampEnv = 1.0f;
    m_attackEnv = 0.0f;
    m_clickEnv = 1.0f;
    m_noiseFilter.reset();
}

float KickEngine::nextSample()
{
    if (!m_active) {
        m_lastOut = 0.0f;
        return 0.0f;
    }

    const double sr = sampleRate();
    if (sr != m_lastSampleRate) {
        updateRates();
    }

    // Base Sine Oscillator
    const double baseFreq { 40.0 + (m_tune * 60.0) };
    const double sweepFreq { baseFreq + (m_pitchDepth * 400.0 * m_pitchEnv) };
    const double phaseStep { sweepFreq / sr };

    // Click Sweep Oscillator
    // Rapidly sweep from tunable start freq down to ~100Hz
    const double clickStartFreq = 1000.0 + (m_clickTune * 7000.0);
    const double clickEndFreq = 100.0;
    const double clickFreq = clickEndFreq + (clickStartFreq - clickEndFreq) * m_clickEnv * m_clickEnv * m_clickEnv;
    const double clickPhaseStep = clickFreq / sr;
    const float clickOsc { static_cast<float>(std::sin(m_clickPhase * 2.0 * std::numbers::pi)) };
    m_clickPhase += clickPhaseStep;
    if (m_clickPhase >= 1.0)
        m_clickPhase -= 1.0;

    // Click Component
    const float click { clickOsc * m_clickEnv * m_attack * 0.35f };

    // Sine Component
    const float sine { static_cast<float>(std::sin(m_phase * 2.0 * std::numbers::pi)) * 0.65f };
    m_phase += phaseStep;
    if (m_phase >= 1.0)
        m_phase -= 1.0;

    float out { (sine + click) * m_ampEnv * m_attackEnv * m_velocity };

    // Apply re-trigger offset to smooth out discontinuities
    out += m_retriggerOffset;
    m_retriggerOffset *= 0.95f;

    // Envelopes
    const float attackRate { 1.0f / (0.0005f * static_cast<float>(sr)) };
    m_attackEnv = std::min(1.0f, m_attackEnv + attackRate);
    m_ampEnv *= m_ampDecayRate;
    m_clickEnv *= m_clickDecayRate;
    m_pitchEnv *= m_pitchDecayRate;

    if (m_ampEnv < AmplitudeThreshold && std::abs(out) < AmplitudeThreshold) {
        m_active = false;
        m_ampEnv = 0.0f;
        m_retriggerOffset = 0.0f;
    }

    m_lastOut = out;
    return out;
}

bool KickEngine::isActive() const
{
    return m_active;
}

void KickEngine::reset()
{
    m_active = false;
    m_ampEnv = 0.0f;
    m_clickEnv = 0.0f;
    m_lastOut = 0.0f;
    m_retriggerOffset = 0.0f;
}

void KickEngine::setTune(float tune)
{
    m_tune = tune;
    updateRates();
}

void KickEngine::setAttack(float attack)
{
    m_attack = attack;
}

void KickEngine::setClickTune(float tune)
{
    m_clickTune = tune;
}

void KickEngine::setDecay(float decay)
{
    m_decay = decay;
    updateRates();
}

void KickEngine::setPitchDepth(float depth)
{
    m_pitchDepth = depth;
}

void KickEngine::setPitchDecay(float decay)
{
    m_pitchDecay = decay;
    updateRates();
}

void KickEngine::updateRates()
{
    if (const double sr = sampleRate(); sr > 0) {
        m_lastSampleRate = sr;
        m_ampDecayRate = 1.0f - (1.0f / (std::max(0.001f, m_decay) * 0.5f * static_cast<float>(sr)));
        m_clickDecayRate = 1.0f - (1.0f / (0.005f * static_cast<float>(sr)));
        m_pitchDecayRate = 1.0f - (1.0f / (std::max(0.001f, m_pitchDecay * 0.1f) * static_cast<float>(sr)));
    }
}

} // namespace noteahead
