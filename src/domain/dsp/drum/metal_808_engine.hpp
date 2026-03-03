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

#ifndef METAL_808_ENGINE_HPP
#define METAL_808_ENGINE_HPP

#include "drum_808_parts.hpp"
#include "drum_engine.hpp"

namespace noteahead {

//! The TR-808's hats, cymbal and cowbell, modelled on recordings of an RD-8 and on sweeps of its knobs.
//!
//! All four come from the same six square waves, as on the 808, each through its own filters and
//! VCA: the hats a narrow band high up, the open one held flat for most of a third of a second and
//! then closed, the closed one gone in tens of milliseconds; the cymbal a high band that dies early
//! over a lower one that swells in and rings; the cowbell only the 540 and 799 Hz squares, through
//! a band pass and a fade. On the hats Tone is a shelf that thins them out above noon; on the
//! cymbal it balances the two bands. Decay holds the open hat open for longer or shuts it early, and
//! stretches the cymbal's two bands, the low one more.
//!
//! The cymbal can also be played backwards, as the pad Drum Synth V2 plays its reverse crash on.
//! The squares through their filters do not change over time, so only the envelopes run backwards.
class Metal808Engine : public DrumEngine
{
public:
    enum class Sound
    {
        ClosedHat,
        OpenHat,
        Cymbal,
        Cowbell
    };

    explicit Metal808Engine(Sound sound, bool reversed = false);
    ~Metal808Engine() override = default;

    void trigger(float velocity) override;
    float nextSample() override;
    bool isActive() const override;
    void reset() override;

    void setTone(float tone);
    void setDecay(float decay);

    //! How long a reversed cymbal plays before it reaches its hit
    double reversedSeconds() const;

private:
    double envelope(double t) const;
    double lengthSeconds() const;

    Sound m_sound;
    bool m_reversed;
    float m_tone { 0.5f };
    float m_decay { 0.5f };

    bool m_active { false };
    double m_time { 0.0 };
    double m_velocity { 1.0 };
    double m_seconds { 1.0 };
    double m_holdSeconds { 1.0 };
    double m_highSeconds { 1.0 };
    double m_lowSeconds { 1.0 };
    double m_highLevel { 1.0 };
    double m_lowLevel { 1.0 };
    uint32_t m_strikes { 0 };
    Drum808::SquareBank m_squares;
    Drum808::Noise m_noise { 0x42e7u };
    Drum808::Biquad m_noiseBand {};
    Drum808::Biquad m_band {};
    Drum808::Biquad m_lowPass {};
    Drum808::Biquad m_lowBand {};
    Drum808::Biquad m_highPass1 {};
    Drum808::Biquad m_highPass2 {};
    Drum808::Biquad m_toneShelf {};
};

} // namespace noteahead

#endif // METAL_808_ENGINE_HPP
