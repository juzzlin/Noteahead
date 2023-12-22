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

#ifndef SNARE_909_ENGINE_HPP
#define SNARE_909_ENGINE_HPP

#include "drum_engine.hpp"

#include <array>
#include <complex>
#include <cstdint>

namespace noteahead {

//! The TR-909 snare drum, modelled on recordings of an RD-9 with Tune, Tone and Snappy swept.
//!
//! The body is two resonators a fourth apart, 194 and 258 Hz at noon, struck: each rings first a
//! little sharp and dies within 8 ms, then settles into a longer tone, with their harmonics and
//! difference tones under them. It is played as the damped modes a decomposition of the RD-9's
//! body found, each a rotating phasor. Over it, noise between 1.2 and 3.8 kHz is held open by a
//! gate and then shut. What the recordings showed:
//! - Tune moves every mode alike, by 0.66 to 1.35 times, as the toms' Tune does.
//! - Tone is not the noise's colour but how long the gate stays open: 80 to 195 ms.
//! - Snappy brings the noise in and takes the body down with it, by 5 dB at full.
class Snare909Engine : public DrumEngine
{
public:
    Snare909Engine();
    ~Snare909Engine() override = default;

    void trigger(float velocity) override;
    float nextSample() override;
    bool isActive() const override;
    void reset() override;

    void setTune(float tune);
    void setTone(float tone);
    void setSnappy(float snappy);

    static constexpr size_t ModeCount { 11 };

private:
    //! A two-pole filter in direct form 1
    struct Biquad
    {
        void setLowPass(double hz, double sampleRate);
        void setHighPass(double hz, double sampleRate);
        double process(double input);
        void reset();

        double b0 { 0.0 };
        double b1 { 0.0 };
        double b2 { 0.0 };
        double a1 { 0.0 };
        double a2 { 0.0 };
        double x1 { 0.0 };
        double x2 { 0.0 };
        double y1 { 0.0 };
        double y2 { 0.0 };
    };

    void updateShape();
    double nextNoise();
    void startModes();

    float m_tune { 0.5f };
    float m_tone { 0.5f };
    float m_snappy { 0.5f };

    // What the knobs stand for, worked out when one is turned
    double m_tuneRatio { 1.0 };
    double m_gateSeconds { 0.0 };
    double m_releaseSeconds { 0.0 };
    double m_bodyLevel { 0.0 };
    double m_noiseLevel { 0.0 };

    // The hit under way
    bool m_active { false };
    bool m_modesStarted { false };
    double m_time { 0.0 };
    double m_velocity { 1.0 };
    double m_filterRate { 0.0 };
    std::array<std::complex<double>, ModeCount> m_modes {};
    std::array<std::complex<double>, ModeCount> m_rotations {};
    Biquad m_noiseHighPass {};
    Biquad m_noiseLowPass {};
    //! The click's one-pole high-pass
    double m_clickA { 0.0 };
    double m_clickX1 { 0.0 };
    double m_clickY1 { 0.0 };
    uint32_t m_noiseSeed { 0 };
};

} // namespace noteahead

#endif // SNARE_909_ENGINE_HPP
