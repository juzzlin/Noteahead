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

namespace {

//! What the Rd9 voicing is made of, fitted to a recording of the hardware rather than chosen.
//!
//! The fit minimised a distance with two halves: the spectral envelope in third-octave bands, and
//! how tonal the sound is inside each octave taken separately. Both halves are needed. Matching
//! only the envelope lets a wash of noise pass for a cymbal, and tonality measured across a wide
//! band mostly reports the spectral tilt rather than the noisiness -- which is how an earlier
//! attempt at this drove the crash to a quarter of the recording's noise while its own number
//! said it was matching.
constexpr float Rd9MetalLevel { 0.525f };
constexpr float Rd9WashLevel { 0.066f };
constexpr float Rd9SizzleLevel { 0.300f };
//! Nearly one: the recording's attack is broadband, and a crash that is struck tonally has no
//! splash to it.
constexpr float Rd9StrikeLevel { 0.941f };
//! Negative, so the partial weights lean back down towards the fundamental: the recording's weight
//! sits at four to eight kilohertz, and the top of the bank overshot it.
constexpr double Rd9MetalTilt { -0.178 };
constexpr float Rd9HpfCutoff { 0.375f };
constexpr float Rd9BpfCutoff { 0.569f };
constexpr float Rd9LpfCutoff { 0.88f };
constexpr double Rd9BaseFreq { 450.0 };

} // namespace

CrashEngine::CrashEngine()
{
    m_rng.seed(0);
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
    m_stopping = false;
    m_pitchEnv = 1.0f;
    m_sizzleEnv = 1.0f;
    m_bodyEnv = 1.0f;
    m_attackEnv = 0.0f;
    m_hpf.reset();
    m_bpf.reset();
    m_lpf.reset();
    m_bodyFilter.reset();
    m_wobblePhase = 0.0;
    m_metallicBank.reset();
    m_noiseBank.reset();
    for (auto && phase : m_phases) {
        phase = 0.0;
    }
    if (m_mode == Mode::Normal) {
        m_ampEnv = 1.0f;
    } else {
        m_ampEnv = 0.0f;
    }
}

float CrashEngine::nextMetallicBaseSample(double pitchScale)
{
    const double baseFreq { ((m_voicing == Voicing::Rd9 ? Rd9BaseFreq : 350.0) + m_tune * 400.0) * pitchScale };
    static constexpr std::array<double, 12> ratios {
        1.0, 1.27, 2.11, 3.47, 4.21, 5.17, 6.39, 7.63, 8.87, 10.13, 12.39, 14.57
    };

    // The splash is the band from four to eight kilohertz, which is where the ratios from 7.63 up
    // land over this base. Summed flat they sat six decibels under the record there, and that band
    // is most of what makes a crash sound like struck metal rather than like a wash.
    static constexpr std::array<double, 12> rd9Weights {
        0.55, 0.55, 0.65, 0.75, 0.85, 0.95, 1.05, 1.45, 1.5, 1.45, 1.3, 1.15
    };

    double metallicSource = 0.0;
    double weightSum = 0.0;
    const double invSr = 1.0 / baseSampleRate();
    for (size_t i = 0; i < 12; ++i) {
        m_phases[i] += baseFreq * ratios[i] * invSr;
        if (m_phases[i] >= 1.0)
            m_phases[i] -= 1.0;
        const double weight = m_voicing == Voicing::Rd9 ? rd9Weights[i] * std::pow(ratios[i], Rd9MetalTilt) : 1.0;
        metallicSource += weight * (m_phases[i] < 0.5 ? 1.0 : -1.0);
        weightSum += weight;
    }
    return static_cast<float>(metallicSource / std::max(1.0, weightSum));
}

float CrashEngine::nextSample()
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

    // Attack envelope to soften the initial hit
    if (m_attackEnv < 1.0f) {
        const float attackTime { std::max(0.0005f, m_attack * 0.2f) };
        const float attackRate { 1.0f / (attackTime * static_cast<float>(sampleRate())) };
        m_attackEnv = std::min(1.0f, m_attackEnv + attackRate);
    }

    // Pitch envelope for the initial "hit" - used only for subtle shimmer, no "laser" sweeps
    const float pitchEnvDecay { 1.0f - (1.0f / (0.02f * static_cast<float>(sampleRate()))) };
    m_pitchEnv *= pitchEnvDecay;
    const double pitchMod = 1.0 + m_pitchEnv * 0.05;

    // Sizzle envelope for high-frequency splash
    const float sizzleDecay { 1.0f - (1.0f / (0.15f * static_cast<float>(sampleRate()))) };
    m_sizzleEnv *= sizzleDecay;

    // Body envelope for low-mid weight. Twenty milliseconds is an impact rather than a body: the
    // record carries its 200 to 600 Hz for the whole of the crash, twenty decibels above what this
    // was leaving there.
    const float bodySeconds = m_voicing == Voicing::Rd9 ? 0.25f : 0.02f;
    const float bodyDecay { 1.0f - (1.0f / (bodySeconds * static_cast<float>(sampleRate()))) };
    m_bodyEnv *= bodyDecay;

    // Wobble: Subtler modulation for character without too much "beating"
    const double wobbleFreq = 1.5 + m_tune * 2.0;
    m_wobblePhase += wobbleFreq / sr;
    if (m_wobblePhase >= 1.0)
        m_wobblePhase -= 1.0;
    const double wobble = 1.0 + 0.005 * std::sin(m_wobblePhase * 2.0 * std::numbers::pi);

    // Body component: noise-based impact for "weight" (OHH approach)
    // Centered around the 909's 568Hz peak
    m_bodyFilter.setSampleRate(sr);
    m_bodyFilter.setCutoff(0.35f + m_tune * 0.2f);
    m_bodyFilter.setResonance(0.4f);

    // Body is more prominent if decay is long (OHH approach)
    const float bodyGain = (m_voicing == Voicing::Rd9 ? 0.7f : 0.6f) * std::min(1.0f, m_decay * 2.0f);
    const float bodySource = m_bodyFilter.process(noise) * m_bodyEnv * bodyGain;

    // Metallic part: 12 square wave oscillators with ratios tuned for 2kHz-8kHz clusters, generated
    // at the base rate and interpolated up so the bank sounds the same at every oversampling
    // factor. See BaseRateSource.
    m_metallicBank.setOversampleFactor(oversampleFactor());
    if (m_metallicBank.needsBaseSample()) {
        m_metallicBank.setBaseSample(nextMetallicBaseSample(pitchMod * wobble));
    }
    const double metallicSource { m_metallicBank.nextSample() };

    // Blend: metallic core with noise "wash", "sizzle"
    // Four tenths of metal against four tenths of steady noise, plus the strike and the sizzle on
    // top, left the top two octaves measuring flatness 0.60 where the record sits at 0.37. That is
    // the difference between a cymbal and a wash of noise shaped like one.
    const bool rd9 = m_voicing == Voicing::Rd9;
    const float metalLevel = rd9 ? Rd9MetalLevel : 0.4f;
    const float washLevel = rd9 ? Rd9WashLevel : 0.4f;
    const float sizzleLevel = rd9 ? Rd9SizzleLevel : 0.5f;
    // The record is metal from the first sample: measured over its attack it is markedly more
    // tonal than this was, and a strike made of noise is most of why.
    const float strikeNoise = noise * m_pitchEnv * (rd9 ? Rd9StrikeLevel : 0.6f);
    const float sizzleNoise = noise * m_sizzleEnv * sizzleLevel;
    float source = (static_cast<float>(metallicSource) * metalLevel + noise * washLevel + strikeNoise + sizzleNoise) * m_attackEnv;

    // Triple filtering to shape the spectral profile
    m_hpf.setSampleRate(sr);
    m_hpf.setCutoff((rd9 ? Rd9HpfCutoff : 0.35f) + m_tune * 0.4f);
    m_hpf.setResonance(m_resonance * 0.2f);

    m_bpf.setSampleRate(sr);
    m_bpf.setCutoff((rd9 ? Rd9BpfCutoff : 0.45f) + m_tune * 0.5f);
    m_bpf.setResonance(0.5f);

    m_lpf.setSampleRate(sr);
    // Higher for the fitted voicing: the record still has real weight in its top octave, and at
    // 0.85 the splash was being rolled off five decibels below it.
    m_lpf.setCutoff(rd9 ? Rd9LpfCutoff : 0.85f); // 12kHz roll-off
    m_lpf.setResonance(0.1f);

    const auto hpfOut = static_cast<float>(m_hpf.process(source));
    const auto bpfOut = static_cast<float>(m_bpf.process(source));
    const auto filtered = hpfOut * 0.5f + bpfOut * 0.5f;

    const auto out = static_cast<float>((m_lpf.process(filtered) + bodySource * m_attackEnv) * m_ampEnv * m_velocity);

    const float chokeDecayRate { 1.0f - (1.0f / (ChokeFadeSeconds * static_cast<float>(sampleRate()))) };
    if (m_mode == Mode::Normal) {
        // The record falls sixteen decibels over its first nine tenths of a second; this was
        // falling nine.
        const float decayScale = rd9 ? 1.1f : 2.5f;
        const float decayRate = m_stopping ? chokeDecayRate : 1.0f - (1.0f / (std::max(0.01f, m_decay) * decayScale * static_cast<float>(sampleRate())));
        m_ampEnv *= decayRate;
        if (m_ampEnv < AmplitudeThreshold) {
            m_active = false;
            m_ampEnv = 0.0f;
        }
    } else {
        if (m_stopping) {
            m_ampEnv *= chokeDecayRate;
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
    }

    return out;
}

bool CrashEngine::isActive() const
{
    return m_active;
}

void CrashEngine::reset()
{
    m_rng.seed(RngSeed);
    m_active = false;
    m_stopping = false;
    m_ampEnv = 0.0f;
}

void CrashEngine::stop()
{
    m_stopping = true;
}

void CrashEngine::setVoicing(Voicing voicing)
{
    m_voicing = voicing;
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
