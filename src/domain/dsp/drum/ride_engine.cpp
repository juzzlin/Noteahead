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

namespace {

//! Restores what the fit cost in level: it compared spectra normalised by their own total, so
//! nothing in it constrained how loud the voice came out. Measured against the kit it plays in,
//! the ride had fallen three decibels under V1's while every other voice sits about two above it.
constexpr float Rd9OutputGain { 1.21f };

//! What the Rd9 voicing is made of, fitted to a recording of the hardware. See CrashEngine for what
//! the fit minimises.
//!
//! The base frequency is the one to be careful with. Left free, the fit ran it to 1.5 kHz, where
//! the bank's own lowest partial is above the low mids -- so the whole 200 Hz to 1.5 kHz had to be
//! filled with noise, and the cymbal hissed. It is held low enough that the metal reaches down
//! there itself.
constexpr double Rd9BaseFreq { 515.0 };
constexpr float Rd9NoiseLevel { 0.129f };
//! Positive and large: struck on the bow, the hardware puts its weight between four and sixteen
//! kilohertz, so the upper partials are the loud ones.
constexpr double Rd9MetalTilt { 1.325 };
constexpr float Rd9CutoffBase { 0.412f };
constexpr float Rd9CutoffTune { 0.319f };
//! Where the shimmer starts, on its own path: see why it is not damped with the metal, below.
constexpr float Rd9NoiseCutoff { 0.247f };
//! The damping sweeps between these two, which is how the cymbal darkens as it decays.
constexpr float Rd9DampStart { 0.990f };
constexpr float Rd9DampEnd { 0.885f };
constexpr float Rd9DarkenSeconds { 0.172f };
constexpr float Rd9DecayScale { 2.0f };

//! How far the cutoffs move across the Tune range, from their fitted values at the middle of it.
//!
//! The same fault the crash had: Tune raised the high pass while the damping stayed put, so it
//! pushed the cymbal against a ceiling instead of brightening it and the control wandered between
//! 6.2 and 8.0 kHz with no direction to it. All three move together now, centred so that the
//! fitted sound at the middle of the range is unchanged.
constexpr float Rd9DampTuneRange { 0.12f };
constexpr float Rd9NoiseTuneRange { 0.3f };
constexpr float Rd9MaxCutoff { 0.985f };

} // namespace

RideEngine::RideEngine()
{
    m_rng.seed(0);
    m_filter.setMode(CascadedSvf::Mode::HighPass);
    m_damping.setMode(CascadedSvf::Mode::LowPass);
    m_noiseFilter.setMode(CascadedSvf::Mode::HighPass);
}

void RideEngine::trigger(float velocity)
{
    m_velocity = velocity;
    m_active = true;
    m_stopping = false;
    m_filter.reset();
    m_damping.reset();
    m_noiseFilter.reset();
    m_darkenEnv = 1.0f;
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
    const double baseFreq { (m_voicing == Voicing::Rd9 ? Rd9BaseFreq : 300.0) + m_tune * 500.0 };
    // The first six are the cymbal this has always been. The four above them carry it up into the
    // band the record actually lives in, and are inharmonic with the rest so they read as the same
    // piece of metal rather than as a tone laid over it.
    // Sixteen, where a real cymbal has hundreds. Counted as spectral peaks the recording carries
    // about two dozen in its middle and this bank reached nine, which is heard as the difference
    // between a shimmer and a chord: the modes have to be dense enough, and mutually inharmonic
    // enough, that no two of them ever settle into a beat you can follow.
    static constexpr std::array<double, 16> ratios {
        1.0, 1.48, 1.92, 2.54, 3.41, 4.23, 5.78, 7.13, 8.94, 11.37, 14.1, 17.6, 20.8, 24.9, 29.3, 34.7
    };
    const size_t partials = m_voicing == Voicing::Rd9 ? ratios.size() : ClassicPartials;

    // Struck on the bow, a ride puts most of its energy in the high modes: the record is twelve
    // decibels stronger between four and sixteen kilohertz than it is in the octave above its
    // fundamental. Summed flat, the fundamental group drowns the rest and the cymbal reads as a
    // gong.
    static constexpr std::array<double, 16> rd9Weights {
        0.22, 0.26, 0.3, 0.38, 0.5, 0.62, 0.82, 1.0, 1.0, 1.0, 0.96, 0.92, 0.86, 0.8, 0.72, 0.64
    };

    double metallicSource = 0.0;
    double weightSum = 0.0;
    const double invSr = 1.0 / baseSampleRate();
    // A partial above the Nyquist rate does not disappear, it folds back down as a tone that has
    // nothing to do with the cymbal and that moves the wrong way when Tune is raised. With the
    // bank reaching thirty-four times its base, several of them are over it before Tune is touched.
    const double highest = baseSampleRate() * 0.5;
    for (size_t i = 0; i < partials; ++i) {
        if (baseFreq * ratios[i] >= highest) {
            break;
        }
        m_phases[i] += baseFreq * ratios[i] * invSr;
        if (m_phases[i] >= 1.0)
            m_phases[i] -= 1.0;
        const double weight = m_voicing == Voicing::Rd9 ? rd9Weights[i] * std::pow(ratios[i], Rd9MetalTilt) : 1.0;
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
    const float noiseLevel = m_voicing == Voicing::Rd9 ? Rd9NoiseLevel : 0.3f;
    float source = static_cast<float>(metallicSource) * (1.0f - noiseLevel);
    if (m_voicing != Voicing::Rd9) {
        source += noise * noiseLevel;
    }

    m_filter.setSampleRate(sr);
    // The record carries as much between 200 and 600 Hz as it does in the octave above, and the
    // high pass sat far too high to leave any of it: the cymbal had no body at all.
    m_filter.setCutoff(m_voicing == Voicing::Rd9 ? Rd9CutoffBase + m_tune * Rd9CutoffTune : 0.4f + m_tune * 0.5f);
    const float tuneOffset = m_tune - 0.5f;
    m_filter.setResonance(m_resonance);
    // Frequency-dependent damping, which is the difference between a cymbal and a bank of
    // partials sharing one envelope. Real metal loses its high modes first: the recording's
    // spectral centroid falls from 9.0 kHz at the strike to 5.1 by a third of a second, and with a
    // single envelope over the whole bank this could not fall at all.
    float shaped = static_cast<float>(m_filter.process(source));
    if (m_voicing == Voicing::Rd9) {
        m_darkenEnv -= m_darkenEnv / (Rd9DarkenSeconds * static_cast<float>(sampleRate()));
        m_damping.setSampleRate(sr);
        m_damping.setCutoff(std::min(Rd9MaxCutoff, Rd9DampEnd + (Rd9DampStart - Rd9DampEnd) * m_darkenEnv + tuneOffset * Rd9DampTuneRange));
        m_damping.setResonance(0.0f);
        shaped = static_cast<float>(m_damping.process(shaped));

        // The shimmer is added after the damping rather than before it, and on its own high pass.
        // Damped with the metal it was the first thing to go, leaving the top octave three times
        // more tonal than the recording -- the very dryness this voicing was meant to cure -- while
        // what survived sat an octave too low and made the cymbal hissy where it should be clear.
        m_noiseFilter.setSampleRate(sr);
        m_noiseFilter.setCutoff(std::clamp(Rd9NoiseCutoff + tuneOffset * Rd9NoiseTuneRange, 0.02f, Rd9MaxCutoff));
        m_noiseFilter.setResonance(0.0f);
        shaped += static_cast<float>(m_noiseFilter.process(noise)) * noiseLevel;
    }
    const auto out = static_cast<float>(shaped * m_ampEnv * m_attackEnv * m_velocity * (m_voicing == Voicing::Rd9 ? Rd9OutputGain : 1.0f));

    const float attackRate { 1.0f / (0.0005f * static_cast<float>(sampleRate())) };
    m_attackEnv = std::min(1.0f, m_attackEnv + attackRate);

    const float chokeDecayRate { 1.0f - (1.0f / (ChokeFadeSeconds * static_cast<float>(sampleRate()))) };
    // The recording is faded out over its last hundred milliseconds -- it drops thirteen decibels
    // below its own decay there -- so what it falls end to end is not what the cymbal does. Fitted
    // to the ungated part instead, it decays at eighteen decibels per second, where this was
    // falling eight and ringing on under everything that followed it.
    const float decayScale = m_voicing == Voicing::Rd9 ? Rd9DecayScale : 2.0f;
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
