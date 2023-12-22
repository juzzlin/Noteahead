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

#include "clap_909_engine.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>

namespace noteahead {

namespace {

//! When each burst starts and how loud it is
constexpr std::array<double, 4> BurstSeconds { 0.0, 0.011067, 0.023435, 0.032523 };
constexpr std::array<double, 4> BurstLevel { 0.09214, 0.09866, 0.09698, 0.16456 };
//! How fast a burst dies, and the band it is in
constexpr double BurstDecaySeconds = 0.0040145;
constexpr double BurstHz = 1110.8;
constexpr double BurstQ = 1.241;
//! The tail, from the last burst: (1 - exp(-t / attack)) exp(-(t / decay)^curve)
constexpr double TailLevel = 0.14650;
constexpr double TailAttackSeconds = 0.0033637;
constexpr double TailDecaySeconds = 0.016994;
constexpr double TailCurve = 0.51672;
constexpr double TailHz = 1073.7;
constexpr double TailQ = 1.9609;
//! Where the tail is 80 dB down
constexpr double LengthSeconds = 0.9;
//! Brings it to the level the device mixes its drums from, the kick's peak
constexpr double OutputLevel = 18.4;
//! The rate the noise was fitted at; held to the same spectrum at any other
constexpr double ReferenceSampleRate = 48000.0;
constexpr uint32_t BurstSeed = 0x68E31DA4u;
constexpr uint32_t TailSeed = 0xB5297A4Du;

} // namespace

Clap909Engine::Clap909Engine()
{
    setSampleRate(ReferenceSampleRate);
}

void Clap909Engine::trigger(float velocity)
{
    m_active = true;
    m_time = 0.0;
    m_velocity = static_cast<double>(std::clamp(velocity, 0.0f, 1.0f));
    m_burstSeed = BurstSeed;
    m_tailSeed = TailSeed;
    m_filterRate = 0.0;
}

double Clap909Engine::nextNoise(uint32_t & seed)
{
    // xorshift32: the same clap on every hit
    seed ^= seed << 13;
    seed ^= seed >> 17;
    seed ^= seed << 5;
    // Uniform -1..1 scaled to unit deviation, as the fit's Gaussian noise
    return std::sqrt(3.0) * (static_cast<double>(seed) / 2147483648.0 - 1.0);
}

float Clap909Engine::nextSample()
{
    if (!m_active) {
        return 0.0f;
    }
    const double rate = sampleRate();
    if (rate != m_filterRate) {
        // Set up at the first sample rather than at the trigger, so a rate set in between counts
        m_filterRate = rate;
        m_burstFilter.set(BurstHz, BurstQ, rate);
        m_tailFilter.set(TailHz, TailQ, rate);
        m_burstFilter.reset();
        m_tailFilter.reset();
    }

    const double t = m_time;
    double bursts = 0.0;
    for (size_t i = 0; i < BurstSeconds.size(); i++) {
        if (t >= BurstSeconds[i]) {
            bursts += BurstLevel[i] * std::exp(-(t - BurstSeconds[i]) / BurstDecaySeconds);
        }
    }
    const double sinceTail = t - BurstSeconds.back();
    const double tail = sinceTail >= 0.0 ? TailLevel * (1.0 - std::exp(-sinceTail / TailAttackSeconds)) * std::exp(-std::pow(sinceTail / TailDecaySeconds, TailCurve)) : 0.0;

    // White noise carries the same density at any rate when it scales with the rate's root
    const double density = std::sqrt(rate / ReferenceSampleRate);
    const double y = m_burstFilter.process(nextNoise(m_burstSeed) * density) * bursts + m_tailFilter.process(nextNoise(m_tailSeed) * density) * tail;

    m_time += 1.0 / rate;
    if (m_time > LengthSeconds) {
        m_active = false;
    }
    return static_cast<float>(y * OutputLevel * m_velocity);
}

bool Clap909Engine::isActive() const
{
    return m_active;
}

void Clap909Engine::reset()
{
    m_active = false;
    m_time = 0.0;
    m_burstFilter.reset();
    m_tailFilter.reset();
}

void Clap909Engine::BandPass::set(double hz, double q, double sampleRate)
{
    const double w = 2.0 * std::numbers::pi * std::min(hz, 0.45 * sampleRate) / sampleRate;
    const double alpha = std::sin(w) / (2.0 * q);
    const double a0 = 1.0 + alpha;
    b0 = alpha / a0;
    a1 = -2.0 * std::cos(w) / a0;
    a2 = (1.0 - alpha) / a0;
}

double Clap909Engine::BandPass::process(double input)
{
    const double output = b0 * (input - x2) - a1 * y1 - a2 * y2;
    x2 = x1;
    x1 = input;
    y2 = y1;
    y1 = output;
    return output;
}

void Clap909Engine::BandPass::reset()
{
    x1 = x2 = y1 = y2 = 0.0;
}

} // namespace noteahead
