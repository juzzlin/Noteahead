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

#include "clap_engine.hpp"

#include <algorithm>
#include <cmath>

namespace noteahead {

// Low-pass cutoff tracked by tune: fully dark and body-heavy at 0 %, wide open at 100 %
static constexpr float LowPassCutoffBase { 0.53f };
static constexpr float LowPassCutoffRange { 0.44f };

ClapEngine::ClapEngine()
{
    m_rng.seed(0);
    m_highPassFilter.setMode(CascadedSvf::Mode::HighPass);
    m_lowPassFilter.setMode(CascadedSvf::Mode::LowPass);
    m_lowPassFilter.setOrder(2); // Gentle slope keeps some of the transient snap while darkening the body
}

void ClapEngine::setVoicing(Voicing voicing)
{
    m_voicing = voicing;
}

void ClapEngine::trigger(float velocity)
{
    m_velocity = velocity;
    m_active = true;
    m_stopping = false;
    m_sampleCount = 0;
    m_tailEnv = 1.0f;
    m_attackEnv = 0.0f;
    m_noiseBank.reset();
    m_highPassFilter.reset();
    m_lowPassFilter.reset();

    const double sr { sampleRate() };
    if (m_voicing == Voicing::Rd8) {
        m_highPassFilter.setSampleRate(sr);
        m_highPassFilter.setMode(CascadedSvf::Mode::BandPass);
        m_highPassFilter.setOrder(2);
        m_highPassFilter.setCutoff(0.6363f + (m_tune - 0.5f) * 0.25f);
        m_highPassFilter.setResonance(0.7213f);

        m_lowPassFilter.setSampleRate(sr);
        m_lowPassFilter.setMode(CascadedSvf::Mode::LowPass);
        m_lowPassFilter.setOrder(2);
        m_lowPassFilter.setCutoff(0.8425f + (m_tune - 0.5f) * 0.20f);
        m_lowPassFilter.setResonance(0.2929f);
    } else {
        m_highPassFilter.setMode(CascadedSvf::Mode::HighPass);
        m_highPassFilter.setOrder(4);
        m_lowPassFilter.setMode(CascadedSvf::Mode::LowPass);
        m_lowPassFilter.setOrder(2);
    }
}

float ClapEngine::nextSample()
{
    if (!m_active) {
        return 0.0f;
    }

    const double sr { sampleRate() };
    m_noiseBank.setOversampleFactor(oversampleFactor());
    if (m_noiseBank.needsBaseSample()) {
        m_noiseBank.setBaseSample(m_dist(m_rng));
    }
    const float noise { m_noiseBank.nextSample() };

    if (m_voicing == Voicing::Rd8) {
        constexpr double BurstGapSeconds { 0.008787 };
        constexpr double BurstSeconds { 0.004245 };
        constexpr double TailStartSeconds { 0.029314 };
        constexpr double TailAttackSeconds { 0.001007 };
        constexpr double TailLevel { 1.9315 };
        constexpr double TailFast { 0.9439 };
        constexpr double TailFastSeconds { 0.015029 };
        constexpr double TailSeconds { 0.016235 };

        const double t { static_cast<double>(m_sampleCount) / sr };
        double burstAmp { 0.0 };
        for (int i { 0 }; i < 3; i++) {
            if (const double start = i * BurstGapSeconds; t >= start) {
                burstAmp += std::exp(-(t - start) / BurstSeconds);
            }
        }

        double tailAmp { 0.0 };
        if (t >= TailStartSeconds) {
            const double u { t - TailStartSeconds };
            const double decayScale { std::pow(4.0, (static_cast<double>(m_decay) - 0.5) * 2.0) };
            const double tailAttack { 1.0 - std::exp(-u / TailAttackSeconds) };
            const double fastDecay { std::exp(-u / (TailFastSeconds * decayScale)) };
            const double slowDecay { std::exp(-u / (TailSeconds * decayScale)) };
            tailAmp = static_cast<double>(m_tailEnv) * TailLevel * tailAttack * (TailFast * fastDecay + (1.0 - TailFast) * slowDecay);
            if (m_stopping) {
                const float chokeDecayRate { 1.0f - (1.0f / (ChokeFadeSeconds * static_cast<float>(sr))) };
                m_tailEnv *= chokeDecayRate;
            }
        }

        m_highPassFilter.setSampleRate(sr);
        m_highPassFilter.setCutoff(0.6363f + (m_tune - 0.5f) * 0.25f);
        m_lowPassFilter.setSampleRate(sr);
        m_lowPassFilter.setCutoff(0.8425f + (m_tune - 0.5f) * 0.20f);

        const auto filteredNoise = static_cast<float>(m_lowPassFilter.process(m_highPassFilter.process(noise)));
        const float out = filteredNoise * static_cast<float>(burstAmp + tailAmp) * m_attackEnv * m_velocity * 1.18f;

        const float attackRate { 1.0f / (0.0005f * static_cast<float>(sr)) };
        m_attackEnv = std::min(1.0f, m_attackEnv + attackRate);
        m_sampleCount++;

        if (t > TailStartSeconds + TailAttackSeconds && (burstAmp + tailAmp) < AmplitudeThreshold) {
            m_active = false;
            m_stopping = false;
        }

        return out;
    }

    float burstAmp = 0.0f;

    // Three initial bursts
    const int burstInterval = static_cast<int>(0.01f * sr);
    const int burstDuration = static_cast<int>(0.005f * sr);

    for (int i = 0; i < 3; ++i) {
        int start = i * burstInterval;
        if (m_sampleCount >= start && m_sampleCount < start + burstDuration) {
            burstAmp = 1.0f - static_cast<float>(m_sampleCount - start) / burstDuration;
            break;
        }
    }

    // Main tail starts after bursts
    float tailAmp = 0.0f;
    const int tailStart = static_cast<int>(0.03f * sr);
    if (m_sampleCount >= tailStart) {
        tailAmp = m_tailEnv;
        const float chokeDecayRate { 1.0f - (1.0f / (ChokeFadeSeconds * static_cast<float>(sr))) };
        const float decayRate = m_stopping ? chokeDecayRate : 1.0f - (1.0f / (std::max(0.01f, m_decay) * 0.2f * static_cast<float>(sr)));
        m_tailEnv *= decayRate;
    }

    m_highPassFilter.setSampleRate(sr);
    m_highPassFilter.setCutoff(0.2f + m_tune * 0.5f);
    m_highPassFilter.setResonance(0.3f);
    m_lowPassFilter.setSampleRate(sr);
    m_lowPassFilter.setCutoff(LowPassCutoffBase + m_tune * LowPassCutoffRange);
    m_lowPassFilter.setResonance(0.2f);

    const auto filteredNoise = static_cast<float>(m_lowPassFilter.process(m_highPassFilter.process(noise)));

    // Narrowing the band drops the noise energy, so compensate to keep the perceived level even across the tune range
    const float bandNarrowing { 1.0f - m_tune };
    const float bandCompensation { 1.0f + bandNarrowing * bandNarrowing * 3.0f };

    const float out = filteredNoise * bandCompensation * (burstAmp + tailAmp) * m_attackEnv * m_velocity;

    const float attackRate { 1.0f / (0.0005f * static_cast<float>(sampleRate())) };
    m_attackEnv = std::min(1.0f, m_attackEnv + attackRate);

    m_sampleCount++;

    if (m_sampleCount > tailStart && m_tailEnv < AmplitudeThreshold) {
        m_active = false;
    }

    return out;
}

bool ClapEngine::isActive() const
{
    return m_active;
}

void ClapEngine::reset()
{
    m_rng.seed(RngSeed);
    m_active = false;
    m_stopping = false;
    m_sampleCount = 0;
}

void ClapEngine::stop()
{
    m_stopping = true;
}

void ClapEngine::setTune(float tune)
{
    m_tune = tune;
}

void ClapEngine::setDecay(float decay)
{
    m_decay = decay;
}

} // namespace noteahead
