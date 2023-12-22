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
  { 198.17, 0.006808, 0.174925, -0.3801 },
  { 209.69, 0.008064, 0.173375, 2.4643 },
  { 541.22, 0.001324, 0.165641, -2.0496 },
  { 256.67, 0.001884, 0.121102, -2.8942 },
  { 687.21, 0.002735, 0.0896017, 0.2841 },
  { 459.26, 0.006819, 0.0668206, -3.1395 },
  { 650.03, 0.003776, 0.0426491, -1.9545 },
  { 1090.31, 0.003369, 0.0422905, 1.0557 },
  { 27.12, 0.007033, 0.025156, 2.9950 },
  { 1209.47, 0.002708, 0.0217679, 1.7641 },
  { 3157.58, 0.001453, 0.0174311, 2.1579 },
  { 1884.79, 0.002301, 0.0142441, -0.2171 },
  { 2906.67, 0.001504, 0.0131501, 0.4519 },
  { 1561.17, 0.003161, 0.0119841, 1.4934 },
  { 872.90, 0.006529, 0.011935, -0.3835 },
  { 2464.51, 0.004260, 0.00784226, -1.0531 },
  { 1720.82, 0.005733, 0.00665487, -1.0656 },
  { 1446.45, 0.005016, 0.00318477, 0.4697 },
  { 426.90, 0.012519, 0.00259676, 1.2564 },
  { 914.23, 0.013738, 0.00105044, -3.0735 },
} };
//! The modes start a little after the trigger, as in the recording
constexpr double ModeStartSeconds = 0.000196;
//! Every mode 80 dB down
constexpr double LengthSeconds = 0.13;
//! Brings it to the level the device mixes its drums from, the kick's peak
constexpr double OutputLevel = 7.0;
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
