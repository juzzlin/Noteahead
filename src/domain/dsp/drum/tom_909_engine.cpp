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

#include "tom_909_engine.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace noteahead {

//! One drum, an RD-9's with every knob at noon, fitted on its spectrogram within 4.4 to 5 dB. Each
//! envelope is exp(-(t / tau)^curve) and the sweep f (1 + depth exp(-t / tau)).
struct Tom909Engine::Voicing
{
    double lowHz;
    double ratio;
    double sweepDepth;
    double sweepSeconds;
    double lowLevel;
    double lowSeconds;
    double lowCurve;
    double highSeconds;
    double highCurve;
    //! The second harmonic, which the resonance lifts on the low drum
    double secondHarmonic;
    double resonanceHz;
    double resonanceQ;
    double highPassHz;
    double noiseLevel;
    double noiseSeconds;
    double noiseHz;
    double noiseQ;
    double clickLevel;
    double clickSeconds;
    double clickHz;
    double gain;
};

namespace {

constexpr Tom909Engine::Voicing LowTom {
    .lowHz = 57.07,
    .ratio = 1.488,
    .sweepDepth = 0.3016,
    .sweepSeconds = 0.05647,
    .lowLevel = 0.06422,
    .lowSeconds = 0.3098,
    .lowCurve = 4.0,
    .highSeconds = 0.1418,
    .highCurve = 1.275,
    .secondHarmonic = 0.05127,
    .resonanceHz = 177.0,
    .resonanceQ = 2.11,
    .highPassHz = 416.2,
    .noiseLevel = 0.2323,
    .noiseSeconds = 0.02464,
    .noiseHz = 429.2,
    .noiseQ = 1.477,
    .clickLevel = 0.005606,
    .clickSeconds = 0.01,
    .clickHz = 2875.0,
    .gain = 4.420,
};
constexpr Tom909Engine::Voicing MidTom {
    .lowHz = 79.84,
    .ratio = 1.5855,
    .sweepDepth = 0.3317,
    .sweepSeconds = 0.03953,
    .lowLevel = 0.3022,
    .lowSeconds = 0.1955,
    .lowCurve = 2.254,
    .highSeconds = 0.1629,
    .highCurve = 1.912,
    .secondHarmonic = 0.0,
    .resonanceHz = 225.2,
    .resonanceQ = 1.924,
    .highPassHz = 555.6,
    .noiseLevel = 0.1114,
    .noiseSeconds = 0.02366,
    .noiseHz = 1135.0,
    .noiseQ = 3.966,
    .clickLevel = 0.1016,
    .clickSeconds = 0.000454,
    .clickHz = 271.7,
    .gain = 5.793,
};
constexpr Tom909Engine::Voicing HighTom {
    .lowHz = 109.86,
    .ratio = 1.5769,
    .sweepDepth = 0.2964,
    .sweepSeconds = 0.05908,
    .lowLevel = 0.1491,
    .lowSeconds = 0.2396,
    .lowCurve = 3.943,
    .highSeconds = 0.2007,
    .highCurve = 2.655,
    .secondHarmonic = 0.00174,
    .resonanceHz = 304.0,
    .resonanceQ = 3.889,
    .highPassHz = 527.4,
    .noiseLevel = 0.5413,
    .noiseSeconds = 0.02,
    .noiseHz = 293.2,
    .noiseQ = 2.031,
    .clickLevel = 0.00673,
    .clickSeconds = 0.01,
    .clickHz = 10000.0,
    .gain = 4.523,
};

//! Where the RD-9's Tune and Decay were measured, the low drum swept with the others at noon. The
//! other two follow its curves: on a 909 all three move alike.
constexpr std::array<double, 5> RdKnobPoints { 0.0, 0.2, 0.5, 0.8, 1.0 };
//! Tune moves both oscillators, keeping their ratio, by about an octave in all
constexpr std::array<double, 5> TuneRatio { 0.6704, 0.7855, 1.0, 1.229, 1.3168 };
//! Decay lengthens the upper oscillator's envelope, which is most of the drum, and the lower one's more
constexpr std::array<double, 5> HighDecayRatio { 0.6244, 0.7688, 1.0, 1.0514, 1.0726 };
constexpr std::array<double, 5> LowDecayRatio { 0.274, 0.613, 1.0, 1.194, 1.226 };

//! The rate the noise was fitted at; held to the same spectrum at any other
constexpr double ReferenceSampleRate = 48000.0;
constexpr double SilenceDb = -80.0;
constexpr uint32_t NoiseSeed = 0x2545F491u;

//! Odd harmonics as a triangle has them, the shape both oscillators are close to
constexpr std::array<double, 5> TriangleHarmonics { 1.0, -1.0 / 9.0, 1.0 / 25.0, -1.0 / 49.0, 1.0 / 81.0 };

double interpolate(const std::array<double, 5> & table, double knob)
{
    const double x = std::clamp(knob, 0.0, 1.0);
    size_t i = 0;
    while (i + 2 < RdKnobPoints.size() && x > RdKnobPoints[i + 1]) {
        i++;
    }
    const double fraction = (x - RdKnobPoints[i]) / (RdKnobPoints[i + 1] - RdKnobPoints[i]);
    return std::exp(std::log(table[i]) + fraction * (std::log(table[i + 1]) - std::log(table[i])));
}

const Tom909Engine::Voicing & voicingOf(Tom909Engine::Tom tom)
{
    switch (tom) {
    case Tom909Engine::Tom::Low:
        return LowTom;
    case Tom909Engine::Tom::Mid:
        return MidTom;
    case Tom909Engine::Tom::High:
        break;
    }
    return HighTom;
}

double envelope(double seconds, double tau, double curve)
{
    return std::exp(-std::pow(seconds / tau, curve));
}

//! When exp(-(t / tau)^curve) is 80 dB down
double silenceSeconds(double tau, double curve)
{
    return tau * std::pow(-SilenceDb / 8.685889638, 1.0 / curve);
}

} // namespace

Tom909Engine::Tom909Engine(Tom tom)
  : m_voicing { voicingOf(tom) }
{
    for (size_t i = 0; i < WaveTableSize; i++) {
        const double angle = 2.0 * std::numbers::pi * static_cast<double>(i) / static_cast<double>(WaveTableSize);
        double value = m_voicing.secondHarmonic * std::cos(2.0 * angle);
        for (size_t harmonic = 0; harmonic < TriangleHarmonics.size(); harmonic++) {
            value += TriangleHarmonics[harmonic] * std::sin(static_cast<double>(2 * harmonic + 1) * angle);
        }
        m_waveTable[i] = value;
    }
    m_waveTable[WaveTableSize] = m_waveTable[0];
    setSampleRate(ReferenceSampleRate);
    updateShape();
}

void Tom909Engine::updateShape()
{
    const double tune = static_cast<double>(m_tune);
    const double decay = static_cast<double>(m_decay);
    m_lowHz = m_voicing.lowHz * interpolate(TuneRatio, tune);
    m_highSeconds = m_voicing.highSeconds * interpolate(HighDecayRatio, decay);
    m_lowSeconds = m_voicing.lowSeconds * interpolate(LowDecayRatio, decay);
}

void Tom909Engine::setTune(float tune)
{
    m_tune = std::clamp(tune, 0.0f, 1.0f);
    updateShape();
}

void Tom909Engine::setDecay(float decay)
{
    m_decay = std::clamp(decay, 0.0f, 1.0f);
    updateShape();
}

void Tom909Engine::trigger(float velocity)
{
    m_active = true;
    m_time = 0.0;
    m_lowPhase = 0.0;
    m_highPhase = 0.0;
    m_velocity = static_cast<double>(std::clamp(velocity, 0.0f, 1.0f));
    m_noiseSeed = NoiseSeed;
    m_filterRate = 0.0;
}

double Tom909Engine::waveform(double phase) const
{
    const double position = phase * static_cast<double>(WaveTableSize);
    const auto index = std::min(static_cast<size_t>(position), WaveTableSize - 1);
    const double fraction = position - static_cast<double>(index);
    return m_waveTable[index] + fraction * (m_waveTable[index + 1] - m_waveTable[index]);
}

double Tom909Engine::nextNoise()
{
    // xorshift32: the same burst on every hit
    m_noiseSeed ^= m_noiseSeed << 13;
    m_noiseSeed ^= m_noiseSeed >> 17;
    m_noiseSeed ^= m_noiseSeed << 5;
    // Uniform -1..1 scaled to unit deviation, as the fit's Gaussian noise
    return std::sqrt(3.0) * (static_cast<double>(m_noiseSeed) / 2147483648.0 - 1.0);
}

double Tom909Engine::lengthSeconds() const
{
    return std::max(silenceSeconds(m_highSeconds, m_voicing.highCurve), silenceSeconds(m_lowSeconds, m_voicing.lowCurve));
}

float Tom909Engine::nextSample()
{
    if (!m_active) {
        return 0.0f;
    }

    const double rate = sampleRate();
    if (rate != m_filterRate) {
        // Set up at the first sample rather than at the trigger, so a rate set in between counts
        m_filterRate = rate;
        m_resonance.set(m_voicing.resonanceHz, m_voicing.resonanceQ, rate);
        m_bodyHighPass.set(m_voicing.highPassHz, rate);
        m_noise.set(m_voicing.noiseHz, m_voicing.noiseQ, rate);
        m_click.set(m_voicing.clickHz, rate);
        m_resonance.reset();
        m_bodyHighPass.reset();
        m_noise.reset();
        m_click.reset();
    }

    const double t = m_time;
    const double hz = m_lowHz * (1.0 + m_voicing.sweepDepth * std::exp(-t / m_voicing.sweepSeconds));
    const double body = m_voicing.lowLevel * waveform(m_lowPhase) * envelope(t, m_lowSeconds, m_voicing.lowCurve) //
      + waveform(m_highPhase) * envelope(t, m_highSeconds, m_voicing.highCurve);
    m_lowPhase += std::min(hz / rate, 0.5);
    m_lowPhase -= std::floor(m_lowPhase);
    m_highPhase += std::min(hz * m_voicing.ratio / rate, 0.5);
    m_highPhase -= std::floor(m_highPhase);

    double y = m_bodyHighPass.process(m_resonance.process(body));
    // White noise carries the same density at any rate when it scales with the rate's root
    const double density = std::sqrt(rate / ReferenceSampleRate);
    y += m_noise.process(nextNoise() * density) * m_voicing.noiseLevel * std::exp(-t / m_voicing.noiseSeconds);
    y += m_click.process(nextNoise() * density) * m_voicing.clickLevel * std::exp(-t / m_voicing.clickSeconds);
    y *= m_voicing.gain * m_velocity;

    m_time += 1.0 / rate;
    if (m_time > lengthSeconds()) {
        m_active = false;
    }
    return static_cast<float>(y);
}

bool Tom909Engine::isActive() const
{
    return m_active;
}

void Tom909Engine::reset()
{
    m_active = false;
    m_time = 0.0;
    m_noiseSeed = NoiseSeed;
    m_resonance.reset();
    m_bodyHighPass.reset();
    m_noise.reset();
    m_click.reset();
}

void Tom909Engine::BandPass::set(double hz, double q, double sampleRate)
{
    const double w = 2.0 * std::numbers::pi * std::min(hz, 0.45 * sampleRate) / sampleRate;
    const double alpha = std::sin(w) / (2.0 * q);
    const double a0 = 1.0 + alpha;
    b0 = alpha / a0;
    a1 = -2.0 * std::cos(w) / a0;
    a2 = (1.0 - alpha) / a0;
}

double Tom909Engine::BandPass::process(double input)
{
    const double output = b0 * (input - x2) - a1 * y1 - a2 * y2;
    x2 = x1;
    x1 = input;
    y2 = y1;
    y1 = output;
    return output;
}

void Tom909Engine::BandPass::reset()
{
    x1 = x2 = y1 = y2 = 0.0;
}

void Tom909Engine::HighPass::set(double hz, double sampleRate)
{
    a = std::exp(-2.0 * std::numbers::pi * std::min(hz, 0.45 * sampleRate) / sampleRate);
}

double Tom909Engine::HighPass::process(double input)
{
    y1 = 0.5 * (1.0 + a) * (input - x1) + a * y1;
    x1 = input;
    return y1;
}

void Tom909Engine::HighPass::reset()
{
    x1 = y1 = 0.0;
}

} // namespace noteahead
