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

#ifndef RIM_909_ENGINE_HPP
#define RIM_909_ENGINE_HPP

#include "drum_engine.hpp"

#include <array>
#include <complex>

namespace noteahead {

//! The TR-909 rim shot, modelled on a recording of an RD-9's.
//!
//! Resonators struck by a pulse, as the snare's body is, but far shorter: it is gone in 50 ms. It is
//! played as the twenty damped modes a decomposition of the RD-9's found, each a rotating phasor,
//! which explain all but half a per cent of it. The RD-9 has no knobs for it.
class Rim909Engine : public DrumEngine
{
public:
    Rim909Engine();
    ~Rim909Engine() override = default;

    void trigger(float velocity) override;
    float nextSample() override;
    bool isActive() const override;
    void reset() override;

    static constexpr size_t ModeCount { 20 };

private:
    void startModes();

    bool m_active { false };
    bool m_modesStarted { false };
    double m_time { 0.0 };
    double m_velocity { 1.0 };
    std::array<std::complex<double>, ModeCount> m_modes {};
    std::array<std::complex<double>, ModeCount> m_rotations {};
};

} // namespace noteahead

#endif // RIM_909_ENGINE_HPP
