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

#include "tom_engine.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace noteahead {

namespace {

//! What the Rd9 voicing is made of, fitted to recordings of the hardware's three toms at once --
//! they share this engine and differ only by Tune, so one set of constants has to serve all three.
//!
//! The pitch and the sweep are not fitted. They were measured: the recordings settle at 71, 124
//! and 170 Hz and fall only seven to twelve Hertz getting there. Left to the fit they became a way
//! of cheating the spectrum, because a deep sweep smears energy across the bands and passes for a
//! broadband drum -- it put the toms an octave and a half low and matched the recording with the
//! wrong drum.
constexpr double Rd9BaseFreq { 39.0 };
constexpr double Rd9TuneRange { 165.2 };
//! A tenth of what the control asks for: the hardware barely bends at all.
constexpr float Rd9PitchDepthScale { 0.10f };
constexpr double Rd9Partial2Ratio { 1.737 };
constexpr float Rd9Partial2Level { 0.387f };
constexpr double Rd9Partial3Ratio { 2.675 };
constexpr float Rd9Partial3Level { 0.257f };
//! The stick. The Classic voicing is a single sine and has none, which left it ninety decibels
//! down where the recordings carry real energy, and with no spectral peaks at all against their
//! nine to eleven.
constexpr float Rd9NoiseLevel { 0.905f };
constexpr float Rd9NoiseSeconds { 0.030f };
constexpr float Rd9NoiseCutoff { 0.800f };
//! The recordings fall between eighty-five and a hundred and twenty-five decibels a second, where
//! this was falling twenty-two and ringing on under everything after it.
constexpr float Rd9DecayScale { 0.188f };
//! The partials and the stick added about four decibels the Classic voicing did not have.
constexpr float Rd9OutputGain { 0.63f };

} // namespace

TomEngine::TomEngine()
{
    m_rng.seed(0);
    m_noiseFilter.setMode(CascadedSvf::Mode::LowPass);
}

void TomEngine::setVoicing(Voicing voicing)
{
    m_voicing = voicing;
}

void TomEngine::trigger(float velocity)
{
    m_retriggerOffset = m_lastOut;
    m_velocity = velocity;
    m_ampEnv = 1.0f;
    m_attackEnv = 0.0f;
    m_pitchEnv = 1.0f;
    m_phase = 0.0;
    m_active = true;
    m_stopping = false;
    m_noiseEnv = 1.0f;
    m_noiseFilter.reset();
    m_noiseBank.reset();
    m_rng.seed(0);
    for (auto && phase : m_partialPhases) {
        phase = 0.0;
    }
}

float TomEngine::nextSample()
{
    if (!m_active) {
        m_lastOut = 0.0f;
        return 0.0f;
    }

    // The recordings sit at 76, 135 and 182 Hz for the three toms, where this mapping put them at
    // 110, 155 and 200: every drum a little sharp, and the spread between them too narrow.
    const bool rd9 = m_voicing == Voicing::Rd9;
    const double baseFreq { rd9 ? Rd9BaseFreq + m_tune * Rd9TuneRange : 80.0 + (m_tune * 150.0) };
    const double sweepFreq { baseFreq + (m_pitchDepth * 200.0 * m_pitchEnv * (rd9 ? Rd9PitchDepthScale : 1.0f)) };

    float out { static_cast<float>(std::sin(m_phase * 2.0 * std::numbers::pi)) * m_ampEnv * m_attackEnv * m_velocity * 0.7f };

    if (rd9) {
        // The head's own modes, which decay with the fundamental rather than on their own: what
        // they are for is the timbre of the drum, not a second sound on top of it.
        const std::array<double, 2> ratios { Rd9Partial2Ratio, Rd9Partial3Ratio };
        const std::array<double, 2> levels { Rd9Partial2Level, Rd9Partial3Level };
        for (size_t i = 0; i < m_partialPhases.size(); i++) {
            m_partialPhases[i] += sweepFreq * ratios[i] / sampleRate();
            if (m_partialPhases[i] >= 1.0) {
                m_partialPhases[i] -= 1.0;
            }
            out += static_cast<float>(std::sin(m_partialPhases[i] * 2.0 * std::numbers::pi) * levels[i])
              * m_ampEnv * m_attackEnv * m_velocity * 0.7f;
        }

        // The stick. Short, low passed and gone before the drum has properly started ringing.
        m_noiseBank.setOversampleFactor(oversampleFactor());
        if (m_noiseBank.needsBaseSample()) {
            m_noiseBank.setBaseSample(m_dist(m_rng));
        }
        m_noiseFilter.setSampleRate(sampleRate());
        m_noiseFilter.setCutoff(Rd9NoiseCutoff);
        m_noiseFilter.setResonance(0.0f);
        out += static_cast<float>(m_noiseFilter.process(m_noiseBank.nextSample())) * m_noiseEnv * Rd9NoiseLevel * m_velocity;
        m_noiseEnv -= m_noiseEnv / (Rd9NoiseSeconds * static_cast<float>(sampleRate()));
    }

    // Apply re-trigger offset to smooth out discontinuities
    out += m_retriggerOffset;
    m_retriggerOffset *= 0.95f;

    const double phaseStep { sweepFreq / sampleRate() };
    m_phase += phaseStep;
    if (m_phase >= 1.0) {
        m_phase -= 1.0;
    }

    const float chokeDecayRate { 1.0f - (1.0f / (ChokeFadeSeconds * static_cast<float>(sampleRate()))) };
    // The recordings fall between eighty-five and a hundred and twenty-five decibels a second;
    // this was falling twenty-two, so a tom rang on under everything after it.
    const float decayScale = rd9 ? Rd9DecayScale : 0.8f;
    const float ampDecayRate = m_stopping ? chokeDecayRate : 1.0f - (1.0f / (std::max(0.001f, m_decay) * decayScale * static_cast<float>(sampleRate())));
    const float pitchDecayRate { 1.0f - (1.0f / (std::max(0.001f, m_pitchDecay * 0.1f) * static_cast<float>(sampleRate()))) };

    const float attackRate { 1.0f / (0.0005f * static_cast<float>(sampleRate())) };
    m_attackEnv = std::min(1.0f, m_attackEnv + attackRate);

    m_ampEnv *= ampDecayRate;
    m_pitchEnv *= pitchDecayRate;

    if (m_ampEnv < AmplitudeThreshold && std::abs(out) < AmplitudeThreshold) {
        m_active = false;
        m_ampEnv = 0.0f;
        m_retriggerOffset = 0.0f;
    }

    if (rd9) {
        out *= Rd9OutputGain;
    }

    m_lastOut = out;
    return out;
}

void TomEngine::reset()
{
    m_active = false;
    m_stopping = false;
    m_ampEnv = 0.0f;
    m_lastOut = 0.0f;
    m_retriggerOffset = 0.0f;
}

void TomEngine::stop()
{
    m_stopping = true;
}

bool TomEngine::isActive() const
{
    return m_active;
}

void TomEngine::setTune(float tune)
{
    m_tune = tune;
}

void TomEngine::setDecay(float decay)
{
    m_decay = decay;
}

void TomEngine::setPitchDepth(float depth)
{
    m_pitchDepth = depth;
}

void TomEngine::setPitchDecay(float decay)
{
    m_pitchDecay = decay;
}

} // namespace noteahead
