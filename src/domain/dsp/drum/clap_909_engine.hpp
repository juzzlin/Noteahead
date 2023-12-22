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

#ifndef CLAP_909_ENGINE_HPP
#define CLAP_909_ENGINE_HPP

#include "drum_engine.hpp"

#include <cstdint>

namespace noteahead {

//! The TR-909 hand clap, modelled on a recording of an RD-9's.
//!
//! Noise, as on the 909: four short bursts 11 to 12 ms apart, broad and centred at 1.1 kHz, then
//! from the last of them a narrower tail around 1.07 kHz that dies away over 200 ms. Fitted on the
//! spectrogram within 5.6 dB, its envelope within 2 to 4. The RD-9 has no knobs for it.
class Clap909Engine : public DrumEngine
{
public:
    Clap909Engine();
    ~Clap909Engine() override = default;

    void trigger(float velocity) override;
    float nextSample() override;
    bool isActive() const override;
    void reset() override;

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

    static double nextNoise(uint32_t & seed);

    bool m_active { false };
    double m_time { 0.0 };
    double m_velocity { 1.0 };
    double m_filterRate { 0.0 };
    BandPass m_burstFilter {};
    BandPass m_tailFilter {};
    uint32_t m_burstSeed { 0 };
    uint32_t m_tailSeed { 0 };
};

} // namespace noteahead

#endif // CLAP_909_ENGINE_HPP
