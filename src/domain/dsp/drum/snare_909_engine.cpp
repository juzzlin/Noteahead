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

#include "snare_909_engine.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace noteahead {

namespace {

//! One damped mode of the body: amplitude exp(-t / tau) cos(2 pi f t + phase)
struct Mode
{
    double hz;
    double seconds;
    double amplitude;
    double phase;
};

//! The RD-9's body at every knob's noon with Snappy at 0, decomposed by matrix pencil: within 1 to
//! 2 dB of it from 10 ms on. The first two are the strikes, the next three the tones they settle
//! into; the rest are harmonics and difference tones.
constexpr std::array<Mode, Snare909Engine::ModeCount> Modes { {
  { 195.85, 0.00752, 0.273824, 1.1594 },
  { 264.48, 0.00675, 0.265568, -2.5946 },
  { 194.08, 0.03731, 0.116600, -2.4342 },
  { 401.06, 0.00687, 0.083625, -2.9055 },
  { 258.26, 0.03445, 0.049936, 0.6777 },
  { 388.76, 0.02783, 0.021217, 1.0982 },
  { 541.62, 0.01660, 0.008666, 2.9150 },
  { 126.42, 0.05100, 0.008324, -0.3759 },
  { 291.38, 0.01755, 0.005874, 2.9507 },
  { 514.37, 0.04541, 0.004745, 2.2067 },
  { 66.65, 0.19723, 0.001287, -1.7553 },
} };
//! The modes start a little after the click
constexpr double ModeStartSeconds = 0.000396;

//! The noise, fitted on the spectrogram with the body above fixed, within 2 to 4 dB of the RD-9
constexpr double NoiseHighPassHz = 1160.3;
constexpr double NoiseLowPassHz = 3790.6;
constexpr double NoiseAttackSeconds = 0.000362;
//! How the noise sags while the gate is open
constexpr double NoiseSagSeconds = 0.9357;

//! The stick on the head: high-passed noise over the first fraction of a millisecond
constexpr double ClickLevel = 0.010293;
constexpr double ClickSeconds = 0.000129;
constexpr double ClickHighPassHz = 11716.6;

//! Where the RD-9's knobs were measured, each with the others at noon
constexpr std::array<double, 5> RdKnobPoints { 0.0, 0.2, 0.5, 0.8, 1.0 };
constexpr std::array<double, 5> TuneRatio { 0.6595, 0.7606, 1.0, 1.2504, 1.3489 };
constexpr std::array<double, 5> GateSeconds { 0.0804, 0.1002, 0.1608, 0.1955, 0.1940 };
constexpr std::array<double, 5> ReleaseSeconds { 0.01373, 0.01624, 0.0236, 0.03398, 0.03615 };
constexpr std::array<double, 5> BodyLevel { 1.243, 1.07, 0.9206, 0.771, 0.6933 };
constexpr std::array<double, 5> NoiseLevel { 0.0, 0.008924, 0.02237, 0.03317, 0.035 };

//! The rate the noise was fitted at; held to the same spectrum at any other
constexpr double ReferenceSampleRate = 48000.0;
//! Where the hit is over: the longest gate and release, and the longest mode 80 dB down
constexpr double LengthSeconds = 0.5;
//! Brings it to the level the device mixes its drums from, the kick's peak at noon
constexpr double OutputLevel = 4.365;
constexpr uint32_t NoiseSeed = 0x6C8E9CF5u;

double interpolate(const std::array<double, 5> & table, double knob)
{
    const double x = std::clamp(knob, 0.0, 1.0);
    size_t i = 0;
    while (i + 2 < RdKnobPoints.size() && x > RdKnobPoints[i + 1]) {
        i++;
    }
    const double fraction = (x - RdKnobPoints[i]) / (RdKnobPoints[i + 1] - RdKnobPoints[i]);
    return table[i] + fraction * (table[i + 1] - table[i]);
}

} // namespace

Snare909Engine::Snare909Engine()
  : m_noiseSeed { NoiseSeed }
{
    setSampleRate(ReferenceSampleRate);
    updateShape();
}

void Snare909Engine::updateShape()
{
    m_tuneRatio = interpolate(TuneRatio, static_cast<double>(m_tune));
    m_gateSeconds = interpolate(GateSeconds, static_cast<double>(m_tone));
    m_releaseSeconds = interpolate(ReleaseSeconds, static_cast<double>(m_tone));
    m_bodyLevel = interpolate(BodyLevel, static_cast<double>(m_snappy));
    m_noiseLevel = interpolate(NoiseLevel, static_cast<double>(m_snappy));
}

void Snare909Engine::setTune(float tune)
{
    m_tune = std::clamp(tune, 0.0f, 1.0f);
    updateShape();
}

void Snare909Engine::setTone(float tone)
{
    m_tone = std::clamp(tone, 0.0f, 1.0f);
    updateShape();
}

void Snare909Engine::setSnappy(float snappy)
{
    m_snappy = std::clamp(snappy, 0.0f, 1.0f);
    updateShape();
}

void Snare909Engine::trigger(float velocity)
{
    m_active = true;
    m_modesStarted = false;
    m_time = 0.0;
    m_velocity = static_cast<double>(std::clamp(velocity, 0.0f, 1.0f));
    m_noiseSeed = NoiseSeed;
    m_filterRate = 0.0;
}

void Snare909Engine::startModes()
{
    const double rate = sampleRate();
    for (size_t i = 0; i < ModeCount; i++) {
        const auto & mode = Modes[i];
        m_modes[i] = std::polar(mode.amplitude, mode.phase);
        m_rotations[i] = std::polar(std::exp(-1.0 / (mode.seconds * rate)), 2.0 * std::numbers::pi * mode.hz * m_tuneRatio / rate);
    }
    m_modesStarted = true;
}

double Snare909Engine::nextNoise()
{
    // xorshift32: the same burst on every hit
    m_noiseSeed ^= m_noiseSeed << 13;
    m_noiseSeed ^= m_noiseSeed >> 17;
    m_noiseSeed ^= m_noiseSeed << 5;
    // Uniform -1..1 scaled to unit deviation, as the fit's Gaussian noise
    return std::sqrt(3.0) * (static_cast<double>(m_noiseSeed) / 2147483648.0 - 1.0);
}

float Snare909Engine::nextSample()
{
    if (!m_active) {
        return 0.0f;
    }

    const double rate = sampleRate();
    if (rate != m_filterRate) {
        // Set up at the first sample rather than at the trigger, so a rate set in between counts
        m_filterRate = rate;
        m_noiseHighPass.setHighPass(NoiseHighPassHz, rate);
        m_noiseLowPass.setLowPass(NoiseLowPassHz, rate);
        m_noiseHighPass.reset();
        m_noiseLowPass.reset();
        m_clickA = std::exp(-2.0 * std::numbers::pi * std::min(ClickHighPassHz, 0.45 * rate) / rate);
        m_clickX1 = m_clickY1 = 0.0;
    }

    const double t = m_time;
    double body = 0.0;
    if (t >= ModeStartSeconds) {
        if (!m_modesStarted) {
            startModes();
        }
        for (size_t i = 0; i < ModeCount; i++) {
            body += m_modes[i].real();
            m_modes[i] *= m_rotations[i];
        }
    }

    // White noise carries the same density at any rate when it scales with the rate's root
    const double density = std::sqrt(rate / ReferenceSampleRate);
    const double gate = t < m_gateSeconds ? 1.0 : std::exp(-(t - m_gateSeconds) / m_releaseSeconds);
    const double noiseEnvelope = (1.0 - std::exp(-t / NoiseAttackSeconds)) * std::exp(-t / NoiseSagSeconds) * gate;
    const double noise = m_noiseLowPass.process(m_noiseHighPass.process(nextNoise() * density)) * noiseEnvelope * m_noiseLevel;

    const double clickInput = nextNoise() * density;
    m_clickY1 = 0.5 * (1.0 + m_clickA) * (clickInput - m_clickX1) + m_clickA * m_clickY1;
    m_clickX1 = clickInput;
    const double click = m_clickY1 * ClickLevel * std::exp(-t / ClickSeconds);

    const double y = (m_bodyLevel * body + noise + click) * OutputLevel * m_velocity;

    m_time += 1.0 / rate;
    if (m_time > LengthSeconds) {
        m_active = false;
    }
    return static_cast<float>(y);
}

bool Snare909Engine::isActive() const
{
    return m_active;
}

void Snare909Engine::reset()
{
    m_active = false;
    m_modesStarted = false;
    m_time = 0.0;
    m_noiseSeed = NoiseSeed;
    m_noiseHighPass.reset();
    m_noiseLowPass.reset();
    m_clickX1 = m_clickY1 = 0.0;
}

void Snare909Engine::Biquad::setLowPass(double hz, double sampleRate)
{
    const double w = 2.0 * std::numbers::pi * std::min(hz, 0.45 * sampleRate) / sampleRate;
    const double alpha = std::sin(w) / (2.0 * std::numbers::sqrt2 / 2.0);
    const double c = std::cos(w);
    const double a0 = 1.0 + alpha;
    b0 = 0.5 * (1.0 - c) / a0;
    b1 = (1.0 - c) / a0;
    b2 = b0;
    a1 = -2.0 * c / a0;
    a2 = (1.0 - alpha) / a0;
}

void Snare909Engine::Biquad::setHighPass(double hz, double sampleRate)
{
    const double w = 2.0 * std::numbers::pi * std::min(hz, 0.45 * sampleRate) / sampleRate;
    const double alpha = std::sin(w) / (2.0 * std::numbers::sqrt2 / 2.0);
    const double c = std::cos(w);
    const double a0 = 1.0 + alpha;
    b0 = 0.5 * (1.0 + c) / a0;
    b1 = -(1.0 + c) / a0;
    b2 = b0;
    a1 = -2.0 * c / a0;
    a2 = (1.0 - alpha) / a0;
}

double Snare909Engine::Biquad::process(double input)
{
    const double output = b0 * input + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
    x2 = x1;
    x1 = input;
    y2 = y1;
    y1 = output;
    return output;
}

void Snare909Engine::Biquad::reset()
{
    x1 = x2 = y1 = y2 = 0.0;
}

} // namespace noteahead
