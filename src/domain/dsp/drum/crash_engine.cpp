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

#include "crash_engine.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace noteahead {

CrashEngine::CrashEngine()
{
    m_rng.seed(std::random_device{}());
    m_hpf.setMode(CascadedSvf::Mode::HighPass);
    m_bpf.setMode(CascadedSvf::Mode::BandPass);
    m_lpf.setMode(CascadedSvf::Mode::LowPass);
    m_bodyFilter.setMode(CascadedSvf::Mode::BandPass);
}

void CrashEngine::setMode(Mode mode)
{
    m_mode = mode;
}

void CrashEngine::trigger(float velocity)
{
    m_velocity = velocity;
    m_active = true;
    m_pitchEnv = 1.0f;
    m_sizzleEnv = 1.0f;
    m_bodyEnv = 1.0f;
    m_attackEnv = (m_attack > 0.0f) ? 0.0f : 1.0f;
    m_hpf.reset();
    m_bpf.reset();
    m_lpf.reset();
    m_bodyFilter.reset();
    m_wobblePhase = 0.0;
    for (auto && phase : m_phases) {
        phase = 0.0;
    }
    if (m_mode == Mode::Normal) {
        m_ampEnv = 1.0f;
    } else {
        m_ampEnv = 0.0f;
    }
}

float CrashEngine::nextSample()
{
    if (!m_active) {
        return 0.0f;
    }

    const double sr { sampleRate() };
    const float noise { m_dist(m_rng) };

    // Attack envelope to soften the initial hit
    if (m_attack > 0.0f && m_attackEnv < 1.0f) {
        const float attackRate { 1.0f / (std::max(0.001f, m_attack) * 0.2f * static_cast<float>(sampleRate())) };
        m_attackEnv += attackRate;
        if (m_attackEnv > 1.0f) m_attackEnv = 1.0f;
    }
    
    // Pitch envelope for the initial "hit" - used only for subtle shimmer, no "laser" sweeps
    const float pitchEnvDecay { 1.0f - (1.0f / (0.02f * static_cast<float>(sampleRate()))) };
    m_pitchEnv *= pitchEnvDecay;
    const double pitchMod = 1.0 + m_pitchEnv * 0.05;

    // Sizzle envelope for high-frequency splash
    const float sizzleDecay { 1.0f - (1.0f / (0.15f * static_cast<float>(sampleRate()))) };
    m_sizzleEnv *= sizzleDecay;

    // Body envelope for low-mid weight - fast decay for impact
    const float bodyDecay { 1.0f - (1.0f / (0.02f * static_cast<float>(sampleRate()))) };
    m_bodyEnv *= bodyDecay;

    // Wobble: Subtler modulation for character without too much "beating"
    const double wobbleFreq = 1.5 + m_tune * 2.0;
    m_wobblePhase += wobbleFreq / sr;
    if (m_wobblePhase >= 1.0) m_wobblePhase -= 1.0;
    const double wobble = 1.0 + 0.005 * std::sin(m_wobblePhase * 2.0 * std::numbers::pi);

    // Body component: noise-based impact for "weight" (OHH approach)
    // Centered around the 909's 568Hz peak
    m_bodyFilter.setSampleRate(sr);
    m_bodyFilter.setCutoff(0.35f + m_tune * 0.2f); 
    m_bodyFilter.setResonance(0.4f);
    
    // Body is more prominent if decay is long (OHH approach)
    const float bodyGain = 0.6f * std::min(1.0f, m_decay * 2.0f);
    const float bodySource = m_bodyFilter.process(noise) * m_bodyEnv * bodyGain;

    // Metallic part: 12 square wave oscillators with ratios tuned for 2kHz-8kHz clusters
    const double baseFreq { (350.0 + m_tune * 400.0) * pitchMod * wobble };
    static constexpr std::array<double, 12> ratios { 
        1.0, 1.27, 2.11, 3.47, 4.21, 5.17, 6.39, 7.63, 8.87, 10.13, 12.39, 14.57 
    };

    double metallicSource = 0.0;
    for (size_t i = 0; i < 12; ++i) {
        const double freq = baseFreq * ratios[i];
        m_phases[i] += freq / sr;
        if (m_phases[i] >= 1.0) m_phases[i] -= 1.0;
        metallicSource += (m_phases[i] < 0.5 ? 1.0 : -1.0);
    }
    metallicSource /= 12.0;

    // Blend: metallic core with noise "wash", "sizzle"
    const float strikeNoise = noise * m_pitchEnv * 0.6f; 
    const float sizzleNoise = noise * m_sizzleEnv * 0.5f;
    float source = (static_cast<float>(metallicSource) * 0.4f + noise * 0.4f + strikeNoise + sizzleNoise) * m_attackEnv;

    // Triple filtering to shape the spectral profile
    m_hpf.setSampleRate(sr);
    m_hpf.setCutoff(0.35f + m_tune * 0.4f); 
    m_hpf.setResonance(m_resonance * 0.2f);
    
    m_bpf.setSampleRate(sr);
    m_bpf.setCutoff(0.45f + m_tune * 0.5f);
    m_bpf.setResonance(0.5f); 

    m_lpf.setSampleRate(sr);
    m_lpf.setCutoff(0.85f); // 12kHz roll-off
    m_lpf.setResonance(0.1f);

    const float hpfOut { m_hpf.process(source) };
    const float bpfOut { m_bpf.process(source) };
    const float filtered { (hpfOut * 0.85f + bpfOut * 0.55f) };
    
    const float out { (m_lpf.process(filtered) + bodySource * m_attackEnv) * m_ampEnv * m_velocity };

    if (m_mode == Mode::Normal) {
        const float decayRate { 1.0f - (1.0f / (std::max(0.01f, m_decay) * 2.5f * static_cast<float>(sampleRate()))) };
        m_ampEnv *= decayRate;
        if (m_ampEnv < AmplitudeThreshold) {
            m_active = false;
            m_ampEnv = 0.0f;
        }
    } else {
        const float riseRate { 1.0f / (std::max(0.01f, m_decay) * 4.0f * static_cast<float>(sampleRate())) };
        m_ampEnv += riseRate;
        if (m_ampEnv >= 1.0f) {
            m_ampEnv = 1.0f;
            m_active = false; 
        }
    }

    return out;
}

bool CrashEngine::isActive() const
{
    return m_active;
}

void CrashEngine::reset()
{
    m_active = false;
    m_ampEnv = 0.0f;
}

void CrashEngine::setTune(float tune)
{
    m_tune = tune;
}

void CrashEngine::setDecay(float decay)
{
    m_decay = decay;
}

void CrashEngine::setResonance(float resonance)
{
    m_resonance = resonance;
}

void CrashEngine::setAttack(float attack)
{
    m_attack = attack;
}

} // namespace noteahead
