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

#include "metal_808_engine.hpp"

#include <algorithm>
#include <cmath>

namespace noteahead {

namespace {

constexpr double ButterworthQ = 0.70710678;
constexpr double SilenceLevel = 1.0e-4;
//! A reversed cymbal fades in over this rather than starting with a step
constexpr double ReverseFadeInSeconds = 0.01;
constexpr std::array<double, 5> SweptKnobs { 0.0, 0.2, 0.5, 0.8, 1.0 };

// The RD-8's closed hat with its Tone at noon, fitted on third octaves in 2-150 ms windows within
// 2.7 dB. Its squares come with some noise, through a band and two high passes.
namespace ClosedHat {
constexpr double BandHz = 1083.43;
constexpr double BandQ = 6.70053;
constexpr double HighPassHz = 6877.58;
constexpr double NoiseLevel = 0.318049;
constexpr double NoiseHz = 6742.14;
constexpr double NoiseQ = 0.459173;
constexpr double Seconds = 0.0183787;
constexpr double Gain = 133.646;
//! Tone on the RD-8's hats is a shelf under 6 to 12 kHz: a few dB up below noon, down by up to
//! 15 dB above it, which leaves what is over the corner standing out. Fitted on a sweep within 2.5
//! to 3.4 dB.
constexpr Drum808::KnobCurve ToneHz { SweptKnobs, { 8105.0, 8110.0, 9100.0, 10280.0, 12520.0 } };
constexpr Drum808::KnobCurve ToneDb { SweptKnobs, { 3.10, 2.72, 0.0, -7.51, -15.10 } };
constexpr Drum808::KnobCurve ToneQ { SweptKnobs, { 2.999, 2.994, 2.0, 1.086, 1.144 } };
} // namespace ClosedHat

// The open hat with both knobs at noon, fitted against RD-8 V3 recordings: a broader band with
// subtle filtered noise so it breathes instead of ringing metallically, held and then closed.
namespace OpenHat {
constexpr double BandHz = 6530.19;
constexpr double BandQ = 6.215;
constexpr double HighPassHz = 1885.28;
constexpr double NoiseLevel = 0.031;
constexpr double NoiseHz = 7843.16;
constexpr double NoiseQ = 4.076;
constexpr double Seconds = 0.26947;
constexpr double HoldSeconds = 0.318563;
constexpr double ReleaseSeconds = 0.0171341;
constexpr double Gain = 28.9181;
//! Tone, the same shelf as the closed hat's, fitted within 1.9 to 2.2 dB
constexpr Drum808::KnobCurve ToneHz { SweptKnobs, { 5363.0, 5433.0, 6300.0, 7180.0, 8836.0 } };
constexpr Drum808::KnobCurve ToneDb { SweptKnobs, { 3.61, 3.19, 0.0, -6.75, -13.81 } };
constexpr Drum808::KnobCurve ToneQ { SweptKnobs, { 2.163, 2.178, 1.95, 1.729, 1.575 } };
//! Decay: below noon the hat dies before it is held for long, above it it is held for longer at
//! the same decay. Against noon, fitted within 2.1 to 4.1 dB.
constexpr Drum808::KnobCurve DecaySeconds { SweptKnobs, { 0.1115, 0.1562, 1.0, 0.9363, 0.9607 } };
constexpr Drum808::KnobCurve DecayHold { SweptKnobs, { 1.365, 1.182, 1.0, 1.786, 1.805 } };
} // namespace OpenHat

// The cymbal with both knobs at noon, within 3.0 dB
namespace Cymbal {
constexpr double HighHz = 10591.2;
constexpr double HighQ = 1.09002;
constexpr double HighLowPassHz = 20000.0;
constexpr double HighSeconds = 0.122453;
constexpr double LowHz = 2227.78;
constexpr double LowQ = 1.65979;
constexpr double LowSeconds = 0.453816;
//! The low band swells in over this, which is what makes the 808's cymbal bloom
constexpr double LowAttackSeconds = 0.217541;
constexpr double LowLevel = 0.81034;
constexpr double AttackSeconds = 0.00131047;
constexpr double HighPassHz = 5754.95;
constexpr double Gain = 10.2815;
//! Tone balances the two bands: below noon the high one is turned down, above it the low one.
//! Fitted on a sweep within 2.9 to 4.1 dB.
constexpr Drum808::KnobCurve ToneHigh { SweptKnobs, { 0.6291, 0.8084, 1.0, 0.923, 0.7909 } };
constexpr Drum808::KnobCurve ToneLow { SweptKnobs, { 1.121, 1.09, 1.0, 0.7249, 0.3971 } };
//! Decay stretches the low band's ring more than the high one's, and hardly at all past 8
constexpr Drum808::KnobCurve DecayHigh { SweptKnobs, { 0.673, 0.709, 1.0, 1.147, 1.138 } };
constexpr Drum808::KnobCurve DecayLow { SweptKnobs, { 0.4237, 0.4763, 1.0, 1.496, 1.549 } };
} // namespace Cymbal

// The cowbell, within 4.4 dB
namespace Cowbell {
constexpr double BandHz = 753.68;
constexpr double BandQ = 4.10535;
constexpr double HighPassHz = 841.29;
constexpr double Fast = 0.450158;
constexpr double FastSeconds = 0.0185667;
constexpr double SlowSeconds = 0.0610139;
constexpr double Gain = 5.02586;
} // namespace Cowbell

//! The squares each sound uses, as bits of SquareBank::FrequenciesHz
uint8_t squareMask(Metal808Engine::Sound sound)
{
    return sound == Metal808Engine::Sound::Cowbell ? 0b011000 : 0b111111;
}

} // namespace

Metal808Engine::Metal808Engine(Sound sound, bool reversed)
  : m_sound { sound }
  , m_reversed { reversed && sound == Sound::Cymbal }
  , m_squares { squareMask(sound) }
{
}

void Metal808Engine::setTone(float tone)
{
    m_tone = tone;
}

void Metal808Engine::setDecay(float decay)
{
    m_decay = decay;
}

double Metal808Engine::envelope(double t) const
{
    switch (m_sound) {
    case Sound::ClosedHat:
        return std::exp(-t / ClosedHat::Seconds);
    case Sound::OpenHat: {
        const double closing = t < m_holdSeconds ? 1.0 : std::exp(-(t - m_holdSeconds) / OpenHat::ReleaseSeconds);
        return std::exp(-t / m_seconds) * closing;
    }
    case Sound::Cowbell:
        return Cowbell::Fast * std::exp(-t / Cowbell::FastSeconds) + (1.0 - Cowbell::Fast) * std::exp(-t / Cowbell::SlowSeconds);
    case Sound::Cymbal:
        break;
    }
    // The cymbal's two bands have envelopes of their own, worked out where they are mixed
    return 1.0;
}

double Metal808Engine::lengthSeconds() const
{
    // Until the slowest part of the sound is 80 dB down
    const double fall = std::log(1.0 / SilenceLevel);
    switch (m_sound) {
    case Sound::ClosedHat:
        return fall * ClosedHat::Seconds;
    case Sound::OpenHat:
        return std::min(m_holdSeconds + fall * OpenHat::ReleaseSeconds, fall * m_seconds);
    case Sound::Cowbell:
        return fall * Cowbell::SlowSeconds;
    case Sound::Cymbal:
        break;
    }
    return fall * std::max(m_lowSeconds, m_highSeconds);
}

double Metal808Engine::reversedSeconds() const
{
    // Long enough to start out of near silence, short enough not to drag: the swell of the low
    // band and three of its time constants
    return Cymbal::LowAttackSeconds + 3.0 * Cymbal::LowSeconds * Cymbal::DecayLow.ratioAt(m_decay);
}

void Metal808Engine::trigger(float velocity)
{
    const double rate = sampleRate();
    m_active = true;
    m_time = 0.0;
    m_velocity = static_cast<double>(velocity);
    m_seconds = OpenHat::Seconds * OpenHat::DecaySeconds.ratioAt(m_decay);
    m_holdSeconds = OpenHat::HoldSeconds * OpenHat::DecayHold.ratioAt(m_decay);
    m_highSeconds = Cymbal::HighSeconds * Cymbal::DecayHigh.ratioAt(m_decay);
    m_lowSeconds = Cymbal::LowSeconds * Cymbal::DecayLow.ratioAt(m_decay);
    m_highLevel = Cymbal::ToneHigh.ratioAt(m_tone);
    m_lowLevel = Cymbal::ToneLow.ratioAt(m_tone);
    m_squares.setSampleRate(rate);
    // The oscillators run free on the 808, so no two hits catch them at the same place
    m_squares.scramble(0x808u + m_strikes++);
    m_noise.reseed();
    switch (m_sound) {
    case Sound::ClosedHat:
        m_noiseBand.setBandPass(ClosedHat::NoiseHz, ClosedHat::NoiseQ, rate);
        m_band.setBandPass(ClosedHat::BandHz, ClosedHat::BandQ, rate);
        m_highPass1.setHighPass(ClosedHat::HighPassHz, ButterworthQ, rate);
        m_highPass2.setHighPass(ClosedHat::HighPassHz, ButterworthQ, rate);
        m_toneShelf.setLowShelf(ClosedHat::ToneHz.ratioAt(m_tone), ClosedHat::ToneDb.at(m_tone), ClosedHat::ToneQ.ratioAt(m_tone), rate);
        break;
    case Sound::OpenHat:
        m_noiseBand.setBandPass(OpenHat::NoiseHz, OpenHat::NoiseQ, rate);
        m_band.setBandPass(OpenHat::BandHz, OpenHat::BandQ, rate);
        m_highPass1.setHighPass(OpenHat::HighPassHz, ButterworthQ, rate);
        m_highPass2.setHighPass(OpenHat::HighPassHz, ButterworthQ, rate);
        m_toneShelf.setLowShelf(OpenHat::ToneHz.ratioAt(m_tone), OpenHat::ToneDb.at(m_tone), OpenHat::ToneQ.ratioAt(m_tone), rate);
        break;
    case Sound::Cymbal:
        m_band.setBandPass(Cymbal::HighHz, Cymbal::HighQ, rate);
        m_lowPass.setLowPass(Cymbal::HighLowPassHz, ButterworthQ, rate);
        m_lowBand.setBandPass(Cymbal::LowHz, Cymbal::LowQ, rate);
        m_highPass1.setHighPass(Cymbal::HighPassHz, ButterworthQ, rate);
        break;
    case Sound::Cowbell:
        m_band.setBandPass(Cowbell::BandHz, Cowbell::BandQ, rate);
        m_highPass1.setHighPass(Cowbell::HighPassHz, ButterworthQ, rate);
        m_highPass2.setHighPass(Cowbell::HighPassHz, ButterworthQ, rate);
        break;
    }
}

float Metal808Engine::nextSample()
{
    if (!m_active) {
        return 0.0f;
    }
    const double played = m_time;
    const double length = m_reversed ? reversedSeconds() : lengthSeconds();
    // Backwards, the envelopes are read from the end of the ring towards the hit
    const double t = m_reversed ? std::max(0.0, length - played) : played;
    const double squares = m_squares.next();
    double y = 0.0;
    switch (m_sound) {
    case Sound::ClosedHat: {
        const double source = squares + ClosedHat::NoiseLevel * m_noiseBand.process(m_noise.next());
        y = m_toneShelf.process(m_highPass2.process(m_highPass1.process(m_band.process(source))) * ClosedHat::Gain * envelope(t));
        break;
    }
    case Sound::OpenHat: {
        const double source = squares + OpenHat::NoiseLevel * m_noiseBand.process(m_noise.next());
        y = m_toneShelf.process(m_highPass2.process(m_highPass1.process(m_band.process(source))) * OpenHat::Gain * envelope(t));
        break;
    }
    case Sound::Cowbell:
        y = m_highPass2.process(m_highPass1.process(m_band.process(squares))) * Cowbell::Gain * envelope(t);
        break;
    case Sound::Cymbal: {
        const double high = m_lowPass.process(m_band.process(squares)) * std::exp(-t / m_highSeconds);
        const double low = m_lowBand.process(squares) * std::exp(-t / m_lowSeconds) * (1.0 - std::exp(-t / Cymbal::LowAttackSeconds));
        y = m_highPass1.process(m_highLevel * high + m_lowLevel * Cymbal::LowLevel * low) * (1.0 - std::exp(-t / Cymbal::AttackSeconds)) * Cymbal::Gain;
        if (m_reversed) {
            y *= std::min(1.0, played / ReverseFadeInSeconds);
        }
        break;
    }
    }
    m_time += 1.0 / sampleRate();
    if (played >= length) {
        m_active = false;
    }
    return static_cast<float>(y * m_velocity);
}

bool Metal808Engine::isActive() const
{
    return m_active;
}

void Metal808Engine::reset()
{
    m_active = false;
    // From a reset the hits play as they did the first time
    m_strikes = 0;
    for (auto * filter : { &m_noiseBand, &m_band, &m_lowPass, &m_lowBand, &m_highPass1, &m_highPass2, &m_toneShelf }) {
        filter->reset();
    }
}

} // namespace noteahead
