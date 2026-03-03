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

#ifndef TOM_808_ENGINE_HPP
#define TOM_808_ENGINE_HPP

#include "drum_808_parts.hpp"
#include "drum_engine.hpp"

namespace noteahead {

//! A TR-808 tom or conga, modelled on recordings of an RD-8 and on sweeps of its Tuning.
//!
//! Each is one damped sine, pitched down from above the note over its first few milliseconds, with
//! the trigger pulse leaking through as a sharp spike ahead of it. The toms also carry a low band of
//! noise that outlasts the tone, which the congas do not: the 808's toms are given pink noise and
//! its congas none. The channel's switch picks which of the two a pad plays, as on the RD-8.
class Tom808Engine : public DrumEngine
{
public:
    enum class Position
    {
        Low,
        Mid,
        High
    };

    explicit Tom808Engine(Position position);
    ~Tom808Engine() override = default;

    void trigger(float velocity) override;
    float nextSample() override;
    bool isActive() const override;
    void reset() override;

    //! Tuning, 0..1 as on the RD-8: a little under four semitones from end to end, most of it
    //! between 2 and 8. Taken at the next trigger.
    void setTune(float tune);
    //! The channel's switch: a conga instead of a tom
    void setConga(bool conga);

    //! One drum's fitted constants
    struct Voicing;

private:
    const Voicing & voicing() const;

    Position m_position;
    bool m_conga { false };
    float m_tune { 0.5f };

    bool m_active { false };
    double m_time { 0.0 };
    double m_phase { 0.0 };
    double m_velocity { 1.0 };
    double m_pitchRatio { 1.0 };
    double m_level { 1.0 };
    Drum808::Noise m_noise { 0x8086u };
    Drum808::Biquad m_noiseBand1 {};
    Drum808::Biquad m_noiseBand2 {};
    double m_noiseScale { 0.0 };
};

} // namespace noteahead

#endif // TOM_808_ENGINE_HPP
