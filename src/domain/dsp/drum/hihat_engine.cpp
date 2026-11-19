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

#include "hihat_engine.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace noteahead {

namespace {

constexpr float Rd9TuneCenter { 0.65f };
constexpr float Rd9HpfCutoff { 0.64f };
constexpr float Rd9HpfTuneRange { 0.25f };
constexpr float Rd9LpfCutoff { 0.945f };
constexpr float Rd9LpfTuneRange { 0.08f };
constexpr float Rd9BodyCutoff { 0.48f };
constexpr float Rd9BodyTuneRange { 0.18f };

//! 24 inharmonic partials spanning the RD-9 hat spectrum (1.6 to 11.8 kHz) at the default Tune.
constexpr std::array<double, 24> Rd9PartialFreqs {
    1643.0, 2462.0, 3442.0, 3967.0, 4175.0, 4535.0, 4758.0, 5278.0,
    5607.0, 6038.0, 6257.0, 6675.0, 7010.0, 7189.0, 7582.0, 7911.0,
    8379.0, 8986.0, 9122.0, 9476.0, 10288.0, 10458.0, 11163.0, 11752.0
};

} // namespace

HiHatEngine::HiHatEngine()
{
    m_rng.seed(0);
    m_filter.setMode(CascadedSvf::Mode::HighPass);
    m_bodyFilter.setMode(CascadedSvf::Mode::BandPass);
    m_lpf.setMode(CascadedSvf::Mode::LowPass);
    m_lpf.setOrder(2);
}

void HiHatEngine::setVoicing(Voicing voicing)
{
    m_voicing = voicing;
}

void HiHatEngine::trigger(float velocity)
{
    updateRates();
    m_velocity = velocity;
    m_ampEnv = 1.0f;
    m_attackEnv = 0.0f;
    m_bodyEnv = 1.0f;
    m_active = true;
    m_choking = false;
    m_filter.reset();
    m_bodyFilter.reset();
    m_lpf.reset();
    m_metallicBank.reset();
    m_noiseBank.reset();
    for (size_t i = 0; i < m_phases.size(); i++) {
        m_phases[i] = m_voicing == Voicing::Rd9 ? std::fmod(static_cast<double>(i) * 0.61803398875, 1.0) : 0.0;
    }

    const double sr { sampleRate() };
    m_bodyFilter.setSampleRate(sr);
    m_filter.setSampleRate(sr);
    m_lpf.setSampleRate(sr);

    if (m_voicing == Voicing::Rd9) {
        const float tuneOffset = m_tune - Rd9TuneCenter;
        m_filter.setOrder(2);
        m_filter.setCutoff(std::clamp(Rd9HpfCutoff + tuneOffset * Rd9HpfTuneRange, 0.35f, 0.88f));
        m_filter.setResonance(m_resonance * 0.35f);

        m_lpf.setOrder(2);
        m_lpf.setCutoff(std::clamp(Rd9LpfCutoff + tuneOffset * Rd9LpfTuneRange, 0.80f, 0.98f));
        m_lpf.setResonance(0.15f);

        m_bodyFilter.setOrder(2);
        m_bodyFilter.setCutoff(std::clamp(Rd9BodyCutoff + tuneOffset * Rd9BodyTuneRange, 0.25f, 0.70f));
        m_bodyFilter.setResonance(0.35f);
    } else {
        m_filter.setOrder(4);
        m_bodyFilter.setOrder(4);
        m_bodyFilter.setCutoff(0.28f + m_tune * 0.15f);
        m_bodyFilter.setResonance(0.5f);

        m_filter.setCutoff(0.68f + m_tune * 0.2f);
        m_filter.setResonance(m_resonance * 0.35f);
    }
}

float HiHatEngine::nextMetallicBaseSample()
{
    const double invSr = 1.0 / baseSampleRate();
    if (m_voicing == Voicing::Rd9) {
        const double pitchScale = std::pow(2.0, static_cast<double>(m_tune - Rd9TuneCenter) * 1.2);
        const double highest = baseSampleRate() * 0.45;
        double metallicSource = 0.0;
        for (size_t i = 0; i < Rd9PartialFreqs.size(); i++) {
            const double freq = Rd9PartialFreqs[i] * pitchScale;
            if (freq >= highest) {
                break;
            }
            m_phases[i] += freq * invSr;
            if (m_phases[i] >= 1.0) {
                m_phases[i] -= 1.0;
            }
            const double weight = Rd9PartialFreqs[i] < 4000.0 ? 0.65 : 1.0;
            metallicSource += weight * std::sin(2.0 * std::numbers::pi * m_phases[i]);
        }
        return static_cast<float>(metallicSource * 0.15);
    }

    const double baseFreq { 450.0 + m_tune * 750.0 };
    static constexpr std::array<double, 6> ratios { 1.0, 1.47, 1.91, 2.51, 3.39, 4.21 };

    double metallicSource = 0.0;
    for (size_t i = 0; i < 6; i++) {
        m_phases[i] += baseFreq * ratios[i] * invSr;
        if (m_phases[i] >= 1.0)
            m_phases[i] -= 1.0;
        metallicSource += (m_phases[i] < 0.5 ? 1.0 : -1.0);
    }
    return static_cast<float>(metallicSource / 6.0);
}

float HiHatEngine::nextSample()
{
    if (!m_active) {
        return 0.0f;
    }

    const double sr { sampleRate() };
    if (sr != m_lastSampleRate) {
        updateRates();
        m_bodyFilter.setSampleRate(sr);
        m_filter.setSampleRate(sr);
        m_lpf.setSampleRate(sr);
    }

    m_noiseBank.setOversampleFactor(oversampleFactor());
    if (m_noiseBank.needsBaseSample()) {
        m_noiseBank.setBaseSample(m_dist(m_rng));
    }
    const float noise { m_noiseBank.nextSample() };

    // Metallic part: 6 square wave oscillators with ratios (808-style), generated at the base rate
    // and interpolated up so the bank sounds the same at every oversampling factor. See BaseRateSource.
    m_metallicBank.setOversampleFactor(oversampleFactor());
    if (m_metallicBank.needsBaseSample()) {
        m_metallicBank.setBaseSample(nextMetallicBaseSample());
    }
    const double metallicSource { m_metallicBank.nextSample() };

    float out { 0.0f };
    if (m_voicing == Voicing::Rd9) {
        // Metallic sine cluster + noise: 909-style shimmer with smooth high-end roll-off after saturation
        const float source = static_cast<float>(metallicSource) * 0.42f + noise * 0.58f;
        const float bodyGain = 0.48f + 0.25f * m_bodyEnv;
        const float bodyOut = static_cast<float>(m_bodyFilter.process(noise)) * bodyGain;
        const float hpOut = static_cast<float>(m_filter.process(source));
        const float saturated = std::tanh((hpOut + bodyOut) * 1.05f);
        const float filtered = static_cast<float>(m_lpf.process(saturated));
        out = filtered * m_ampEnv * m_attackEnv * m_velocity * 1.12f;
    } else {
        // Blend: metallic core with noise - more noise for 909-style character
        float source = static_cast<float>(metallicSource) * 0.7f + noise * 0.6f;

        // Body component: metallic + noise-based impact for "thickness"
        // Body is more prominent if decay is long (Open Hat)
        const float bodyGain = 0.85f * std::min(1.0f, m_decay * 2.0f);
        const float bodyOut = m_bodyFilter.process(source) * m_bodyEnv * bodyGain;

        // Sum and saturate for "body" and warmth (909-style)
        float mixed = (m_filter.process(source) + bodyOut);
        out = std::tanh(mixed * 1.4f) * m_ampEnv * m_attackEnv * m_velocity * 1.1f;
    }

    const float attackRate { 1.0f / (0.0005f * static_cast<float>(sampleRate())) };
    m_attackEnv = std::min(1.0f, m_attackEnv + attackRate);

    // Body decay is slightly longer to provide more "meat"
    m_bodyEnv *= m_bodyDecayRate;
    m_ampEnv *= m_choking ? m_chokeDecayRate : m_decayRate;

    if (m_ampEnv < AmplitudeThreshold) {
        m_active = false;
        m_choking = false;
        m_ampEnv = 0.0f;
    }

    return out;
}

bool HiHatEngine::isActive() const
{
    return m_active;
}

void HiHatEngine::reset()
{
    m_rng.seed(RngSeed);
    m_active = false;
    m_choking = false;
    m_ampEnv = 0.0f;
    m_filter.reset();
    m_bodyFilter.reset();
    m_lpf.reset();
}

void HiHatEngine::setTune(float tune)
{
    m_tune = tune;
    if (m_voicing == Voicing::Rd9) {
        const float tuneOffset = m_tune - Rd9TuneCenter;
        m_filter.setCutoff(std::clamp(Rd9HpfCutoff + tuneOffset * Rd9HpfTuneRange, 0.35f, 0.88f));
        m_lpf.setCutoff(std::clamp(Rd9LpfCutoff + tuneOffset * Rd9LpfTuneRange, 0.80f, 0.98f));
        m_bodyFilter.setCutoff(std::clamp(Rd9BodyCutoff + tuneOffset * Rd9BodyTuneRange, 0.25f, 0.70f));
    } else {
        m_bodyFilter.setCutoff(0.28f + m_tune * 0.15f);
        m_filter.setCutoff(0.68f + m_tune * 0.2f);
    }
}

void HiHatEngine::setDecay(float decay)
{
    m_decay = decay;
    updateRates();
}

void HiHatEngine::setResonance(float resonance)
{
    m_resonance = resonance;
    m_filter.setResonance(m_resonance * 0.35f);
}

void HiHatEngine::stop()
{
    m_choking = true;
}

void HiHatEngine::updateRates()
{
    if (const double sr = sampleRate(); sr > 0) {
        m_lastSampleRate = sr;
        m_bodyDecayRate = 1.0f - (1.0f / (0.06f * static_cast<float>(sr)));
        m_decayRate = 1.0f - (1.0f / (std::max(0.001f, m_decay) * 0.18f * static_cast<float>(sr)));
        m_chokeDecayRate = 1.0f - (1.0f / (ChokeFadeSeconds * static_cast<float>(sr)));
    }
}

} // namespace noteahead
