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

#ifndef RIM_808_ENGINE_HPP
#define RIM_808_ENGINE_HPP

#include "drum_808_parts.hpp"
#include "drum_engine.hpp"

#include <array>

namespace noteahead {

//! A TR-808 rim shot or claves, modelled on recordings of an RD-8.
//!
//! Both are a few measured modes with the trigger pulse ahead of them. The rim shot's are cut off
//! dead after 11 ms, as the 808's rim VCA does; the claves ring out. The channel's switch picks
//! which, as on the RD-8. Neither has a knob there.
class Rim808Engine : public DrumEngine
{
public:
    Rim808Engine();
    ~Rim808Engine() override = default;

    void trigger(float velocity) override;
    float nextSample() override;
    bool isActive() const override;
    void reset() override;

    //! The channel's switch: claves instead of a rim shot
    void setClaves(bool claves);

    static constexpr size_t MaxModes { 4 };

private:
    bool m_claves { false };
    bool m_active { false };
    double m_time { 0.0 };
    double m_velocity { 1.0 };
    std::array<Drum808::ModeOscillator, MaxModes> m_modes {};
};

} // namespace noteahead

#endif // RIM_808_ENGINE_HPP
