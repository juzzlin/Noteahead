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

#include "tom_808_engine.hpp"

#include <cmath>
#include <numbers>

namespace noteahead {

//! One drum, fitted on its spectrogram within 2.0 to 3.3 dB at the Tuning it was recorded at. The
//! pitch is hz (1 + sweep exp(-t / sweepSeconds)), the tone -sin of its phase times exp(-t / seconds).
struct Tom808Engine::Voicing
{
    //! The Tuning the drum was recorded at
    double tuning;
    double hz;
    double sweep;
    double sweepSeconds;
    double seconds;
    double spike;
    double spikeSeconds;
    //! The noise's level against the tone, at unit deviation through its band, and its band:
    //! two band passes in a row
    double noise;
    double noiseSeconds;
    double noiseHz;
    double noiseQ;
    double gain;
};

namespace {

// The drums at noon, fitted on sweeps of their Tuning. The mid conga was not swept: it is fitted on
// a hit recorded with Tuning at about three quarters, where its channel's mid tom was.
constexpr Tom808Engine::Voicing LowTom { 0.5, 97.4516, 0.7173, 0.006097, 0.07049, 0.2127, 0.003, 0.008, 0.4, 250.0, 1.2, 0.4716 };
constexpr Tom808Engine::Voicing MidTom { 0.5, 146.355, 1.952, 0.0009331, 0.05408, 0.8424, 0.0006827, 0.004, 0.4, 200.0, 0.7, 0.6446 };
constexpr Tom808Engine::Voicing HighTom { 0.5, 198.649, 1.445, 0.001445, 0.04557, 1.949, 0.0002416, 0.004, 0.4, 300.0, 0.7, 0.6427 };
constexpr Tom808Engine::Voicing LowConga { 0.5, 214.967, 2.999, 0.0006645, 0.06934, 0.7993, 0.00199, 0.0, 0.4, 300.0, 0.7, 0.742 };
constexpr Tom808Engine::Voicing MidConga { 0.74, 329.709, 2.0, 0.0009054, 0.04323, 0.8152, 0.001517, 0.0, 0.4, 300.0, 0.7, 0.7862 };
constexpr Tom808Engine::Voicing HighConga { 0.5, 433.205, 2.285, 0.0006276, 0.03364, 1.554, 0.0002029, 0.0, 0.4, 300.0, 0.7, 0.7012 };

//! Tuning on the RD-8, in semitones from noon: the same on all five drums it was swept on within a
//! fifth of a semitone. Most of its range is between 2 and 8, the ends barely move.
constexpr Drum808::KnobCurve Tuning { { 0.0, 0.2, 0.5, 0.8, 1.0 }, { -1.733, -1.595, 0.0, 1.950, 2.237 } };
//! The drum is louder the higher it is tuned, by this many dB a semitone
constexpr double TuningLevelDbPerSemitone = 1.291;

//! A hit is over when everything in it is this far down
constexpr double SilenceLevel = 1.0e-4;

} // namespace

Tom808Engine::Tom808Engine(Position position)
  : m_position { position }
{
}

const Tom808Engine::Voicing & Tom808Engine::voicing() const
{
    switch (m_position) {
    case Position::Low:
        return m_conga ? LowConga : LowTom;
    case Position::Mid:
        return m_conga ? MidConga : MidTom;
    case Position::High:
        break;
    }
    return m_conga ? HighConga : HighTom;
}

void Tom808Engine::setTune(float tune)
{
    m_tune = tune;
}

void Tom808Engine::setConga(bool conga)
{
    m_conga = conga;
}

void Tom808Engine::trigger(float velocity)
{
    const double rate = sampleRate();
    const auto & v = voicing();
    m_active = true;
    m_time = 0.0;
    m_phase = 0.0;
    m_velocity = static_cast<double>(velocity);
    const double semitones = Tuning.at(m_tune) - Tuning.at(static_cast<float>(v.tuning));
    m_pitchRatio = std::pow(2.0, semitones / 12.0);
    m_level = std::pow(10.0, TuningLevelDbPerSemitone * semitones / 20.0);
    m_noise.reseed();
    m_noiseBand1.setBandPass(v.noiseHz, v.noiseQ, rate);
    m_noiseBand2.setBandPass(v.noiseHz, v.noiseQ, rate);
    m_noiseBand1.reset();
    m_noiseBand2.reset();
    // The fit divided the band by its own deviation for white noise at unit deviation, which is the
    // root of the energy of its impulse response
    m_noiseScale = v.noise > 0.0 ? 1.0 / Drum808::impulseDeviation({ &m_noiseBand1, &m_noiseBand2 }) : 0.0;
}

float Tom808Engine::nextSample()
{
    if (!m_active) {
        return 0.0f;
    }
    const double rate = sampleRate();
    const auto & v = voicing();
    const double t = m_time;
    const double hz = v.hz * m_pitchRatio * (1.0 + v.sweep * std::exp(-t / v.sweepSeconds));
    const double envelope = std::exp(-t / v.seconds);
    double y = -std::sin(m_phase) * envelope + v.spike * std::exp(-t / v.spikeSeconds);
    m_phase += 2.0 * std::numbers::pi * hz / rate;
    if (m_phase > 2.0 * std::numbers::pi) {
        m_phase -= 2.0 * std::numbers::pi;
    }
    double noiseEnvelope = 0.0;
    if (v.noise > 0.0) {
        noiseEnvelope = v.noise * std::exp(-t / v.noiseSeconds);
        y += noiseEnvelope * m_noiseScale * m_noiseBand2.process(m_noiseBand1.process(m_noise.next()));
    }
    m_time += 1.0 / rate;
    if (envelope < SilenceLevel && noiseEnvelope < SilenceLevel && t > v.spikeSeconds * 10.0) {
        m_active = false;
    }
    return static_cast<float>(y * v.gain * m_level * m_velocity);
}

bool Tom808Engine::isActive() const
{
    return m_active;
}

void Tom808Engine::reset()
{
    m_active = false;
    m_noiseBand1.reset();
    m_noiseBand2.reset();
}

} // namespace noteahead
