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

#include "snare_808_engine.hpp"

#include <algorithm>
#include <cmath>

namespace noteahead {

namespace {

// The RD-8's snare with both knobs at noon, fitted on its spectrogram within 3.0 dB
constexpr std::array<Drum808::Mode, 2> Modes { {
  { 334.2, 0.0089, 0.579819, 1.870007 },
  { 173.7, 0.0279, 0.450927, 4.306934 },
} };
constexpr double SpikeLevel = 0.058754;
constexpr double SpikeSeconds = 0.001;
//! The noise, at unit deviation through a high pass and then a low pass whose corners cross: what
//! is left is a band around 2.5 kHz
constexpr double NoiseLevel = 0.15221;
constexpr double NoiseSeconds = 0.019832;
constexpr double NoiseHighPassHz = 2303.84;
constexpr double NoiseLowPassHz = 2726.70;

//! Tone on the RD-8 is a balance between the two tones, which keep their pitches and decays. What
//! it does to the amplitude of each, against noon, and how far it turns its phase, measured on the
//! low passed hits of a sweep.
constexpr Drum808::KnobCurve HighToneLevel { { 0.0, 0.2, 0.5, 0.8, 1.0 }, { 0.2723, 0.4031, 1.0, 1.860, 2.406 } };
constexpr Drum808::KnobCurve HighTonePhase { { 0.0, 0.2, 0.5, 0.8, 1.0 }, { -1.47, -0.44, 0.0, 0.11, 0.25 } };
constexpr Drum808::KnobCurve LowToneLevel { { 0.0, 0.2, 0.5, 0.8, 1.0 }, { 2.239, 1.716, 1.0, 0.502, 0.459 } };
constexpr Drum808::KnobCurve LowTonePhase { { 0.0, 0.2, 0.5, 0.8, 1.0 }, { 0.27, 0.17, 0.0, -0.55, -1.07 } };
//! Snappy, in dB against noon: none at all at zero, and only 8.5 dB more than noon at full
constexpr Drum808::KnobCurve SnappyDb { { 0.0, 0.2, 0.5, 0.8, 1.0 }, { -120.0, -30.77, 0.0, 7.25, 8.50 } };

constexpr double ButterworthQ = 0.70710678;
constexpr double SilenceLevel = 1.0e-4;

} // namespace

Snare808Engine::Snare808Engine() = default;

void Snare808Engine::setTone(float tone)
{
    m_tone = tone;
}

void Snare808Engine::setSnappy(float snappy)
{
    m_snappy = snappy;
}

void Snare808Engine::trigger(float velocity)
{
    const double rate = sampleRate();
    m_active = true;
    m_time = 0.0;
    m_velocity = static_cast<double>(velocity);
    auto high = Modes.at(0);
    high.amplitude *= HighToneLevel.ratioAt(m_tone);
    high.phase += HighTonePhase.at(m_tone);
    auto low = Modes.at(1);
    low.amplitude *= LowToneLevel.ratioAt(m_tone);
    low.phase += LowTonePhase.at(m_tone);
    m_modes.at(0).start(high, rate);
    m_modes.at(1).start(low, rate);
    m_noiseLevel = NoiseLevel * std::pow(10.0, SnappyDb.at(m_snappy) / 20.0);
    m_noise.reseed();
    m_noiseHighPass.setHighPass(NoiseHighPassHz, ButterworthQ, rate);
    m_noiseLowPass.setLowPass(NoiseLowPassHz, ButterworthQ, rate);
    m_noiseHighPass.reset();
    m_noiseLowPass.reset();
    m_noiseScale = 1.0 / Drum808::impulseDeviation({ &m_noiseHighPass, &m_noiseLowPass });
}

float Snare808Engine::nextSample()
{
    if (!m_active) {
        return 0.0f;
    }
    const double t = m_time;
    double y = SpikeLevel * std::exp(-t / SpikeSeconds);
    for (auto && mode : m_modes) {
        y += mode.next();
    }
    y += m_noiseLevel * std::exp(-t / NoiseSeconds) * m_noiseScale * m_noiseLowPass.process(m_noiseHighPass.process(m_noise.next()));
    m_time += 1.0 / sampleRate();
    if (std::exp(-t / std::max(Modes.at(1).seconds, NoiseSeconds)) < SilenceLevel) {
        m_active = false;
    }
    return static_cast<float>(y * m_velocity);
}

bool Snare808Engine::isActive() const
{
    return m_active;
}

void Snare808Engine::reset()
{
    m_active = false;
    for (auto && mode : m_modes) {
        mode.stop();
    }
    m_noiseHighPass.reset();
    m_noiseLowPass.reset();
}

} // namespace noteahead
