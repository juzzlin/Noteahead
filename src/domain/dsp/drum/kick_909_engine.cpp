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

//! Where the RD-9's Tune, Attack, Decay, Pitch Depth and Pitch were measured: 0, 2, 5, 8 and 10
//! with the rest at noon.
constexpr std::array<double, 5> RdKnobPoints { 0.0, 0.2, 0.5, 0.8, 1.0 };

//! The sweep, f = end + (start - end) exp(-(t / tau)^curve), per Tune setting, fitted within a
//! hertz to the RD-9's zero crossings. At noon it sweeps from 338.5 Hz to 83 Hz, tau 35.3 ms,
//! curve 0.821; Tune lengthens tau from 21.8 ms at 0 to 100.9 ms at 10.
constexpr std::array<double, 5> SweepStartHz { 346.9, 343.2, 338.5, 328.2, 327.4 };
constexpr std::array<double, 5> SweepSeconds { 0.02180, 0.02668, 0.03534, 0.07760, 0.10087 };
constexpr std::array<double, 5> SweepCurve { 0.805, 0.811, 0.821, 0.848, 0.848 };

//! The amplitude, exp(-(t / tau)^curve), per Decay setting, within 0.7 to 0.9 dB rms down to
//! -45 dB. Given as the time to -40 dB, which is what Decay is heard as, and interpolated
//! geometrically. The knob's top half is where the length is: 0.39 s at noon and 1.74 s at full,
//! and short settings fall away faster towards their end.
constexpr std::array<double, 5> DecayTo40DbSeconds { 0.2279, 0.2957, 0.3890, 1.4070, 1.7420 };
constexpr std::array<double, 5> DecayCurve { 2.076, 1.845, 1.665, 1.201, 1.126 };

//! The oscillator's waveform, its first twelve harmonics against the fundamental. The RD-9's
//! settled cycle is the same in every recording, to half a decibel: a rounded triangle, the second
//! harmonic 31 dB down and the third 29 dB.
constexpr size_t HarmonicCount = 12;
constexpr std::array<double, HarmonicCount> HarmonicLevel { 1.0, 0.02835, 0.03534, 0.00398, 0.00795, 0.00248, 0.00733, 0.00327, 0.00304, 0.00107, 0.00018, 0.00089 };
constexpr std::array<double, HarmonicCount> HarmonicPhase { 0.0, -2.3933, 2.7763, -0.0630, 2.9365, 2.6986, -0.3918, -2.5394, -0.7504, -1.7098, 1.7480, -2.7876 };
//! Where in its cycle the oscillator starts
constexpr double StartPhase = 0.7935;
constexpr size_t WaveTableSize = 4096;

//! The oscillator starts this soon after the trigger, which puts its zero crossings within a
//! twentieth of a millisecond of the RD-9's
constexpr double BodyDelaySeconds = 0.0006;
//! The click, an RD-9's at every Attack, fitted on its waveform and its spectra over the first
//! 30 ms within 0.9 to 2.1 dB and on its peak within a third of a decibel. It is three pulses,
//! t/tau exp(1 - t/tau) from where each starts, against the body's peak: a small hump at the
//! trigger, a slower fall from 0.95 ms and a sharp spike down at 2.2 ms. The body is held under
//! them until the spike.
constexpr double HumpLevel = 0.068;
constexpr double HumpSeconds = 0.000548;
constexpr double LeadLevel = 0.246;
constexpr double LeadStartSeconds = 0.000946;
constexpr double LeadSeconds = 0.000804;
constexpr double SpikeLevel = 0.472;
constexpr double SpikeStartSeconds = 0.002203;
constexpr double SpikeSeconds = 0.000082;
constexpr double BodyGateSeconds = 0.002162;
constexpr double BodyGateRiseSeconds = 0.000200;
//! What Attack at full adds to the hump and the spike: most of its work is between 5 and 8.
constexpr double AttackHumpLevel = 0.143;
constexpr double AttackSpikeLevel = 1.136;
constexpr std::array<double, 5> AttackCurve { 0.0, 0.098, 0.247, 0.845, 1.0 };
//! Past this the pulses are spent and the gate open
constexpr double ClickSeconds = 0.01;

//! The noise over the start of the drum: band-passed and the same at every Attack, as on the RD-9.
//! Fitted to the RD-9 recordings' spectra from 4 to 60 ms within 1.2 to 2.8 dB.
constexpr double NoiseLevel = 0.0600;
constexpr double NoiseSeconds = 0.0100;
constexpr double NoiseHz = 991.0;
constexpr double NoiseQ = 0.54;
//! The rate the noise was fitted at; held to the same spectrum at any other
constexpr double ReferenceSampleRate = 44100.0;

//! The level of the whole: Init's body peaks where the Kick Synth puts its own peak. The click's
//! spike goes 0.6 to 6.7 dB over it, as an RD-9's does.
constexpr double OutputLevel = 1.148;

//! Drive at full: the gain into the saturator
constexpr double MaxDriveGain = 8.0;

//! Pitch multiplies the whole sweep, start and end alike, within a per cent across the hit
constexpr std::array<double, 5> PitchScale { 0.239, 0.343, 1.0, 1.830, 2.460 };
//! Pitch Depth moves where the sweep ends, all the way to 0 Hz, and the sweep's size only a little
constexpr std::array<double, 5> PitchDepthEnd { 0.0, 0.179, 1.0, 1.805, 2.018 };
constexpr std::array<double, 5> PitchDepthSweep { 1.142, 1.114, 1.0, 0.904, 0.875 };

constexpr double SilenceDb = -80.0;

constexpr uint32_t NoiseSeed = 0x9E3779B9u;

//! @p table at @p knob, straight between the knob positions it was measured at
template<size_t Size>
double interpolate(const std::array<double, Size> & points, const std::array<double, Size> & table, double knob)
{
    const double x = std::clamp(knob, 0.0, 1.0);
    size_t i = 0;
    while (i + 2 < Size && x > points[i + 1]) {
        i++;
    }
    const double fraction = (x - points[i]) / (points[i + 1] - points[i]);
    return table[i] + fraction * (table[i + 1] - table[i]);
}

//! The same, geometrically: for times and frequency ratios, which the ear hears that way
template<size_t Size>
double interpolateGeometric(const std::array<double, Size> & points, const std::array<double, Size> & table, double knob)
{
    std::array<double, Size> logs {};
    std::ranges::transform(table, logs.begin(), [](double value) { return std::log(value); });
    return std::exp(interpolate(points, logs, knob));
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

//! A pulse of unit peak, (t / tau) exp(1 - t / tau) from @p start
double pulse(double time, double start, double tau)
{
    const double u = (time - start) / tau;
    return u > 0.0 ? u * std::exp(1.0 - u) : 0.0;
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
    m_startHz = interpolate(RdKnobPoints, SweepStartHz, tune);
    m_sweepSeconds = interpolateGeometric(RdKnobPoints, SweepSeconds, tune);
    m_sweepCurve = interpolate(RdKnobPoints, SweepCurve, tune);

    const double decay = static_cast<double>(m_decay);
    m_decayCurve = interpolate(RdKnobPoints, DecayCurve, decay);
    // exp(-(t / tau)^q) is -40 dB where (t / tau)^q = 40 / 8.686
    m_decaySeconds = interpolateGeometric(RdKnobPoints, DecayTo40DbSeconds, decay) / std::pow(40.0 / 8.685889638, 1.0 / m_decayCurve);

    m_pitchScale = interpolateGeometric(RdKnobPoints, PitchScale, static_cast<double>(m_pitch)) * m_noteRatio;
    m_endRatio = interpolate(RdKnobPoints, PitchDepthEnd, static_cast<double>(m_pitchDepth));
    m_sweepRatio = interpolate(RdKnobPoints, PitchDepthSweep, static_cast<double>(m_pitchDepth));

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
    m_attackAmount = interpolate(RdKnobPoints, AttackCurve, static_cast<double>(m_attack));
}

void Kick909Engine::setDecay(float decay)
{
    m_decay = std::clamp(decay, 0.0f, 1.0f);
    updateShape();
}

void Kick909Engine::setPitch(float pitch)
{
    m_pitch = std::clamp(pitch, 0.0f, 1.0f);
    updateShape();
}

void Kick909Engine::setPitchDepth(float depth)
{
    m_pitchDepth = std::clamp(depth, 0.0f, 1.0f);
    updateShape();
}

void Kick909Engine::setNoteRatio(double ratio)
{
    m_noteRatio = std::max(ratio, 0.0);
    updateShape();
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
    m_noise.set(NoiseHz, NoiseQ, sampleRate());
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
    return std::exp(-std::pow(time / m_decaySeconds, m_decayCurve));
}

double Kick909Engine::lengthSeconds() const
{
    return BodyDelaySeconds + m_decaySeconds * std::pow(-SilenceDb / 8.685889638, 1.0 / m_decayCurve);
}

float Kick909Engine::nextSample()
{
    if (!m_active) {
        return 0.0f;
    }

    const double rate = sampleRate();
    const double t = m_time;
    double y = 0.0;
    if (t >= BodyDelaySeconds) {
        const double bodyTime = t - BodyDelaySeconds;
        const double endHz = DefaultPitchHz * m_endRatio;
        const double sweepHz = (m_startHz - DefaultPitchHz) * m_sweepRatio;
        const double hz = m_pitchScale * (endHz + sweepHz * std::exp(-std::pow(bodyTime / m_sweepSeconds, m_sweepCurve)));
        m_phase += std::min(hz / rate, 0.5);
        m_phase -= std::floor(m_phase);
        y = oscillator(m_phase) * bodyEnvelope(bodyTime);
    }

    if (t < ClickSeconds) {
        y = y / (1.0 + std::exp(-(t - BodyGateSeconds) / BodyGateRiseSeconds)) //
          + (HumpLevel + AttackHumpLevel * m_attackAmount) * pulse(t, 0.0, HumpSeconds) //
          - LeadLevel * pulse(t, LeadStartSeconds, LeadSeconds) //
          - (SpikeLevel + AttackSpikeLevel * m_attackAmount) * pulse(t, SpikeStartSeconds, SpikeSeconds);
    }

    // White noise carries the same density at any rate when it scales with the rate's root
    const double white = nextNoise() * std::sqrt(rate / ReferenceSampleRate);
    y += m_noise.process(white) * NoiseLevel * std::exp(-t / NoiseSeconds);

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
    m_noise.reset();
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
