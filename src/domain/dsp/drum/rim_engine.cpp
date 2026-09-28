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

#include "rim_engine.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace noteahead {

namespace {

//! What the recording sits at with Tune in the middle, and how far Tune moves it.
constexpr double BaseFreq { 211.0 };
constexpr double TuneRange { 220.0 };
//! The recording does not decay exponentially: it falls 2, 4, 9, 12 and 13 decibels over
//! successive ten-millisecond windows, slow at first and then all at once, and is silent by
//! eighty-five. That is a Gaussian, and a plain exponential fitted to its end rings far too long
//! at the start while one fitted to its start leaves the tail hanging.
constexpr double DecaySeconds { 0.0255 };
//! The stick: a couple of milliseconds of broadband noise and nothing after it.
//!
//! Broadband rather than banded, because it is the only thing in the voice that reaches either end
//! of the spectrum. The partials stop at 3 kHz, so without it the recording's 4 to 16 kHz was
//! thirty decibels away, and its 60 to 200 Hz ten -- a struck rim has an impact in it, and an
//! impact is broad.
constexpr float ClickSeconds { 0.004f };
constexpr float ClickLevel { 3.0f };
//! Rolled off, because the recording falls fourteen decibels from its 4-8 kHz to its 8-16, and
//! flat noise puts as much in the top octave as the one below it. The filter's own scale is
//! logarithmic from 20 Hz -- freq = 20 * 2^(cutoff * log2(max/20)) -- so this is 8 kHz, not the
//! four fifths of Nyquist it looks like.
constexpr float ClickCutoff { 0.80f }; // 5 kHz
//! The shell answering the stick. Longer and far lower than the click, and the only thing in the
//! voice with any reach below 200 Hz -- a two-millisecond burst has none.
constexpr float ThumpSeconds { 0.008f };
constexpr float ThumpCutoff { 0.333f }; // 200 Hz
constexpr float ThumpLevel { 2.2f };
constexpr float OutputGain { 0.62f };

} // namespace

RimEngine::RimEngine()
{
    m_rng.seed(0);
    m_clickFilter.setMode(CascadedSvf::Mode::LowPass);
    m_thumpFilter.setMode(CascadedSvf::Mode::LowPass);
}

void RimEngine::trigger(float velocity)
{
    m_velocity = velocity;
    m_active = true;
    m_stopping = false;
    m_ampEnv = 1.0f;
    m_clickEnv = 1.0f;
    m_thumpEnv = 1.0f;
    m_clickFilter.reset();
    m_thumpFilter.reset();
    m_elapsed = 0.0;
    m_noiseBank.reset();
    m_rng.seed(0);
    for (auto && phase : m_phases) {
        phase = 0.0;
    }
}

float RimEngine::nextSample()
{
    if (!m_active) {
        return 0.0f;
    }

    const double sr = sampleRate();
    const double baseFreq { BaseFreq + (static_cast<double>(m_tune) - 0.5) * TuneRange };

    double body = 0.0;
    double levelSum = 0.0;
    for (size_t i = 0; i < Ratios.size(); i++) {
        const double frequency = baseFreq * Ratios[i];
        if (frequency >= sr * 0.5) {
            break;
        }
        m_phases[i] += frequency / sr;
        if (m_phases[i] >= 1.0) {
            m_phases[i] -= 1.0;
        }
        body += std::sin(m_phases[i] * 2.0 * std::numbers::pi) * Levels[i];
        levelSum += Levels[i];
    }
    body /= std::max(1.0, levelSum);

    m_noiseBank.setOversampleFactor(oversampleFactor());
    if (m_noiseBank.needsBaseSample()) {
        m_noiseBank.setBaseSample(m_dist(m_rng));
    }
    const auto noise = m_noiseBank.nextSample();
    m_clickFilter.setSampleRate(sr);
    m_clickFilter.setCutoff(ClickCutoff);
    m_clickFilter.setResonance(0.0f);
    m_thumpFilter.setSampleRate(sr);
    m_thumpFilter.setCutoff(ThumpCutoff);
    m_thumpFilter.setResonance(0.0f);
    const auto click = static_cast<float>(m_clickFilter.process(noise)) * m_clickEnv * m_click * ClickLevel
      + static_cast<float>(m_thumpFilter.process(noise)) * m_thumpEnv * ThumpLevel;

    const auto out = static_cast<float>((body + click) * m_ampEnv * m_velocity * OutputGain);

    m_clickEnv -= m_clickEnv / (ClickSeconds * static_cast<float>(sr));
    m_thumpEnv -= m_thumpEnv / (ThumpSeconds * static_cast<float>(sr));

    m_elapsed += 1.0;
    if (m_stopping) {
        m_ampEnv *= 1.0f - (1.0f / (ChokeFadeSeconds * static_cast<float>(sr)));
    } else {
        const double tau = std::max(0.001f, m_decay) * 2.0 * DecaySeconds;
        const double age = m_elapsed / sr / tau;
        m_ampEnv = static_cast<float>(std::exp(-age * age));
    }
    if (m_ampEnv < AmplitudeThreshold) {
        m_active = false;
        m_ampEnv = 0.0f;
    }

    return out;
}

bool RimEngine::isActive() const
{
    return m_active;
}

void RimEngine::reset()
{
    m_active = false;
    m_stopping = false;
    m_ampEnv = 0.0f;
}

void RimEngine::stop()
{
    m_stopping = true;
}

void RimEngine::setTune(float tune)
{
    m_tune = tune;
}

void RimEngine::setDecay(float decay)
{
    m_decay = decay;
}

void RimEngine::setClick(float click)
{
    m_click = click;
}

} // namespace noteahead
