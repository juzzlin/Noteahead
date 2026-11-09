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

#ifndef KICK_909_ENGINE_HPP
#define KICK_909_ENGINE_HPP

#include "drum_engine.hpp"

#include <cstdint>

namespace noteahead {

//! The TR-909 bass drum, modelled on recordings of one at every combination of Tune, Attack and
//! Decay at 0, 3, 7 and 10.
//!
//! A rounded triangle swept down from two hundred hertz or so to its pitch, after a click: the
//! pulse that triggers the drum leaks through as a short ramp before the oscillator starts, then
//! clicks as it does, and a burst of noise rides on the first few tens of milliseconds, more of it
//! with Attack. What the recordings showed:
//! - Tune is the length of the sweep, not its depth: every setting settles at the same 49.5 Hz,
//!   and starts from about the same 210 to 250 Hz.
//! - Decay changes the shape of the fall as well as its length: short settings fall away faster
//!   than an exponential towards their end.
//! - The waveform is the same in every recording once settled: lopsided, flatter-topped than a
//!   triangle. The oscillator plays it from a table of its first twelve harmonics.
//!
//! The knobs are the RD-9's, 0..1 for its 0..10, with its two extra ones: Pitch, where the sweep
//! ends, and Pitch Depth, how far it starts above that.
class Kick909Engine : public DrumEngine
{
public:
    Kick909Engine();
    ~Kick909Engine() override = default;

    void trigger(float velocity) override;
    float nextSample() override;
    bool isActive() const override;
    void reset() override;
    void stop() override;

    void setTune(float tune);
    void setAttack(float attack);
    void setDecay(float decay);
    //! Where the sweep ends, in Hz. The start moves with it.
    void setPitch(double hz);
    //! How far the sweep starts above its end: 1 is the 909's own.
    void setPitchDepth(double depth);
    void setDrive(float drive);

    //! The 909's own pitch, where every Tune setting settles
    static constexpr double DefaultPitchHz { 49.5 };

    //! The amplitude contour on its own, for the waveform preview: 1 at the start of the body.
    double bodyEnvelope(double seconds) const;
    //! How long the drum sounds, until it is 80 dB down
    double lengthSeconds() const;

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

    void updateShape();
    double nextNoise();

    float m_tune { 0.5f };
    float m_attack { 0.5f };
    float m_decay { 0.5f };
    double m_pitchHz { DefaultPitchHz };
    double m_pitchDepth { 1.0 };
    float m_drive { 0.0f };

    // What the knobs stand for, worked out when one is turned
    double m_startHz { 0.0 };
    double m_sweepSeconds { 0.0 };
    double m_sweepCurve { 1.0 };
    double m_decaySeconds { 0.0 };
    double m_decayCurve { 1.0 };
    double m_driveGain { 1.0 };
    double m_driveNormalization { 1.0 };

    // The hit under way
    bool m_active { false };
    double m_time { 0.0 };
    double m_phase { 0.0 };
    double m_velocity { 1.0 };
    BandPass m_click {};
    BandPass m_noise {};
    bool m_clicked { false };
    uint32_t m_noiseSeed { 0 };
    //! Fades a hit cut short by stop() out instead of leaving a step
    double m_choke { 1.0 };
    bool m_choking { false };
};

} // namespace noteahead

#endif // KICK_909_ENGINE_HPP
