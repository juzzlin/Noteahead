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

#include "rim_808_engine.hpp"

#include <cmath>
#include <limits>

namespace noteahead {

namespace {

//! One sound at the RD-8's settings, fitted on its onset spectrogram and envelope
struct Voicing
{
    std::array<Drum808::Mode, Rim808Engine::MaxModes> modes;
    double spike;
    double spikeSeconds;
    //! Where the modes are cut off, if they are
    double gateSeconds;
    double gain;
    //! How long the sound lasts, for knowing when it is over
    double lengthSeconds;
};

// Within 4.4 dB
constexpr Voicing RimShot {
    .modes = { { { 1868.33, 0.00230557, 1.05848, -0.802504 }, { 1991.97, 0.000433626, 1.54166, 1.74828 }, { 1052.01, 0.00327956, 0.4781, -0.714111 }, { 464.773, 0.00619079, 0.672914, -1.3075 } } },
    .spike = 0.118052,
    .spikeSeconds = 0.000217537,
    .gateSeconds = 0.0112789,
    .gain = 0.84264,
    .lengthSeconds = 0.015,
};

// Within 2.2 dB
constexpr Voicing Claves {
    .modes = { { { 2533.51, 0.00476173, 2.04937, -2.60403 }, { 2956.28, 0.000670382, 2.52721, -0.0112947 }, { 0.0, 1.0, 0.0, 0.0 }, { 0.0, 1.0, 0.0, 0.0 } } },
    .spike = 0.166315,
    .spikeSeconds = 0.00130348,
    .gateSeconds = std::numeric_limits<double>::infinity(),
    .gain = 0.544834,
    .lengthSeconds = 0.06,
};

//! The 808's rim VCA closes this fast
constexpr double GateReleaseSeconds = 0.0003;

} // namespace

Rim808Engine::Rim808Engine() = default;

void Rim808Engine::setClaves(bool claves)
{
    m_claves = claves;
}

void Rim808Engine::trigger(float velocity)
{
    const auto & voicing = m_claves ? Claves : RimShot;
    m_active = true;
    m_time = 0.0;
    m_velocity = static_cast<double>(velocity);
    for (size_t i = 0; i < MaxModes; i++) {
        m_modes.at(i).start(voicing.modes.at(i), sampleRate());
    }
}

float Rim808Engine::nextSample()
{
    if (!m_active) {
        return 0.0f;
    }
    const auto & voicing = m_claves ? Claves : RimShot;
    const double t = m_time;
    double y = 0.0;
    for (auto && mode : m_modes) {
        y += mode.next();
    }
    if (t >= voicing.gateSeconds) {
        y *= std::exp(-(t - voicing.gateSeconds) / GateReleaseSeconds);
    }
    y += voicing.spike * std::exp(-t / voicing.spikeSeconds);
    m_time += 1.0 / sampleRate();
    if (t > voicing.lengthSeconds) {
        m_active = false;
    }
    return static_cast<float>(y * voicing.gain * m_velocity);
}

bool Rim808Engine::isActive() const
{
    return m_active;
}

void Rim808Engine::reset()
{
    m_active = false;
    for (auto && mode : m_modes) {
        mode.stop();
    }
}

} // namespace noteahead
