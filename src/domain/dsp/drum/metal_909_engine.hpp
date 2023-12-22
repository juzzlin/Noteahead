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

#ifndef METAL_909_ENGINE_HPP
#define METAL_909_ENGINE_HPP

#include "../base_rate_source.hpp"
#include "drum_engine.hpp"

#include <complex>
#include <cstdint>
#include <vector>

namespace noteahead {

//! The 909's hi-hats and cymbals, modelled on recordings of an RD-9 with their knobs swept.
//!
//! On a 909 these are samples, and the recordings show it: Tune plays them faster, every partial
//! moving up by the same factor and, where nothing else shapes it, the whole sound getting shorter
//! by it, down to where a cymbal's sample simply ends. So each sound is a fixed spectrum, the
//! partials and third-octave bands of noise between them that an analysis of the RD-9 found, each
//! with the time it dies away in, played at Tune's speed:
//! - The hats die away under an envelope Decay sets, in real time: the closed hat's starts with a
//!   chick that is gone in a few milliseconds and then holds, steeply shut; the open hat's simply
//!   dies away.
//! - The cymbals have no Decay: their partials and bands die away at their own rates, a struck
//!   burst of noise on top, until the sample ends 0.83 s in at noon.
//! A cymbal can also be played backwards, as the pad Drum Synth V2 plays its reverse crash on.
class Metal909Engine : public DrumEngine
{
public:
    enum class Sound
    {
        ClosedHat,
        OpenHat,
        Crash,
        Ride
    };

    explicit Metal909Engine(Sound sound, bool reversed = false);
    ~Metal909Engine() override = default;

    void trigger(float velocity) override;
    float nextSample() override;
    bool isActive() const override;
    void reset() override;
    //! Chokes the sound, as a closed hat does an open one
    void stop() override;

    void setTune(float tune);
    //! The hats' Decay. The cymbals have none.
    void setDecay(float decay);

    //! How long the sound lasts, until it is 80 dB down or its sample ends
    double lengthSeconds() const;

    struct Voicing;

private:
    //! A two-pole filter in direct form 1
    struct Biquad
    {
        void setBandPass(double hz, double q, double sampleRate);
        void setLowPass(double hz, double sampleRate);
        double process(double input);

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

    //! One band of the noise, with a generator of its own where the bands must not correlate
    struct NoiseBand
    {
        Biquad filter;
        double level { 0.0 };
        double decay { 1.0 };
        uint32_t seed { 0 };
    };

    void updateShape();
    void start();
    double nextBaseSample();
    double envelope(double seconds) const;
    //! Where in the sample the hit is, in the sample's own time
    double sampleSeconds() const;
    static double nextNoise(uint32_t & seed);

    Sound m_sound;
    bool m_reversed;
    const Voicing & m_voicing;

    float m_tune { 0.5f };
    float m_decay { 0.5f };

    // What the knobs stand for, worked out when one is turned
    double m_tuneRatio { 1.0 };
    double m_decaySeconds { 0.0 };
    double m_decayCurve { 1.0 };
    double m_chickSeconds { 0.001 };
    double m_gain { 1.0 };

    // The hit under way
    bool m_active { false };
    bool m_started { false };
    double m_time { 0.0 };
    double m_velocity { 1.0 };
    std::vector<std::complex<double>> m_partials;
    std::vector<std::complex<double>> m_rotations;
    std::vector<NoiseBand> m_bands;
    NoiseBand m_strike;
    Biquad m_lowPass1;
    Biquad m_lowPass2;
    BaseRateSource m_source;
    uint32_t m_noiseSeed { 0 };
    double m_choke { 1.0 };
    bool m_choking { false };
};

} // namespace noteahead

#endif // METAL_909_ENGINE_HPP
