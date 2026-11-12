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

#include "metal_909_engine.hpp"

#include "metal_909_sources.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <span>

namespace noteahead {

//! One sound: its spectrum and how the knobs act on it.
//!
//! A hat's envelope is gain (1 - exp(-t / attack)) (chick exp(-t / chickTime) + (1 - chick)
//! exp(-(t / decay)^curve)), per Decay setting: the open hat's fitted on the spectrogram within 4 to
//! 5 dB, the closed hat's on its 1 to 16 kHz envelope within 1.2 dB. A cymbal's partials and bands
//! carry their own decays instead, and it ends where its sample does: within 3 to 4 dB of the RD-9's
//! bands at noon, 4 to 8 dB across Tune.
struct Metal909Engine::Voicing
{
    std::span<const Metal909Sources::Partial> partials;
    std::span<const Metal909Sources::Band> bands;
    std::array<double, 5> tuneRatio;
    //! Whether Decay shapes the sound, which only the hats have
    bool hasDecay;
    double attackSeconds;
    double chickLevel;
    std::array<double, 5> chickSeconds;
    std::array<double, 5> decaySeconds;
    std::array<double, 5> decayCurve;
    std::array<double, 5> gain;
    //! Whether each band has noise of its own. The hats were fitted with one noise through them all.
    bool independentBands;
    //! A burst of band-passed noise on the strike: level, centre, Q and the time it dies in
    double strikeLevel;
    double strikeHz;
    double strikeQ;
    double strikeSeconds;
    //! Where the sample's top end is cut, or 0 for nowhere
    double lowPassHz;
    //! Where the sample ends at noon, and how fast it fades there, or 0 for a sound that does not
    double sampleSeconds;
    double fadeSeconds;
    //! Brings the sound to the level the device mixes its drums from, the kick's peak at noon
    double outputLevel;
};

namespace {

//! Where the RD-9's Tune and Decay were measured, each with the other at noon
constexpr std::array<double, 5> RdKnobPoints { 0.0, 0.2, 0.5, 0.8, 1.0 };
//! A third of an octave
constexpr double BandQ = 4.32;
//! The rate the noise was fitted at; held to the same spectrum at any other
constexpr double ReferenceSampleRate = 48000.0;
constexpr double SilenceLevel = 1e-4;
//! A reversed cymbal fades in over this, rather than starting with a step
constexpr double ReverseFadeInSeconds = 0.01;
constexpr uint32_t NoiseSeed = 0x1B873593u;

const Metal909Engine::Voicing & voicingOf(Metal909Engine::Sound sound)
{
    static const Metal909Engine::Voicing closedHat {
        .partials = Metal909Sources::closedHatPartials(),
        .bands = Metal909Sources::closedHatBands(),
        .tuneRatio = { 0.7611, 0.8437, 1.0, 1.2883, 1.5221 },
        .hasDecay = true,
        .attackSeconds = 0.000698,
        .chickLevel = 0.6973,
        .chickSeconds = { 0.0065, 0.0032, 0.0043, 0.0050, 0.0049 },
        .decaySeconds = { 0.0012, 0.0321, 0.0607, 0.0820, 0.0845 },
        .decayCurve = { 0.456, 1.385, 1.880, 2.475, 2.493 },
        .gain = { 4.216, 4.1, 3.717, 3.493, 3.711 },
        .independentBands = false,
        .strikeLevel = 0.0,
        .strikeHz = 1000.0,
        .strikeQ = 1.0,
        .strikeSeconds = 0.01,
        .lowPassHz = 0.0,
        .sampleSeconds = 0.0,
        .fadeSeconds = 0.0,
        .outputLevel = 23.05,
    };
    static const Metal909Engine::Voicing openHat {
        .partials = Metal909Sources::openHatPartials(),
        .bands = Metal909Sources::openHatBands(),
        .tuneRatio = { 0.7546, 0.8176, 1.0, 1.3072, 1.5092 },
        .hasDecay = true,
        .attackSeconds = 0.00013,
        .chickLevel = 0.0,
        .chickSeconds = { 0.001, 0.001, 0.001, 0.001, 0.001 },
        .decaySeconds = { 0.0610, 0.06747, 0.07473, 0.1206, 0.1258 },
        .decayCurve = { 1.12, 1.122, 1.099, 1.104, 1.104 },
        .gain = { 1.688, 1.658, 1.659, 1.547, 1.538 },
        .independentBands = false,
        .strikeLevel = 0.0,
        .strikeHz = 1000.0,
        .strikeQ = 1.0,
        .strikeSeconds = 0.01,
        .lowPassHz = 0.0,
        .sampleSeconds = 0.0,
        .fadeSeconds = 0.0,
        .outputLevel = 34.0,
    };
    static const Metal909Engine::Voicing crash {
        .partials = Metal909Sources::crashPartials(),
        .bands = Metal909Sources::crashBands(),
        // From where the sample ends at each Tune setting, which moves as its speed does
        .tuneRatio = { 0.7586, 0.8201, 1.0, 1.3184, 1.5170 },
        .hasDecay = false,
        .attackSeconds = 0.055,
        .chickLevel = 0.42,
        .chickSeconds = { 0.001, 0.001, 0.001, 0.001, 0.001 },
        .decaySeconds = { 1.0, 1.0, 1.0, 1.0, 1.0 },
        .decayCurve = { 1.0, 1.0, 1.0, 1.0, 1.0 },
        .gain = { 1.0, 1.0, 1.0, 1.0, 1.0 },
        .independentBands = true,
        .strikeLevel = 0.2624,
        .strikeHz = 540.0,
        .strikeQ = 1.80,
        .strikeSeconds = 0.030,
        .lowPassHz = 8500.0,
        .sampleSeconds = 0.83,
        .fadeSeconds = 0.02,
        .outputLevel = 18.4,
    };
    static const Metal909Engine::Voicing ride {
        .partials = Metal909Sources::ridePartials(),
        .bands = Metal909Sources::rideBands(),
        .tuneRatio = { 0.7582, 0.8138, 1.0, 1.2861, 1.5603 },
        .hasDecay = false,
        .attackSeconds = 0.0,
        .chickLevel = 0.0,
        .chickSeconds = { 0.001, 0.001, 0.001, 0.001, 0.001 },
        .decaySeconds = { 1.0, 1.0, 1.0, 1.0, 1.0 },
        .decayCurve = { 1.0, 1.0, 1.0, 1.0, 1.0 },
        .gain = { 1.0, 1.0, 1.0, 1.0, 1.0 },
        .independentBands = true,
        .strikeLevel = 0.02093,
        .strikeHz = 482.3,
        .strikeQ = 0.6292,
        .strikeSeconds = 0.01061,
        .lowPassHz = 13000.0,
        .sampleSeconds = 0.87,
        .fadeSeconds = 0.015,
        .outputLevel = 24.7,
    };
    switch (sound) {
    case Metal909Engine::Sound::ClosedHat:
        return closedHat;
    case Metal909Engine::Sound::OpenHat:
        return openHat;
    case Metal909Engine::Sound::Crash:
        return crash;
    case Metal909Engine::Sound::Ride:
        break;
    }
    return ride;
}

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

//! What a component with @p seconds of decay is multiplied by per base-rate sample, played at
//! @p ratio times its speed: growing instead when played backwards
double decayStep(double seconds, double ratio, double rate, bool reversed)
{
    if (seconds <= 0.0) {
        return 1.0;
    }
    const double exponent = ratio / (seconds * rate);
    return std::exp(reversed ? exponent : -exponent);
}

} // namespace

Metal909Engine::Metal909Engine(Sound sound, bool reversed)
  : m_sound { sound }
  , m_reversed { reversed && (sound == Sound::Crash || sound == Sound::Ride) }
  , m_voicing { voicingOf(sound) }
  , m_noiseSeed { NoiseSeed }
{
    setSampleRate(ReferenceSampleRate);
    updateShape();
}

void Metal909Engine::updateShape()
{
    const double tune = static_cast<double>(m_tune);
    const double decay = static_cast<double>(m_decay);
    m_tuneRatio = interpolate(m_voicing.tuneRatio, tune);
    m_decaySeconds = interpolate(m_voicing.decaySeconds, decay);
    m_decayCurve = interpolate(m_voicing.decayCurve, decay);
    m_chickSeconds = interpolate(m_voicing.chickSeconds, decay);
    m_gain = interpolate(m_voicing.gain, decay);
}

void Metal909Engine::setTune(float tune)
{
    m_tune = std::clamp(tune, 0.0f, 1.0f);
    updateShape();
}

void Metal909Engine::setDecay(float decay)
{
    m_decay = std::clamp(decay, 0.0f, 1.0f);
    updateShape();
}

double Metal909Engine::lengthSeconds() const
{
    if (m_voicing.sampleSeconds > 0.0) {
        // Forwards it fades out where the sample ends; backwards it ends on the strike
        const double tail = m_reversed ? 0.0 : m_voicing.fadeSeconds * std::log(1.0 / SilenceLevel);
        return (m_voicing.sampleSeconds + tail) / m_tuneRatio;
    }
    const double chick = m_voicing.chickLevel > 0.0 ? m_chickSeconds * std::log(m_voicing.chickLevel / SilenceLevel) : 0.0;
    const double body = m_decaySeconds * std::pow(std::log((1.0 - m_voicing.chickLevel) / SilenceLevel), 1.0 / m_decayCurve);
    return std::max(chick, body);
}

double Metal909Engine::sampleSeconds() const
{
    const double played = m_time * m_tuneRatio;
    return m_reversed ? m_voicing.sampleSeconds - played : played;
}

double Metal909Engine::envelope(double seconds) const
{
    if (!m_voicing.hasDecay) {
        const double position = sampleSeconds();
        const double bloom = m_voicing.attackSeconds > 0.0 ? 1.0 - (1.0 - m_voicing.chickLevel) * std::exp(-std::max(0.0, position) / m_voicing.attackSeconds) : 1.0;
        if (m_reversed) {
            return bloom * std::min(1.0, seconds / ReverseFadeInSeconds);
        }
        const double fade = position < m_voicing.sampleSeconds ? 1.0 : std::exp(-(position - m_voicing.sampleSeconds) / m_voicing.fadeSeconds);
        return bloom * fade;
    }
    const double chick = m_voicing.chickLevel * std::exp(-seconds / m_chickSeconds);
    const double body = (1.0 - m_voicing.chickLevel) * std::exp(-std::pow(seconds / m_decaySeconds, m_decayCurve));
    return m_gain * (1.0 - std::exp(-seconds / m_voicing.attackSeconds)) * (chick + body);
}

void Metal909Engine::trigger(float velocity)
{
    m_active = true;
    m_started = false;
    m_time = 0.0;
    m_velocity = static_cast<double>(std::clamp(velocity, 0.0f, 1.0f));
    m_noiseSeed = NoiseSeed;
    m_choke = 1.0;
    m_choking = false;
}

void Metal909Engine::stop()
{
    if (m_active) {
        m_choking = true;
    }
}

void Metal909Engine::start()
{
    // Played at Tune's speed, so every partial and band moves together, and what the faster speed
    // takes past the base rate's Nyquist is left out. Backwards, each component starts where the
    // sample ends and grows towards the strike.
    const double rate = baseSampleRate();
    const double startSeconds = m_reversed ? m_voicing.sampleSeconds : 0.0;
    const auto startLevel = [startSeconds](double seconds) { return seconds > 0.0 ? std::exp(-startSeconds / seconds) : 1.0; };

    m_partials.clear();
    m_rotations.clear();
    for (auto && partial : m_voicing.partials) {
        const double hz = partial.hz * m_tuneRatio;
        if (hz < 0.45 * rate) {
            m_partials.push_back(std::polar(partial.amplitude * startLevel(partial.seconds), partial.phase));
            m_rotations.push_back(std::polar(decayStep(partial.seconds, m_tuneRatio, rate, m_reversed), 2.0 * std::numbers::pi * hz / rate));
        }
    }
    m_bands.clear();
    // Moving a constant-Q band's centre by m_tuneRatio widens its passband by the same factor, so
    // an independent band's level is scaled by the root of the speed to keep its energy invariant
    const double bandScale = m_voicing.independentBands ? 1.0 / std::sqrt(m_tuneRatio) : 1.0;
    uint32_t seed = NoiseSeed;
    for (auto && band : m_voicing.bands) {
        const double hz = band.hz * m_tuneRatio;
        if (hz < 0.45 * rate) {
            NoiseBand noise;
            noise.filter.setBandPass(hz, BandQ, rate);
            noise.level = band.gain * bandScale * startLevel(band.seconds);
            noise.decay = decayStep(band.seconds, m_tuneRatio, rate, m_reversed);
            seed = seed * 1664525u + 1013904223u;
            noise.seed = seed | 1u;
            m_bands.push_back(noise);
        }
    }
    m_strike = NoiseBand {};
    if (m_voicing.strikeLevel > 0.0) {
        m_strike.filter.setBandPass(std::min(m_voicing.strikeHz * m_tuneRatio, 0.45 * rate), m_voicing.strikeQ, rate);
        m_strike.level = m_voicing.strikeLevel * bandScale * startLevel(m_voicing.strikeSeconds);
        m_strike.decay = decayStep(m_voicing.strikeSeconds, m_tuneRatio, rate, m_reversed);
        m_strike.seed = 0x5BD1E995u;
    }
    if (m_voicing.lowPassHz > 0.0) {
        m_lowPass1 = Biquad {};
        m_lowPass2 = Biquad {};
        const double cutoffHz = std::min(m_voicing.lowPassHz, 0.45 * rate);
        m_lowPass1.setLowPass(cutoffHz, rate);
        m_lowPass2.setLowPass(cutoffHz, rate);
    }
    m_source.reset();
    m_started = true;
}

double Metal909Engine::nextNoise(uint32_t & seed)
{
    // xorshift32: the same noise on every hit
    seed ^= seed << 13;
    seed ^= seed >> 17;
    seed ^= seed << 5;
    // Uniform -1..1 scaled to unit deviation, as the fit's Gaussian noise
    return std::sqrt(3.0) * (static_cast<double>(seed) / 2147483648.0 - 1.0);
}

double Metal909Engine::nextBaseSample()
{
    double y = 0.0;
    for (size_t i = 0; i < m_partials.size(); i++) {
        y += m_partials[i].real();
        m_partials[i] *= m_rotations[i];
    }
    // White noise carries the same density at any rate when it scales with the rate's root
    const double density = std::sqrt(baseSampleRate() / ReferenceSampleRate);
    const double shared = m_voicing.independentBands ? 0.0 : nextNoise(m_noiseSeed) * density;
    for (auto && band : m_bands) {
        const double white = m_voicing.independentBands ? nextNoise(band.seed) * density : shared;
        y += band.filter.process(white) * band.level;
        band.level *= band.decay;
    }
    if (m_strike.level > 0.0) {
        y += m_strike.filter.process(nextNoise(m_strike.seed) * density) * m_strike.level;
        m_strike.level *= m_strike.decay;
    }
    if (m_voicing.lowPassHz > 0.0) {
        y = m_lowPass2.process(m_lowPass1.process(y));
    }
    return y * envelope(m_time);
}

float Metal909Engine::nextSample()
{
    if (!m_active) {
        return 0.0f;
    }
    if (!m_started) {
        start();
    }

    // The spectrum is made at the base rate, where it was measured, and brought up to the rate
    // the device runs at: a hundred partials are not worth making at four times the rate
    m_source.setOversampleFactor(oversampleFactor());
    if (m_source.needsBaseSample()) {
        m_source.setBaseSample(static_cast<float>(nextBaseSample()));
        m_time += 1.0 / baseSampleRate();
    }
    double y = static_cast<double>(m_source.nextSample()) * m_voicing.outputLevel * m_velocity;

    if (m_choking) {
        m_choke -= 1.0 / (static_cast<double>(ChokeFadeSeconds) * sampleRate());
        if (m_choke <= 0.0) {
            m_active = false;
            return 0.0f;
        }
        y *= m_choke;
    }

    if (m_time > lengthSeconds()) {
        m_active = false;
    }
    return static_cast<float>(y);
}

bool Metal909Engine::isActive() const
{
    return m_active;
}

void Metal909Engine::reset()
{
    m_active = false;
    m_started = false;
    m_time = 0.0;
    m_noiseSeed = NoiseSeed;
    m_choke = 1.0;
    m_choking = false;
    m_source.reset();
}

void Metal909Engine::Biquad::setBandPass(double hz, double q, double sampleRate)
{
    const double w = 2.0 * std::numbers::pi * std::min(hz, 0.45 * sampleRate) / sampleRate;
    const double alpha = std::sin(w) / (2.0 * q);
    const double a0 = 1.0 + alpha;
    b0 = alpha / a0;
    b1 = 0.0;
    b2 = -b0;
    a1 = -2.0 * std::cos(w) / a0;
    a2 = (1.0 - alpha) / a0;
}

void Metal909Engine::Biquad::setLowPass(double hz, double sampleRate)
{
    const double w = 2.0 * std::numbers::pi * std::min(hz, 0.45 * sampleRate) / sampleRate;
    const double alpha = std::sin(w) / std::numbers::sqrt2;
    const double c = std::cos(w);
    const double a0 = 1.0 + alpha;
    b0 = 0.5 * (1.0 - c) / a0;
    b1 = (1.0 - c) / a0;
    b2 = b0;
    a1 = -2.0 * c / a0;
    a2 = (1.0 - alpha) / a0;
}

double Metal909Engine::Biquad::process(double input)
{
    const double output = b0 * input + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
    x2 = x1;
    x1 = input;
    y2 = y1;
    y1 = output;
    return output;
}

} // namespace noteahead
