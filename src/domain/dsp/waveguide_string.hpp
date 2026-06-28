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

#ifndef WAVEGUIDE_STRING_HPP
#define WAVEGUIDE_STRING_HPP

#include "all_pass_chain.hpp"
#include "delay_line.hpp"
#include "dsp_component.hpp"

#include <cstdint>
#include <vector>

namespace noteahead {

// Karplus-Strong digital waveguide string model.
// The excitation pulse is pre-loaded into the delay buffer at trigger time.
// The loop filter controls frequency-dependent decay (brightness).
// An all-pass chain inside the loop adds inharmonicity (string stiffness).
// Damping on note-off multiplies the per-cycle loop gain towards zero.
class WaveguideString : public DspComponent
{
public:
    static constexpr size_t MaxDelaySamples = 2048;

    void setSampleRate(double sampleRate) override;

    // brightness: 0=dark, 1=bright. decayTime: 0=short, 1=long. detuneCents: frequency offset in cents.
    void trigger(uint8_t note, float velocity, float brightness, float inharmonicity, float decayTime, double detuneCents = 0.0);
    void release(float releaseTime);

    double nextSample();
    bool isActive() const;
    void reset();

private:
    static constexpr double SilenceThreshold = 1e-9;

    static double midiNoteToFreq(uint8_t note);
    void buildExcitation(size_t width, float velocity);

    DelayLine m_delay;
    AllPassChain m_dispersion;

    double m_loopGain { 0.0 };
    double m_loopFilterCoeff { 0.25 };
    double m_loopFilterPrev { 0.0 };

    double m_damperGain { 1.0 };
    double m_damperDecay { 1.0 };
    bool m_releasing { false };

    double m_energy { 0.0 };
};

} // namespace noteahead

#endif // WAVEGUIDE_STRING_HPP
