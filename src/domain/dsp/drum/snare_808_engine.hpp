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

#ifndef SNARE_808_ENGINE_HPP
#define SNARE_808_ENGINE_HPP

#include "drum_808_parts.hpp"
#include "drum_engine.hpp"

#include <array>

namespace noteahead {

//! A TR-808 snare, modelled on recordings of an RD-8 and on sweeps of its knobs.
//!
//! Two tones, 334 Hz short and 174 Hz longer, and noise in a band around 2.5 kHz that dies away in
//! tens of milliseconds. Tone balances the two tones against each other, Snappy is how much of the
//! noise there is.
class Snare808Engine : public DrumEngine
{
public:
    Snare808Engine();
    ~Snare808Engine() override = default;

    void trigger(float velocity) override;
    float nextSample() override;
    bool isActive() const override;
    void reset() override;

    void setTone(float tone);
    void setSnappy(float snappy);

private:
    float m_tone { 0.5f };
    float m_snappy { 0.5f };

    bool m_active { false };
    double m_time { 0.0 };
    double m_velocity { 1.0 };
    std::array<Drum808::ModeOscillator, 2> m_modes {};
    Drum808::Noise m_noise { 0x5d808u };
    Drum808::Biquad m_noiseHighPass {};
    Drum808::Biquad m_noiseLowPass {};
    double m_noiseScale { 0.0 };
    double m_noiseLevel { 0.0 };
};

} // namespace noteahead

#endif // SNARE_808_ENGINE_HPP
