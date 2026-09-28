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

#include "ride_engine.hpp"

#include <algorithm>
#include <cmath>

namespace noteahead {

RideEngine::RideEngine()
{
    m_rng.seed(0);
    m_filter.setMode(CascadedSvf::Mode::HighPass);
}

void RideEngine::trigger(float velocity)
{
    m_velocity = velocity;
    m_active = true;
    m_stopping = false;
    m_filter.reset();
    m_ampEnv = 1.0f;
    m_attackEnv = 0.0f;
    m_metallicBank.reset();
    m_noiseBank.reset();
    for (auto && phase : m_phases) {
        phase = 0.0;
    }
}

float RideEngine::nextMetallicBaseSample()
{
    const double baseFreq { 300.0 + m_tune * 500.0 };
    // The first six are the cymbal this has always been. The four above them carry it up into the
    // band the record actually lives in, and are inharmonic with the rest so they read as the same
    // piece of metal rather than as a tone laid over it.
    static constexpr std::array<double, 10> ratios { 1.0, 1.48, 1.92, 2.54, 3.41, 4.23, 7.13, 11.37, 17.6, 24.9 };
    const size_t partials = m_voicing == Voicing::Rd9 ? ratios.size() : ClassicPartials;

    // Struck on the bow, a ride puts most of its energy in the high modes: the record is twelve
    // decibels stronger between four and sixteen kilohertz than it is in the octave above its
    // fundamental. Summed flat, the fundamental group drowns the rest and the cymbal reads as a
    // gong.
    static constexpr std::array<double, 10> rd9Weights { 0.22, 0.26, 0.3, 0.38, 0.5, 0.62, 1.0, 1.0, 0.92, 0.8 };

    double metallicSource = 0.0;
    double weightSum = 0.0;
    const double invSr = 1.0 / baseSampleRate();
    for (size_t i = 0; i < partials; ++i) {
        m_phases[i] += baseFreq * ratios[i] * invSr;
        if (m_phases[i] >= 1.0)
            m_phases[i] -= 1.0;
        const double weight = m_voicing == Voicing::Rd9 ? rd9Weights[i] : 1.0;
        metallicSource += weight * (m_phases[i] < 0.5 ? 1.0 : -1.0);
        weightSum += weight;
    }
    return static_cast<float>(metallicSource / std::max(1.0, weightSum));
}

float RideEngine::nextSample()
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

    // Generated at the base rate and interpolated up, so the bank sounds the same at every
    // oversampling factor. See BaseRateSource.
    m_metallicBank.setOversampleFactor(oversampleFactor());
    if (m_metallicBank.needsBaseSample()) {
        m_metallicBank.setBaseSample(nextMetallicBaseSample());
    }
    const double metallicSource { m_metallicBank.nextSample() };

    // Measured against the hardware: its top two octaves are markedly more tonal than this was
    // playing -- a ride is struck metal, and the noise is the air around it rather than the sound
    // itself. Three tenths of noise put the spectral flatness above 0.8 up there where the record
    // sits at 0.6, which is heard as hiss laid over the cymbal.
    const float noiseLevel = m_voicing == Voicing::Rd9 ? 0.12f : 0.3f;
    float source = static_cast<float>(metallicSource) * (1.0f - noiseLevel) + noise * noiseLevel;

    m_filter.setSampleRate(sr);
    // The record carries as much between 200 and 600 Hz as it does in the octave above, and the
    // high pass sat far too high to leave any of it: the cymbal had no body at all.
    m_filter.setCutoff(m_voicing == Voicing::Rd9 ? 0.30f + m_tune * 0.32f : 0.4f + m_tune * 0.5f);
    m_filter.setResonance(m_resonance);
    const auto out = static_cast<float>(m_filter.process(source) * m_ampEnv * m_attackEnv * m_velocity);

    const float attackRate { 1.0f / (0.0005f * static_cast<float>(sampleRate())) };
    m_attackEnv = std::min(1.0f, m_attackEnv + attackRate);

    const float chokeDecayRate { 1.0f - (1.0f / (ChokeFadeSeconds * static_cast<float>(sampleRate()))) };
    // The record falls twenty-two decibels over its first nine tenths of a second; this was
    // falling six, so it rang on under everything that followed it.
    const float decayScale = m_voicing == Voicing::Rd9 ? 0.75f : 2.0f;
    const float decayRate = m_stopping ? chokeDecayRate : 1.0f - (1.0f / (std::max(0.01f, m_decay) * decayScale * static_cast<float>(sampleRate())));
    m_ampEnv *= decayRate;
    if (m_ampEnv < AmplitudeThreshold) {
        m_active = false;
        m_ampEnv = 0.0f;
    }

    return out;
}

bool RideEngine::isActive() const
{
    return m_active;
}

void RideEngine::reset()
{
    m_rng.seed(RngSeed);
    m_active = false;
    m_stopping = false;
    m_ampEnv = 0.0f;
}

void RideEngine::stop()
{
    m_stopping = true;
}

void RideEngine::setVoicing(Voicing voicing)
{
    m_voicing = voicing;
}

void RideEngine::setTune(float tune)
{
    m_tune = tune;
}

void RideEngine::setDecay(float decay)
{
    m_decay = decay;
}

void RideEngine::setResonance(float resonance)
{
    m_resonance = resonance;
}

} // namespace noteahead
