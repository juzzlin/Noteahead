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

#include "kick_909_engine.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>

namespace noteahead {

namespace {

//! The knob positions the recordings were made at: 0, 3, 7 and 10
constexpr std::array<double, 4> KnobPoints { 0.0, 0.3, 0.7, 1.0 };

//! The sweep, f = end + (start - end) exp(-(t / tau)^curve), per Tune setting: fitted to the zero
//! crossings of all six recordings at each, then refined against their first 60 ms
constexpr std::array<double, 4> SweepStartHz { 210.3, 226.5, 244.7, 256.8 };
constexpr std::array<double, 4> SweepSeconds { 0.01006, 0.01258, 0.01901, 0.02842 };
constexpr std::array<double, 4> SweepCurve { 1.294, 1.209, 1.078, 0.975 };

//! The amplitude, exp(-(t / tau)^curve), per Decay setting, within 0.5 to 1.9 dB. Given as the
//! time to -40 dB, which is what Decay is heard as, and interpolated geometrically. The short
//! settings fall away faster than an exponential towards their end.
constexpr std::array<double, 4> DecayTo40DbSeconds { 0.1048, 0.178, 0.3026, 0.4543 };
constexpr std::array<double, 4> DecayCurve { 3.498, 1.889, 1.345, 1.123 };
//! The first few cycles are louder than the decay accounts for: a boost on top that is gone in a
//! few milliseconds
constexpr double EarlyBoost = 0.482;
constexpr double EarlyBoostSeconds = 0.00614;

//! The oscillator's waveform, its first twelve harmonics against the fundamental. The 909's settled
//! cycle is the same in every recording, to a decibel and a few degrees: lopsided and flatter-topped
//! than a triangle, the third harmonic 20 dB down and the fifth and seventh both 34 dB.
constexpr size_t HarmonicCount = 12;
constexpr std::array<double, HarmonicCount> HarmonicLevel { 1.0, 0.03721, 0.09889, 0.00527, 0.02029, 0.01179, 0.01925, 0.00826, 0.00657, 0.00533, 0.00239, 0.00217 };
constexpr std::array<double, HarmonicCount> HarmonicPhase { 0.0, -2.7547, 0.2455, -1.3477, 3.0969, -1.0475, -2.8385, 1.0110, 0.9884, -3.0677, 2.5138, 2.3784 };
//! Where in its cycle the oscillator starts
constexpr double StartPhase = 0.7639;
constexpr size_t WaveTableSize = 4096;

//! The click: the trigger pulse leaking through as a ramp before the oscillator starts
constexpr double ClickDelaySeconds = 0.001;
constexpr double ClickLevel = 0.424;
constexpr double ClickRiseSeconds = 0.00139;

//! The click as the oscillator starts: an impulse rung through a broad band-pass. It is what puts the
//! recordings' first 4 ms 10 to 17 dB above the body alone from 2 to 8 kHz; with it the spectrum
//! there is within 3.1 dB of theirs, with the noise below.
constexpr double ClickImpulse = 1.505;
constexpr double ClickHz = 1237.0;
constexpr double ClickQ = 0.80;
//! How much louder Attack at full makes the click. The recordings' own Attack hardly changes it, so
//! this goes beyond them: the knob's square, so the lower half stays near the recordings (+3 dB at
//! noon) and the top reaches the hard click an RD-9 gets.
constexpr double MaxAttackClickDb = 12.0;

//! The noise over the start of the drum: band-passed, there at every setting, and Attack at full
//! doubles it. Fitted to the recordings' spectra from 4 to 60 ms within 2.7 dB.
constexpr double NoiseLevel = 0.0860;
constexpr double AttackNoiseLevel = 0.0803;
constexpr double NoiseSeconds = 0.0100;
constexpr double NoiseHz = 991.0;
constexpr double NoiseQ = 0.54;
//! The rate the click and the noise were fitted at; held to the same spectrum at any other
constexpr double ReferenceSampleRate = 44100.0;

//! The level of the whole, as the recordings have it
constexpr double OutputLevel = 0.888;

//! Drive at full: the gain into the saturator
constexpr double MaxDriveGain = 8.0;

constexpr double SilenceDb = -80.0;

constexpr uint32_t NoiseSeed = 0x9E3779B9u;

double interpolate(const std::array<double, 4> & table, double knob)
{
    const double x = std::clamp(knob, 0.0, 1.0);
    size_t i = 0;
    while (i + 2 < KnobPoints.size() && x > KnobPoints[i + 1]) {
        i++;
    }
    const double fraction = (x - KnobPoints[i]) / (KnobPoints[i + 1] - KnobPoints[i]);
    return table[i] + fraction * (table[i + 1] - table[i]);
}

double interpolateGeometric(const std::array<double, 4> & table, double knob)
{
    std::array<double, 4> logs {};
    std::ranges::transform(table, logs.begin(), [](double value) { return std::log(value); });
    return std::exp(interpolate(logs, knob));
}

//! One cycle of the 909's waveform, its peak at 1, with the first point repeated at the end
const std::array<double, WaveTableSize + 1> & waveTable()
{
    static const auto table = [] {
        std::array<double, WaveTableSize + 1> cycle {};
        double peak = 0.0;
        for (size_t i = 0; i < WaveTableSize; i++) {
            const double angle = 2.0 * std::numbers::pi * static_cast<double>(i) / static_cast<double>(WaveTableSize);
            for (size_t harmonic = 0; harmonic < HarmonicCount; harmonic++) {
                cycle[i] += HarmonicLevel[harmonic] * std::cos(static_cast<double>(harmonic + 1) * angle + HarmonicPhase[harmonic]);
            }
            peak = std::max(peak, std::abs(cycle[i]));
        }
        for (size_t i = 0; i < WaveTableSize; i++) {
            cycle[i] /= peak;
        }
        cycle[WaveTableSize] = cycle[0];
        return cycle;
    }();
    return table;
}

double oscillator(double phase)
{
    const auto & table = waveTable();
    const double position = phase * static_cast<double>(WaveTableSize);
    const auto index = std::min(static_cast<size_t>(position), WaveTableSize - 1);
    const double fraction = position - static_cast<double>(index);
    return table[index] + fraction * (table[index + 1] - table[index]);
}

} // namespace

Kick909Engine::Kick909Engine()
  : m_noiseSeed { NoiseSeed }
{
    setSampleRate(ReferenceSampleRate);
    updateShape();
}

void Kick909Engine::updateShape()
{
    const double tune = static_cast<double>(m_tune);
    m_startHz = interpolate(SweepStartHz, tune);
    m_sweepSeconds = interpolateGeometric(SweepSeconds, tune);
    m_sweepCurve = interpolate(SweepCurve, tune);

    const double decay = static_cast<double>(m_decay);
    m_decayCurve = interpolate(DecayCurve, decay);
    // exp(-(t / tau)^q) is -40 dB where (t / tau)^q = 40 / 8.686
    m_decaySeconds = interpolateGeometric(DecayTo40DbSeconds, decay) / std::pow(40.0 / 8.685889638, 1.0 / m_decayCurve);

    const double drive = static_cast<double>(m_drive);
    m_driveGain = 1.0 + (MaxDriveGain - 1.0) * drive * drive;
    m_driveNormalization = 1.0 / std::tanh(m_driveGain);
}

void Kick909Engine::setTune(float tune)
{
    m_tune = std::clamp(tune, 0.0f, 1.0f);
    updateShape();
}

void Kick909Engine::setAttack(float attack)
{
    m_attack = std::clamp(attack, 0.0f, 1.0f);
}

void Kick909Engine::setDecay(float decay)
{
    m_decay = std::clamp(decay, 0.0f, 1.0f);
    updateShape();
}

void Kick909Engine::setPitch(double hz)
{
    m_pitchHz = std::max(hz, 1.0);
}

void Kick909Engine::setPitchDepth(double depth)
{
    m_pitchDepth = std::max(depth, 0.0);
}

void Kick909Engine::setDrive(float drive)
{
    m_drive = std::clamp(drive, 0.0f, 1.0f);
    updateShape();
}

void Kick909Engine::trigger(float velocity)
{
    m_active = true;
    m_time = 0.0;
    m_phase = StartPhase;
    m_velocity = static_cast<double>(std::clamp(velocity, 0.0f, 1.0f));
    m_noiseSeed = NoiseSeed;
    m_click.set(ClickHz, ClickQ, sampleRate());
    m_noise.set(NoiseHz, NoiseQ, sampleRate());
    m_clicked = false;
    m_choke = 1.0;
    m_choking = false;
}

void Kick909Engine::BandPass::set(double hz, double q, double sampleRate)
{
    const double w = 2.0 * std::numbers::pi * std::min(hz, 0.45 * sampleRate) / sampleRate;
    const double alpha = std::sin(w) / (2.0 * q);
    const double a0 = 1.0 + alpha;
    b0 = alpha / a0;
    a1 = -2.0 * std::cos(w) / a0;
    a2 = (1.0 - alpha) / a0;
    reset();
}

double Kick909Engine::BandPass::process(double input)
{
    const double output = b0 * (input - x2) - a1 * y1 - a2 * y2;
    x2 = x1;
    x1 = input;
    y2 = y1;
    y1 = output;
    return output;
}

void Kick909Engine::BandPass::reset()
{
    x1 = x2 = y1 = y2 = 0.0;
}

double Kick909Engine::nextNoise()
{
    // xorshift32: the same burst on every hit, as cheap as noise gets
    m_noiseSeed ^= m_noiseSeed << 13;
    m_noiseSeed ^= m_noiseSeed >> 17;
    m_noiseSeed ^= m_noiseSeed << 5;
    // Uniform -1..1 scaled to unit deviation, as the fit's Gaussian noise
    return std::sqrt(3.0) * (static_cast<double>(m_noiseSeed) / 2147483648.0 - 1.0);
}

double Kick909Engine::bodyEnvelope(double seconds) const
{
    const double time = std::max(seconds, 0.0);
    return std::exp(-std::pow(time / m_decaySeconds, m_decayCurve)) * (1.0 + EarlyBoost * std::exp(-time / EarlyBoostSeconds));
}

double Kick909Engine::lengthSeconds() const
{
    return ClickDelaySeconds + m_decaySeconds * std::pow(-SilenceDb / 8.685889638, 1.0 / m_decayCurve);
}

float Kick909Engine::nextSample()
{
    if (!m_active) {
        return 0.0f;
    }

    const double rate = sampleRate();
    const double t = m_time;
    double y = 0.0;
    if (t < ClickDelaySeconds) {
        y = ClickLevel * (1.0 - std::exp(-t / ClickRiseSeconds));
    } else {
        const double bodyTime = t - ClickDelaySeconds;
        const double startHz = m_startHz * m_pitchHz / DefaultPitchHz;
        const double hz = m_pitchHz + m_pitchDepth * (startHz - m_pitchHz) * std::exp(-std::pow(bodyTime / m_sweepSeconds, m_sweepCurve));
        m_phase += std::min(hz / rate, 0.5);
        m_phase -= std::floor(m_phase);
        y = oscillator(m_phase) * bodyEnvelope(bodyTime);
    }

    // An impulse carries the same spectrum at any rate when it scales with the rate, and white noise
    // the same density when it scales with its root
    double impulse = 0.0;
    if (!m_clicked && t >= ClickDelaySeconds) {
        const double attack = static_cast<double>(m_attack);
        impulse = ClickImpulse * std::pow(10.0, MaxAttackClickDb * attack * attack / 20.0) * rate / ReferenceSampleRate;
        m_clicked = true;
    }
    y += m_click.process(impulse);
    const double white = nextNoise() * std::sqrt(rate / ReferenceSampleRate);
    y += m_noise.process(white) * (NoiseLevel + AttackNoiseLevel * static_cast<double>(m_attack)) * std::exp(-t / NoiseSeconds);

    y *= OutputLevel * m_velocity;
    if (m_drive > 0.0f) {
        y = std::tanh(m_driveGain * y) * m_driveNormalization;
    }

    if (m_choking) {
        m_choke -= 1.0 / (static_cast<double>(ChokeFadeSeconds) * rate);
        if (m_choke <= 0.0) {
            m_active = false;
            return 0.0f;
        }
        y *= m_choke;
    }

    m_time += 1.0 / rate;
    if (m_time > lengthSeconds()) {
        m_active = false;
    }
    return static_cast<float>(y);
}

bool Kick909Engine::isActive() const
{
    return m_active;
}

void Kick909Engine::reset()
{
    m_active = false;
    m_time = 0.0;
    m_phase = StartPhase;
    m_click.reset();
    m_noise.reset();
    m_clicked = false;
    m_noiseSeed = NoiseSeed;
    m_choke = 1.0;
    m_choking = false;
}

void Kick909Engine::stop()
{
    if (m_active) {
        m_choking = true;
    }
}

} // namespace noteahead
