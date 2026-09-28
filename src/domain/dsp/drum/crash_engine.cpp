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
//! The fit minimised a distance with four parts: the spectral envelope in third-octave bands, how
//! tonal the sound is inside each octave taken separately, the trajectory of level, splash band and
//! spectral centroid over six sixty-millisecond slices, and how far the last two of those swing
//! from the strike. Every part earned its place by a failure without it. Envelope alone lets a wash
//! of noise pass for a cymbal. Tonality measured across a wide band reports the spectral tilt
//! rather than the noisiness. And an average over the whole sound cannot tell a crash from a ride
//! at all: fitted without the trajectory this matched the recording's spectrum closely while
//! sounding like a ride, because it was struck fully open and left to decay.
constexpr float Rd9MetalLevel { 0.387f };
constexpr float Rd9WashLevel { 0.080f };
constexpr float Rd9SizzleLevel { 0.284f };
constexpr float Rd9StrikeLevel { 0.569f };
constexpr double Rd9MetalTilt { -0.162 };
//! Cutoffs at the middle of the Tune range, which is where the fit was made, and how far each one
//! moves from there.
//!
//! Tune used to raise the high pass and band pass from a fixed floor while the low pass stayed
//! put, so past about seventy per cent it pushed the source band above the low pass, which then
//! took it away again: the crash grew brighter to 6.2 kHz and then fell back to 3.6, as dark at
//! full Tune as at none. The band moves as a whole now, centred so that the fitted sound at the
//! middle of the range is unchanged.
constexpr float Rd9HpfCutoff { 0.631f };
constexpr float Rd9HpfTuneRange { 0.4f };
constexpr float Rd9BpfTuneRange { 0.25f };
constexpr float Rd9LpfTuneRange { 0.2f };
//! Short of one: the filter is not asked for a cutoff it cannot have.
constexpr float Rd9MaxCutoff { 0.985f };
constexpr float Rd9BpfCutoff { 0.828f };
//! The filter is swept between these two by the bloom, not parked at one of them.
constexpr float Rd9LpfStart { 0.753f };
constexpr float Rd9LpfCutoff { 0.885f };
constexpr double Rd9BaseFreq { 700.0 };
//! How long the wash takes to open out. Forty-six milliseconds, against a recording whose centroid
//! climbs from 2.2 to 4.6 kHz over its first two slices.
constexpr float Rd9BloomSeconds { 0.046f };
//! How much of the top is already there at the strike. Not zero: a crash is struck, not faded in.
constexpr float Rd9BloomFloor { 0.25f };
constexpr float Rd9BodySeconds { 0.080f };
constexpr float Rd9BodyGain { 0.850f };
constexpr float Rd9DecayScale { 1.05f };
//! Restores what the fit cost in level.
//!
//! The fit compared spectra normalised by their own total, so nothing in it constrained how loud
//! the voice came out -- and holding the metal back at the strike took the peak with it. Measured
//! against the kit it plays in, the crash had fallen six decibels under V1's while every other
//! voice sits one to two above it. A plain gain, so the fitted spectrum and trajectory are
//! untouched.
constexpr float Rd9OutputGain { 2.5f };
//! The same gain leaves the reverse swell four decibels hotter than the struck crash, because it
//! rises to a full envelope rather than decaying from one, so it is trimmed on its own.
constexpr float Rd9ReverseOutputGain { 1.63f };

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
    m_bloomEnv = 0.0f;
    m_reverseProgress = 0.0;
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
    // Twenty-four, of which the Classic voicing sounds the first twelve. Twelve is too few for a
    // crash: with the weight piled on two of them the bank rang as a pitched tone rather than a
    // wash -- measured at the default Tune it stood twenty-one decibels above its neighbourhood at
    // 6.87 kHz, which is 900 Hz times the 7.63 ratio, and again at 7.98. The recording's peaks are
    // spread across the spectrum and its strongest is up at 17.6 kHz, where it reads as air.
    static constexpr std::array<double, 24> ratios {
        1.0, 1.27, 1.73, 2.11, 2.68, 3.47, 3.86, 4.21, 4.79, 5.17, 5.83, 6.39,
        7.02, 7.63, 8.21, 8.87, 9.54, 10.13, 11.21, 12.39, 13.47, 14.57, 16.31, 18.13
    };
    static constexpr size_t ClassicPartials { 12 };
    const size_t partials = m_voicing == Voicing::Rd9 ? ratios.size() : ClassicPartials;

    // The splash is the band from four to eight kilohertz, which is where the ratios from 7.63 up
    // land over this base. Summed flat they sat six decibels under the record there, and that band
    // is most of what makes a crash sound like struck metal rather than like a wash.
    // Deliberately flat. Every bump here is a partial that can be picked out by ear, and what a
    // crash needs is density rather than any particular mode being loud.
    static constexpr std::array<double, 24> rd9Weights {
        0.62, 0.64, 0.68, 0.72, 0.78, 0.84, 0.90, 0.95, 1.0, 1.0, 1.0, 1.0,
        1.0, 1.0, 1.0, 1.0, 0.98, 0.96, 0.94, 0.9, 0.86, 0.82, 0.76, 0.7
    };

    double metallicSource = 0.0;
    double weightSum = 0.0;
    const double invSr = 1.0 / baseSampleRate();
    // A partial over the Nyquist rate folds back down as a tone that has nothing to do with the
    // cymbal and moves the wrong way when Tune is raised.
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

    // Reverse is a time mirror of the struck crash, not just its amplitude run backwards.
    //
    // It used to ramp the level up while the bloom, the filter sweep, the body and the sizzle all
    // still ran forwards, so what swelled was a crash already fully open -- the one thing the
    // forward voice is careful not to be. Here a progress counter runs the length of the swell and
    // every envelope is evaluated at the time still remaining, so the sound arrives at its own
    // strike: the wash closes down into the hit instead of opening out of it.
    const bool rd9 = m_voicing == Voicing::Rd9;
    const bool reversed = rd9 && m_mode == Mode::Reverse;
    float remaining = 0.0f;
    if (reversed) {
        const double swellSeconds = std::max(0.01f, m_decay) * 4.0f;
        m_reverseProgress = std::min(1.0, m_reverseProgress + 1.0 / (swellSeconds * sr));
        remaining = static_cast<float>((1.0 - m_reverseProgress) * swellSeconds);
    }

    // Attack envelope to soften the initial hit
    if (m_attackEnv < 1.0f) {
        const float attackTime { std::max(0.0005f, m_attack * 0.2f) };
        const float attackRate { 1.0f / (attackTime * static_cast<float>(sampleRate())) };
        m_attackEnv = std::min(1.0f, m_attackEnv + attackRate);
    }

    // Pitch envelope for the initial "hit" - used only for subtle shimmer, no "laser" sweeps
    constexpr float PitchSeconds { 0.02f };
    if (reversed) {
        m_pitchEnv = std::exp(-remaining / PitchSeconds);
    } else {
        m_pitchEnv *= 1.0f - (1.0f / (PitchSeconds * static_cast<float>(sampleRate())));
    }
    const double pitchMod = 1.0 + m_pitchEnv * 0.05;

    // The bloom, which is what makes a crash a crash rather than a ride.
    //
    // The recording starts dark -- its spectral centroid is 2.2 kHz over the first sixty
    // milliseconds -- and opens out to 4.6 kHz, its splash band climbing five decibels, over the
    // next hundred. That spreading wash is the sound; struck fully open and left to decay, as this
    // was, the same spectrum reads as a ride.
    if (reversed) {
        m_bloomEnv = 1.0f - std::exp(-remaining / Rd9BloomSeconds);
    } else if (rd9) {
        m_bloomEnv += (1.0f - m_bloomEnv) / (Rd9BloomSeconds * static_cast<float>(sampleRate()));
    }

    // Sizzle envelope for high-frequency splash
    constexpr float SizzleSeconds { 0.15f };
    if (reversed) {
        m_sizzleEnv = std::exp(-remaining / SizzleSeconds);
    } else {
        m_sizzleEnv *= 1.0f - (1.0f / (SizzleSeconds * static_cast<float>(sampleRate())));
    }

    // Body envelope for low-mid weight. Twenty milliseconds is an impact rather than a body: the
    // record carries its 200 to 600 Hz for the whole of the crash, twenty decibels above what this
    // was leaving there.
    const float bodySeconds = m_voicing == Voicing::Rd9 ? Rd9BodySeconds : 0.02f;
    if (reversed) {
        m_bodyEnv = std::exp(-remaining / bodySeconds);
    } else {
        m_bodyEnv *= 1.0f - (1.0f / (bodySeconds * static_cast<float>(sampleRate())));
    }

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
    const float bodyGain = (m_voicing == Voicing::Rd9 ? Rd9BodyGain : 0.6f) * std::min(1.0f, m_decay * 2.0f);
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
    const float metalLevel = rd9 ? Rd9MetalLevel : 0.4f;
    const float washLevel = rd9 ? Rd9WashLevel : 0.4f;
    const float sizzleLevel = rd9 ? Rd9SizzleLevel : 0.5f;
    // The record is metal from the first sample: measured over its attack it is markedly more
    // tonal than this was, and a strike made of noise is most of why.
    const float strikeNoise = noise * m_pitchEnv * (rd9 ? Rd9StrikeLevel : 0.6f);
    const float sizzleNoise = noise * m_sizzleEnv * sizzleLevel;
    // Held back at the strike and let in as the bloom opens, so the top spreads rather than being
    // there from the first sample.
    const float bloom = rd9 ? Rd9BloomFloor + (1.0f - Rd9BloomFloor) * m_bloomEnv : 1.0f;
    float source = (static_cast<float>(metallicSource) * metalLevel * bloom + noise * washLevel + strikeNoise + sizzleNoise * bloom) * m_attackEnv;

    // Triple filtering to shape the spectral profile. Every cutoff is taken from the middle of the
    // Tune range so that the three move together: see Rd9HpfCutoff.
    const float tuneOffset = m_tune - 0.5f;
    m_hpf.setSampleRate(sr);
    m_hpf.setCutoff(rd9 ? std::min(Rd9MaxCutoff, Rd9HpfCutoff + tuneOffset * Rd9HpfTuneRange) : 0.35f + m_tune * 0.4f);
    m_hpf.setResonance(m_resonance * 0.2f);

    m_bpf.setSampleRate(sr);
    m_bpf.setCutoff(rd9 ? std::min(Rd9MaxCutoff, Rd9BpfCutoff + tuneOffset * Rd9BpfTuneRange) : 0.45f + m_tune * 0.5f);
    m_bpf.setResonance(0.5f);

    m_lpf.setSampleRate(sr);
    // Higher for the fitted voicing: the record still has real weight in its top octave, and at
    // 0.85 the splash was being rolled off five decibels below it.
    // Swept open by the bloom rather than fixed. This is where the crash comes from: the recording
    // is dark at the strike, a 2.2 kHz centroid, and spreads to 4.6 kHz over the next hundred
    // milliseconds. Holding the filter down and letting it open is what that spreading is.
    m_lpf.setCutoff(rd9 ? std::min(Rd9MaxCutoff, Rd9LpfStart + (Rd9LpfCutoff - Rd9LpfStart) * m_bloomEnv + tuneOffset * Rd9LpfTuneRange) : 0.85f); // 12kHz roll-off
    m_lpf.setResonance(0.1f);

    const auto hpfOut = static_cast<float>(m_hpf.process(source));
    const auto bpfOut = static_cast<float>(m_bpf.process(source));
    const auto filtered = hpfOut * 0.5f + bpfOut * 0.5f;

    const auto out = static_cast<float>((m_lpf.process(filtered) + bodySource * m_attackEnv) * m_ampEnv * m_velocity * (rd9 ? (m_mode == Mode::Normal ? Rd9OutputGain : Rd9ReverseOutputGain) : 1.0f));

    const float chokeDecayRate { 1.0f - (1.0f / (ChokeFadeSeconds * static_cast<float>(sampleRate()))) };
    if (m_mode == Mode::Normal) {
        // The record falls sixteen decibels over its first nine tenths of a second; this was
        // falling nine.
        const float decayScale = rd9 ? Rd9DecayScale : 2.5f;
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
        } else if (reversed) {
            // The forward voice's own decay, read backwards, so the swell is the shape the crash
            // actually has rather than a straight line.
            m_ampEnv = std::exp(-remaining / (std::max(0.01f, m_decay) * Rd9DecayScale));
            if (m_reverseProgress >= 1.0) {
                m_ampEnv = 1.0f;
                m_active = false;
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
