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

//! The sweep, f = end + (start - end) exp(-(t / tau)^curve), per Tune setting. At noon it is an
//! RD-9's with every knob there: from 343 Hz to 83 Hz, tau 36 ms, curve 0.84, fitted within a hertz
//! to its zero crossings. That is longer and higher than the 909 recordings, whose own sweep was
//! 235 Hz to 49.5 Hz, tau 15 ms, curve 1.14; how Tune changes it around noon is still theirs,
//! scaled to the RD-9 there.
constexpr std::array<double, 4> SweepStartHz { 306.2, 329.8, 356.3, 373.9 };
constexpr std::array<double, 4> SweepSeconds { 0.02362, 0.02954, 0.04464, 0.06673 };
constexpr std::array<double, 4> SweepCurve { 0.951, 0.888, 0.792, 0.716 };

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

//! The RD-9's Pitch and Pitch Depth, measured on one at 0, 2, 5, 8 and 10 with the rest at noon.
//! Its sweep keeps the same shape at every setting, so they are ratios against its noon, which is
//! the sweep above.
constexpr std::array<double, 5> RdKnobPoints { 0.0, 0.2, 0.5, 0.8, 1.0 };
//! Pitch multiplies the whole sweep, start and end alike, within a few per cent at every moment
constexpr std::array<double, 5> PitchScale { 0.22, 0.345, 1.0, 1.79, 2.45 };
//! Pitch Depth moves where the sweep ends, all the way to 0 Hz, and the sweep's size only a little
constexpr std::array<double, 5> PitchDepthEnd { 0.0, 0.186, 1.0, 1.975, 2.114 };
constexpr std::array<double, 5> PitchDepthSweep { 0.453, 1.12, 1.0, 0.891, 0.876 };

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
    m_startHz = interpolate(KnobPoints, SweepStartHz, tune);
    m_sweepSeconds = interpolateGeometric(KnobPoints, SweepSeconds, tune);
    m_sweepCurve = interpolate(KnobPoints, SweepCurve, tune);

    const double decay = static_cast<double>(m_decay);
    m_decayCurve = interpolate(KnobPoints, DecayCurve, decay);
    // exp(-(t / tau)^q) is -40 dB where (t / tau)^q = 40 / 8.686
    m_decaySeconds = interpolateGeometric(KnobPoints, DecayTo40DbSeconds, decay) / std::pow(40.0 / 8.685889638, 1.0 / m_decayCurve);

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
        const double endHz = DefaultPitchHz * m_endRatio;
        const double sweepHz = (m_startHz - DefaultPitchHz) * m_sweepRatio;
        const double hz = m_pitchScale * (endHz + sweepHz * std::exp(-std::pow(bodyTime / m_sweepSeconds, m_sweepCurve)));
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
