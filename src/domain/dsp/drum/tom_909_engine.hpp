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

#ifndef TOM_909_ENGINE_HPP
#define TOM_909_ENGINE_HPP

#include "drum_engine.hpp"

#include <array>
#include <cstdint>

namespace noteahead {

//! A TR-909 tom, modelled on recordings of an RD-9 with Tune and Decay swept.
//!
//! Two oscillators a fixed ratio apart, both close to triangles, swept down together by about a
//! third over the first tens of milliseconds and summed, then through a resonance that stays where
//! it is whatever Tune does: it is what puts the harmonics near it over the fundamental at low
//! settings, and what makes the drum louder as it is tuned up. A burst of noise rides on the start.
//! What the recordings showed:
//! - The two oscillators keep their ratio at every Tune setting, 1.49 to 1.59 depending on the drum.
//! - Each decays on its own envelope, the lower one far less loud.
//! - Decay hardly lengthens the drum past noon.
class Tom909Engine : public DrumEngine
{
public:
    enum class Tom
    {
        Low,
        Mid,
        High
    };

    explicit Tom909Engine(Tom tom);
    ~Tom909Engine() override = default;

    void trigger(float velocity) override;
    float nextSample() override;
    bool isActive() const override;
    void reset() override;

    void setTune(float tune);
    void setDecay(float decay);

    //! How long the drum sounds, until it is 80 dB down
    double lengthSeconds() const;

    static constexpr size_t WaveTableSize { 2048 };

    //! One drum's fitted constants
    struct Voicing;

private:
    //! A two-pole band-pass, its peak at unity gain
    struct BandPass
    {
        void set(double hz, double q, double sampleRate);
        double process(double input);
        void reset();

        double b0 { 0.0 };
        double a1 { 0.0 };
        double a2 { 0.0 };
        double x1 { 0.0 };
        double x2 { 0.0 };
        double y1 { 0.0 };
        double y2 { 0.0 };
    };

    //! A one-pole high-pass
    struct HighPass
    {
        void set(double hz, double sampleRate);
        double process(double input);
        void reset();

        double a { 0.0 };
        double x1 { 0.0 };
        double y1 { 0.0 };
    };

    void updateShape();
    double nextNoise();
    double waveform(double phase) const;

    const Voicing & m_voicing;
    std::array<double, WaveTableSize + 1> m_waveTable {};

    float m_tune { 0.5f };
    float m_decay { 0.5f };

    // What the knobs stand for, worked out when one is turned
    double m_lowHz { 0.0 };
    double m_lowSeconds { 0.0 };
    double m_highSeconds { 0.0 };

    // The hit under way
    bool m_active { false };
    double m_time { 0.0 };
    double m_lowPhase { 0.0 };
    double m_highPhase { 0.0 };
    double m_velocity { 1.0 };
    double m_filterRate { 0.0 };
    BandPass m_resonance {};
    HighPass m_bodyHighPass {};
    BandPass m_noise {};
    HighPass m_click {};
    uint32_t m_noiseSeed { 0 };
};

} // namespace noteahead

#endif // TOM_909_ENGINE_HPP
