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

#ifndef DIVIDE_DOWN_GENERATOR_HPP
#define DIVIDE_DOWN_GENERATOR_HPP

#include "dsp_component.hpp"

#include <array>
#include <cstdint>

namespace noteahead {

//! Top-octave-divider oscillator bank in the string machine tradition.
//!
//! One free-running master phasor per pitch class runs at that class's lowest frequency, and every
//! key taps it through a power-of-two phase multiplication. Notes an octave apart therefore share a
//! single accumulator and stay phase-locked by construction, exactly as they do behind a divider
//! chain: no beating between octaves, and the ensemble chorus becomes the only source of movement.
//! The whole keyboard costs twelve accumulators regardless of how many keys are held.
//!
//! Taps are band-limited with polyBLEP. PolyBlepOscillator cannot be reused here because it owns its
//! phase, whereas every tap here is derived from a shared one.
class DivideDownGenerator : public DspComponent
{
public:
    static constexpr int PitchClassCount { 12 };
    //! MIDI note of the lowest master phasor (C-1, ~8.18 Hz), so every playable note divides up from it.
    static constexpr int LowestNote { 0 };

    DivideDownGenerator();

    void setSampleRate(double sampleRate) override;

    //! Advances every master phasor by one sample. Call once per frame, before reading taps.
    void tick();

    //! Band-limited saw for a key, transposed by octaveOffset octaves (0 = 8', -1 = 16', 1 = 4').
    double saw(uint8_t note, int octaveOffset) const;

    void reset();

private:
    struct Master
    {
        double phase { 0.0 };
        double increment { 0.0 };
    };

    void updateIncrements();

    std::array<Master, PitchClassCount> m_masters;
};

} // namespace noteahead

#endif // DIVIDE_DOWN_GENERATOR_HPP
