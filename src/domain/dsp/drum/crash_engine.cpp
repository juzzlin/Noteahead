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
constexpr float Rd9MetalLevel { 0.24f };
constexpr float Rd9WashLevel { 0.080f };
constexpr float Rd9SizzleLevel { 0.180f };
constexpr float Rd9StrikeLevel { 0.480f };
//! Cutoffs at the middle of the Tune range, which is where the fit was made, and how far each one
//! moves from there.
//!
//! Tune used to raise the high pass and band pass from a fixed floor while the low pass stayed
//! put, so past about seventy per cent it pushed the source band above the low pass, which then
//! took it away again: the crash grew brighter to 6.2 kHz and then fell back to 3.6, as dark at
//! full Tune as at none. The band moves as a whole now, centred so that the fitted sound at the
//! middle of the range is unchanged.
constexpr float Rd9HpfCutoff { 0.580f };
constexpr float Rd9HpfTuneRange { 0.25f };
constexpr float Rd9BpfTuneRange { 0.22f };
constexpr float Rd9LpfTuneRange { 0.20f };
//! Short of one: the filter is not asked for a cutoff it cannot have.
constexpr float Rd9MaxCutoff { 0.985f };
constexpr float Rd9BpfCutoff { 0.800f };
//! The filter is swept between these two by the bloom, not parked at one of them.
//! At noon, 0.895 with Butterworth Q sits right on the RD-9's 8.5-10 kHz low-pass roll-off.
constexpr float Rd9LpfStart { 0.665f };
constexpr float Rd9LpfCutoff { 0.895f };
//! How long the wash takes to open out. Forty-six milliseconds, against a recording whose centroid
//! climbs from 2.2 to 4.6 kHz over its first two slices.
constexpr float Rd9BloomSeconds { 0.046f };
//! How much of the top is already there at the strike. Not zero: a crash is struck, not faded in.
constexpr float Rd9BloomFloor { 0.25f };
//! Pitch wobble at the middle of the Wobble knob, which is what the crash had before the knob.
//! The depth goes with the square of the knob from there, up to four times this at the top.
constexpr double DefaultWobbleDepth { 0.002 };
//! How much faster a partial dies the higher it is: its amplitude falls by this times its
//! frequency squared, in nepers a second, on top of the envelope everything shares.
constexpr double Rd9HighDamping { 1.1e-8 };
//! The body: a band of noise struck at full and decaying to a floor that it then holds, falling
//! only with the envelope everything shares.
constexpr float Rd9BodyCutoff { 0.36f };
constexpr float Rd9BodySeconds { 0.060f };
constexpr float Rd9BodyFloor { 0.25f };
constexpr float Rd9BodyGain { 0.78f };
constexpr float Rd9DecayScale { 1.05f };
//! Restores what the fit cost in level.
constexpr float Rd9OutputGain { 2.5f };
//! The same gain leaves the reverse swell hotter than the struck crash, because it rises to a full
//! envelope rather than decaying from one, so it is trimmed on its own.
constexpr float Rd9ReverseOutputGain { 1.63f };

struct Rd9Partial
{
    double hz;
    double amp;
    double phase;
};

//! Top 48 partials of the RD-9 crash cymbal: closely-spaced beating clusters around 3.7, 4.35, 4.63,
//! 4.78, 5.4, 6.37, 7.34 and 9.6-10.7 kHz that give a metallic shimmer instead of a sparse buzz.
constexpr std::array<Rd9Partial, 48> Rd9Partials { {
  { 5407.69, 1.0000, 0.7465 },
  { 4342.86, 0.9924, 1.9060 },
  { 6373.32, 0.9135, 2.6366 },
  { 4358.42, 0.9011, -0.8099 },
  { 9611.46, 0.8687, -1.4620 },
  { 6378.77, 0.8408, -1.3370 },
  { 3801.38, 0.8386, 2.8076 },
  { 5402.75, 0.7782, -1.0681 },
  { 4338.35, 0.6719, 1.2451 },
  { 4350.99, 0.6567, 0.4017 },
  { 4783.34, 0.6559, 0.6542 },
  { 9615.86, 0.6509, 2.4779 },
  { 4635.28, 0.6330, -2.0066 },
  { 6368.81, 0.6211, -0.3900 },
  { 3719.24, 0.6164, -2.1691 },
  { 10098.41, 0.5881, 2.1859 },
  { 4781.14, 0.5847, -0.6362 },
  { 3652.92, 0.5815, -2.4318 },
  { 4785.53, 0.5770, 2.2353 },
  { 7346.70, 0.5591, 2.2537 },
  { 4373.91, 0.5572, -2.0076 },
  { 4632.24, 0.5524, 3.0353 },
  { 10730.49, 0.5415, -1.9415 },
  { 9636.69, 0.5391, 1.8465 },
  { 4746.31, 0.5274, 1.6188 },
  { 10091.82, 0.5237, -2.8568 },
  { 4616.86, 0.5118, 0.3786 },
  { 3666.39, 0.5040, -3.1215 },
  { 4364.90, 0.4987, 1.2866 },
  { 2040.12, 0.4766, -1.6841 },
  { 3790.39, 0.4742, 1.4957 },
  { 5382.79, 0.4730, -0.5072 },
  { 7682.89, 0.4661, 1.6103 },
  { 3794.27, 0.4586, 1.7212 },
  { 3699.68, 0.4455, -1.6932 },
  { 4620.85, 0.4393, 0.7340 },
  { 7340.33, 0.4248, 0.3242 },
  { 4911.62, 0.3997, -0.3727 },
  { 4759.06, 0.3971, 2.0894 },
  { 4570.02, 0.3792, 0.8014 },
  { 2859.27, 0.3791, 0.1552 },
  { 6946.07, 0.3748, 0.7077 },
  { 6391.11, 0.3730, 1.5734 },
  { 2791.48, 0.3626, -2.9095 },
  { 3863.75, 0.3595, -2.7726 },
  { 4605.87, 0.3590, -2.0743 },
  { 4381.24, 0.3495, -1.2354 },
  { 2939.32, 0.3454, 0.7906 },
} };

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
    m_elapsedSeconds = 0.0;
    m_hpf.reset();
    m_bpf.reset();
    m_lpf.reset();
    m_bodyFilter.reset();
    m_wobblePhase = 0.0;
    m_metallicBank.reset();
    m_noiseBank.reset();
    for (size_t i = 0; i < m_phases.size(); i++) {
        if (m_voicing == Voicing::Rd9) {
            double p = Rd9Partials[i].phase / (2.0 * std::numbers::pi);
            if (p < 0.0) {
                p += 1.0;
            }
            m_phases[i] = p;
        } else {
            m_phases[i] = 0.0;
        }
    }
    if (m_mode == Mode::Normal) {
        m_ampEnv = 1.0f;
    } else {
        m_ampEnv = 0.0f;
    }
}

float CrashEngine::nextMetallicBaseSample(double pitchScale, double partialAge)
{
    const double invSr = 1.0 / baseSampleRate();
    // A partial over the Nyquist rate folds back down as a tone that has nothing to do with the
    // cymbal and moves the wrong way when Tune is raised.
    const double highest = baseSampleRate() * 0.48;

    if (m_voicing == Voicing::Rd9) {
        const double tuneScale = std::pow(2.0, (static_cast<double>(m_tune) - 0.5) * 1.0) * pitchScale;
        double metallicSource = 0.0;
        double weightSum = 0.0;
        for (size_t i = 0; i < Rd9Partials.size(); i++) {
            const double frequency = Rd9Partials[i].hz * tuneScale;
            if (frequency >= highest) {
                continue;
            }
            m_phases[i] += frequency * invSr;
            if (m_phases[i] >= 1.0) {
                m_phases[i] -= 1.0;
            }
            const double weight = Rd9Partials[i].amp;
            const double damping = std::exp(-Rd9HighDamping * frequency * frequency * partialAge);
            metallicSource += weight * damping * std::sin(2.0 * std::numbers::pi * m_phases[i]);
            weightSum += weight;
        }
        return static_cast<float>(metallicSource / std::max(1.0, weightSum) * 2.5);
    }

    const double baseFreq { (350.0 + m_tune * 400.0) * pitchScale };
    static constexpr std::array<double, 12> ratios {
        1.0, 1.27, 1.73, 2.11, 2.68, 3.47, 3.86, 4.21, 4.79, 5.17, 5.83, 6.39
    };

    double metallicSource = 0.0;
    for (size_t i = 0; i < ratios.size(); i++) {
        if (baseFreq * ratios[i] >= highest) {
            break;
        }
        m_phases[i] += baseFreq * ratios[i] * invSr;
        if (m_phases[i] >= 1.0)
            m_phases[i] -= 1.0;
        metallicSource += (m_phases[i] < 0.5 ? 1.0 : -1.0);
    }
    return static_cast<float>(metallicSource / static_cast<double>(ratios.size()));
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
    m_elapsedSeconds += 1.0 / sr;

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
    const double wobbleScale = static_cast<double>(m_wobble) / 0.5;
    const double wobble = 1.0 + DefaultWobbleDepth * wobbleScale * wobbleScale * std::sin(m_wobblePhase * 2.0 * std::numbers::pi);

    // Body component: noise-based impact for "weight" (OHH approach)
    // Centered around the 909's 568Hz peak
    m_bodyFilter.setSampleRate(sr);
    m_bodyFilter.setCutoff((rd9 ? Rd9BodyCutoff : 0.35f) + m_tune * 0.2f);
    m_bodyFilter.setResonance(0.4f);

    // Body is more prominent if decay is long (OHH approach)
    const float bodyGain = (m_voicing == Voicing::Rd9 ? Rd9BodyGain : 0.6f) * std::min(1.0f, m_decay * 2.0f);
    const float bodyLevel = rd9 ? Rd9BodyFloor + (1.0f - Rd9BodyFloor) * m_bodyEnv : m_bodyEnv;
    const float bodySource = m_bodyFilter.process(noise) * bodyLevel * bodyGain;

    // Metallic part: 12 square wave oscillators with ratios tuned for 2kHz-8kHz clusters, generated
    // at the base rate and interpolated up so the bank sounds the same at every oversampling
    // factor. See BaseRateSource.
    m_metallicBank.setOversampleFactor(oversampleFactor());
    if (m_metallicBank.needsBaseSample()) {
        m_metallicBank.setBaseSample(nextMetallicBaseSample(pitchMod * wobble, reversed ? remaining : m_elapsedSeconds));
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
    // Swept open by the bloom rather than fixed, then gently darkened over the long tail so the
    // wash loses its top along with the damped partials.
    const float tailAge = static_cast<float>(reversed ? remaining : m_elapsedSeconds);
    const float tailDarken = std::min(0.06f, std::max(0.0f, tailAge - 0.35f) * 0.03f);
    m_lpf.setCutoff(rd9 ? std::clamp(Rd9LpfStart + (Rd9LpfCutoff - Rd9LpfStart) * m_bloomEnv + tuneOffset * Rd9LpfTuneRange - tailDarken, 0.2f, Rd9MaxCutoff) : 0.85f); // 12kHz roll-off
    m_lpf.setResonance(rd9 ? 0.2929f : 0.1f);

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

void CrashEngine::setWobble(float wobble)
{
    m_wobble = std::clamp(wobble, 0.0f, 1.0f);
}

} // namespace noteahead
