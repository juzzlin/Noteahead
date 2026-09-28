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

#ifndef RIM_ENGINE_HPP
#define RIM_ENGINE_HPP

#include "../base_rate_source.hpp"
#include "../cascaded_svf.hpp"
#include "drum_engine.hpp"

#include <array>
#include <random>

namespace noteahead {

//! The rim, fitted to a recording of the hardware.
//!
//! Not the crack the name suggests: measured, it is a woody pitched click. Everything above 600 Hz
//! is twenty to sixty decibels down, so what carries it is four partials in the low mids, and it is
//! gone in eighty-five milliseconds. Built as its own engine rather than out of the snare's,
//! because a rim is the one kit piece with no snares in it at all -- what the snare engine would
//! contribute here is exactly the part that has to be taken away again.
class RimEngine : public DrumEngine
{
public:
    RimEngine();
    virtual ~RimEngine() override = default;

    void trigger(float velocity) override;
    float nextSample() override;
    bool isActive() const override;
    void reset() override;
    void stop() override;

    void setTune(float tune);
    void setDecay(float decay);
    //! How much of the stick is in it, 0 to 1.
    void setClick(float click);

private:
    //! Measured off the recording, every ratio and every level. They are inharmonic, which is what
    //! makes it read as a struck rim rather than as a pitched note, and they reach 1.7 kHz: over
    //! one to four kilohertz the recording measures a spectral flatness of 0.11, so what lives up
    //! there is partials rather than the stick. Built with four of them and a noise burst instead,
    //! this sat forty decibels under the recording in that band.
    static constexpr std::array<double, 18> Ratios {
        1.0, 1.89, 2.17, 2.83, 3.22, 4.22, 4.61, 5.00, 5.28, 7.44, 8.11,
        9.45, 10.06, 11.17, 11.67, 12.17, 12.95, 14.67
    };
    //! The recording's own balance, as amplitudes: 0, -27.5, -8.7, -31.4, -25.2, -27.1, -37.0,
    //! -28.0, -28.0, -36.1 and -38.8 dB under the fundamental.
    //! The last seven are the cluster the recording carries between two and four kilohertz, none of
    //! them louder than -42 dB but together forty decibels more than the nothing that was there.
    static constexpr std::array<double, 18> Levels {
        1.0, 0.042, 0.367, 0.027, 0.055, 0.044, 0.014, 0.040, 0.040, 0.0157, 0.0115,
        0.0054, 0.0035, 0.0055, 0.0081, 0.0053, 0.0038, 0.0060
    };

    std::array<double, 18> m_phases {};
    //! Samples since the strike, for the Gaussian decay.
    double m_elapsed { 0.0 };
    float m_ampEnv { 0.0f };
    float m_clickEnv { 0.0f };
    float m_thumpEnv { 0.0f };
    bool m_active { false };
    bool m_stopping { false };

    float m_tune { 0.5f };
    float m_decay { 0.5f };
    float m_click { 0.5f };
    float m_velocity { 1.0f };

    CascadedSvf m_clickFilter;
    //! The shell answering the stick: what puts anything at all below 200 Hz.
    CascadedSvf m_thumpFilter;
    BaseRateSource m_noiseBank;
    std::mt19937 m_rng;
    std::uniform_real_distribution<float> m_dist { -1.0f, 1.0f };
};

} // namespace noteahead

#endif // RIM_ENGINE_HPP
