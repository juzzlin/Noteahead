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

#include "rim_909_engine.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace noteahead {

namespace {

//! One damped mode: amplitude exp(-t / tau) cos(2 pi f t + phase)
struct Mode
{
    double hz;
    double seconds;
    double amplitude;
    double phase;
};

//! The RD-9's rim shot decomposed by matrix pencil: its envelope within a decibel to 50 ms
constexpr std::array<Mode, Rim909Engine::ModeCount> Modes { {
  { 201.34, 0.007812, 0.728180, -1.8680 },
  { 219.27, 0.005700, 0.694948, 0.8876 },
  { 457.78, 0.006110, 0.536861, -3.0769 },
  { 104.27, 0.001011, 0.934579, -2.4309 },
  { 476.91, 0.002852, 0.431177, -0.0171 },
  { 1080.55, 0.002970, 0.325443, 1.0749 },
  { 2504.34, 0.000956, 0.364436, 2.1747 },
  { 383.15, 0.002429, 0.208870, -0.5944 },
  { 2270.65, 0.000566, 0.384562, -0.4039 },
  { 694.77, 0.005035, 0.125705, 0.3294 },
  { 1873.07, 0.001188, 0.245730, -1.4169 },
  { 1159.74, 0.004503, 0.101041, -2.9689 },
  { 1744.74, 0.003350, 0.104718, -2.9755 },
  { 2403.50, 0.002824, 0.101120, -0.2250 },
  { 900.72, 0.005991, 0.069800, -2.6584 },
  { 856.84, 0.005987, 0.068973, 0.6347 },
  { 1531.68, 0.006614, 0.034890, 2.0373 },
  { 1346.48, 0.004692, 0.038912, -2.7623 },
  { 3089.68, 0.003767, 0.033671, 1.7290 },
  { 621.14, 0.009318, 0.010224, -0.4980 },
} };
//! The modes start a little after the trigger, as in the recording
constexpr double ModeStartSeconds = 0.000196;
//! Every mode 80 dB down
constexpr double LengthSeconds = 0.13;
//! Brings it to the level the device mixes its drums from, the kick's peak
constexpr double OutputLevel = 2.58;
constexpr double ReferenceSampleRate = 48000.0;

} // namespace

Rim909Engine::Rim909Engine()
{
    setSampleRate(ReferenceSampleRate);
}

void Rim909Engine::trigger(float velocity)
{
    m_active = true;
    m_modesStarted = false;
    m_time = 0.0;
    m_velocity = static_cast<double>(std::clamp(velocity, 0.0f, 1.0f));
}

void Rim909Engine::startModes()
{
    const double rate = sampleRate();
    for (size_t i = 0; i < ModeCount; i++) {
        const auto & mode = Modes[i];
        m_modes[i] = std::polar(mode.amplitude, mode.phase);
        m_rotations[i] = std::polar(std::exp(-1.0 / (mode.seconds * rate)), 2.0 * std::numbers::pi * mode.hz / rate);
    }
    m_modesStarted = true;
}

float Rim909Engine::nextSample()
{
    if (!m_active) {
        return 0.0f;
    }
    double y = 0.0;
    if (m_time >= ModeStartSeconds) {
        if (!m_modesStarted) {
            startModes();
        }
        for (size_t i = 0; i < ModeCount; i++) {
            y += m_modes[i].real();
            m_modes[i] *= m_rotations[i];
        }
    }
    m_time += 1.0 / sampleRate();
    if (m_time > LengthSeconds) {
        m_active = false;
    }
    return static_cast<float>(y * OutputLevel * m_velocity);
}

bool Rim909Engine::isActive() const
{
    return m_active;
}

void Rim909Engine::reset()
{
    m_active = false;
    m_modesStarted = false;
    m_time = 0.0;
}

} // namespace noteahead
