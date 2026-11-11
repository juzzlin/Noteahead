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

#ifndef SYNTH_OSCILLATOR_STAGE_HPP
#define SYNTH_OSCILLATOR_STAGE_HPP

#include "../dsp/poly_blep_oscillator.hpp"

#include <array>
#include <functional>

namespace noteahead {

//! The Synth's four VCOs and what couples them: Cross Mod and Ring from VCO1 into VCO2, and the
//! hard sync chain 2 to 1, 3 to 2, 4 to 3. One sample at a time, so that a voice and the dialog's
//! waveform previews run the very same code.
class SynthOscillatorStage
{
public:
    static constexpr size_t VcoCount = 4;
    //! The Minilogue XD's: Shape folds the saw rather than bending it towards a sine.
    static constexpr auto SawShaping = PolyBlepOscillator::SawShaping::Fold;

    using Values = std::array<double, VcoCount>;
    using Oscillators = std::array<std::reference_wrapper<PolyBlepOscillator>, VcoCount>;

    struct Settings
    {
        Values levels {};
        Values shapes {};
        double crossModDepth { 0.0 };
        //! Index 0 is unused: VCO1 has nothing before it to sync to.
        std::array<bool, VcoCount> syncs {};
        bool vco2Ring { false };
    };

    //! Which oscillators have to run for the sound: a muted one is skipped unless another one
    //! listens to it. A muted VCO1 still runs for Cross Mod and Ring, and a muted one runs as the
    //! master of a running one synced to it.
    static std::array<bool, VcoCount> running(const Settings & settings);

    //! Advances the oscillators by one sample at the given frequencies. Returns each oscillator's
    //! raw output, VCO2's already ring modulated when Ring is on, and zero for one not running.
    static Values process(const Oscillators & oscillators, Values frequencies, const std::array<bool, VcoCount> & running, const Settings & settings, double shapeMod, double sampleRate);
};

} // namespace noteahead

#endif // SYNTH_OSCILLATOR_STAGE_HPP
